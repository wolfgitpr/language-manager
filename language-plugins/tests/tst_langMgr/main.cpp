#include <filesystem>
#include <iostream>
#include <string>

#include <LangPlugins/Api/Drivers/Onnx/1/OnnxDriverApiL1.h>
#include <LangPlugins/Inference/InferenceDriver.h>
#include <LangPlugins/Inference/InferenceDriverPlugin.h>
#include <stdcorelib/str.h>

#include <LangMgr/Core/Contribute.h>
#include <LangMgr/Core/LanguageManager.h>
#include <LangMgr/Core/NamedObject.h>
#include <stdcorelib/system.h>

#include <LangMgr/Core/PackageRef.h>

#include <LangMgr/Tool/Inference.h>
#include <LangMgr/Tool/InferenceContrib.h>
#include <LangMgr/Tool/InferenceInterpreterPlugin.h>

#include <LangPlugins/Api/Inferences/LstmG2p/1/LstmG2pL1.h>
#include "LangPlugins/Api/Inferences/TemplateG2p/1/TemplateG2pL1.h"

#ifdef WIN32
#include <Windows.h>
#endif

using EP = LangPlugins::Api::Onnx::L1::ExecutionProvider;

template <typename InferenceType>
class InferenceCreator {
public:
    template <typename RuntimeOptionsType, typename InitArgsType>
    static LangMgr::Expected<LangMgr::NO<LangMgr::Inference>>
    create(LangMgr::ContribCategory &inferenceCategory, const std::string &interpreterName,
           LangMgr::InferenceSpec *inferenceSpec, const std::string &inferenceName) {
        const auto interpreter = inferenceCategory.getFirstObject(interpreterName).as<LangMgr::InferenceInterpreter>();

        if (!interpreter) {
            return LangMgr::Error(LangMgr::Error::InterpreterNotFound,
                                  stdc::formatN(R"(%1 interpreter not found)", interpreterName));
        }

        const LangMgr::JsonObject importOptionsJson;
        auto importOptionsExp = interpreter->createImportOptions(inferenceSpec, importOptionsJson);
        if (!importOptionsExp) {
            return LangMgr::Error(
                LangMgr::Error::InvalidArgument,
                stdc::formatN(R"(Failed to create import options: %1)", importOptionsExp.error().message()));
        }

        auto importOptions = importOptionsExp.take();
        auto runtimeOptions = LangMgr::NO<RuntimeOptionsType>::create();
        auto inferenceExp = interpreter->createInference(inferenceSpec, importOptions, runtimeOptions);

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
LangMgr::Expected<LangMgr::NO<LangMgr::Inference>>
createSpecificInference(LangMgr::ContribCategory &inferenceCategory,
                        const typename InferenceType::InferenceSpec &inferenceSpec) {
    return InferenceCreator<InferenceType>::template create<typename InferenceType::RuntimeOptions,
                                                            typename InferenceType::InitArgs>(
        inferenceCategory, InferenceType::InterpreterName, inferenceSpec, InferenceType::InferenceName);
}

struct LstmG2pTraits {
    using RuntimeOptions = LangPlugins::Api::LstmG2p::L1::LstmG2pRuntimeOptions;
    using InitArgs = LangPlugins::Api::LstmG2p::L1::LstmG2pInitArgs;
    using InferenceSpec = LangMgr::InferenceSpec *;
    static constexpr auto InterpreterName = "lstmG2pInterpreter";
    static constexpr auto InferenceName = "lstmG2pInference";
};

struct TemplateG2pTraits {
    using RuntimeOptions = LangPlugins::Api::TemplateG2p::L1::TemplateG2pRuntimeOptions;
    using InitArgs = LangPlugins::Api::TemplateG2p::L1::TemplateG2pInitArgs;
    using InferenceSpec = LangMgr::InferenceSpec *;
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

LangMgr::Expected<LangMgr::NO<LangPlugins::InferenceDriver>> initializeOnnxDriver(const LangMgr::LanguageManager &mgr,
                                                                                  const EP ep, const int deviceIndex,
                                                                                  const bool loadFromProgress) {
    const auto onnxDriverPlugin = mgr.plugin<LangPlugins::InferenceDriverPlugin>("onnx");
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

LangMgr::Expected<LangMgr::NO<LangMgr::InferenceInterpreter>>
loadInterpreter(const LangMgr::LanguageManager &mgr, const std::string &pluginName, const std::string &errorMsg) {
    const auto plugin = mgr.plugin<LangMgr::InferenceInterpreterPlugin>(pluginName.c_str());
    if (!plugin) {
        return LangMgr::Error(LangMgr::Error::FileNotOpen, errorMsg);
    }
    return plugin->create();
}

LangMgr::Expected<void> initializeMgr(LangMgr::LanguageManager &mgr, const EP ep, const int deviceIndex,
                                      const bool loadFromProgress) {
    const auto pluginRootDir = getPluginRootDirectory();
    const auto defaultPluginDir = pluginRootDir / _TSTR("LangPlugins");

    mgr.addPluginPath("org.openvpi.InferenceDriver", defaultPluginDir / _TSTR("inferencedrivers"));
    mgr.addPluginPath("org.openvpi.InferenceInterpreter", defaultPluginDir / _TSTR("g2ps"));

    auto onnxDriverExp = initializeOnnxDriver(mgr, ep, deviceIndex, loadFromProgress);
    if (!onnxDriverExp) {
        return onnxDriverExp.error();
    }

    struct InterpreterInfo {
        std::string pluginName;
        std::string objectName;
        std::string errorMsg;
    };

    const InterpreterInfo interpreters[] = {
        {"g2p.model.LstmG2pInference", "lstmG2pInterpreter", "failed to load LstmG2p interpreter plugin"},
        {"g2p.template.TemplateInference", "templateG2pInterpreter", "failed to load TemplateG2p interpreter plugin"}};

    auto &inferenceCategory = *mgr.category("inference");
    inferenceCategory.addObject("g2pOnnxDriver", onnxDriverExp.take());

    for (const auto &[pluginName, objectName, errorMsg] : interpreters) {
        auto interpreterExp = loadInterpreter(mgr, pluginName, errorMsg);
        if (!interpreterExp) {
            return interpreterExp.error();
        }
        inferenceCategory.addObject(objectName, interpreterExp.take());
    }

    return {};
}

LangMgr::PackageRef loadPackage(LangMgr::LanguageManager &langMgr, const std::filesystem::path &packagePath) {
    langMgr.addPackagePath(packagePath.parent_path());

    auto exp = langMgr.open(packagePath, false);
    if (!exp) {
        throw std::runtime_error(
            stdc::formatN(R"(failed to open package "%1": %2)", packagePath, exp.error().message()));
    }

    LangMgr::PackageRef pkg = exp.take();
    if (!pkg.isLoaded()) {
        throw std::runtime_error(
            stdc::formatN(R"(failed to load package "%1": %2)", packagePath, pkg.error().message()));
    }
    return pkg;
}

void processG2pImports(const LangMgr::InferenceCategory &inferenceCate, LangMgr::InferenceSpec *&lstmSpec,
                       LangMgr::InferenceSpec *&templateSpec) {
    struct ImportEntry {
        std::string_view className;
        std::string_view apiName;
        LangMgr::InferenceSpec *&spec;
    };

    ImportEntry imports[] = {
        {LangPlugins::Api::LstmG2p::L1::API_CLASS, LangPlugins::Api::LstmG2p::L1::API_NAME, lstmSpec},
        {LangPlugins::Api::TemplateG2p::L1::API_CLASS, LangPlugins::Api::TemplateG2p::L1::API_NAME, templateSpec},
    };

    for (const auto inference : inferenceCate.inferences()) {
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

LangMgr::Expected<LangMgr::NO<LangMgr::Inference>> createLstmG2pInference(LangMgr::ContribCategory &inferenceCategory,
                                                                          LangMgr::InferenceSpec *lstmSpec) {
    return createSpecificInference<LstmG2pTraits>(inferenceCategory, lstmSpec);
}

LangMgr::Expected<LangMgr::NO<LangMgr::Inference>>
createTemplateG2pInference(LangMgr::ContribCategory &inferenceCategory, LangMgr::InferenceSpec *templateSpec) {
    return createSpecificInference<TemplateG2pTraits>(inferenceCategory, templateSpec);
}

void executeTemplateInference(const LangMgr::NO<LangMgr::Inference> &templateInference) {
    const auto input = LangMgr::NO<LangPlugins::Api::TemplateG2p::L1::TemplateG2pStartInput>::create();
    input->g2pInput = {LangPlugins::Api::Common::L1::G2pInput({"hello", "eng"}),
                       LangPlugins::Api::Common::L1::G2pInput({"hellobazhahei", "eng"})};

    std::cout << "Starting inference - Id: " << templateInference->spec()->name().text() << std::endl;
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
    try {
        const EP g2pProvider = parseExecutionProvider("cpu");

        LangMgr::LanguageManager langMgr;
        if (const auto exp =
                handleError(initializeMgr, "Failed to initialize LanguageManager", langMgr, g2pProvider, 0, false);
            !exp) {
            return -1;
        }

        const auto packagePath = std::filesystem::path(R"(D:\projects\language-manager\tst_package\g2p-template-eng)");
        LangMgr::PackageRef pkg = loadPackage(langMgr, packagePath);

        auto &inferenceCategory = *langMgr.category("inference");

        LangMgr::InferenceSpec *importLstm = nullptr, *importTemplate = nullptr;
        processG2pImports(*inferenceCategory.as<LangMgr::InferenceCategory>(), importLstm, importTemplate);

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
        std::cout << "Inference completed successfully" << std::endl;
        return 0;
    }
    catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
}
