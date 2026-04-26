#include "LangCore/Task/Task.h"
#include "Task_p.h"

#include <stdcorelib/pimpl.h>
#include <stdcorelib/str.h>

#include <LangCore/Core/PackageManager.h>
#include <LangCore/Module/Module.h>
#include <LangCore/Support/JSON.h>
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
        if (!impl.spec_)
            return nullptr;
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

    Expected<std::string> Task::loadConfig() const {
        __stdc_impl_t;

        // 从默认配置路径加载配置
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

        auto configPath = packagePath / configPathStr;

        if (!std::filesystem::exists(configPath)) {
            return Error(Error::FileSystemError,
                       "Config file not found: " + configPath.string());
        }

        std::ifstream file(configPath);
        if (!file.is_open()) {
            return Error(Error::FileSystemError,
                       "Failed to open config file: " + configPath.string());
        }

        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());

        return content;
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