#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <stdcorelib/str.h>
#include <stdcorelib/system.h>

#include <LangCore/Core/Manager.h>
#include <LangCore/Module/Module.h>
#include <LangCore/Task/SessionTask.h>
#include <LangCore/Task/TaskPlugin.h>

std::filesystem::path getPluginRootDirectory() {
#if defined(Q_OS_MAC)
    return MacOSUtils::getMainBundlePath() / _TSTR("Contents/PlugIns");
#elif defined(Q_OS_WIN)
    return stdc::system::application_directory() / _TSTR("plugins");
#else
    return stdc::system::application_directory().parent_path() / _TSTR("lib/plugins");
#endif
}

using EP = LangCore::ExecutionProvider;

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
    const auto onnxDriverPlugin = mgr->plugin<LangCore::DriverPlugin>("onnx");
    if (!onnxDriverPlugin) {
        std::cerr << "Failed to load ONNX inference driver" << std::endl;
        return false;
    }

    auto expOnnxDriver = onnxDriverPlugin->create();
    if (!expOnnxDriver) {
        std::cerr << "Failed to load ONNX inference driver" << std::endl;
        return false;
    }

    const auto onnxArgs = LangCore::NO<LangCore::DriverInitArgs>::create();

    const auto ep_ = parseExecutionProvider(ep);
    onnxArgs->ep = ep_;
    const auto ortParentPath = onnxDriverPlugin->path().parent_path() / _TSTR("runtimes") / _TSTR("onnx");
    onnxArgs->runtimePath =
        ep_ == EP::CUDAExecutionProvider ? ortParentPath / _TSTR("cuda") : ortParentPath / _TSTR("default");

    onnxArgs->loadFromProcess = loadFromProgress;
    onnxArgs->deviceIndex = deviceIndex;

    const auto onnxDriver = expOnnxDriver.take();

    if (const auto exp = onnxDriver->initialize(onnxArgs); !exp) {
        std::cerr << "Failed to initialize ONNX driver: " << exp.error().message() << std::endl;
        return false;
    }

    auto &driverCategory = *mgr->category("driver");
    driverCategory.addObject("g2pOnnxDriver", onnxDriver);
    return true;
}

bool initializeManager() {
    const auto langMgr = LangCore::Manager::instance();

    const auto defaultPluginDir = getPluginRootDirectory() / _TSTR("LangPlugins");
    langMgr->addPluginPath("org.openvpi.Driver", defaultPluginDir / _TSTR("Drivers"));
    langMgr->addPluginPath("org.openvpi.Task", defaultPluginDir / _TSTR("G2ps"));
    langMgr->addPluginPath("org.openvpi.Task", defaultPluginDir / _TSTR("Taggers"));
    langMgr->addPluginPath("org.openvpi.Task", defaultPluginDir / _TSTR("Splitters"));

    // 添加包路径
    const std::filesystem::path packagesRootDir = R"(D:\projects\language-manager\res\G2pPackages)";
    langMgr->addPackagePath(packagesRootDir);

    // 初始化 ONNX Driver
    if (const auto onnxDriverInitialized = initializeOnnxDriver(langMgr, "cpu", 0, false); !onnxDriverInitialized) {
        std::cerr << "Failed to initialize ONNX driver" << std::endl;
        return false;
    }

    // 初始化 Manager
    std::string errorMessage;
    langMgr->initialize(errorMessage);
    if (!langMgr->initialized()) {
        std::cerr << "Failed to initialize langMgr: " << errorMessage << std::endl;
        return false;
    }

    return true;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Language Manager - G2p Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;

    try {
        // 初始化 Manager（加载所有插件和包）
        if (!initializeManager()) {
            std::cerr << "Failed to initialize Manager" << std::endl;
            return -1;
        }

        // ========================================
        // 测试配置 API
        // ========================================
        std::cout << "\n=== Testing Configuration API ===" << std::endl;

        const auto langMgr = LangCore::Manager::instance();

        // 测试配置 API（使用普通 G2p 任务）
        if (auto g2pTask = langMgr->task("g2p", "g2p-cmn")) {
            std::cout << "Testing getConfig()..." << std::endl;
            auto configJson = g2pTask.get()->getConfig();
            std::cout << "Config size: " << configJson.size() << " bytes" << std::endl;

            // 插件应该自己解析 JSON 配置
            std::cout << "\nNote: Config parsing should be done by plugins using ConfigAccessor" << std::endl;
        }

        // ========================================
        // 测试 G2p 转换
        // ========================================
        std::cout << "\n=== Testing G2p Chain Task ===" << std::endl;

        // 测试文本
        const auto text =
            "wo neng tun xia Glass er bu shang shen ti\nhalloween蝉ce "
            "声--陪かな伴着qwe行云流浪---\nka回-忆-开始132后安静遥望远方\n荒草覆没的古井--枯塘\n匀-散asdaw一缕过往\n";

        const auto splitRes = langMgr->split(text);
        const auto tagExp = langMgr->tag(splitRes, false, true, {"cmn"});

        std::vector<LangCore::G2pInput *> g2pInput;
        std::cout << "Tag result:" << std::endl;
        for (const auto &res : tagExp) {
            g2pInput.emplace_back(new LangCore::G2pInput(res.lyric, res.language));
            std::cout << "  lyric: '" << res.lyric << "' language: '" << res.language << "' tag: '" << res.tag << "'"
                      << std::endl;
        }

        // 测试 G2p 转换
        std::cout << "\nTesting G2p Chain:" << std::endl;
        const auto g2pResult = langMgr->convert(g2pInput);

        for (const auto &g2pRes : g2pResult) {
            std::cout << "  lyric: '" << g2pRes.lyric << "' g2pId: '" << g2pRes.g2pId << "' pronunciation: '"
                      << g2pRes.pronunciation << "' mode: '" << g2pRes.mode << "' error: " << g2pRes.error
                      << "' errorType: " << g2pRes.errorType << "'" << std::endl;
        }

        // 清理
        for (auto *input : g2pInput) {
            delete input;
        }

        std::cout << "\n========================================" << std::endl;
        std::cout << "All tests completed successfully!" << std::endl;
        std::cout << "========================================" << std::endl;
    }
    catch (const std::exception &e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
