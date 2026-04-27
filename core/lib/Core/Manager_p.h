#ifndef LANGCORE_MANAGER_P_H
#define LANGCORE_MANAGER_P_H

#include <map>
#include <unordered_set>
#include <vector>

#include <LangCore/Core/Manager.h>
#include <LangCore/Module/Module.h>

#include "PackageManager_p.h"

namespace LangCore
{
    class LANGCORE_EXPORT Manager::Impl : public PackageManager::Impl {
    public:
        explicit Impl(Manager *decl);
        ~Impl() override;

        using Decl = Manager;

        // Note: `initialized`, `moduleInfoSet`, `moduleInfos` are inherited from
        // PackageManager::Impl. Do NOT redeclare them here — that would shadow the
        // parent fields and cause state inconsistency.

        // 3-level map: category → ContextKey → moduleId → Task
        std::map<std::string, std::map<ContextKey, std::map<std::string, NO<Task>>>> tasks;
    };

} // namespace LangCore

#endif // LANGCORE_MANAGER_P_H
