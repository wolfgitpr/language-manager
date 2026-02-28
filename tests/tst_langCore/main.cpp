#include <filesystem>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#include <stdcorelib/str.h>
#include <stdcorelib/system.h>

#include <LangCore/Core/Manager.h>
#include <LangCore/Module/Module.h>
#include <LangCore/Task/TaskFactoryPlugin.h>

#include <LangPlugins/Api/Drivers/Onnx/1/OnnxDriverApiL1.h>

using EP = LangPlugins::Api::Onnx::L1::ExecutionProvider;

std::filesystem::path getPluginRootDirectory() {
#if defined(Q_OS_MAC)
    return MacOSUtils::getMainBundlePath() / _TSTR("Contents/PlugIns");
#elif defined(Q_OS_WIN)
    return stdc::system::application_directory() / _TSTR("plugins");
#else
    return stdc::system::application_directory().parent_path() / _TSTR("lib/plugins");
#endif
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

bool initializeOnnxDriver(const LangCore::Manager *mgr, const std::string &ep, const int deviceIndex,
                          const bool loadFromProgress) {
    const auto onnxDriverPlugin = mgr->plugin<LangCore::DriverFactoryPlugin>("onnx");
    if (!onnxDriverPlugin) {
        std::cerr << "Failed to load ONNX inference driver" << std::endl;
        return false;
    }

    const auto onnxDriver = onnxDriverPlugin->create();
    const auto onnxArgs = LangCore::NO<LangPlugins::Api::Onnx::L1::DriverInitArgs>::create();

    const auto ep_ = parseExecutionProvider(ep);
    onnxArgs->ep = ep_;
    const auto ortParentPath = onnxDriverPlugin->path().parent_path() / _TSTR("runtimes") / _TSTR("onnx");
    onnxArgs->runtimePath = ep_ == LangPlugins::Api::Onnx::L1::CUDAExecutionProvider ? ortParentPath / _TSTR("cuda")
                                                                                     : ortParentPath / _TSTR("default");

    onnxArgs->loadFromProgress = loadFromProgress;
    onnxArgs->deviceIndex = deviceIndex;

    if (const auto exp = onnxDriver->initialize(onnxArgs); !exp) {
        std::cerr << "Failed to initialize ONNX driver: " << exp.error().message() << std::endl;
        return false;
    }

    auto &driverCategory = *mgr->category("driver");
    driverCategory.addObject("g2pOnnxDriver", onnxDriver);
    return true;
}

int main() {
    try {
        const auto langMgr = LangCore::Manager::instance();

        const auto defaultPluginDir = getPluginRootDirectory() / _TSTR("LangPlugins");
        langMgr->addPluginPath("org.openvpi.DriverFactory", defaultPluginDir / _TSTR("Drivers"));
        langMgr->addPluginPath("org.openvpi.TaskFactory", defaultPluginDir / _TSTR("G2ps"));
        langMgr->addPluginPath("org.openvpi.TaskFactory", defaultPluginDir / _TSTR("Taggers"));

        const std::filesystem::path packagesRootDir = R"(D:\projects\language-manager\res\G2pPackages)";
        langMgr->addPackagePath(packagesRootDir);

        if (const auto onnxDriverInitialized = initializeOnnxDriver(langMgr, "cpu", 0, false); !onnxDriverInitialized)
            return -1;

        std::string errorMessage;
        langMgr->initialize(errorMessage);
        if (!langMgr->initialized())
            std::cerr << "Failed to initialize langMgr: " << errorMessage << std::endl;

        const auto text = "halloween蝉 声--陪かな伴着qwe行云流浪---\nka回-忆-开始132后安静遥望远方"
                          "\n荒草覆没的古井--枯塘\n匀-散asdaw一缕过往\n";
        auto splitRes = langMgr->split(text);
        std::cout << "\nsplit result: "
                  << std::accumulate(splitRes.begin(), splitRes.end(), std::string(),
                                     [](const std::string &a, const std::string &b)
                                     { return a.empty() ? b : a + " " + b; })
                  << std::endl;

        const auto resExp = langMgr->tag(splitRes);

        std::vector<LangCore::G2pInput *> g2pInput;
        std::cout << "tag result: " << std::endl;
        for (const auto &res : resExp) {
            std::cout << "lyric: '" << res.lyric << "' language: " << res.language << " tag: " << res.tag << std::endl;
            g2pInput.emplace_back(new LangCore::G2pInput(res.lyric, res.language));
        }

        const auto g2pResult = langMgr->convert(g2pInput);

        for (const auto &g2pRes : g2pResult) {
            std::cout << "lyric: '" << g2pRes.lyric << "' language: '" << g2pRes.g2pId << "' pronunciation: '"
                      << g2pRes.pronunciation << "' mode: " << g2pRes.mode << std::endl;
        }

        return 0;
    }
    catch (const std::exception &e) {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
        return -4;
    }
}
