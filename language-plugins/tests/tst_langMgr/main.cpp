#include <filesystem>
#include <iostream>
#include <string>

#include <stdcorelib/str.h>
#include <stdcorelib/system.h>

#include <LangMgr/Base/NamedObject.h>
#include <LangMgr/Core/Manager.h>
#include <LangMgr/Module/G2pModule.h>
#include <LangMgr/Module/Module.h>
#include <LangMgr/Package/Package.h>
#include <LangMgr/Task/Task.h>
#include <LangMgr/Task/TaskFactoryPlugin.h>

#include <LangPlugins/Api/Drivers/Onnx/1/OnnxDriverApiL1.h>
#include <LangPlugins/Api/G2ps/LstmG2p/1/LstmG2pL1.h>
#include "LangPlugins/Api/G2ps/TemplateG2p/1/TemplateG2pL1.h"

#ifdef WIN32
#include <Windows.h>
#endif

using EP = LangPlugins::Api::Onnx::L1::ExecutionProvider;

template <typename InferenceType>
class InferenceCreator {
public:
    template <typename RuntimeOptionsType, typename InitArgsType>
    static LangMgr::Expected<LangMgr::NO<LangMgr::Task>>
    create(LangMgr::ModuleCategory &inferenceCategory, const std::string &interpreterName,
           LangMgr::ModuleDefinition *inferenceSpec, const std::string &inferenceName) {
        const auto interpreter = inferenceCategory.getFirstObject(interpreterName).as<LangMgr::TaskFactory>();

        if (!interpreter) {
            return LangMgr::Error(LangMgr::Error::InterpreterNotFound,
                                  stdc::formatN(R"(%1 interpreter not found)", interpreterName));
        }

        auto runtimeOptions = LangMgr::NO<RuntimeOptionsType>::create();
        auto inferenceExp = interpreter->createTask(inferenceSpec, runtimeOptions);

        if (!inferenceExp) {
            return LangMgr::Error(LangMgr::Error::InvalidArgument,
                                  stdc::formatN(R"(failed to create inference: %1)", inferenceExp.error().message()));
        }

        auto inference = inferenceExp.take();
        auto initArgs = LangMgr::NO<InitArgsType>::create();

        if constexpr (std::is_member_pointer_v<decltype(&InitArgsType::runtimeOptions)>) {
            initArgs->runtimeOptions = runtimeOptions;
        }

        if (auto exp = inference->initialize(initArgs); !exp) {
            return LangMgr::Error(LangMgr::Error::InvalidArgument,
                                  stdc::formatN(R"(failed to initialize inference: %1)", exp.error().message()));
        }

        inferenceCategory.addObject(inferenceName, inference);
        return inference;
    }
};

template <typename InferenceType>
LangMgr::Expected<LangMgr::NO<LangMgr::Task>>
createSpecificInference(LangMgr::ModuleCategory &inferenceCategory,
                        const typename InferenceType::ModuleDefinition &inferenceSpec) {
    return InferenceCreator<InferenceType>::template create<typename InferenceType::RuntimeOptions,
                                                            typename InferenceType::InitArgs>(
        inferenceCategory, InferenceType::InterpreterName, inferenceSpec, InferenceType::InferenceName);
}

struct LstmG2pTraits {
    using RuntimeOptions = LangPlugins::Api::LstmG2p::L1::LstmG2pRuntimeOptions;
    using InitArgs = LangPlugins::Api::LstmG2p::L1::LstmG2pInitArgs;
    using ModuleDefinition = LangMgr::ModuleDefinition *;
    static constexpr auto InterpreterName = "lstmG2pInterpreter";
    static constexpr auto InferenceName = "lstmG2pInference";
};

struct TemplateG2pTraits {
    using RuntimeOptions = LangPlugins::Api::TemplateG2p::L1::TemplateG2pRuntimeOptions;
    using InitArgs = LangPlugins::Api::TemplateG2p::L1::TemplateG2pInitArgs;
    using ModuleDefinition = LangMgr::ModuleDefinition *;
    static constexpr auto InterpreterName = "templateG2pInterpreter";
    static constexpr auto InferenceName = "templateG2pInference";
};

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

std::filesystem::path getPluginRootDirectory() {
#if defined(Q_OS_MAC)
    return MacOSUtils::getMainBundlePath() / _TSTR("Contents/PlugIns");
#elif defined(Q_OS_WIN)
    return stdc::system::application_directory() / _TSTR("plugins");
#else
    return stdc::system::application_directory().parent_path() / _TSTR("lib/plugins");
#endif
}

