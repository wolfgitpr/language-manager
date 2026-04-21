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

        bool initialized = false;
        std::map<std::string, std::map<std::string, NO<Task>>> tasks;

        std::unordered_set<ModuleMetadata, ModuleMetadata::MainModuleHash, ModuleMetadata::MainModuleEqual>
            moduleInfoSet;
        std::vector<ModuleMetadata> moduleInfos;
    };

} // namespace LangCore

#endif // LANGCORE_MANAGER_P_H
