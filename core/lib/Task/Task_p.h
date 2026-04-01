#ifndef LANGCORE_ITask_P_H
#define LANGCORE_ITask_P_H

#include <LangCore/Task/Task.h>
#include <shared_mutex>

#include "NamedObject_p.h"

namespace LangCore
{
    class Task::Impl : public NamedObject::Impl {
    public:
        explicit Impl(Task *task) : NamedObject::Impl(task) {}

        const ModuleSpec *spec_;
        std::string config;
        mutable std::shared_mutex mutex;
    };
} // namespace LangCore

#endif // LANGCORE_ITask_P_H
