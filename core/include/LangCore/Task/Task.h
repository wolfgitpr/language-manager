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
        TaskInfoBase() {}
        ~TaskInfoBase() override = default;
    };

    class TaskInitArgs : public TaskInfoBase {
    public:
        explicit TaskInitArgs() {}
    };

    class TaskInput : public TaskInfoBase {
    public:
        explicit TaskInput() {}
    };

    class TaskResult : public TaskInfoBase {
    public:
        explicit TaskResult() {}

        Error error;
    };

    class TaskConfiguration : public TaskInfoBase {
    public:
        TaskConfiguration() {}
    };

    class TaskRuntimeOptions : public TaskInfoBase {
    public:
        TaskRuntimeOptions() {}
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

    protected:
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
