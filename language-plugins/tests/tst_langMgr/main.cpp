#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include <stdcorelib/str.h>
#include <stdcorelib/system.h>

#include <LangMgr/Base/NamedObject.h>
#include <LangMgr/Core/Manager.h>
#include <LangMgr/Module/Dependency/Dependency.h>
#include <LangMgr/Module/G2pModule.h>
#include <LangMgr/Module/Module.h>
#include <LangMgr/Package/Package.h>
#include <LangMgr/Task/G2pTask.h>
#include <LangMgr/Task/Task.h>
#include <LangMgr/Task/TaskFactoryPlugin.h>

#include <LangPlugins/Api/Drivers/Onnx/1/OnnxDriverApiL1.h>


#ifdef WIN32
#include <Windows.h>
#endif

using EP = LangPlugins::Api::Onnx::L1::ExecutionProvider;

class AutoModuleInitializer {
public:
    AutoModuleInitializer(LangMgr::Manager &mgr, EP ep, int deviceIndex = 0, bool loadFromProgress = false);

    LangMgr::Expected<void> initializeManager() {
        const auto pluginRootDir = getPluginRootDirectory();
        const auto defaultPluginDir = pluginRootDir / _TSTR("LangPlugins");

        mgr_.addPluginPath("org.openvpi.DriverFactory", defaultPluginDir / _TSTR("InferenceDrivers"));
        mgr_.addPluginPath("org.openvpi.TaskFactory", defaultPluginDir / _TSTR("G2ps"));
        mgr_.addPluginPath("org.openvpi.TaskFactory", defaultPluginDir / _TSTR("Splitters"));
        mgr_.addPluginPath("org.openvpi.TaskFactory", defaultPluginDir / _TSTR("Taggers"));

        auto onnxDriverExp = initializeOnnxDriver(ep_, deviceIndex_, loadFromProgress_);
        if (!onnxDriverExp) {
            return onnxDriverExp.error();
        }

        auto &driverCategory = *mgr_.category("driver");
        driverCategory.addObject("g2pOnnxDriver", onnxDriverExp.take());

        return {};
    }

    LangMgr::Expected<std::vector<LangMgr::Package>> loadPackagesInOrder(const std::filesystem::path &packagesRootDir) {
        mgr_.addPackagePath(packagesRootDir);
        const auto moduleInfos = mgr_.getModuleInfos();

        if (moduleInfos.empty()) {
            return LangMgr::Error(LangMgr::Error::InvalidArgument,
                                  "Dependency resolution failed. Cannot load packages.");
        }

        const LangMgr::Dependency dependency;
        for (const auto &info : moduleInfos) {
            if (!dependency.addModule(info)) {
                std::cerr << "Failed to add module: " << info.key() << std::endl;
            }
        }

        if (!dependency.validate()) {
            if (const auto cycles = dependency.getCycles(); !cycles.empty()) {
                std::cerr << "Dependency cycles detected:" << std::endl;
                for (const auto &cycle : cycles) {
                    std::cerr << "  Cycle: ";
                    for (const auto &mod : cycle) {
                        std::cerr << mod.packageId << ":" << mod.moduleId << " -> ";
                    }
                    std::cerr << std::endl;
                }
            }
            return LangMgr::Error(LangMgr::Error::InvalidArgument, "Dependency validation failed");
        }

        const auto packageOrder = dependency.getPackageInitializationOrder();
        if (packageOrder.empty())
            return LangMgr::Error(LangMgr::Error::InvalidArgument, "Failed to determine package initialization order");

        struct TaskFactoryInitArgs {
            std::string id_;
            std::string iid_;
            std::string type_;

            bool operator<(const TaskFactoryInitArgs &other) const {
                return std::tie(id_, iid_, type_) < std::tie(other.id_, other.iid_, other.type_);
            }

            bool operator==(const TaskFactoryInitArgs &other) const {
                return id_ == other.id_ && iid_ == other.iid_ && type_ == other.type_;
            }
        };

        std::set<TaskFactoryInitArgs> iids;
        for (const auto &packageInfo : packageOrder) {
            for (const auto &moduleInfo : packageInfo.modules)
                iids.insert(TaskFactoryInitArgs{moduleInfo.moduleId, moduleInfo.iid, moduleInfo.type});
        }

        for (const auto &[id_, iid_, type_] : iids) {
            const auto taskFactoryPlugin = mgr_.plugin<LangMgr::TaskFactoryPlugin>(iid_.c_str());
            if (!taskFactoryPlugin) {
                return LangMgr::Error(LangMgr::Error::FileNotOpen, "failed to load RegexSplitter interpreter plugin");
            }
            const auto &task = taskFactoryPlugin->create();
            auto &ic = *mgr_.category(type_.c_str());
            ic.addObject(id_ + "Factory", task);
        }

        std::vector<LangMgr::Package> loadedPackages;

        for (const auto &packageInfo : packageOrder) {
            std::cout << "Loading package: " << packageInfo.packageId << " from " << packageInfo.packagePath
                      << std::endl;

            auto exp = mgr_.open(packageInfo.packagePath, false);
            if (!exp) {
                std::cerr << "Failed to open package " << packageInfo.packageId << ": " << exp.error().message()
                          << std::endl;
                continue;
            }

            LangMgr::Package pkg = exp.take();
            if (!pkg.isLoaded()) {
                std::cerr << "Failed to load package " << packageInfo.packageId << ": " << pkg.error().message()
                          << std::endl;
                continue;
            }

            loadedPackages.push_back(pkg);

            for (const auto &moduleInfo : packageInfo.initializationOrder) {
                if (auto taskExp = createModuleTask(moduleInfo, pkg)) {
                    std::cout << "  Created task for module: " << moduleInfo.moduleId << " (type: " << moduleInfo.type
                              << ", class: " << moduleInfo.iid << ")" << std::endl;
                    loadedTasks_[moduleInfo.moduleId] = taskExp.take();
                } else {
                    std::cerr << "  Failed to create task for module: " << moduleInfo.moduleId << ": "
                              << taskExp.error().message() << std::endl;
                }
            }
        }

        return loadedPackages;
    }

