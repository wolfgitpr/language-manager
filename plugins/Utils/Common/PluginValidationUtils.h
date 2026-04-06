#ifndef LANGPLUGINS_COMMON_PLUGINVALIDATIONUTILS_H
#define LANGPLUGINS_COMMON_PLUGINVALIDATIONUTILS_H

#include <string>
#include <vector>
#include <functional>

#include <LangCore/Support/Error.h>
#include <LangCore/Support/Expected.h>

namespace LangPlugins::Common
{

    /// PluginValidationUtils - 插件验证工具类
    ///
    /// 提供通用的验证功能，用于检查配置、参数等
    class PluginValidationUtils {
    public:
        /// 验证字符串不为空
        /// @param value 要验证的值
        /// @param fieldName 字段名称（用于错误消息）
        /// @return 成功返回 true，失败返回错误
        static LangCore::Expected<bool> validateNotEmpty(const std::string &value, const std::string &fieldName);

        /// 验证字符串数组不为空
        /// @param value 要验证的值
        /// @param fieldName 字段名称（用于错误消息）
        /// @return 成功返回 true，失败返回错误
        static LangCore::Expected<bool> validateNotEmpty(const std::vector<std::string> &value,
                                                         const std::string &fieldName);

        /// 验证整数值在指定范围内
        /// @param value 要验证的值
        /// @param min 最小值（包含）
        /// @param max 最大值（包含）
        /// @param fieldName 字段名称（用于错误消息）
        /// @return 成功返回 true，失败返回错误
        static LangCore::Expected<bool> validateRange(int value, int min, int max, const std::string &fieldName);

        /// 验证字符串值在允许的集合中
        /// @param value 要验证的值
        /// @param allowedValues 允许的值集合
        /// @param fieldName 字段名称（用于错误消息）
        /// @return 成功返回 true，失败返回错误
        static LangCore::Expected<bool> validateAllowed(const std::string &value,
                                                        const std::vector<std::string> &allowedValues,
                                                        const std::string &fieldName);

        /// 验证正则表达式
        /// @param regex 正则表达式字符串
        /// @param fieldName 字段名称（用于错误消息）
        /// @return 成功返回 true，失败返回错误
        static LangCore::Expected<bool> validateRegex(const std::string &regex, const std::string &fieldName);

        /// 验证文件路径存在
        /// @param path 文件路径
        /// @param fieldName 字段名称（用于错误消息）
        /// @return 成功返回 true，失败返回错误
        static LangCore::Expected<bool> validateFileExists(const std::string &path, const std::string &fieldName);

        /// 验证目录路径存在
        /// @param path 目录路径
        /// @param fieldName 字段名称（用于错误消息）
        /// @return 成功返回 true，失败返回错误
        static LangCore::Expected<bool> validateDirExists(const std::string &path, const std::string &fieldName);

        /// 验证自定义条件
        /// @param condition 条件函数
        /// @param fieldName 字段名称（用于错误消息）
        /// @param errorMessage 错误消息
        /// @return 成功返回 true，失败返回错误
        static LangCore::Expected<bool> validateCondition(std::function<bool()> condition,
                                                          const std::string &fieldName,
                                                          const std::string &errorMessage);
    };

} // namespace LangPlugins::Common

#endif // LANGPLUGINS_COMMON_PLUGINVALIDATIONUTILS_H