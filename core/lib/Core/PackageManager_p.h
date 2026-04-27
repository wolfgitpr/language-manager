#ifndef LANGCORE_PACKAGEMANAGER_P_H
#define LANGCORE_PACKAGEMANAGER_P_H

#include <list>
#include <map>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <stdcorelib/3rdparty/llvm/smallvector.h>

#include <LangCore/Core/PackageManager.h>
#include <LangCore/Module/Module.h>
#include <LangCore/Support/ContextUtils.h>

#include "PluginFactory_p.h"

namespace LangCore
{

    class ModuleSpec;
    class PackageData;

    class LANGCORE_EXPORT PackageManager::Impl : public PluginFactory::Impl {
    public:
        explicit Impl(PackageManager *decl);
        ~Impl() override;

        using Decl = PackageManager;

        Expected<PackageData *> open(const std::filesystem::path &path);
        bool close(PackageData *spec);

        void closeAllLoadedPackages();
        void refreshPackageIndexes(const ContextKey &ctxKey);
        bool resolveModuleDependencies(const ContextKey &ctxKey,
                                       const std::vector<ModuleMetadata> &fallbackModules = {});

        DependencyGraph dependencyGraph;

        std::map<std::string, ModuleCategory *, std::less<>> categories;
        std::map<std::string, ModuleCategory *, std::less<>> cateKeyMap;

        // Per-context package paths
        std::map<ContextKey, llvm::SmallVector<std::filesystem::path>> contextPackagePaths;

        struct LoadedPackageBlock {
            PackageData *spec = nullptr;
            int ref = 0;
            llvm::SmallVector<ModuleSpec *> contributes;
            llvm::SmallVector<PackageData *> linked;
        };

        class LoadedPackageMap {
        public:
            std::list<LoadedPackageBlock> packages;
            std::map<std::filesystem::path::string_type, decltype(packages)::iterator, std::less<>> pathIndexes;
            std::map<std::string, std::unordered_map<stdc::VersionNumber, decltype(packages)::iterator>, std::less<>>
                idIndexes;
            std::unordered_map<PackageData *, decltype(packages)::iterator> pointerIndexes;
        };

        LoadedPackageMap loadedPackageMap;
        std::unordered_set<PackageData *> resourcePackages;

        struct PackageBrief {
            std::filesystem::path path;
            stdc::VersionNumber compatVersion;
            int level = 1;
        };

        bool packagePathsDirty = false;
        // Per-context cached package indexes
        std::map<ContextKey, std::map<std::string, std::map<stdc::VersionNumber, PackageBrief>, std::less<>>>
            contextCachedIndexes;

        std::map<std::string, std::unordered_map<stdc::VersionNumber, std::filesystem::path>, std::less<>>
            pendingPackages;

        bool initialized = false;

        // Per-context module metadata
        std::map<ContextKey,
                 std::unordered_set<ModuleMetadata, ModuleMetadata::MainModuleHash, ModuleMetadata::MainModuleEqual>>
            contextModuleInfoSets;
        std::map<ContextKey, std::vector<ModuleMetadata>> contextModuleInfos;

        // Per-context states
        enum class ContextState { Pending, Ready, Failed };
        std::map<ContextKey, ContextState> contextStates;

        bool dependencyResolutionSuccessful = true;
        std::vector<std::string> dependencyErrors;

        int currentLevel = 2;
        int maximumLevel = 2;
        int minimumLevel = 1;

        mutable std::shared_mutex su_mtx;
        static llvm::SmallVector<ModuleCategory *(*)(PackageManager *)> categoryFactories;
    };

} // namespace LangCore
#endif // LANGCORE_PACKAGEMANAGER_P_H
