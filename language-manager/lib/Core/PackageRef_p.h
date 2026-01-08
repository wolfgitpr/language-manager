#ifndef LANGUAGE_MANAGER_PACKAGEREF_P_H
#define LANGUAGE_MANAGER_PACKAGEREF_P_H

#include <filesystem>
#include <map>
#include <string>

#include <stdcorelib/3rdparty/llvm/smallvector.h>

#include "DisplayText.h"
#include "Expected.h"
#include "Package.h"

namespace LangMgr
{

    class ModuleDefinition;

    class ModuleCategory;

    class PackageData {
    public:
        explicit PackageData(Manager *mgr) : mgr(mgr) {}
        ~PackageData();

        Expected<void> parse(const std::filesystem::path &dir,
                             const std::map<std::string, ModuleCategory *, std::less<>> &categories,
                             llvm::SmallVectorImpl<ModuleDefinition *> *outModules);

        static Expected<JsonObject> readDesc(const std::filesystem::path &dir);

        Manager *mgr;

        std::filesystem::path path;
        std::string id;

        stdc::VersionNumber version;
        stdc::VersionNumber compatVersion;

        DisplayText description;
        DisplayText vendor;
        DisplayText copyright;
        std::filesystem::path readme;
        std::string url;

        std::map<std::string, std::map<std::string, ModuleDefinition *, std::less<>>, std::less<>>
            moduleSpecs; // category -> [ name -> spec ]

        llvm::SmallVector<PackageDependency> dependencies;

        // state
        Error err;
        bool loaded = false;
    };

} // namespace LangMgr

#endif // LANGUAGE_MANAGER_PACKAGEREF_P_H
