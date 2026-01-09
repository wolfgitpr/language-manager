#ifndef LANGMGR_ITask_P_H
#define LANGMGR_ITask_P_H

#include <../../include/LangMgr/Task/Task.h>

#include "NamedObject_p.h"

namespace LangMgr
{

    class Task::Impl : public NamedObject::Impl {
    public:
        explicit Impl(Task *task) : NamedObject::Impl(task) {}

        State state = Idle;
        const ModuleDefinition *spec_;
    };

} // namespace LangMgr

#endif // LANGMGR_ITask_P_H