    template <typename TaskType>
    std::vector<LangMgr::NO<LangMgr::Task>> getTasksByType() const {
        std::vector<LangMgr::NO<LangMgr::Task>> result;
        for (const auto &[key, task] : loadedTasks_) {
            result.push_back(task);
        }
        return result;
    }

    const std::unordered_map<std::string, LangMgr::NO<LangMgr::Task>> &getAllTasks() const { return loadedTasks_; }

private:
    LangMgr::Manager &mgr_;
    EP ep_;
    int deviceIndex_;
    bool loadFromProgress_;
    std::unordered_map<std::string, LangMgr::NO<LangMgr::Task>> loadedTasks_;

    LangMgr::Expected<LangMgr::NO<LangMgr::Task>> createModuleTask(const LangMgr::ModuleInfo &moduleInfo,
                                                                   const LangMgr::Package &pkg) const {
        const auto moduleDef = pkg.moduleSpec(moduleInfo.type, moduleInfo.moduleId);
        if (!moduleDef) {
            return LangMgr::Error(LangMgr::Error::FileNotFound,
                                  stdc::formatN("Module %1 not found in package %2", moduleInfo.moduleId, pkg.id()));
        }

        const auto &moduleCategory = *mgr_.category(moduleInfo.type);
        const auto moduleName = moduleDef->id();
        const auto taskFactory = moduleCategory.getFirstObject(moduleName + "Factory").as<LangMgr::TaskFactory>();
        if (!taskFactory) {
            return LangMgr::Error(LangMgr::Error::InterpreterNotFound,
                                  stdc::formatN("%1 task factory not found", moduleName));
        }

        const auto runtimeOptions =
            LangMgr::NO<LangMgr::TaskRuntimeOptions>::create("", moduleDef->id(), moduleDef->apiLevel());

        auto taskExp = taskFactory->createTask(moduleDef, runtimeOptions);
        if (!taskExp) {
            return LangMgr::Error(LangMgr::Error::InvalidArgument,
                                  stdc::formatN("Failed to create task: %1", taskExp.error().message()));
        }

        auto task = taskExp.take();

        const auto initArgs =
            LangMgr::NO<LangMgr::TaskInitArgs>::create("", moduleDef->className(), moduleDef->apiLevel());
        if (const auto exp = task->initialize(initArgs); !exp) {
            return LangMgr::Error(LangMgr::Error::InvalidArgument,
                                  stdc::formatN("Failed to initialize task: %1", exp.error().message()));
        }

        auto &ic = *mgr_.category(moduleDef->category().c_str());
        ic.addObject(moduleDef->id(), task);
        return task;
    }

    LangMgr::Expected<LangMgr::NO<LangMgr::SessionFactory>> initializeOnnxDriver(EP ep, int deviceIndex,
                                                                                 bool loadFromProgress) const;


    std::filesystem::path getPluginRootDirectory() {
#if defined(Q_OS_MAC)
        return MacOSUtils::getMainBundlePath() / _TSTR("Contents/PlugIns");
#elif defined(Q_OS_WIN)
        return stdc::system::application_directory() / _TSTR("plugins");
#else
        return stdc::system::application_directory().parent_path() / _TSTR("lib/plugins");
#endif
    }
};

AutoModuleInitializer::AutoModuleInitializer(LangMgr::Manager &mgr, const EP ep, const int deviceIndex,
                                             const bool loadFromProgress) :
    mgr_(mgr), ep_(ep), deviceIndex_(deviceIndex), loadFromProgress_(loadFromProgress) {}
