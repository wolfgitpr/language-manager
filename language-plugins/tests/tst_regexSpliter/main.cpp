#include <filesystem>
#include <iostream>
#include <string>

#include <LangPlugins/Api/Drivers/Onnx/1/OnnxDriverApiL1.h>
#include <re2/re2.h>
#include <stdcorelib/str.h>

#include <LangMgr/Core/Contribute.h>
#include <LangMgr/Core/LanguageManager.h>
#include <LangMgr/Core/NamedObject.h>
#include <stdcorelib/system.h>

#include <LangMgr/Tool/Inference.h>
#include <LangMgr/Tool/InferenceContrib.h>
#include <LangMgr/Tool/InferenceInterpreterPlugin.h>
#include <LangPlugins/Api/Inferences/RegexSpliter/1/RegexSpliterL1.h>

#include <LangMgr/Core/PackageRef.h>

using EP = LangPlugins::Api::Onnx::L1::ExecutionProvider;

static LangMgr::Expected<void> initializeMgr(LangMgr::LanguageManager &mgr) {
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
    mgr.addPluginPath("org.openvpi.InferenceInterpreter", defaultPluginDir / _TSTR("g2ps"));
    mgr.addPluginPath("org.openvpi.InferenceInterpreter", defaultPluginDir / _TSTR("spliters"));

    // Load RegexSpliterInterpreter
    const auto regexSpliterInterpreterPlugin =
        mgr.plugin<LangMgr::InferenceInterpreterPlugin>("spliter.regex.RegexSpliterInference");
    if (!regexSpliterInterpreterPlugin) {
        return LangMgr::Error(LangMgr::Error::FileNotOpen, "failed to load RegexSpliter interpreter plugin");
    }

    const auto regexSpliterInterpreter = regexSpliterInterpreterPlugin->create();

    // Add drivers and interpreters to manager
    auto &ic = *mgr.category("inference");
    ic.addObject("regexSpliterInterpreter", regexSpliterInterpreter);
    return {};
}

int main() {
    LangMgr::LanguageManager langMgr;
    if (auto exp = initializeMgr(langMgr); !exp) {
        std::cerr << "failed to initialize LanguageManager: " << exp.error().message() << std::endl;
        return -1;
    }

    const auto modelBasePath = std::filesystem::path(R"(D:\projects\language-manager\tst_package)");

    LangMgr::InferenceSpec *spliterSpec = nullptr;
    std::vector<LangMgr::PackageRef> pkgs;

    auto loadPackage = [&](const std::filesystem::path &path)
    {
        if (auto exp = langMgr.open(path, false); !exp) {
            std::cerr << "failed to open model package: " << exp.error().message() << std::endl;
            return false;
        } else {
            const auto pkg = exp.take();
            pkgs.push_back(pkg);
            const auto regexSpliterG2pContrib = pkg.contributes("inference");
            if (regexSpliterG2pContrib.empty()) {
                std::cerr << "no inference contributions found in package" << std::endl;
                return false;
            }
            spliterSpec = dynamic_cast<LangMgr::InferenceSpec *>(regexSpliterG2pContrib.front());
            if (!spliterSpec) {
                std::cerr << "failed to cast to InferenceSpec" << std::endl;
                return false;
            }
        }
        return true;
    };

    // loadPackage(modelBasePath / "spliter-cmn");
    loadPackage(modelBasePath / "spliter-num");

    const auto &inferenceCategory = *langMgr.category("inference");
    const auto regexSpliterInterpreter =
        inferenceCategory.getFirstObject("regexSpliterInterpreter").as<LangMgr::InferenceInterpreter>();

    if (!regexSpliterInterpreter) {
        std::cerr << "RegexSpliter interpreter not found" << std::endl;
        return -1;
    }

    std::cout << "Found inference spec: " << spliterSpec->name().text() << std::endl;
    std::cout << "Class name: " << spliterSpec->className() << std::endl;
    std::cout << "API Level: " << spliterSpec->apiLevel() << std::endl;

    auto runtimeOptions = LangMgr::NO<LangPlugins::Api::RegexSpliter::L1::RegexSpliterRuntimeOptions>::create();

    auto inferenceExp = regexSpliterInterpreter->createInference(spliterSpec, runtimeOptions);
    if (!inferenceExp) {
        std::cerr << "failed to create inference: " << inferenceExp.error().message() << std::endl;
        return -1;
    }
    auto inference = inferenceExp.take();

    auto initArgs = LangMgr::NO<LangPlugins::Api::RegexSpliter::L1::RegexSpliterInitArgs>::create();

    if (auto exp = inference->initialize(initArgs); !exp) {
        std::cerr << "failed to initialize inference: " << exp.error().message() << std::endl;
        return -1;
    }

    std::cout << "Inference initialized successfully" << std::endl;

    auto input = LangMgr::NO<LangPlugins::Api::RegexSpliter::L1::RegexSpliterStartInput>::create();
    input->rawStrVec = {u8"你好hello1加23"};

    std::cout << "Starting inference..." << std::endl;
    auto resultExp = inference->start(input);
    if (!resultExp) {
        std::cerr << "inference failed: " << resultExp.error().message() << std::endl;
        return -1;
    }

    auto result = resultExp.take();
    if (auto spliterResult = result.as<LangPlugins::Api::RegexSpliter::L1::RegexSpliterResult>()) {
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
