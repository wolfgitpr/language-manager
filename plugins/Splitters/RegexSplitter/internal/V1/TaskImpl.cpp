#include "TaskImpl.h"
#include <LangCore/Support/ConfigAccessor.h>
#include <LangCore/Support/Error.h>
#include "../Common/SplitUtils.h"

namespace LangPlugins::RegexSplitter::Internal::V1
{
    RegexSplitterTaskImpl::RegexSplitterTaskImpl(const LangCore::ModuleSpec *spec) : m_spec(spec) {}

    LangCore::Expected<void> RegexSplitterTaskImpl::initialize() {
        auto cfg = LangCore::config(m_spec);

        // Level 1 支持 "pattern" 或 "regexes" 配置项
        std::string pattern;

        // 优先尝试 "pattern" 字段
        if (cfg.has("pattern")) {
            auto patternResult = cfg.getString("pattern");
            if (!patternResult) {
                return patternResult.takeError();
            }
            pattern = *patternResult;
        }
        // 其次尝试 "regexes" 字段（取第一个）
        else if (cfg.has("regexes")) {
            auto regexes = cfg.getStringArray("regexes");
            if (!regexes) {
                return regexes.takeError();
            }
            if (regexes->empty()) {
                return LangCore::Error(LangCore::Error::ConfigError, "Configuration error: 'regexes' array is empty");
            }
            pattern = (*regexes)[0];
        }
        // 如果都不存在，检查是否有 "patterns" 字段（不支持）
        else if (cfg.has("patterns")) {
            return LangCore::Error(
                LangCore::Error::ConfigError,
                "Configuration error: 'patterns' is not supported in Level 1, use 'regexes' or 'pattern' instead",
                "Change 'patterns' to 'regexes' or 'pattern'");
        } else {
            return LangCore::Error(
                LangCore::Error::ConfigError, "Configuration error: Missing required field: 'pattern' or 'regexes'",
                "Add either 'pattern' (string) or 'regexes' (array of strings) to the configuration");
        }

        // 验证正则表达式
        auto validation = Common::SplitUtils::validateRegex(pattern);
        if (!validation) {
            return validation.takeError();
        }

        m_pattern = pattern;

        return {};
    }

    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    RegexSplitterTaskImpl::start(const LangCore::NO<LangCore::TaskInput> &input) {
        const auto splitterInput = input.as<LangCore::SplitterInputV1>();
        if (!splitterInput) {
            return LangCore::Error(LangCore::Error::RuntimeError, "Invalid input type for Level 1");
        }

        // 对每个输入字符串应用分割，并合并结果
        auto result = LangCore::NO<LangCore::SplitterResultV1>::create();
        result->splitterResult.clear();
        result->splitterResult.reserve(splitterInput->splitterInput.size());

        for (const auto &text : splitterInput->splitterInput) {
            auto segments = Common::SplitUtils::splitWithPattern(text, m_pattern);
            result->splitterResult.insert(result->splitterResult.end(), segments.begin(), segments.end());
        }

        return result;
    }

    std::string RegexSplitterTaskImpl::getConfig() const { return m_config; }

    LangCore::Expected<void> RegexSplitterTaskImpl::setConfig(const std::string &config) {
        // Level 1 的配置更新逻辑

        // 验证配置格式
        std::string parseError;
        auto json = LangCore::JsonValue::fromJson(config, false, &parseError);
        if (!json.isObject()) {
            return LangCore::Error(LangCore::Error::ConfigError, "Invalid configuration format: " + parseError);
        }

        const auto &configObj = json.toObject();

        // 配置验证：确保不包含 Level 2+ 的配置项
        if (configObj.find("patterns") != configObj.end() || configObj.find("caseSensitive") != configObj.end()) {
            return LangCore::Error(LangCore::Error::ConfigError,
                                   "Configuration error: 'patterns' and 'caseSensitive' are not supported in Level 1",
                                   "Remove these fields or upgrade to Level 2+");
        }

        // 提取 pattern
        std::string newPattern;

        if (configObj.find("pattern") != configObj.end()) {
            const auto &patternValue = configObj.at("pattern");
            if (patternValue.isString()) {
                newPattern = patternValue.toString();
            }
        } else if (configObj.find("regexes") != configObj.end()) {
            const auto &regexesValue = configObj.at("regexes");
            if (regexesValue.isArray()) {
                const auto &regexes = regexesValue.toArray();
                if (!regexes.empty() && regexes[0].isString()) {
                    newPattern = regexes[0].toString();
                }
            }
        }

        // 验证正则表达式
        if (!newPattern.empty()) {
            auto validation = Common::SplitUtils::validateRegex(newPattern);
            if (!validation) {
                return validation.takeError();
            }
            m_pattern = newPattern;
        }

        // 保存配置
        m_config = config;

        return {};
    }
} // namespace LangPlugins::RegexSplitter::Internal::V1
