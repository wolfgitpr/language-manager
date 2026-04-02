#ifndef LANGPLUGINS_REGEXSPLITTER_INTERNAL_COMMON_CONFIGMIGRATION_H
#define LANGPLUGINS_REGEXSPLITTER_INTERNAL_COMMON_CONFIGMIGRATION_H

#include <string>
#include <LangCore/Support/Expected.h>

namespace LangPlugins::RegexSplitter::Internal::Common
{
    /// 配置迁移工具类
    class ConfigMigration {
    public:
        /// 从 Level 1 配置迁移到 Level 2+
        static std::string migrateFromLevel1(
            const std::string &config,
            int targetLevel
        );

        /// 从 Level 2 配置迁移到 Level 3+
        static std::string migrateFromLevel2(
            const std::string &config,
            int targetLevel
        );

        /// 验证配置是否兼容目标 Level
        static LangCore::Expected<void> validateConfigForLevel(
            const std::string &config,
            int targetLevel
        );

        /// 获取迁移建议
        static std::string getMigrationSuggestion(
            const std::string &config,
            int currentLevel,
            int targetLevel
        );
    };
} // namespace LangPlugins::RegexSplitter::Internal::Common

#endif // LANGPLUGINS_REGEXSPLITTER_INTERNAL_COMMON_CONFIGMIGRATION_H