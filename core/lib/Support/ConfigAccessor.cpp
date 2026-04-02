#include <LangCore/Support/ConfigAccessor.h>

#include <stdcorelib/path.h>

#include <LangCore/Module/Module.h>

namespace LangCore
{

    ConfigAccessor::ConfigAccessor(const ModuleSpec *spec) :
        m_config(spec->manifestConfiguration()), m_basePath(spec->path()) {
    }

    ConfigAccessor::ConfigAccessor(const JsonObject &config, const std::filesystem::path &basePath) :
        m_config(config), m_basePath(basePath) {
    }

    // ==================== 必需字段 ====================

    Expected<std::string> ConfigAccessor::getString(const std::string &key) const {
        auto it = m_config.find(key);
        if (it == m_config.end()) {
            return Error(Error::ConfigError, "Missing required field: " + key,
                         "Add the '" + key + "' field to the configuration");
        }
        const auto &value = it->second;
        if (!value.isString()) {
            return Error(Error::ConfigError, "Field '" + key + "' must be a string",
                         "Change the value of '" + key + "' to a string type");
        }
        return value.toString();
    }

    Expected<int> ConfigAccessor::getInt(const std::string &key) const {
        auto it = m_config.find(key);
        if (it == m_config.end()) {
            return Error(Error::ConfigError, "Missing required field: " + key,
                         "Add the '" + key + "' field to the configuration");
        }
        const auto &value = it->second;
        if (!value.isNumber()) {
            return Error(Error::ConfigError, "Field '" + key + "' must be an integer",
                         "Change the value of '" + key + "' to an integer type");
        }
        return static_cast<int>(value.toInt());
    }

    Expected<double> ConfigAccessor::getDouble(const std::string &key) const {
        auto it = m_config.find(key);
        if (it == m_config.end()) {
            return Error(Error::ConfigError, "Missing required field: " + key,
                         "Add the '" + key + "' field to the configuration");
        }
        const auto &value = it->second;
        if (!value.isNumber()) {
            return Error(Error::ConfigError, "Field '" + key + "' must be a number",
                         "Change the value of '" + key + "' to a number type");
        }
        return value.toDouble();
    }

    Expected<bool> ConfigAccessor::getBool(const std::string &key) const {
        auto it = m_config.find(key);
        if (it == m_config.end()) {
            return Error(Error::ConfigError, "Missing required field: " + key,
                         "Add the '" + key + "' field to the configuration");
        }
        const auto &value = it->second;
        if (!value.isBool()) {
            return Error(Error::ConfigError, "Field '" + key + "' must be a boolean",
                         "Change the value of '" + key + "' to a boolean (true/false)");
        }
        return value.toBool();
    }

    Expected<std::filesystem::path> ConfigAccessor::getPath(const std::string &key) const {
        auto strExp = getString(key);
        if (!strExp) {
            return strExp.takeError();
        }
        // 相对于模块路径解析
        return stdc::path::clean_path(m_basePath / stdc::path::from_utf8(*strExp));
    }

    Expected<std::vector<std::string>> ConfigAccessor::getStringArray(const std::string &key) const {
        auto it = m_config.find(key);
        if (it == m_config.end()) {
            return Error(Error::ConfigError, "Missing required field: " + key,
                         "Add the '" + key + "' field to the configuration");
        }
        const auto &value = it->second;
        if (!value.isArray()) {
            return Error(Error::ConfigError, "Field '" + key + "' must be an array",
                         "Change the value of '" + key + "' to an array type");
        }

        std::vector<std::string> result;
        const auto &arr = value.toArray();
        result.reserve(arr.size());

        for (size_t i = 0; i < arr.size(); ++i) {
            if (!arr[i].isString()) {
                return Error(Error::ConfigError,
                             "Array element #" + std::to_string(i) + " of '" + key + "' must be string",
                             "Ensure all elements in the '" + key + "' array are strings");
            }
            result.push_back(arr[i].toString());
        }
        return result;
    }

    // ==================== 可选字段 ====================

    std::string ConfigAccessor::getString(const std::string &key, const std::string &defaultValue) const {
        auto it = m_config.find(key);
        if (it == m_config.end() || !it->second.isString()) {
            return defaultValue;
        }
        return it->second.toString();
    }

    int ConfigAccessor::getInt(const std::string &key, int defaultValue) const {
        auto it = m_config.find(key);
        if (it == m_config.end() || !it->second.isNumber()) {
            return defaultValue;
        }
        return static_cast<int>(it->second.toInt());
    }

    double ConfigAccessor::getDouble(const std::string &key, double defaultValue) const {
        auto it = m_config.find(key);
        if (it == m_config.end() || !it->second.isNumber()) {
            return defaultValue;
        }
        return it->second.toDouble();
    }

    bool ConfigAccessor::getBool(const std::string &key, bool defaultValue) const {
        auto it = m_config.find(key);
        if (it == m_config.end() || !it->second.isBool()) {
            return defaultValue;
        }
        return it->second.toBool();
    }

    std::filesystem::path ConfigAccessor::getPath(const std::string &key,
                                                   const std::filesystem::path &defaultValue) const {
        auto it = m_config.find(key);
        if (it == m_config.end() || !it->second.isString()) {
            return defaultValue;
        }
        return stdc::path::clean_path(m_basePath / stdc::path::from_utf8(it->second.toString()));
    }

    std::vector<std::string> ConfigAccessor::getStringArray(const std::string &key,
                                                             const std::vector<std::string> &defaultValue) const {
        auto it = m_config.find(key);
        if (it == m_config.end() || !it->second.isArray()) {
            return defaultValue;
        }

        std::vector<std::string> result;
        const auto &arr = it->second.toArray();
        result.reserve(arr.size());

        for (const auto &item : arr) {
            if (item.isString()) {
                result.push_back(item.toString());
            }
        }
        return result;
    }

    // ==================== 辅助方法 ====================

    bool ConfigAccessor::has(const std::string &key) const { return m_config.find(key) != m_config.end(); }

} // namespace LangCore
