#include <filesystem>
#include <iostream>
#include <string>

#include <LangPlugins/Api/Drivers/Onnx/OnnxDriverApi.h>
#include <LangPlugins/Inference/InferenceDriver.h>
#include <LangPlugins/Inference/InferenceDriverPlugin.h>
#include <stdcorelib/str.h>

#include <LangMgr/Core/Contribute.h>
#include <LangMgr/Core/LanguageManager.h>
#include <LangMgr/Core/NamedObject.h>
#include <stdcorelib/system.h>

#include <LangMgr/Tool/Inference.h>
#include <LangMgr/Tool/InferenceContrib.h>
#include <LangMgr/Tool/InferenceInterpreterPlugin.h>
#include <LangPlugins/Api/Inferences/LstmG2p/1/LstmG2pL1.h>

#include <LangMgr/Core/PackageRef.h>

#include "../../plugins/g2ps/TemplateG2p/TemplateG2pInference.h"
#include "../../plugins/g2ps/lstmG2p/LstmG2pInference.h"
#include "LangMgr/Tool/G2pContrib.h"
#include "LangPlugins/Api/Inferences/TemplateG2p/1/TemplateG2pL1.h"

#ifdef WIN32
#include <Windows.h>
#endif

using EP = LangPlugins::Api::Onnx::ExecutionProvider;

struct ImportData {
    LangMgr::NO<LangMgr::InferenceImportOptions> options;
    LangMgr::InferenceSpec *inference = nullptr;
};

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
                        const typename InferenceType::ImportData &importData) {
    return InferenceCreator<InferenceType>::template create<typename InferenceType::RuntimeOptions,
                                                            typename InferenceType::InitArgs>(
        inferenceCategory, InferenceType::InterpreterName, importData.inference, InferenceType::InferenceName);
}

struct LstmG2pTraits {
    using RuntimeOptions = LangPlugins::Api::LstmG2p::L1::LstmG2pRuntimeOptions;
    using InitArgs = LangPlugins::Api::LstmG2p::L1::LstmG2pInitArgs;
    using ImportData = ImportData;
    static constexpr auto InterpreterName = "lstmG2pInterpreter";
    static constexpr auto InferenceName = "lstmG2pInference";
};

struct TemplateG2pTraits {
    using RuntimeOptions = LangPlugins::Api::TemplateG2p::L1::TemplateG2pRuntimeOptions;
    using InitArgs = LangPlugins::Api::TemplateG2p::L1::TemplateG2pInitArgs;
    using ImportData = ImportData;
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
    const auto onnxArgs = LangMgr::NO<LangPlugins::Api::Onnx::DriverInitArgs>::create();

    onnxArgs->ep = ep;
    const auto ortParentPath = onnxDriverPlugin->path().parent_path() / _TSTR("runtimes") / _TSTR("onnx");

    onnxArgs->runtimePath = ep == LangPlugins::Api::Onnx::CUDAExecutionProvider ? ortParentPath / _TSTR("cuda")
                                                                                : ortParentPath / _TSTR("default");

    onnxArgs->loadFromProgress = loadFromProgress;
    onnxArgs->deviceIndex = deviceIndex;

    if (auto exp = onnxDriver->initialize(onnxArgs); !exp) {
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

    mgr.addPluginPath("org.openvpi.G2pProvider", defaultPluginDir / _TSTR("g2pProviders"));
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

const LangMgr::G2pSpec *findG2pSpec(const LangMgr::G2pCategory &g2pCategory, const std::string &g2pId) {
    for (const auto &g2p : g2pCategory.g2pSpecs()) {
        if (g2p->id() == g2pId) {
            return g2p;
        }
    }
    return nullptr;
}

void processG2pImports(const LangMgr::G2pSpec *g2pSpec, ImportData &lstmImport, ImportData &templateImport) {
    struct ImportEntry {
        std::string_view className;
        std::string_view apiName;
        ImportData *data;
    };

    const ImportEntry imports[] = {
        {LangPlugins::Lstm::API_CLASS, LangPlugins::Lstm::API_NAME, &lstmImport},
        {LangPlugins::Template::API_CLASS, LangPlugins::Template::API_NAME, &templateImport},
    };

    for (const auto &imp : g2pSpec->imports()) {
        const auto &cls = imp.inference()->className();
        for (const auto &entry : imports) {
            if (cls == entry.className) {
                *entry.data = {imp.options(), imp.inference()};
                break;
            }
        }
    }

    for (const auto &entry : imports) {
        if (!entry.data->inference) {
            throw std::runtime_error(
                stdc::formatN(R"(%1 inference not found for g2p "%2")", entry.apiName, "templateG2pId"));
        }
    }
}

LangMgr::Expected<LangMgr::NO<LangMgr::Inference>> createLstmG2pInference(LangMgr::ContribCategory &inferenceCategory,
                                                                          const ImportData &importData) {
    return createSpecificInference<LstmG2pTraits>(inferenceCategory, importData);
}

LangMgr::Expected<LangMgr::NO<LangMgr::Inference>>
createTemplateG2pInference(LangMgr::ContribCategory &inferenceCategory, const ImportData &importData) {
    return createSpecificInference<TemplateG2pTraits>(inferenceCategory, importData);
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
        if (auto exp =
                handleError(initializeMgr, "Failed to initialize LanguageManager", langMgr, g2pProvider, 0, false);
            !exp) {
            return -1;
        }

        const auto packagePath = std::filesystem::path(R"(D:\projects\language-manager\tst_package\g2p-template-eng)");
        LangMgr::PackageRef pkg = loadPackage(langMgr, packagePath);

        const auto &g2pCategory = *langMgr.category("g2p")->as<LangMgr::G2pCategory>();
        const LangMgr::G2pSpec *g2pSpec = findG2pSpec(g2pCategory, "eng");

        if (!g2pSpec) {
            throw std::runtime_error(stdc::formatN(R"(g2p "%1" not found in package)", "templateG2pId"));
        }

        ImportData importLstm, importTemplate;
        processG2pImports(g2pSpec, importLstm, importTemplate);

        auto &inferenceCategory = *langMgr.category("inference");

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

        std::cout << "Found inference spec: " << g2pSpec->name().text() << std::endl;
        std::cout << "API Level: " << g2pSpec->apiLevel() << std::endl;

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
