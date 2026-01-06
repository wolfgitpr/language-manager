#ifndef LANGMGR_ITask_P_H
#define LANGMGR_ITask_P_H

#include <LangMgr/Task/ITask.h>

#include "Core/NamedObject_p.h"

namespace LangMgr
{

    class ITask::Impl : public NamedObject::Impl {
    public:
        explicit Impl(ITask *task) : NamedObject::Impl(task) {}

        State state = Idle;
    };

} // namespace LangMgr

#endif // LANGMGR_ITask_P_H
