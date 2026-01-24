#ifndef LANGCORE_PACKAGEREF_P_H
#define LANGCORE_PACKAGEREF_P_H

#include <filesystem>
#include <map>
#include <string>

#include <stdcorelib/3rdparty/llvm/smallvector.h>

#include <LangCore/Support/Expected.h>

namespace LangCore
{
    class ModuleSpec;
    class ModuleCategory;

    class PackageData {
    public:
        explicit PackageData(PackageManager *mgr) : mgr(mgr) {}
        ~PackageData();

        Expected<void> parse(const std::filesystem::path &dir,
                             const std::map<std::string, ModuleCategory *, std::less<>> &categories,
                             llvm::SmallVectorImpl<ModuleSpec *> *outModules);

        static Expected<JsonObject> readDesc(const std::filesystem::path &descPath);

        PackageManager *mgr;

        std::filesystem::path path;
        std::string id;

        stdc::VersionNumber version;
        stdc::VersionNumber compatVersion;

        DisplayText description;
        DisplayText vendor;
        DisplayText copyright;
        std::filesystem::path readme;
        std::string url;

        int level = 1;

        std::map<std::string, std::map<std::string, ModuleSpec *, std::less<>>, std::less<>> moduleSpecs;

        Error err;
        bool loaded = false;
    };

} // namespace LangCore

#endif // LANGCORE_PACKAGEREF_P_H
