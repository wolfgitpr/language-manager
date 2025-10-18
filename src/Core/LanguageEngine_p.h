#ifndef LANGUAGE_MANAGER_SYNTHUNIT_P_H
#define LANGUAGE_MANAGER_SYNTHUNIT_P_H

#include <list>
#include <map>
#include <unordered_map>

#include <stdcorelib/3rdparty/llvm/smallvector.h>

#include <language-manager/Core/LanguageEngine.h>
#include <language-manager/Plugin/PluginFactory_p.h>


namespace LangMgr
{

    class ContribSpec;

    class PackageData;

    class LanguageEngine::Impl : public PluginFactory::Impl {
    public:
        explicit Impl(LanguageEngine *decl);
        ~Impl();

        using Decl = LanguageEngine;

        Expected<PackageData *> open(const std::filesystem::path &path, bool noLoad);
        bool close(PackageData *spec);

    public:
        void closeAllLoadedPackages();
        void refreshPackageIndexes();

        std::map<std::string, ContribCategory *, std::less<>> categories;
        std::map<std::string, ContribCategory *, std::less<>> cateKeyMap;

        llvm::SmallVector<std::filesystem::path> packagePaths;

        struct LoadedPackageBlock {
            PackageData *spec = nullptr;
            int ref = 0;
            llvm::SmallVector<ContribSpec *> contributes;
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
        };
        bool packagePathsDirty = false;
        std::map<std::string, std::map<stdc::VersionNumber, PackageBrief>, std::less<>> cachedPackageIndexesMap;

        // temp
        std::map<std::string, std::unordered_map<stdc::VersionNumber, std::filesystem::path>, std::less<>>
            pendingPackages;

        mutable std::shared_mutex su_mtx;

    public:
        static llvm::SmallVector<ContribCategory *(*)(LanguageEngine *)> categoryFactories;
    };

} // namespace LangMgr

#endif // LANGUAGE_MANAGER_SYNTHUNIT_P_H
