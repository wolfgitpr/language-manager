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

#ifdef WIN32
#include <Windows.h>
#endif

using EP = LangPlugins::Api::Onnx::ExecutionProvider;

static LangMgr::Expected<void> initializeMgr(LangMgr::LanguageManager &mgr, const EP ep, int deviceIndex,
                                             bool loadFromProgress = false) {
    // Get basic directories
    const auto pluginRootDir =
#if defined(Q_OS_MAC)
        MacOSUtils::getMainBundlePath() / _TSTR("Contents/PlugIns");
#elif defined(Q_OS_WIN)
        stdc::system::application_directory() / _TSTR("plugins");
#else
        stdc::system::application_directory().parent_path() / _TSTR("lib/plugins");
#endif
    const auto defaultPluginDir = pluginRootDir / _TSTR("LangPlugins");

    // Set default plugin directories
    mgr.addPluginPath("org.openvpi.InferenceDriver", defaultPluginDir / _TSTR("inferencedrivers"));
    mgr.addPluginPath("org.openvpi.InferenceInterpreter", defaultPluginDir / _TSTR("g2ps"));

    // Load ONNX driver
    const auto onnxDriverPlugin = mgr.plugin<LangPlugins::InferenceDriverPlugin>("onnx");
    if (!onnxDriverPlugin) {
        return LangMgr::Error(LangMgr::Error::FileNotOpen, "failed to load ONNX inference driver");
    }

    const auto onnxDriver = onnxDriverPlugin->create();
    const auto onnxArgs = LangMgr::NO<LangPlugins::Api::Onnx::DriverInitArgs>::create();

    onnxArgs->ep = ep;
    const auto ortParentPath = onnxDriverPlugin->path().parent_path() / _TSTR("runtimes") / _TSTR("onnx");
    if (ep == LangPlugins::Api::Onnx::CUDAExecutionProvider) {
        onnxArgs->runtimePath = ortParentPath / _TSTR("cuda");
    } else {
        onnxArgs->runtimePath = ortParentPath / _TSTR("default");
    }
    onnxArgs->loadFromProgress = loadFromProgress;
    onnxArgs->deviceIndex = deviceIndex;

    if (auto exp = onnxDriver->initialize(onnxArgs); !exp) {
        return LangMgr::Error(LangMgr::Error::FileNotOpen,
                              stdc::formatN(R"(failed to initialize onnx driver: %1)", exp.error().message()));
    }

    // Load LstmG2pInterpreter
    const auto lstmG2pInterpreterPlugin = mgr.plugin<LangMgr::InferenceInterpreterPlugin>("g2p.model.LstmG2pInference");
    if (!lstmG2pInterpreterPlugin) {
        return LangMgr::Error(LangMgr::Error::FileNotOpen, "failed to load LstmG2p interpreter plugin");
    }

    const auto lstmG2pInterpreter = lstmG2pInterpreterPlugin->create();

    // Add drivers and interpreters to manager
    auto &ic = *mgr.category("inference");
    ic.addObject("g2pOnnxDriver", onnxDriver);
    ic.addObject("lstmG2pInterpreter", lstmG2pInterpreter);
    return {};
}

