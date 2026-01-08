#ifndef LANGMGR_ITASK_H
#define LANGMGR_ITASK_H

#include <functional>

#include <LangMgr/Core/NamedObject.h>
#include <LangMgr/LangMgrGlobal.h>
#include <LangMgr/Support/Expected.h>

namespace LangMgr
{

    class ModuleDefinition;
    class Manager;

    class TaskInitArgs : public NamedObject {
    public:
        explicit TaskInitArgs(std::string name) : NamedObject(std::move(name)) {}
    };

    class TaskStartInput : public NamedObject {
    public:
        explicit TaskStartInput(std::string name) : NamedObject(std::move(name)) {}
    };

    class TaskResult : public NamedObject {
    public:
        explicit TaskResult(std::string name) : NamedObject(std::move(name)) {}

        Error error;
    };

    /// TaskInfoBase - The base class storing inference information which should be created
    /// by a specific inference interpreter.
    class TaskInfoBase : public NamedObject {
    public:
        TaskInfoBase(std::string name, std::string className, const int apiLevel) :
            NamedObject(std::move(name)), _className(std::move(className)), _apiLevel(apiLevel) {}
        ~TaskInfoBase() override = default;

        /// Related interpreter information.
        const std::string &className() const { return _className; }
        int apiLevel() const { return _apiLevel; }

    protected:
        std::string _className;
        int _apiLevel;
    };

    class TaskConfiguration : public TaskInfoBase {
    public:
        TaskConfiguration(std::string name, std::string iid, const int apiLevel) :
            TaskInfoBase(std::move(name), std::move(iid), apiLevel) {}
    };

    class TaskRuntimeOptions : public TaskInfoBase {
    public:
        TaskRuntimeOptions(std::string name, std::string iid, const int apiLevel) :
            TaskInfoBase(std::move(name), std::move(iid), apiLevel) {}
    };

    class LANGMGR_EXPORT Task : public NamedObject {
    public:
        Task();
        explicit Task(const ModuleDefinition *spec);
        ~Task() override;

        enum State {
            Idle,
            Running,
            Failed,
            Terminated,
        };

        using StartAsyncCallback = std::function<void(const NO<TaskResult> &, const Error &)>;

        virtual Expected<void> initialize(const NO<TaskInitArgs> &args);

        virtual Expected<NO<TaskResult>> start(const NO<TaskStartInput> &input) = 0;
        virtual Expected<void> startAsync(const NO<TaskStartInput> &input, const StartAsyncCallback &callback);
        virtual bool stop() = 0;

        State state() const;

        virtual NO<TaskResult> result() const = 0;

        const ModuleDefinition *spec() const;
        Manager *Mgr() const;

        Expected<NO<NamedObject>> getObject(const std::string &category, const std::string &id) const;

    protected:
        void setState(State state);

        class Impl;
        explicit Task(Impl &impl);
    };

} // namespace LangMgr

#endif // LANGMGR_ITASK_H
