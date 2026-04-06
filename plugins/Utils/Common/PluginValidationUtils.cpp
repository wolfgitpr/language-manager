#include "PluginValidationUtils.h"

#include <stdcorelib/str.h>
#include <stdcorelib/path.h>

#include <LangCore/Support/ConfigAccessor.h>

#include <sstream>

namespace LangPlugins::Common
{

    LangCore::Expected<bool> PluginValidationUtils::validateNotEmpty(const std::string &value,
                                                                    const std::string &fieldName) {
        if (value.empty()) {
            return LangCore::Error(LangCore::Error::ValidationError,
                                 "Field '" + fieldName + "' cannot be empty",
                                 "Provide a non-empty value for '" + fieldName + "'");
        }
        return true;
    }

    LangCore::Expected<bool> PluginValidationUtils::validateNotEmpty(const std::vector<std::string> &value,
                                                                     const std::string &fieldName) {
        if (value.empty()) {
            return LangCore::Error(LangCore::Error::ValidationError,
                                 "Array '" + fieldName + "' cannot be empty",
                                 "Add at least one element to '" + fieldName + "'");
        }
        return true;
    }

    LangCore::Expected<bool> PluginValidationUtils::validateRange(int value, int min, int max,
                                                                  const std::string &fieldName) {
        if (value < min || value > max) {
            return LangCore::Error(LangCore::Error::ValidationError,
                                 stdc::formatN("Field '%1' value %2 is out of range [%3, %4]",
                                             fieldName, value, min, max),
                                 "Adjust the value of '" + fieldName + "' to be within the valid range");
        }
        return true;
    }

    LangCore::Expected<bool> PluginValidationUtils::validateAllowed(const std::string &value,
                                                                    const std::vector<std::string> &allowedValues,
                                                                    const std::string &fieldName) {
        for (const auto &allowed : allowedValues) {
            if (value == allowed) {
                return true;
            }
        }

        // 使用 stringstream 高效构建 allowedList
        std::stringstream allowedList;
        for (size_t i = 0; i < allowedValues.size(); ++i) {
            if (i > 0) {
                allowedList << ", ";
            }
            allowedList << "'" << allowedValues[i] << "'";
        }

        return LangCore::Error(LangCore::Error::ValidationError,
                             stdc::formatN("Field '%1' value '%2' is not allowed. Allowed values: %3",
                                         fieldName, value, allowedList.str()),
                             "Use one of the allowed values for '" + fieldName + "'");
    }

    LangCore::Expected<bool> PluginValidationUtils::validateRegex(const std::string &regex,
                                                                  const std::string &fieldName) {
        try {
            // 使用标准库的正则表达式进行验证
            std::regex testRegex(regex);
            return true;
        } catch (const std::regex_error &e) {
            return LangCore::Error(LangCore::Error::ValidationError,
                                 "Field '" + fieldName + "' contains invalid regular expression: " + e.what(),
                                 "Fix the regular expression syntax in '" + fieldName + "'");
        }
    }

    LangCore::Expected<bool> PluginValidationUtils::validateFileExists(const std::string &path,
                                                                       const std::string &fieldName) {
        if (!std::filesystem::exists(stdc::path::from_utf8(path))) {
            return LangCore::Error(LangCore::Error::FileSystemError,
                                 "File specified in '" + fieldName + "' does not exist: " + path,
                                 "Check that the file path is correct and the file exists");
        }
        return true;
    }

    LangCore::Expected<bool> PluginValidationUtils::validateDirExists(const std::string &path,
                                                                      const std::string &fieldName) {
        if (!std::filesystem::exists(stdc::path::from_utf8(path))) {
            return LangCore::Error(LangCore::Error::FileSystemError,
                                 "Directory specified in '" + fieldName + "' does not exist: " + path,
                                 "Check that the directory path is correct and the directory exists");
        }
        if (!std::filesystem::is_directory(stdc::path::from_utf8(path))) {
            return LangCore::Error(LangCore::Error::FileSystemError,
                                 "Path specified in '" + fieldName + "' is not a directory: " + path,
                                 "Provide a valid directory path for '" + fieldName + "'");
        }
        return true;
    }

    LangCore::Expected<bool> PluginValidationUtils::validateCondition(std::function<bool()> condition,
                                                                      const std::string &fieldName,
                                                                      const std::string &errorMessage) {
        if (!condition()) {
            return LangCore::Error(LangCore::Error::ValidationError,
                                 "Validation failed for field '" + fieldName + "': " + errorMessage,
                                 "Review the value of '" + fieldName + "'");
        }
        return true;
    }

} // namespace LangPlugins::Common