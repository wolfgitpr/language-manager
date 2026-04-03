#include "CleanStep.h"
#include <LangCore/Support/ConfigAccessor.h>
#include <cctype>
#include <algorithm>

namespace LangPlugins::ChainG2p
{
    LangCore::Expected<void> CleanStep::configure(const LangCore::ModuleSpec *spec,
                                                   const LangCore::JsonObject &config)
    {
        auto cfg = LangCore::config(spec);

        // 解析基础配置
        auto trimIt = cfg.raw().find("trim");
        m_trim = (trimIt != cfg.raw().end() && trimIt->second.isBool()) ? trimIt->second.toBool() : false;

        auto lowercaseIt = cfg.raw().find("lowercase");
        m_lowercase = (lowercaseIt != cfg.raw().end() && lowercaseIt->second.isBool()) ? lowercaseIt->second.toBool() : false;

        auto uppercaseIt = cfg.raw().find("uppercase");
        m_uppercase = (uppercaseIt != cfg.raw().end() && uppercaseIt->second.isBool()) ? uppercaseIt->second.toBool() : false;

        auto removeSymbolsIt = cfg.raw().find("removeSymbols");
        m_removeSymbols = (removeSymbolsIt != cfg.raw().end() && removeSymbolsIt->second.isBool()) ? removeSymbolsIt->second.toBool() : false;

        auto removeNumbersIt = cfg.raw().find("removeNumbers");
        m_removeNumbers = (removeNumbersIt != cfg.raw().end() && removeNumbersIt->second.isBool()) ? removeNumbersIt->second.toBool() : false;

        return {};
    }

    void CleanStep::handle(G2pContext &context)
    {
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