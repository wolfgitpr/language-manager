#ifndef LANGUAGE_MANAGER_P_H
#define LANGUAGE_MANAGER_P_H

#include <map>
#include <vector>

#include <stdcorelib/3rdparty/llvm/smallvector.h>

#include <LangMgr/Core/Manager.h>
#include <LangMgr/Module/Module.h>

#include "PackageManager_p.h"

namespace LangMgr
{

    class ModuleSpec;
    class PackageData;

    class LANGMGR_EXPORT Manager::Impl : public PackageManager::Impl {
    public:
        explicit Impl(Manager *decl);
        ~Impl() override;

        using Decl = Manager;

        std::vector<NO<Task>> priorityTaggers(const std::vector<std::string> &priorityTaggerIds = {});

        bool initialized = false;
        std::vector<std::string> defaultTaggerOrder = {"cmn", "yue", "jpn", "eng", "space", "punc", "num", "unknown"};
        std::map<std::string, std::map<std::string, NO<Task>>> tasks;
        std::string m_pinyinDictPath;

        std::unordered_set<ModuleMetadata, ModuleMetadata::MainModuleHash, ModuleMetadata::MainModuleEqual>
            moduleInfoSet;
        std::vector<ModuleMetadata> moduleInfos;
    };

} // namespace LangMgr

#endif // LANGUAGE_MANAGER_P_H
