#include "LangCore/Task/Task.h"
#include "Task_p.h"

#include <stdcorelib/pimpl.h>

#include <LangCore/Core/PackageManager.h>
#include <LangCore/Module/Module.h>
#include <fstream>
#include <filesystem>

namespace LangCore
{

    Task::Task() : Task(*new Impl(this)) {}
    Task::Task(const ModuleSpec *spec) : Task(*new Impl(this)) {
        __stdc_impl_t;
        impl.spec_ = spec;
        if (spec) {
            impl.cachedApiLevel_ = spec->apiLevel();  // 缓存 apiLevel() 值
        }
    }

    Task::~Task() = default;

    Task::Task(Impl &impl) : NamedObject(impl) {}

    const ModuleSpec *Task::spec() const {
        __stdc_impl_t;
        return impl.spec_;
    }

    PackageManager *Task::Mgr() const {
        __stdc_impl_t;
        return impl.spec_->Mgr();
    }

    Expected<NO<NamedObject>> Task::getObject(const std::string &category, const std::string &id) const {
        const auto inferenceCate = this->Mgr()->category(category);
        if (!inferenceCate)
            return Error(Error::RuntimeError, "could not find category: " + category);

        const auto inferenceObject = inferenceCate->getFirstObject(id);
        if (!inferenceObject)
            return Error(Error::RuntimeError, "could not find id: " + id);

        return inferenceObject;
    }

    std::string Task::getConfig() const {
        __stdc_impl_t;
        std::shared_lock lock(impl.mutex);
        return impl.config;
    }

    std::string Task::getUiSchema() const {
        // 默认实现：返回空 JSON 对象
        // 插件可以重写此方法提供自定义 UI Schema
        return "{}";
    }

    Expected<void> Task::setConfig(const std::string &config) {
        __stdc_impl_t;
        
        // 保存配置到用户配置目录
        auto saveResult = saveConfig(config);
        if (!saveResult) {
            return saveResult;
        }
        
        // 更新内部配置
        std::unique_lock lock(impl.mutex);
        impl.config = config;
        impl.userConfigExists_ = true;
        
        return {};
    }

    Expected<void> Task::resetToDefault() {
        __stdc_impl_t;
        
        // 获取用户配置路径
        auto userConfigPath = getUserConfigPath();
        
        // 如果用户配置存在，删除它
        if (std::filesystem::exists(userConfigPath)) {
            try {
                std::filesystem::remove(userConfigPath);
            } catch (const std::filesystem::filesystem_error &e) {
                return Error(Error::FileSystemError,
                           "Failed to delete user config: " + std::string(e.what()));
            }
        }
        
        // 重新加载默认配置
        auto loadResult = loadConfig();
        if (!loadResult) {
            return loadResult.error();
        }
        
        // 更新内部配置
        std::unique_lock lock(impl.mutex);
        impl.config = loadResult.value();
        impl.userConfigExists_ = false;
        
        return {};
    }

    bool Task::isUsingDefaultConfig() const {
        __stdc_impl_t;
        std::shared_lock lock(impl.mutex);
        return !impl.userConfigExists_;
    }

    std::filesystem::path Task::getUserConfigPath() const {
        // 用户配置路径：{packagePath}/configs/{moduleId}.json
        auto packagePath = spec()->path();
        auto configsDir = packagePath / "configs";
        auto userConfigPath = configsDir / (spec()->id() + ".json");
        return userConfigPath;
    }

    std::filesystem::path Task::getDefaultConfigPath() const {
        // 默认配置路径：从 ModuleSpec::manifestConfiguration() 获取
        auto packagePath = spec()->path();
        auto manifestConfig = spec()->manifestConfiguration();
        
        // 解析配置路径（通常是相对路径）
        // manifestConfiguration 是 JsonObject，需要从中提取 "configuration" 字段
        std::string configPathStr;
        auto configIt = manifestConfig.find("configuration");
        if (configIt != manifestConfig.end() && configIt->second.isString()) {
            configPathStr = configIt->second.toString();
        } else {
            // 如果找不到 configuration 字段，使用默认路径
            configPathStr = "modules/" + spec()->id() + "/config.json";
        }
        
        auto defaultConfigPath = packagePath / configPathStr;
        
        return defaultConfigPath;
    }

    Expected<std::string> Task::loadConfig() const {
        // 优先加载用户配置
        auto userConfigPath = getUserConfigPath();
        if (std::filesystem::exists(userConfigPath)) {
            std::ifstream file(userConfigPath);
            if (!file.is_open()) {
                return Error(Error::FileSystemError,
                           "Failed to open user config file: " + userConfigPath.string());
            }
            
            std::string content((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
            
            // 标记使用了用户配置
            __stdc_impl_t;
            impl.userConfigExists_ = true;
            
            return content;
        }
        
        // 回退到默认配置
        auto defaultConfigPath = getDefaultConfigPath();
        if (!std::filesystem::exists(defaultConfigPath)) {
            return Error(Error::FileSystemError,
                       "Default config file not found: " + defaultConfigPath.string());
        }
        
        std::ifstream file(defaultConfigPath);
        if (!file.is_open()) {
            return Error(Error::FileSystemError,
                       "Failed to open default config file: " + defaultConfigPath.string());
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        
        // 标记使用了默认配置
        __stdc_impl_t;
        impl.userConfigExists_ = false;
        
        return content;
    }

    Expected<void> Task::saveConfig(const std::string &config) const {
        auto userConfigPath = getUserConfigPath();
        
        // 创建用户配置目录
        auto configsDir = userConfigPath.parent_path();
        try {
            std::filesystem::create_directories(configsDir);
        } catch (const std::filesystem::filesystem_error &e) {
            return Error(Error::FileSystemError,
                       "Failed to create configs directory: " + std::string(e.what()));
        }
        
        // 使用临时文件进行原子保存
        auto tempPath = userConfigPath.string() + ".tmp";
        {
            std::ofstream file(tempPath);
            if (!file.is_open()) {
                return Error(Error::FileSystemError,
                           "Failed to create temp config file: " + tempPath);
            }
            file << config;
        }
        
        // 原子重命名
        try {
            std::filesystem::rename(tempPath, userConfigPath);
        } catch (const std::filesystem::filesystem_error &e) {
            // 清理临时文件
            std::filesystem::remove(tempPath);
            return Error(Error::FileSystemError,
                       "Failed to save config file: " + std::string(e.what()));
        }
        
        return {};
    }

    Expected<void> Task::initializeConfig() {
        // 加载配置（优先用户配置，回退到默认配置）
        auto loadResult = loadConfig();
        if (!loadResult) {
            return loadResult.error();
        }
        
        // 更新内部配置
        __stdc_impl_t;
        std::unique_lock lock(impl.mutex);
        impl.config = loadResult.value();
        
        return {};
    }

} // namespace LangCore