LangMgr::Expected<LangMgr::NO<LangMgr::SessionFactory>>
initializeOnnxDriver(const LangMgr::Manager &mgr, const EP ep, const int deviceIndex, const bool loadFromProgress) {
    const auto onnxDriverPlugin = mgr.plugin<LangMgr::DriverFactoryPlugin>("onnx");
    if (!onnxDriverPlugin) {
        return LangMgr::Error(LangMgr::Error::FileNotOpen, "failed to load ONNX inference driver");
    }

    const auto onnxDriver = onnxDriverPlugin->create();
    const auto onnxArgs = LangMgr::NO<LangPlugins::Api::Onnx::L1::DriverInitArgs>::create();

    onnxArgs->ep = ep;
    const auto ortParentPath = onnxDriverPlugin->path().parent_path() / _TSTR("runtimes") / _TSTR("onnx");

    onnxArgs->runtimePath = ep == LangPlugins::Api::Onnx::L1::CUDAExecutionProvider ? ortParentPath / _TSTR("cuda")
                                                                                    : ortParentPath / _TSTR("default");

    onnxArgs->loadFromProgress = loadFromProgress;
    onnxArgs->deviceIndex = deviceIndex;

    if (const auto exp = onnxDriver->initialize(onnxArgs); !exp) {
        return LangMgr::Error(LangMgr::Error::FileNotOpen,
                              stdc::formatN(R"(failed to initialize onnx driver: %1)", exp.error().message()));
    }

    return onnxDriver;
}

LangMgr::Expected<LangMgr::NO<LangMgr::TaskFactory>>
loadInterpreter(const LangMgr::Manager &mgr, const std::string &pluginName, const std::string &errorMsg) {
    const auto plugin = mgr.plugin<LangMgr::TaskFactoryPlugin>(pluginName.c_str());
    if (!plugin) {
        return LangMgr::Error(LangMgr::Error::FileNotOpen, errorMsg);
    }
    return plugin->create();
}

LangMgr::Expected<void> initializeMgr(LangMgr::Manager &mgr, const EP ep, const int deviceIndex,
                                      const bool loadFromProgress) {
    const auto pluginRootDir = getPluginRootDirectory();
    const auto defaultPluginDir = pluginRootDir / _TSTR("LangPlugins");

    mgr.addPluginPath("org.openvpi.DriverFactory", defaultPluginDir / _TSTR("InferenceDrivers"));
    mgr.addPluginPath("org.openvpi.TaskFactory", defaultPluginDir / _TSTR("G2ps"));

    auto onnxDriverExp = initializeOnnxDriver(mgr, ep, deviceIndex, loadFromProgress);
    if (!onnxDriverExp) {
        return onnxDriverExp.error();
    }

    auto &inferenceCategory = *mgr.category("driver");
    inferenceCategory.addObject("g2pOnnxDriver", onnxDriverExp.take());

    struct InterpreterInfo {
        std::string pluginName;
        std::string objectName;
        std::string errorMsg;
    };

    const InterpreterInfo interpreters[] = {
        {"g2p.model.LstmG2pInference", "lstmG2pInterpreter", "failed to load LstmG2p interpreter plugin"},
        {"g2p.template.TemplateInference", "templateG2pInterpreter", "failed to load TemplateG2p interpreter plugin"}};

    auto &g2pCategory = *mgr.category("g2p");

    for (const auto &[pluginName, objectName, errorMsg] : interpreters) {
        auto interpreterExp = loadInterpreter(mgr, pluginName, errorMsg);
        if (!interpreterExp) {
            return interpreterExp.error();
        }
        g2pCategory.addObject(objectName, interpreterExp.take());
    }

    return {};
}

LangMgr::Package loadPackage(LangMgr::Manager &langMgr, const std::filesystem::path &packagePath) {
    langMgr.addPackagePath(packagePath.parent_path());

    auto exp = langMgr.open(packagePath, false);
    if (!exp) {
        throw std::runtime_error(
            stdc::formatN(R"(failed to open package "%1": %2)", packagePath, exp.error().message()));
    }

    LangMgr::Package pkg = exp.take();
    if (!pkg.isLoaded()) {
        throw std::runtime_error(
            stdc::formatN(R"(failed to load package "%1": %2)", packagePath, pkg.error().message()));
    }
    return pkg;
}

