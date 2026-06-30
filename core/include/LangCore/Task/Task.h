#ifndef LANGCORE_TASK_H
#define LANGCORE_TASK_H

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
        TaskInitArgs() = default;
    };

    class TaskInput : public TaskInfoBase {
    public:
        TaskInput() = default;
    };

    class TaskResult : public TaskInfoBase {
    public:
        TaskResult() = default;

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

    protected:
        /// 初始化配置（自动加载配置，子类在 initialize() 中调用）
        /// @return 成功返回 Expected<void>::success()，失败返回错误
        Expected<void> initializeConfig();

        /// 加载配置（从默认配置路径加载）
        /// @return 配置字符串
        Expected<std::string> loadConfig() const;

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

#endif // LANGCORE_TASK_H
