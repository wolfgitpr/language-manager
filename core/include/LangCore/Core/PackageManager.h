#ifndef LANGCORE_PACKAGEMANAGER_H
#define LANGCORE_PACKAGEMANAGER_H

#include <filesystem>
#include <string>
#include <vector>

#include <stdcorelib/support/versionnumber.h>

#include <LangCore/Base/NamedObject.h>
#include <LangCore/Support/ContextUtils.h>

namespace fs = std::filesystem;
#include <LangCore/Core/PluginFactory.h>
#include <LangCore/LangCoreGlobal.h>
#include <LangCore/Module/Dependency/DependencyGraph.h>
#include <LangCore/Module/Module.h>
#include <LangCore/Package/Package.h>


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
        std::vector<std::string> getDependencyErrors() const;

        ModuleCategory *category(const std::string_view &name) const;

        Expected<void> addPackagePath(const std::string &context, const std::filesystem::path &path);
        Expected<void> setPackagePaths(const std::string &context,
                                       const std::vector<std::filesystem::path> &paths);
        std::vector<std::filesystem::path> packagePaths(const std::string &context) const;
        std::vector<std::string> contexts() const;

        Expected<void> addPackagePath(const std::string &context, const stdc::VersionNumber &version,
                                      const std::filesystem::path &path);
        Expected<void> setPackagePaths(const std::string &context, const stdc::VersionNumber &version,
                                       const std::vector<std::filesystem::path> &paths);
        std::vector<std::filesystem::path> packagePaths(const std::string &context,
                                                         const stdc::VersionNumber &version) const;
        std::vector<ContextKey> contextKeys() const;

        Expected<Package> open(const std::filesystem::path &path);
        Package find(const std::string_view &id, const stdc::VersionNumber &version) const;
        std::vector<Package> find(const std::string_view &id) const;
        std::vector<Package> packages() const;
        bool loadPackagesInOrder();
        Expected<NO<Task>> createModuleTask(const ModuleMetadata &moduleInfo, const Package &pkg) const;

        std::vector<ModuleMetadata> getModuleMetadatas(const std::string &context);
        std::vector<ModuleMetadata> getModuleMetadatas(const ContextKey &ctxKey);

    protected:
        class Impl;
        std::unique_ptr<Impl> _impl;
        explicit PackageManager(Impl &impl);

        static void registerCategoryFactory(ModuleCategory *(*fac)(PackageManager *));

        void collectModuleMetadata(const ContextKey &ctxKey, const std::string &packageId,
                                   const std::filesystem::path &packageDir, const JsonObject &modulesObj);
        static void extractModuleMetadataFromJson(const std::string &packageId, const JsonObject &moduleEntry,
                                                  ModuleMetadata &info);


        friend class Package;
        friend class ModuleCategory;

        template <class T>
        friend class ModuleCategoryRegistrar;

    private:
        void scanPackageDirectory(const ContextKey &ctxKey, const std::filesystem::path &basePath);
        void processPackageJson(const ContextKey &ctxKey, const std::filesystem::path &packageDir);
        void printDiscoveryInfo(size_t pathCount, size_t moduleCount);
    };

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
