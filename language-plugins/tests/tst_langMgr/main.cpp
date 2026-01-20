#include <filesystem>
#include <iostream>
#include <numeric>
#include <string>
#include <unordered_map>
#include <vector>

#include <stdcorelib/str.h>
#include <stdcorelib/system.h>

#include <LangMgr/Base/NamedObject.h>
#include <LangMgr/Core/Manager.h>
#include <LangMgr/Module/Dependency/DependencyGraph.h>
#include <LangMgr/Module/G2pModule.h>
#include <LangMgr/Module/Module.h>
#include <LangMgr/Package/Package.h>
#include <LangMgr/Task/G2pTask.h>
#include <LangMgr/Task/Task.h>
#include <LangMgr/Task/TaskFactoryPlugin.h>

#include <LangPlugins/Api/Drivers/Onnx/1/OnnxDriverApiL1.h>


using EP = LangPlugins::Api::Onnx::L1::ExecutionProvider;

class AutoModuleInitializer {
public:
    AutoModuleInitializer(LangMgr::Manager &mgr, EP ep, int deviceIndex = 0, bool loadFromProgress = false);

    LangMgr::Expected<void> initializeManager() {
        const auto pluginRootDir = getPluginRootDirectory();
        const auto defaultPluginDir = pluginRootDir / _TSTR("LangPlugins");

        mgr_.addPluginPath("org.openvpi.DriverFactory", defaultPluginDir / _TSTR("InferenceDrivers"));
        mgr_.addPluginPath("org.openvpi.TaskFactory", defaultPluginDir / _TSTR("G2ps"));
        mgr_.addPluginPath("org.openvpi.TaskFactory", defaultPluginDir / _TSTR("Taggers"));

        auto onnxDriverExp = initializeOnnxDriver(ep_, deviceIndex_, loadFromProgress_);
        if (!onnxDriverExp) {
            return onnxDriverExp.error();
        }

        auto &driverCategory = *mgr_.category("driver");
        driverCategory.addObject("g2pOnnxDriver", onnxDriverExp.take());

        return {};
    }

    bool loadPackagesInOrder(const std::filesystem::path &packagesRootDir) {
        mgr_.addPackagePath(packagesRootDir);
        mgr_.checkDependencies();

        const auto packageOrder = mgr_.getPackageInitializationOrder();
        if (packageOrder.empty()) {
            std::cerr << "Failed to determine package initialization order" << std::endl;
            return false;
        }

        std::set<std::string> iids;
        for (const auto &packageInfo : packageOrder)
            for (const auto &moduleInfo : packageInfo.modules)
                iids.insert(moduleInfo.iid);

        for (const auto &iid : iids) {
            const auto taskFactoryPlugin = mgr_.plugin<LangMgr::TaskFactoryPlugin>(iid.c_str());
            if (!taskFactoryPlugin) {
                std::cerr << "Failed to load FactoryPlugin: " << iid << std::endl;
                return false;
            }
            const auto &task = taskFactoryPlugin->create();
            auto &ic = *mgr_.category("engine");
            ic.addObject(iid, task);
        }

        std::vector<LangMgr::Package> loadedPackages;

        for (const auto &packageInfo : packageOrder) {
            std::cout << "Loading package: " << packageInfo.packageId << " from " << packageInfo.packagePath
                      << std::endl;

            auto exp = mgr_.open(packageInfo.packagePath);
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
        std::cout << "\nSuccessfully loaded " << loadedPackages.size() << " packages" << std::endl;
        return true;
    }

private:
    LangMgr::Manager &mgr_;
    EP ep_;
    int deviceIndex_;
    bool loadFromProgress_;
    std::unordered_map<std::string, LangMgr::NO<LangMgr::Task>> loadedTasks_;

    LangMgr::Expected<LangMgr::NO<LangMgr::Task>> createModuleTask(const LangMgr::ModuleMetadata &moduleInfo,
                                                                   const LangMgr::Package &pkg) const {
        const auto moduleSpec = pkg.moduleSpec(moduleInfo.type, moduleInfo.moduleId);
        if (!moduleSpec) {
            return LangMgr::Error(LangMgr::Error::FileNotFound,
                                  stdc::formatN("Module %1 not found in package %2", moduleInfo.moduleId, pkg.id()));
        }

        const auto &moduleCategory = *mgr_.category("engine");
        const auto taskFactory = moduleCategory.getFirstObject(moduleInfo.iid).as<LangMgr::TaskFactory>();
        if (!taskFactory) {
            return LangMgr::Error(LangMgr::Error::InterpreterNotFound,
                                  stdc::formatN("%1 task Engine not found", moduleSpec->id()));
        }

        const auto runtimeOptions = LangMgr::NO<LangMgr::TaskRuntimeOptions>::create(
            moduleSpec->id(), moduleSpec->className(), moduleSpec->apiLevel());

        auto taskExp = taskFactory->createTask(moduleSpec, runtimeOptions);
        if (!taskExp) {
            return LangMgr::Error(LangMgr::Error::InvalidArgument,
                                  stdc::formatN("Failed to create task: %1", taskExp.error().message()));
        }

        auto task = taskExp.take();

        const auto initArgs = LangMgr::NO<LangMgr::TaskInitArgs>::create(moduleSpec->id(), moduleSpec->className(),
                                                                         moduleSpec->apiLevel());
        if (const auto exp = task->initialize(initArgs); !exp) {
            return LangMgr::Error(LangMgr::Error::InvalidArgument,
                                  stdc::formatN("Failed to initialize task: %1", exp.error().message()));
        }

        auto &ic = *mgr_.category(moduleSpec->category().c_str());
        ic.addObject(moduleSpec->id(), task);
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

int main() {
    try {
        const EP g2pProvider = parseExecutionProvider("cpu");

        LangMgr::Manager langMgr;
        AutoModuleInitializer initializer(langMgr, g2pProvider, 0, false);

        if (const auto exp = initializer.initializeManager(); !exp) {
            return -1;
        }

        const std::filesystem::path packagesRootDir = R"(D:\projects\language-manager\tst_package)";
        if (const auto packagesExp = initializer.loadPackagesInOrder(packagesRootDir); !packagesExp)
            return -2;

        std::string errorMessage;
        langMgr.initialize(errorMessage);

        const auto text = "Halloween蝉声--陪かな伴着qwe行云流浪---ka回-忆-开始132后安静遥望远方;荒草覆没的古井--枯塘;"
                          "匀-散asdaw一缕过往";
        auto splitRes = langMgr.split(text);
        std::cout << "\nsplit result: "
                  << std::accumulate(splitRes.begin(), splitRes.end(), std::string(),
                                     [](const std::string &a, const std::string &b)
                                     { return a.empty() ? b : a + " " + b; })
                  << std::endl;

        const auto resExp = langMgr.tag(splitRes);

        std::vector<LangMgr::G2pInput *> g2pInput;
        std::cout << "tag result: " << std::endl;
        for (const auto &res : resExp) {
            std::cout << "lyric: " << res.lyric << " language: " << res.language << " tag: " << res.tag << std::endl;
            g2pInput.emplace_back(new LangMgr::G2pInput(res.lyric, res.language));
        }

        const auto g2pResult = langMgr.convert(g2pInput);

        for (auto g2pRes : g2pResult) {
            std::cout << "\nlyric: " << g2pRes.lyric << ";\npronunciation: '" << g2pRes.pronunciation
                      << "';\nmode: " << g2pRes.mode << std::endl
                      << std::endl;
        }

        return 0;
    }
    catch (const std::exception &e) {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
        return -4;
    }
}
