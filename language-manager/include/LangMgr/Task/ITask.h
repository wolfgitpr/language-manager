#ifndef LANGMGR_ITASK_H
#define LANGMGR_ITASK_H

#include <functional>

#include <LangMgr/Core/NamedObject.h>
#include <LangMgr/LangMgrGlobal.h>
#include <LangMgr/Support/Expected.h>

namespace LangMgr
{

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

    class LANGMGR_EXPORT ITask : public NamedObject {
    public:
        ITask();
        ~ITask() override;

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

    protected:
        void setState(State state);

        class Impl;
        explicit ITask(Impl &impl);
    };

} // namespace LangMgr

#endif // LANGMGR_ITASK_H
