#ifndef LANGCORE_ITASK_H
#define LANGCORE_ITASK_H

#include <filesystem>

#include <LangCore/Base/NamedObject.h>
#include <LangCore/LangCoreGlobal.h>
#include <LangCore/Support/Expected.h>

namespace LangCore
{

    class ModuleSpec;
    class PackageManager;

    /// TaskInfoBase - The base class storing inference information which should be created
    /// by a specific inference interpreter.
    class TaskInfoBase : public NamedObject {
    public:
        TaskInfoBase() = default;
        ~TaskInfoBase() override = default;
    };

    class TaskInitArgs : public TaskInfoBase {
    public:
        explicit TaskInitArgs() = default;
    };

    class TaskInput : public TaskInfoBase {
    public:
        explicit TaskInput() = default;
    };

    class TaskResult : public TaskInfoBase {
    public:
        explicit TaskResult() = default;

        Error error;
    };

    class TaskConfiguration : public TaskInfoBase {
    public:
        TaskConfiguration() = default;
    };

    class LANGCORE_EXPORT Task : public NamedObject {
    public:
        Task();
        explicit Task(const ModuleSpec *spec);
        ~Task() override;

        virtual int apiLevel() const = 0;

        virtual Expected<void> initialize() = 0;

        virtual Expected<NO<TaskResult>> start(const NO<TaskInput> &input) = 0;

        const ModuleSpec *spec() const;
        PackageManager *Mgr() const;

        Expected<NO<NamedObject>> getObject(const std::string &category, const std::string &id) const;

        /// 获取完整配置（JSON 字符串）
        virtual std::string getConfig() const;

        /// 设置完整配置（JSON 字符串）
        /// 注意：配置会自动保存到用户配置目录
        virtual Expected<void> setConfig(const std::string &config);

        /// 获取 UI Schema（JSON 字符串）
        /// @return UI Schema JSON 字符串，如果插件未提供则返回空 JSON 对象 "{}"
        virtual std::string getUiSchema() const;

        /// 恢复默认配置（删除用户配置文件）
        /// @return 成功返回 Expected<void>::success()，失败返回错误
        Expected<void> resetToDefault();

        /// 检查是否使用默认配置
        /// @return true 如果使用默认配置，false 如果使用用户配置
        bool isUsingDefaultConfig() const;

    protected:
        /// 初始化配置（自动加载配置，子类在 initialize() 中调用）
        /// @return 成功返回 Expected<void>::success()，失败返回错误
        Expected<void> initializeConfig();

        /// 获取用户配置路径
        /// @return 用户配置文件路径
        std::filesystem::path getUserConfigPath() const;

        /// 获取默认配置路径
        /// @return 默认配置文件路径
        std::filesystem::path getDefaultConfigPath() const;

        /// 加载配置（优先用户配置，回退到默认配置）
        /// @return 配置字符串
        Expected<std::string> loadConfig() const;

        /// 保存配置到用户配置目录
        /// @param config 配置字符串
        /// @return 成功返回 Expected<void>::success()，失败返回错误
        Expected<void> saveConfig(const std::string &config) const;

        class Impl;
        explicit Task(Impl &impl);
    };

    /// SessionTask - Provides a basic interface for the memory image of an AI model.
    class SessionTask : public Task {
    public:
        virtual Expected<void> open(const std::filesystem::path &path, const NO<TaskInitArgs> &args) = 0;
        virtual Expected<void> close() = 0;
        virtual bool isOpen() const = 0;

        virtual int64_t id() const = 0;
    };

} // namespace LangCore

#endif // LANGCORE_ITASK_H
