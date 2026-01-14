#ifndef LANGMGR_PACKAGEMANAGER_H
#define LANGMGR_PACKAGEMANAGER_H

#include <filesystem>
#include <string>
#include <vector>

#include <stdcorelib/support/versionnumber.h>

#include <LangMgr/Base/NamedObject.h>
#include <LangMgr/Core/PluginFactory.h>
#include <LangMgr/LangMgrGlobal.h>
#include <LangMgr/Module/Dependency/DependencyGraph.h>
#include <LangMgr/Module/Module.h>


namespace LangMgr
{
    class Package;
    class Task;
    class ModuleCategory;

    template <class T>
    class Expected;

    class LANGMGR_EXPORT PackageManager : public PluginFactory {
    public:
        PackageManager();
        ~PackageManager() override;

        bool checkDependencies();
        std::vector<PackageInitializationPlan> getPackageInitializationOrder() const;

        ModuleCategory *category(const std::string_view &name) const;
        static PackageManager *instance();

        void addPackagePath(const std::filesystem::path &path);
        void addPackagePaths(stdc::array_view<std::filesystem::path> paths);
        void setPackagePaths(stdc::array_view<std::filesystem::path> paths);
        std::vector<std::filesystem::path> packagePaths() const;

        Expected<Package> open(const std::filesystem::path &path);
        Package find(const std::string_view &id, const stdc::VersionNumber &version) const;
        std::vector<Package> find(const std::string_view &id) const;
        std::vector<Package> packages() const;

        std::vector<ModuleMetadata> getModuleMetadatas();

    protected:
        class Impl;
        std::unique_ptr<Impl> _impl;
        explicit PackageManager(Impl &impl);

        static void registerCategoryFactory(ModuleCategory *(*fac)(PackageManager *));

        void collectModuleMetadata(const std::string &packageId, const std::string &packageVersion,
                                   const std::filesystem::path &packageDir, const JsonObject &modulesObj);
        static void extractModuleMetadataFromJson(const std::string &packageId, const std::string &packageVersion,
                                                  const JsonObject &moduleEntry, ModuleMetadata &info);


        friend class Package;
        friend class ModuleCategory;

        template <class T>
        friend class ModuleCategoryRegistrar;

    private:
        void scanPackageDirectory(const std::filesystem::path &basePath);
        void processPackageJson(const std::filesystem::path &packageDir);
        void printDiscoveryInfo(size_t pathCount, size_t moduleCount);
    };

    inline void PackageManager::addPackagePath(const std::filesystem::path &path) { addPackagePaths({path}); }

    template <class T>
    class ModuleCategoryRegistrar {
        static_assert(std::is_base_of_v<ModuleCategory, T>, "T should inherit from LangMgr::ModuleCategory");

    public:
        ModuleCategoryRegistrar(ModuleCategory *(*fac)(PackageManager *)) {
            PackageManager::registerCategoryFactory(fac);
        }

        ModuleCategoryRegistrar() {
            PackageManager::registerCategoryFactory([](PackageManager *mgr) -> ModuleCategory * { return new T(mgr); });
        }
    };

} // namespace LangMgr
#endif // LANGMGR_PACKAGEMANAGER_H
