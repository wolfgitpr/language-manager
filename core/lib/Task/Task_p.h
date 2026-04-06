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
        int cachedApiLevel_ = 1;  // 缓存 apiLevel() 结果
        std::string config;
        mutable std::shared_mutex mutex;
    };
} // namespace LangCore

#endif // LANGCORE_ITask_P_H
