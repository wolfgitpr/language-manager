#include "ConfigMigration.h"
#include <LangCore/Support/Error.h>
#include <algorithm>

namespace LangPlugins::RegexSplitter::Internal::Common
{
    std::string ConfigMigration::migrateFromLevel1(
        const std::string &config,
        int targetLevel
    ) {
        // Level 1 配置格式: {"pattern": "\\s+"}
        // 迁移到 Level 2+ 格式: {"patterns": ["\\s+"]}
        std::string result = config;

        // 简化实现：替换 "pattern" 为 "patterns" 并包装为数组
        size_t pos = result.find("\"pattern\"");
        if (pos != std::string::npos) {
            // 简单的字符串替换
            result.replace(pos, 9, "\"patterns\"");
            // 在值后面添加数组结束符
            size_t valueEnd = result.find("}", pos);
            if (valueEnd != std::string::npos) {
                result.insert(valueEnd, "]");
                // 在值前面添加数组开始符
                size_t valueStart = result.find(":", pos);
                if (valueStart != std::string::npos) {
                    result.insert(valueStart + 1, "[");
                }
            }
        }

        return result;
    }

    std::string ConfigMigration::migrateFromLevel2(
        const std::string &config,
        int targetLevel
    ) {
        // Level 2 配置格式: {"patterns": ["\\s+", "\\t+"]}
        // 迁移到 Level 3+ 格式: {"patterns": ["\\s+", "\\t+"], "caseSensitive": false}
        std::string result = config;

        // 简化实现：添加默认的 caseSensitive 字段
        size_t lastBrace = result.rfind("}");
        if (lastBrace != std::string::npos) {
            result.insert(lastBrace, ", \"caseSensitive\": false");
        }

        return result;
    }

    LangCore::Expected<void> ConfigMigration::validateConfigForLevel(
        const std::string &config,
        int targetLevel
    ) {
        // Level 1 验证
        if (targetLevel == 1) {
            if (config.find("patterns") != std::string::npos) {
                return LangCore::Error(
                    LangCore::Error::ConfigError,
                    "Configuration error: 'patterns' is not supported in Level 1",
                    "Remove this field or upgrade to Level 2+"
                );
            }
            if (config.find("caseSensitive") != std::string::npos) {
                return LangCore::Error(
                    LangCore::Error::ConfigError,
                    "Configuration error: 'caseSensitive' is not supported in Level 1",
                    "Remove this field or upgrade to Level 3"
                );
            }
        }

        // Level 2 验证
        if (targetLevel == 2) {
            if (config.find("caseSensitive") != std::string::npos) {
                return LangCore::Error(
                    LangCore::Error::ConfigError,
                    "Configuration error: 'caseSensitive' is not supported in Level 2",
                    "Remove this field or upgrade to Level 3"
                );
            }
        }

        // Level 3 没有特殊限制

        return {};
    }

    std::string ConfigMigration::getMigrationSuggestion(
        const std::string &config,
        int currentLevel,
        int targetLevel
    ) {
        if (currentLevel < targetLevel) {
            return "Configuration from Level " + std::to_string(currentLevel) +
                   " will be automatically migrated to Level " + std::to_string(targetLevel);
        }

        return "Configuration is already compatible with Level " + std::to_string(targetLevel);
    }
} // namespace LangPlugins::RegexSplitter::Internal::Common