void processG2pImports(const LangMgr::G2pCategory &inferenceCate, LangMgr::ModuleDefinition *&lstmSpec,
                       LangMgr::ModuleDefinition *&templateSpec) {
    struct ImportEntry {
        std::string_view className;
        std::string_view apiName;
        LangMgr::ModuleDefinition *&spec;
    };

    ImportEntry imports[] = {
        {LangPlugins::Api::LstmG2p::L1::API_CLASS, LangPlugins::Api::LstmG2p::L1::API_NAME, lstmSpec},
        {LangPlugins::Api::TemplateG2p::L1::API_CLASS, LangPlugins::Api::TemplateG2p::L1::API_NAME, templateSpec},
    };

    for (const auto inference : inferenceCate.definitions()) {
        const auto &cls = inference->className();
        for (const auto &entry : imports) {
            if (cls == entry.className) {
                entry.spec = inference;
                break;
            }
        }
    }

    for (const auto &entry : imports) {
        if (!entry.spec) {
            throw std::runtime_error(
                stdc::formatN(R"(%1 inference not found for g2p "%2")", entry.apiName, entry.spec->className()));
        }
    }
}

LangMgr::Expected<LangMgr::NO<LangMgr::Task>> createLstmG2pInference(LangMgr::ModuleCategory &inferenceCategory,
                                                                     LangMgr::ModuleDefinition *lstmSpec) {
    return createSpecificInference<LstmG2pTraits>(inferenceCategory, lstmSpec);
}

LangMgr::Expected<LangMgr::NO<LangMgr::Task>> createTemplateG2pInference(LangMgr::ModuleCategory &inferenceCategory,
                                                                         LangMgr::ModuleDefinition *templateSpec) {
    return createSpecificInference<TemplateG2pTraits>(inferenceCategory, templateSpec);
}

void executeTemplateInference(const LangMgr::NO<LangMgr::Task> &templateInference) {
    const auto input = LangMgr::NO<LangPlugins::Api::TemplateG2p::L1::TemplateG2pStartInput>::create();
    input->g2pInput = {LangPlugins::Api::Common::L1::G2pInput({"hello", "eng"}),
                       LangPlugins::Api::Common::L1::G2pInput({"hellobazhahei", "eng"})};

    std::cout << "Starting inference - Id: " << templateInference->spec()->as<LangMgr::G2pDefinition>()->name().text()
              << std::endl;
    auto resultExp = templateInference->start(input);
    if (!resultExp)
        throw std::runtime_error(stdc::formatN("inference failed: %1", resultExp.error().message()));

    const auto result = resultExp.take();
    if (const auto g2pResult = result.as<LangPlugins::Api::TemplateG2p::L1::TemplateG2pResult>()) {
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

template <typename Func, typename... Args>
auto handleError(Func func, const std::string &errorPrefix, Args &&...args) {
    auto result = func(std::forward<Args>(args)...);
    if (!result) {
        std::cerr << errorPrefix << ": " << result.error().message() << std::endl;
    }
    return result;
}

int main() {
    const EP g2pProvider = parseExecutionProvider("cpu");

    LangMgr::Manager langMgr;
    if (const auto exp = handleError(initializeMgr, "Failed to initialize Manager", langMgr, g2pProvider, 0, false);
        !exp) {
        return -1;
    }

    const auto packagePath = std::filesystem::path(R"(D:\projects\language-manager\tst_package\Phonetic-Suite-Eng)");
    LangMgr::Package pkg = loadPackage(langMgr, packagePath);

    auto &inferenceCategory = *langMgr.category("g2p");

    LangMgr::ModuleDefinition *importLstm = nullptr, *importTemplate = nullptr;
    processG2pImports(*inferenceCategory.as<LangMgr::G2pCategory>(), importLstm, importTemplate);

    auto lstmInferenceExp =
        handleError(createLstmG2pInference, "Failed to create LSTM G2P inference", inferenceCategory, importLstm);
    if (!lstmInferenceExp) {
        return -1;
    }
    const auto lstmInference = lstmInferenceExp.take();

    auto templateInferenceExp = handleError(createTemplateG2pInference, "Failed to create Template G2P inference",
                                            inferenceCategory, importTemplate);
    if (!templateInferenceExp) {
        return -1;
    }
    const auto templateInference = templateInferenceExp.take();

    // Starting inference
    executeTemplateInference(templateInference);

    lstmInference->stop();
    std::cout << "G2pTask completed successfully" << std::endl;
    return 0;
}
