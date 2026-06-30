#ifndef LANGCORE_PACKAGEMANAGER_H
#define LANGCORE_PACKAGEMANAGER_H

#include <filesystem>
#include <string>
#include <vector>

#include <stdcorelib/support/versionnumber.h>

#include <LangCore/Base/NamedObject.h>
#include <LangCore/Support/ContextUtils.h>

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

    /// Context 初始化状态（v3 对齐修订：四态设计）。
    /// 与 SingerInfo.resolutionState 三态（Resolved/Pending/Missing）是分层语义：
    ///   - SingerInfo 表达声库元数据解析状态（宿主侧）
    ///   - ContextState 表达框架 context 生命周期（框架侧）
    enum class ContextState {
        Pending,       ///< 已注册但尚未初始化
        Ready,          ///< 初始化成功（至少一个包加载成功）
        Failed,         ///< 初始化失败（依赖缺失/环/Level 不兼容/driver 不可用，不阻塞其他 context）
        NotRegistered,  ///< 未注册（不在 contexts() 枚举中）
    };

    class LANGCORE_EXPORT PackageManager : public PluginFactory {
    public:
        PackageManager();
        ~PackageManager() override;

        /// @deprecated Legacy: flattens all contexts into a single graph. Prefer Manager::initialize()
        /// which processes each ContextKey independently.
        /// v3.x：保留 public + 编译期警告；v4.x：私有化并递增 Level。
        [[deprecated("Use Manager::initialize() instead. Will be made private in next Level.")]]
        bool checkDependencies();
        /// @deprecated Legacy: returns a flat initialization order across all contexts.
        /// Prefer Manager::initialize() for per-context ordering.
        /// v3.x：保留 public + 编译期警告；v4.x：私有化并递增 Level。
        [[deprecated("Use Manager::initialize() instead. Will be made private in next Level.")]]
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

        /// 查询指定 context 的初始化状态。
        /// 未注册的 context（不在 contexts() 枚举中）返回 NotRegistered。
        ContextState contextState(const ContextKey &ctxKey) const;

        /// 列出所有初始化失败的 context（不含默认 context，默认 context 失败会阻塞 initialize）。
        std::vector<ContextKey> failedContexts() const;

        Expected<Package> open(const std::filesystem::path &path);
        Package find(const std::string_view &id, const stdc::VersionNumber &version) const;
        std::vector<Package> find(const std::string_view &id) const;
        std::vector<Package> packages() const;
        /// @deprecated Legacy: loads packages from a flat, cross-context initialization order.
        /// Prefer Manager::initialize() which loads packages per-ContextKey.
        /// v3.x：保留 public + 编译期警告；v4.x：私有化并递增 Level。
        [[deprecated("Use Manager::initialize() instead. Will be made private in next Level.")]]
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
        static_assert(std::is_base_of_v<ModuleCategory, T>, "T should inherit from LangCore::ModuleCategory");

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
