#include <filesystem>
#include <iostream>
#include <string>

#include <LangPlugins/Api/Drivers/Onnx/1/OnnxDriverApiL1.h>
#include <re2/re2.h>
#include <stdcorelib/str.h>

#include <LangMgr/Core/Manager.h>
#include <LangMgr/Core/Module.h>
#include <LangMgr/Core/NamedObject.h>
#include <stdcorelib/system.h>

#include <LangMgr/Modules/EngineFactoryPlugin.h>
#include <LangMgr/Modules/G2pModule.h>
#include <LangMgr/Task/Task.h>
#include <LangPlugins/Api/Inferences/RegexSplitter/1/RegexSplitterL1.h>

#include <LangMgr/Core/Package.h>

using EP = LangPlugins::Api::Onnx::L1::ExecutionProvider;

static LangMgr::Expected<void> initializeMgr(LangMgr::Manager &mgr) {
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
    mgr.addPluginPath("org.openvpi.EngineFactory", defaultPluginDir / _TSTR("g2ps"));
    mgr.addPluginPath("org.openvpi.EngineFactory", defaultPluginDir / _TSTR("spliters"));

    // Load RegexSpliterInterpreter
    const auto regexSpliterInterpreterPlugin =
        mgr.plugin<LangMgr::EngineFactoryPlugin>("spliter.regex.RegexSpliterInference");
    if (!regexSpliterInterpreterPlugin) {
        return LangMgr::Error(LangMgr::Error::FileNotOpen, "failed to load RegexSplitter interpreter plugin");
    }

    const auto regexSpliterInterpreter = regexSpliterInterpreterPlugin->create();

    // Add drivers and interpreters to manager
    auto &ic = *mgr.category("inference");
    ic.addObject("regexSpliterInterpreter", regexSpliterInterpreter);
    return {};
}

int main() {
    LangMgr::Manager langMgr;
    if (const auto exp = initializeMgr(langMgr); !exp) {
        std::cerr << "failed to initialize Manager: " << exp.error().message() << std::endl;
        return -1;
    }

    const auto modelBasePath = std::filesystem::path(R"(D:\projects\language-manager\tst_package)");

    const LangMgr::G2pDefinition *spliterSpec = nullptr;
    std::vector<LangMgr::Package> pkgs;

    auto loadPackage = [&](const std::filesystem::path &path)
    {
        if (auto exp = langMgr.open(path, false); !exp) {
            std::cerr << "failed to open model package: " << exp.error().message() << std::endl;
            return false;
        } else {
            const auto pkg = exp.take();
            pkgs.push_back(pkg);
            const auto regexSpliterG2pContrib = pkg.moduleSpecs("inference");
            if (regexSpliterG2pContrib.empty()) {
                std::cerr << "no inference contributions found in package" << std::endl;
                return false;
            }
            spliterSpec = dynamic_cast<LangMgr::G2pDefinition *>(regexSpliterG2pContrib.front());
            if (!spliterSpec) {
                std::cerr << "failed to cast to InferenceDefinition" << std::endl;
                return false;
            }
        }
        return true;
    };

    // loadPackage(modelBasePath / "spliter-cmn");
    loadPackage(modelBasePath / "spliter-num");

    const auto &inferenceCategory = *langMgr.category("inference");
    const auto regexSpliterInterpreter =
        inferenceCategory.getFirstObject("regexSpliterInterpreter").as<LangMgr::EngineFactory>();

    if (!regexSpliterInterpreter) {
        std::cerr << "RegexSplitter interpreter not found" << std::endl;
        return -1;
    }

    std::cout << "Found inference spec: " << spliterSpec->name().text() << std::endl;
    std::cout << "Class name: " << spliterSpec->className() << std::endl;
    std::cout << "API Level: " << spliterSpec->apiLevel() << std::endl;

    const auto runtimeOptions = LangMgr::NO<LangPlugins::Api::RegexSplitter::L1::RegexSplitterRuntimeOptions>::create();

    auto inferenceExp = regexSpliterInterpreter->createTask(spliterSpec, runtimeOptions);
    if (!inferenceExp) {
        std::cerr << "failed to create inference: " << inferenceExp.error().message() << std::endl;
        return -1;
    }
    const auto inference = inferenceExp.take();

    const auto initArgs = LangMgr::NO<LangPlugins::Api::RegexSplitter::L1::RegexSplitterInitArgs>::create();

    if (const auto exp = inference->initialize(initArgs); !exp) {
        std::cerr << "failed to initialize inference: " << exp.error().message() << std::endl;
        return -1;
    }

    std::cout << "Inference initialized successfully" << std::endl;

    const auto input = LangMgr::NO<LangPlugins::Api::RegexSplitter::L1::RegexSplitterStartInput>::create();
    input->rawStrVec = {u8"你好hello1加23"};

    std::cout << "Starting inference..." << std::endl;
    auto resultExp = inference->start(input);
    if (!resultExp) {
        std::cerr << "inference failed: " << resultExp.error().message() << std::endl;
        return -1;
    }

    const auto result = resultExp.take();
    if (const auto spliterResult = result.as<LangPlugins::Api::RegexSplitter::L1::RegexSplitterResult>()) {
        std::cout << "Input: " << input->rawStrVec.front() << std::endl;
        std::cout << "Res: ";
        for (const auto &resStr : spliterResult->resStrVec) {
            std::cout << resStr << " ";
        }
        std::cout << std::endl;

        if (!spliterResult->errorMessage.empty()) {
            std::cout << "Error: " << spliterResult->errorMessage << std::endl;
        }
    } else {
        std::cerr << "unexpected result type" << std::endl;
        return -1;
    }

    inference->stop();
    std::cout << "Inference completed successfully" << std::endl;
    return 0;
}
