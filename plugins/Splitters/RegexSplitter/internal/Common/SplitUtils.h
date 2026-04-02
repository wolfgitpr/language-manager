#ifndef LANGPLUGINS_REGEXSPLITTER_INTERNAL_COMMON_SPLITUTILS_H
#define LANGPLUGINS_REGEXSPLITTER_INTERNAL_COMMON_SPLITUTILS_H

#include <vector>
#include <string>
#include <LangCore/Support/Expected.h>

namespace LangPlugins::RegexSplitter::Internal::Common
{
    /// 通用的分割工具类（避免虚函数调用）
    class SplitUtils {
    public:
        /// 使用单个模式分割文本
        static std::vector<std::string> splitWithPattern(
            const std::string &text,
            const std::string &pattern
        );

        /// 使用多个模式分割文本
        static std::vector<std::string> splitWithPatterns(
            const std::string &text,
            const std::vector<std::string> &patterns,
            bool caseSensitive = false
        );

        /// 验证正则表达式
        static LangCore::Expected<void> validateRegex(const std::string &pattern);

        /// 解析配置字符串
        static LangCore::Expected<std::vector<std::string>> parsePatterns(
            const std::string &config
        );
    };
} // namespace LangPlugins::RegexSplitter::Internal::Common

#endif // LANGPLUGINS_REGEXSPLITTER_INTERNAL_COMMON_SPLITUTILS_H