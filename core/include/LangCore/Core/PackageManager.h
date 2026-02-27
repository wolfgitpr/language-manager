#ifndef LANGCORE_PACKAGEMANAGER_H
#define LANGCORE_PACKAGEMANAGER_H

#include <filesystem>
#include <string>
#include <vector>

#include <stdcorelib/support/versionnumber.h>

#include <LangCore/Base/NamedObject.h>
#include <LangCore/Core/PluginFactory.h>
#include <LangCore/LangCoreGlobal.h>
#include <LangCore/Module/Dependency/DependencyGraph.h>
#include <LangCore/Module/Module.h>


namespace LangCore
{
    class Package;
    class Task;
    class ModuleCategory;

    template <class T>
    class Expected;

    class LANGCORE_EXPORT PackageManager : public PluginFactory {
    public:
        PackageManager();
        ~PackageManager() override;

        bool checkDependencies();
        std::vector<PackageInitializationPlan> getPackageInitializationOrder();

        ModuleCategory *category(const std::string_view &name) const;

        void addPackagePath(const std::filesystem::path &path);
        void addPackagePaths(stdc::array_view<std::filesystem::path> paths);
        void setPackagePaths(stdc::array_view<std::filesystem::path> paths);
        std::vector<std::filesystem::path> packagePaths() const;

        Expected<Package> open(const std::filesystem::path &path);
        Package find(const std::string_view &id, const stdc::VersionNumber &version) const;
        std::vector<Package> find(const std::string_view &id) const;
        std::vector<Package> packages() const;
        bool loadPackagesInOrder();
        Expected<NO<Task>> createModuleTask(const ModuleMetadata &moduleInfo, const Package &pkg) const;

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
        static_assert(std::is_base_of_v<ModuleCategory, T>, "T should inherit from LangPlugins::ModuleCategory");

    public:
        ModuleCategoryRegistrar(ModuleCategory *(*fac)(PackageManager *)) {
            PackageManager::registerCategoryFactory(fac);
        }

        ModuleCategoryRegistrar() {
            PackageManager::registerCategoryFactory([](PackageManager *mgr) -> ModuleCategory * { return new T(mgr); });
        }
    };

} // namespace LangCore
#endif // LANGCORE_PACKAGEMANAGER_H