int main() {
#ifdef WIN32
    const auto onnxRuntimePath = std::filesystem::current_path() /
        "../lib/plugins/LangPlugins/inferencedrivers/runtimes/onnx/default/onnxruntime.dll";
    const HMODULE hOnnxRuntime = LoadLibraryW(onnxRuntimePath.wstring().c_str());
    if (hOnnxRuntime == nullptr) {
        const DWORD error = GetLastError();
        std::cout << "Failed to load onnxruntime.dll from" << onnxRuntimePath << "Error code:" << error;
        return -1;
    }
    std::cout << "Successfully loaded onnxruntime.dll";
#endif

    const auto g2pProvider = [](const std::string &provider_) -> EP
    {
        const auto provider_lower = stdc::to_lower(provider_);
        if (provider_lower == "dml" || provider_lower == "directml") {
            return EP::DMLExecutionProvider;
        }
        if (provider_lower == "cuda") {
            return EP::CUDAExecutionProvider;
        }
        if (provider_lower == "coreml") {
            return EP::CoreMLExecutionProvider;
        }
        return EP::CPUExecutionProvider;
    }("cpu");

    LangMgr::LanguageManager langMgr;
    if (auto exp = initializeMgr(langMgr, g2pProvider, 0, true); !exp) {
        std::cerr << "failed to initialize LanguageManager: " << exp.error().message() << std::endl;
        return -1;
    }

    const auto modelBasePath = std::filesystem::path(R"(D:\projects\language-manager\tst_package\g2p-en)");

    LangMgr::InferenceSpec *g2pSpec = nullptr;
    LangMgr::ScopedPackageRef pkg;
    if (auto exp = langMgr.open(modelBasePath, false); !exp) {
        std::cerr << "failed to open model package: " << exp.error().message() << std::endl;
        return -1;
    } else {
        pkg = exp.take();
        const auto lstmG2pG2pContrib = pkg.contributes("inference");
        if (lstmG2pG2pContrib.empty()) {
            std::cerr << "no inference contributions found in package" << std::endl;
            return -1;
        }
        g2pSpec = dynamic_cast<LangMgr::InferenceSpec *>(lstmG2pG2pContrib.front());
        if (!g2pSpec) {
            std::cerr << "failed to cast to InferenceSpec" << std::endl;
            return -1;
        }
    }

    const auto &inferenceCategory = *langMgr.category("inference");
    const auto lstmG2pInterpreter =
        inferenceCategory.getFirstObject("lstmG2pInterpreter").as<LangMgr::InferenceInterpreter>();

    if (!lstmG2pInterpreter) {
        std::cerr << "LstmG2p interpreter not found" << std::endl;
        return -1;
    }

    LangMgr::JsonObject importOptionsJson;
    std::cout << "Found inference spec: " << g2pSpec->name().text() << std::endl;
    std::cout << "Class name: " << g2pSpec->className() << std::endl;
    std::cout << "API Level: " << g2pSpec->apiLevel() << std::endl;

    auto importOptionsExp = lstmG2pInterpreter->createImportOptions(g2pSpec, importOptionsJson);
    if (!importOptionsExp) {
        std::cerr << "Failed to create import options: " << importOptionsExp.error().message() << std::endl;
        return -1;
    }

    auto importOptions = importOptionsExp.take();
    auto runtimeOptions = LangMgr::NO<LangPlugins::Api::LstmG2p::L1::LstmG2pRuntimeOptions>::create();
    runtimeOptions->device = "cpu";
    runtimeOptions->optimizePerformance = false;

    auto inferenceExp = lstmG2pInterpreter->createInference(g2pSpec, importOptions, runtimeOptions);
    if (!inferenceExp) {
        std::cerr << "failed to create inference: " << inferenceExp.error().message() << std::endl;
        return -1;
    }
    auto inference = inferenceExp.take();

    auto initArgs = LangMgr::NO<LangPlugins::Api::LstmG2p::L1::LstmG2pInitArgs>::create();
    initArgs->runtimeOptions = runtimeOptions;

    if (auto exp = inference->initialize(initArgs); !exp) {
        std::cerr << "failed to initialize inference: " << exp.error().message() << std::endl;
        return -1;
    }

    std::cout << "Inference initialized successfully" << std::endl;

    auto input = LangMgr::NO<LangPlugins::Api::LstmG2p::L1::LstmG2pStartInput>::create();
    input->words.push_back(LangPlugins::Api::LstmG2p::L1::G2pWord{"hello"});
    input->returnDetailedInfo = true;

    std::cout << "Starting inference..." << std::endl;
    auto resultExp = inference->start(input);
    if (!resultExp) {
        std::cerr << "inference failed: " << resultExp.error().message() << std::endl;
        return -1;
    }

    auto result = resultExp.take();
    if (auto g2pResult = result.as<LangPlugins::Api::LstmG2p::L1::LstmG2pResult>()) {
        std::cout << "Input: hello" << std::endl;
        std::cout << "Phonemes: ";
        for (const auto &phoneme : g2pResult->phonemes) {
            std::cout << phoneme << " ";
        }
        std::cout << std::endl;

        if (!g2pResult->errorMessage.empty()) {
            std::cout << "Error: " << g2pResult->errorMessage << std::endl;
        }
    } else {
        std::cerr << "unexpected result type" << std::endl;
        return -1;
    }

    inference->stop();
    std::cout << "Inference completed successfully" << std::endl;

#ifdef WIN32
    if (hOnnxRuntime) {
        FreeLibrary(hOnnxRuntime);
        std::cout << "Released onnxruntime.dll";
    }
#endif
    return 0;
}
