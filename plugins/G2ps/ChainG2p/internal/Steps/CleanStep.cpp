#include "CleanStep.h"

#include <LangCore/Support/ConfigAccessor.h>
#include <LangCore/Support/Error.h>
#include <LangCore/Task/CleanerTask.h>
#include <LangCore/Core/PackageManager.h>
#include <cctype>
#include <algorithm>

namespace LangPlugins::ChainG2p
{
    LangCore::Expected<void> CleanStep::configure(const LangCore::ModuleSpec *spec,
                                                   const LangCore::JsonObject &config)
    {
        m_spec = spec;
        m_mgr = spec->Mgr();

        // 使用传递的 config 参数，而不是从 spec 读取
        auto cfg = LangCore::ConfigAccessor(config, "");

        // 获取模式（默认为 inline）
        std::string mode = cfg.getString("mode", "inline");

        if (mode == "task") {
            // Task 模式：调用 cleaner task
            m_useTask = true;
            auto cleanerIdExp = cfg.getString("cleanerId");
            if (!cleanerIdExp) {
                return LangCore::Error(LangCore::Error::ConfigError,
                                     "cleanerId is required when mode is 'task'");
            }
            m_cleanerId = cleanerIdExp.take();

            // 获取 cleaner task
            auto cleanerCate = m_mgr->category("cleaner");
            if (!cleanerCate) {
                return LangCore::Error(LangCore::Error::RuntimeError, "Could not find category: cleaner");
            }

            auto cleanerObj = cleanerCate->getFirstObject(m_cleanerId);
            if (!cleanerObj) {
                return LangCore::Error(LangCore::Error::RuntimeError,
                                     "Could not find cleaner task: " + m_cleanerId);
            }

            m_cleanerTask = cleanerObj.as<LangCore::Task>();

            // 初始化 cleaner task
            auto initExp = m_cleanerTask->initialize();
            if (!initExp) {
                return initExp.takeError();
            }

            return {};
        } else if (mode == "inline") {
            // 内联模式：解析基础配置
            m_useTask = false;

            // 解析各种清理选项
            m_trim = cfg.getBool("trim", false);
            m_lowercase = cfg.getBool("lowercase", false);
            m_uppercase = cfg.getBool("uppercase", false);
            m_removeSymbols = cfg.getBool("removeSymbols", false);
            m_removeNumbers = cfg.getBool("removeNumbers", false);

            return {};
        } else {
            return LangCore::Error(LangCore::Error::ConfigError,
                                 "Invalid mode: '" + mode + "'. Expected 'inline' or 'task'");
        }
    }

    void CleanStep::handle(G2pContext &context)
    {
        if (m_useTask && m_cleanerTask) {
            // Task 模式：调用 cleaner task
            std::vector<std::string> inputStrings;
            inputStrings.reserve(context.words().size());

            for (auto &word : context.words()) {
                inputStrings.push_back(word.lyric);
            }

            auto cleanerInput = std::make_shared<LangCore::CleanerInputV1>();
            cleanerInput->cleanerInput = inputStrings;

            auto resultExp = m_cleanerTask->start(cleanerInput);
            if (resultExp) {
                auto result = resultExp.take();
                if (result->error.ok()) {
                    // 尝试转换为 CleanerResultV1
                    auto *cleanerResult = dynamic_cast<LangCore::CleanerResultV1*>(result.get());
                    if (cleanerResult) {
                        auto &cleanedStrings = cleanerResult->cleanerResult;
                        for (size_t i = 0; i < context.words().size() && i < cleanedStrings.size(); ++i) {
                            context.words()[i].cleanedLyric = cleanedStrings[i];
                        }
                    }
                }
            }

            return;
        }

        // 内联模式：应用清洗规则
        for (auto &word : context.words()) {
            std::string cleaned = word.lyric;

            // 应用清洗规则
            if (m_trim) {
                cleaned = trimString(cleaned);
            }

            if (m_lowercase) {
                cleaned = toLowercase(cleaned);
            }

            if (m_uppercase) {
                cleaned = toUppercase(cleaned);
            }

            if (m_removeSymbols) {
                cleaned = removeSymbols(cleaned);
            }

            if (m_removeNumbers) {
                cleaned = removeNumbers(cleaned);
            }

            word.cleanedLyric = cleaned;
        }
    }

    void CleanStep::cleanup()
    {
        m_cleanerTask.reset();
    }

    std::string CleanStep::trimString(const std::string &str)
    {
        size_t start = str.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) {
            return "";
        }

        size_t end = str.find_last_not_of(" \t\n\r");
        return str.substr(start, end - start + 1);
    }

    std::string CleanStep::toLowercase(const std::string &str)
    {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        return result;
    }

    std::string CleanStep::toUppercase(const std::string &str)
    {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(),
                       [](unsigned char c) { return std::toupper(c); });
        return result;
    }

    std::string CleanStep::removeSymbols(const std::string &str)
    {
        std::string result;
        for (char c : str) {
            if (std::isalnum(c) || std::isspace(c)) {
                result += c;
            }
        }
        return result;
    }

    std::string CleanStep::removeNumbers(const std::string &str)
    {
        std::string result;
        for (char c : str) {
            if (!std::isdigit(c)) {
                result += c;
            }
        }
        return result;
    }

} // namespace LangPlugins::ChainG2p