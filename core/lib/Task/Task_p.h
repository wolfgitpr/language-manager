#ifndef LANGCORE_ITask_P_H
#define LANGCORE_ITask_P_H

#include <LangCore/Task/Task.h>

#include "NamedObject_p.h"

namespace LangCore
{
    class Task::Impl : public NamedObject::Impl {
    public:
        explicit Impl(Task *task) : NamedObject::Impl(task) {}

        State state = Idle;
        const ModuleSpec *spec_;
    };
} // namespace LangCore

#endif // LANGCORE_ITask_P_H
