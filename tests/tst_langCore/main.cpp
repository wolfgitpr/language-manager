#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <cstdlib>

#include <stdcorelib/str.h>
#include <stdcorelib/system.h>

#include <LangCore/Core/Manager.h>
#include <LangCore/Module/Module.h>
#include <LangCore/Task/SessionTask.h>
#include <LangCore/Task/TaskPlugin.h>
#include <LangCore/Task/G2pTask.h>

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
        std::cout << "\n=== Testing G2p Task ===" << std::endl;

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
                      << g2pRes.pronunciation << "' mode: '" << g2pRes.mode << "'" << std::endl;
        }

        // 清理
        for (auto *input : g2pInput) {
            delete input;
        }

        std::cout << "\n========================================" << std::endl;
        std::cout << "All tests completed successfully!" << std::endl;
        std::cout << "========================================" << std::endl;

        // ========================================
        // 边界条件测试
        // ========================================
        std::cout << "\n=== Testing Edge Cases ===" << std::endl;

        // 边界测试 1：空字符串
        std::cout << "Edge Case 1: Empty string..." << std::endl;
        const auto emptySplit = langMgr->split("");
        std::cout << "  Empty split result size: " << emptySplit.size() << std::endl;

        // 边界测试 2：特殊字符
        std::cout << "Edge Case 2: Special characters..." << std::endl;
        const auto specialText = "!@#$%^&*()_+-=[]{}|;:',.<>?/~`";
        const auto specialSplit = langMgr->split(specialText);
        const auto specialTags = langMgr->tag(specialSplit, false, false, {});
        std::cout << "  Special split result size: " << specialSplit.size() << std::endl;

        // 边界测试 3：混合语言边界
        std::cout << "Edge Case 3: Mixed language boundaries..." << std::endl;
        const auto mixedText = "a中b日c英d中e";
        const auto mixedSplit = langMgr->split(mixedText);
        const auto mixedTags = langMgr->tag(mixedSplit, false, false, {});
        std::cout << "  Mixed split result size: " << mixedSplit.size() << std::endl;

        // 边界测试 4：单个字符
        std::cout << "Edge Case 4: Single character..." << std::endl;
        const auto singleCharText = "中";
        const auto singleCharSplit = langMgr->split(singleCharText);
        const auto singleCharTags = langMgr->tag(singleCharSplit, false, false, {});
        std::cout << "  Single char split result size: " << singleCharSplit.size() << std::endl;

        // 边界测试 5：包含数字和符号
        std::cout << "Edge Case 5: Mixed text with numbers and symbols..." << std::endl;
        const auto mixedSymbolText = "测试123Test@#456测试";
        const auto mixedSymbolSplit = langMgr->split(mixedSymbolText);
        const auto mixedSymbolTags = langMgr->tag(mixedSymbolSplit, false, false, {});
        std::cout << "  Mixed symbol split result size: " << mixedSymbolSplit.size() << std::endl;

        std::cout << "\n========================================" << std::endl;
        std::cout << "Edge cases tests completed!" << std::endl;
        std::cout << "========================================" << std::endl;

        // ========================================
        // TemplateG2p 性能测试
        // ========================================
        std::cout << "\n=== Testing TemplateG2p Performance ===" << std::endl;

        // 生成100个随机的小写字母字符串（长度5-10）
        std::vector<std::string> testWords;
        testWords.reserve(100);
        const char alphabet[] = "abcdefghijklmnopqrstuvwxyz";
        
        for (int i = 0; i < 100; ++i) {
            int length = 5 + (i % 6); // 长度5-10
            std::string word;
            for (int j = 0; j < length; ++j) {
                word += alphabet[rand() % 26];
            }
            testWords.push_back(word);
        }

        std::cout << "Generated " << testWords.size() << " random lowercase words for testing" << std::endl;

        // 测试 TemplateG2p (g2p-eng)
        std::cout << "\nTesting TemplateG2p (g2p-eng)..." << std::endl;
        auto g2pEngTaskExp = langMgr->task("g2p", "g2p-eng");
        if (!g2pEngTaskExp) {
            std::cerr << "Failed to load g2p-eng task: " << g2pEngTaskExp.error().message() << std::endl;
        } else {
            auto g2pEngTask = g2pEngTaskExp.take();
            auto startTime = std::chrono::high_resolution_clock::now();
            
            // TemplateG2p 支持批量转换
            auto input = LangCore::NO<LangCore::G2pInputV1>::create();
            input->g2pInput = testWords;
            
            auto resultExp = g2pEngTask->start(input);
            std::vector<LangCore::G2pRes> results;
            
            if (resultExp) {
                auto result = resultExp.take();
                if (const auto g2pResult = result.as<LangCore::G2pResultV1>()) {
                    results = g2pResult->g2pResult;
                }
            }
            
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            
            std::cout << "  Total time: " << duration.count() << " ms" << std::endl;
            std::cout << "  Words converted: " << results.size() << std::endl;
            std::cout << "  Average time per word: " << (duration.count() / (double)testWords.size()) << " ms" << std::endl;
            
            // 显示部分结果示例
            std::cout << "\n  Sample results (first 5 words):" << std::endl;
            for (size_t i = 0; i < std::min(size_t(5), results.size()); ++i) {
                std::cout << "    '" << results[i].lyric << "' -> '" << results[i].pronunciation << "'";
                if (results[i].errorType != LangCore::NoError) {
                    std::cout << " [Error: " << static_cast<int>(results[i].errorType) << "]";
                }
                std::cout << std::endl;
            }
        }

        std::cout << "\n========================================" << std::endl;
        std::cout << "Performance tests completed!" << std::endl;
        std::cout << "========================================" << std::endl;
    }
    catch (const std::exception &e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