LangMgr::Expected<LangMgr::NO<LangMgr::SessionFactory>>
AutoModuleInitializer::initializeOnnxDriver(const EP ep, const int deviceIndex, const bool loadFromProgress) const {
    const auto onnxDriverPlugin = mgr_.plugin<LangMgr::DriverFactoryPlugin>("onnx");
    if (!onnxDriverPlugin) {
        return LangMgr::Error(LangMgr::Error::FileNotOpen, "Failed to load ONNX inference driver");
    }

    auto onnxDriver = onnxDriverPlugin->create();
    const auto onnxArgs = LangMgr::NO<LangPlugins::Api::Onnx::L1::DriverInitArgs>::create();

    onnxArgs->ep = ep;
    const auto ortParentPath = onnxDriverPlugin->path().parent_path() / _TSTR("runtimes") / _TSTR("onnx");
    onnxArgs->runtimePath = ep == LangPlugins::Api::Onnx::L1::CUDAExecutionProvider ? ortParentPath / _TSTR("cuda")
                                                                                    : ortParentPath / _TSTR("default");

    onnxArgs->loadFromProgress = loadFromProgress;
    onnxArgs->deviceIndex = deviceIndex;

    if (const auto exp = onnxDriver->initialize(onnxArgs); !exp) {
        return LangMgr::Error(LangMgr::Error::FileNotOpen,
                              stdc::formatN("Failed to initialize ONNX driver: %1", exp.error().message()));
    }

    return onnxDriver;
}

EP parseExecutionProvider(const std::string &provider) {
    const auto providerLower = stdc::to_lower(provider);
    if (providerLower == "dml" || providerLower == "directml") {
        return EP::DMLExecutionProvider;
    }
    if (providerLower == "cuda") {
        return EP::CUDAExecutionProvider;
    }
    if (providerLower == "coreml") {
        return EP::CoreMLExecutionProvider;
    }
    return EP::CPUExecutionProvider;
}


void executeTemplateInference(const LangMgr::NO<LangMgr::Task> &templateInference) {
    const auto input = LangMgr::NO<LangMgr::G2pStartInput>::create(LangMgr::G2P_API_NAME, LangMgr::G2P_API_CLASS,
                                                                   LangMgr::G2P_API_LEVEL);
    input->g2pInput = {LangMgr::G2pInput({"hellobazhahei", "eng"}), LangMgr::G2pInput({"hello", "eng"})};

    std::cout << "Starting inference - Id: " << templateInference->spec()->as<LangMgr::G2pDefinition>()->name().text()
              << std::endl;
    auto resultExp = templateInference->start(input);
    if (!resultExp)
        throw std::runtime_error(stdc::formatN("inference failed: %1", resultExp.error().message()));

    const auto result = resultExp.take();
    if (const auto g2pResult = result.as<LangMgr::G2pOutput>()) {
        for (const auto &res : g2pResult->g2pResult)
            std::cout << "\nlyric: " << res.lyric << ";\npronunciation: '" << res.pronunciation
                      << "';\nmode: " << res.mode << std::endl
                      << std::endl;

        if (!g2pResult->errorMessage.empty())
            std::cout << "Error: " << g2pResult->errorMessage << std::endl;

    } else {
        throw std::runtime_error("unexpected result type");
    }
}

int main() {
    try {
        EP g2pProvider = parseExecutionProvider("cpu");

        LangMgr::Manager langMgr;
        AutoModuleInitializer initializer(langMgr, g2pProvider, 0, false);

        if (const auto exp = initializer.initializeManager(); !exp) {
            return -1;
        }

        std::filesystem::path packagesRootDir = R"(D:\projects\language-manager\tst_package)";

        auto packagesExp = initializer.loadPackagesInOrder(packagesRootDir);
        if (!packagesExp) {
            return -1;
        }

        auto packages = packagesExp.take();
        std::cout << "\nSuccessfully loaded " << packages.size() << " packages" << std::endl;

        auto allTasks = initializer.getAllTasks();
        std::cout << "Total tasks created: " << allTasks.size() << std::endl;

        std::cout << "\nAll tasks:" << std::endl;
        for (const auto &[key, task] : allTasks) {
            if (auto spec = task->spec()) {
                std::cout << "  - " << spec->id() << " (category: " << spec->category()
                          << ", class: " << spec->className() << ")" << std::endl
                          << std::endl;
            }
        }

        const auto inferenceCate = langMgr.category("g2p");
        if (!inferenceCate)
            return -1;

        const auto inferenceObject = inferenceCate->getFirstObject("g2p-official-eng");
        if (!inferenceObject)
            return -1;

        auto task = inferenceObject.as<LangMgr::Task>();
        if (!task)
            throw std::runtime_error("unexpected result type");

        executeTemplateInference(task);

        for (const auto &[key, task] : allTasks) {
            if (key == "g2p-template-eng" || key == "g2p-official-eng")
                // Starting inference
                executeTemplateInference(task);
        }
        std::cout << "G2pTask completed successfully" << std::endl;
        return 0;
    }
    catch (const std::exception &e) {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
        return -1;
    }
}
