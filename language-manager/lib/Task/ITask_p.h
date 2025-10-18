#ifndef LANGMGR_ITask_P_H
#define LANGMGR_ITask_P_H

#include <LangMgr/Task/ITask.h>

#include "Core/NamedObject_p.h"

namespace LangMgr {

    class ITask::Impl : public NamedObject::Impl {
    public:
        inline Impl(ITask *task) : NamedObject::Impl(task) {
        }

        State state = Idle;
    };

}

#endif // LANGMGR_ITask_P_H