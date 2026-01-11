#include <filesystem>
#include <iostream>
#include <string>

#include <re2/re2.h>
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
#include <LangPlugins/Api/Splitters/RegexSplitter/1/RegexSplitterL1.h>


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
    mgr.addPluginPath("org.openvpi.TaskFactory", defaultPluginDir / _TSTR("g2ps"));
    mgr.addPluginPath("org.openvpi.TaskFactory", defaultPluginDir / _TSTR("splitters"));

    // Load RegexSplitterInterpreter
    const auto regexSplitterInterpreterPlugin =
        mgr.plugin<LangMgr::TaskFactoryPlugin>("splitter.regex.RegexSplitterInference");
    if (!regexSplitterInterpreterPlugin) {
        return LangMgr::Error(LangMgr::Error::FileNotOpen, "failed to load RegexSplitter interpreter plugin");
    }

    const auto regexSplitterInterpreter = regexSplitterInterpreterPlugin->create();

    // Add drivers and interpreters to manager
    auto &ic = *mgr.category("inference");
    ic.addObject("regexSplitterInterpreter", regexSplitterInterpreter);
    return {};
}

int main() {
    LangMgr::Manager langMgr;
    if (const auto exp = initializeMgr(langMgr); !exp) {
        std::cerr << "failed to initialize Manager: " << exp.error().message() << std::endl;
        return -1;
    }

    const auto modelBasePath = std::filesystem::path(R"(D:\projects\language-manager\tst_package)");

    const LangMgr::G2pDefinition *splitterSpec = nullptr;
    std::vector<LangMgr::Package> pkgs;

    auto loadPackage = [&](const std::filesystem::path &path)
    {
        if (auto exp = langMgr.open(path, false); !exp) {
            std::cerr << "failed to open model package: " << exp.error().message() << std::endl;
            return false;
        } else {
            const auto pkg = exp.take();
            pkgs.push_back(pkg);
            const auto regexSplitterG2pContrib = pkg.moduleSpecs("inference");
            if (regexSplitterG2pContrib.empty()) {
                std::cerr << "no inference contributions found in package" << std::endl;
                return false;
            }
            splitterSpec = dynamic_cast<LangMgr::G2pDefinition *>(regexSplitterG2pContrib.front());
            if (!splitterSpec) {
                std::cerr << "failed to cast to InferenceDefinition" << std::endl;
                return false;
            }
        }
        return true;
    };

    // loadPackage(modelBasePath / "splitter-cmn");
    loadPackage(modelBasePath / "splitter-num");

    const auto &inferenceCategory = *langMgr.category("inference");
    const auto regexSplitterInterpreter =
        inferenceCategory.getFirstObject("regexSplitterInterpreter").as<LangMgr::TaskFactory>();

    if (!regexSplitterInterpreter) {
        std::cerr << "RegexSplitter interpreter not found" << std::endl;
        return -1;
    }

    std::cout << "Found inference spec: " << splitterSpec->name().text() << std::endl;
    std::cout << "Class name: " << splitterSpec->className() << std::endl;
    std::cout << "API Level: " << splitterSpec->apiLevel() << std::endl;

    const auto runtimeOptions = LangMgr::NO<LangPlugins::Api::RegexSplitter::L1::RegexSplitterRuntimeOptions>::create();

    auto inferenceExp = regexSplitterInterpreter->createTask(splitterSpec, runtimeOptions);
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
    if (const auto splitterResult = result.as<LangPlugins::Api::RegexSplitter::L1::RegexSplitterResult>()) {
        std::cout << "Input: " << input->rawStrVec.front() << std::endl;
        std::cout << "Res: ";
        for (const auto &resStr : splitterResult->resStrVec) {
            std::cout << resStr << " ";
        }
        std::cout << std::endl;

        if (!splitterResult->errorMessage.empty()) {
            std::cout << "Error: " << splitterResult->errorMessage << std::endl;
        }
    } else {
        std::cerr << "unexpected result type" << std::endl;
        return -1;
    }

    inference->stop();
    std::cout << "Inference completed successfully" << std::endl;
    return 0;
}
