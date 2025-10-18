#ifndef LANGUAGE_MANAGER_PACKAGEREF_P_H
#define LANGUAGE_MANAGER_PACKAGEREF_P_H

#include <filesystem>
#include <map>
#include <string>

#include <stdcorelib/3rdparty/llvm/smallvector.h>

#include "DisplayText.h"
#include "Expected.h"
#include "PackageRef.h"

namespace LangMgr
{

    class ContribSpec;

    class ContribCategory;

    class PackageData {
    public:
        explicit PackageData(LanguageEngine *su) : su(su) {}
        ~PackageData();

    public:
        Expected<void> parse(const std::filesystem::path &dir,
                             const std::map<std::string, ContribCategory *, std::less<>> &categories,
                             llvm::SmallVectorImpl<ContribSpec *> *outContributes);

        static Expected<JsonObject> readDesc(const std::filesystem::path &dir);

        LanguageEngine *su;

        std::filesystem::path path;
        std::string id;

        stdc::VersionNumber version;
        stdc::VersionNumber compatVersion;

        DisplayText description;
        DisplayText vendor;
        DisplayText copyright;
        std::filesystem::path readme;
        std::string url;

        std::map<std::string, std::map<std::string, ContribSpec *, std::less<>>, std::less<>>
            contributes; // category -> [ name -> spec ]

        llvm::SmallVector<PackageDependency> dependencies;

        // state
        Error err;
        bool loaded = false;
    };

} // namespace LangMgr

#endif // LANGUAGE_MANAGER_PACKAGEREF_P_H
