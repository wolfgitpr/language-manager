#include <LangCore/Core/PackageManager.h>

#include "Package_p.h"

#include <fstream>
#include <iostream>
#include <mutex>
#include <set>

#include <stdcorelib/path.h>
#include <stdcorelib/pimpl.h>
#include <stdcorelib/stlextra/algorithms.h>

#include <LangCore/Core/ManagerLogger.h>
#include <LangCore/Module/Dependency/DependencyResolver.h>
#include <LangCore/Module/Dependency/LevelCompatibilityChecker.h>
#include <LangCore/Package/Package.h>
#include <LangCore/Support/ContextUtils.h>
#include <LangCore/Support/Expected.h>
#include <LangCore/Support/JSON.h>

#include "Module_p.h"
#include "PackageManager_p.h"
#include "PluginFactory_p.h"
#include "TaskPlugin.h"

namespace fs = std::filesystem;

namespace LangCore
{
    llvm::SmallVector<ModuleCategory *(*)(PackageManager *)> PackageManager::Impl::categoryFactories;

    PackageManager::PackageManager(Impl &impl) : _impl(&impl) {}

    PackageManager::Impl::Impl(PackageManager *decl) : PluginFactory::Impl(decl) {
        for (const auto &factory : categoryFactories) {
            const auto category = factory(decl);
            categories[std::string(category->name())] = category;
            cateKeyMap[std::string(category->key())] = category;
        }
    }

    PackageManager::Impl::~Impl() {
        closeAllLoadedPackages();
        stdc::delete_all(categories);
    }

    Expected<PackageData *> PackageManager::Impl::open(const std::filesystem::path &path) {
        __stdc_decl_t;
        auto canonicalPath = stdc::path::canonical(path);
        if (canonicalPath.empty() || !fs::is_directory(canonicalPath)) {
            return Error{
                Error::FileSystemError,
                stdc::formatN("Invalid package path %1.", path),
            };
        }

        {
            std::unique_lock lock(su_mtx);
            auto &pkgMap = loadedPackageMap;
            if (auto it = pkgMap.pathIndexes.find(canonicalPath); it != pkgMap.pathIndexes.end()) {
                auto &pkg = *it->second;
                pkg.ref++;
                return pkg.spec;
            }
        }

        // Parse spec
        auto pd = new PackageData(&decl);
        llvm::SmallVector<ModuleSpec *> contributes;

        if (auto exp = pd->parse(canonicalPath, cateKeyMap, &contributes); !exp) {
            delete pd;
            stdc::delete_all(contributes); // Maybe redundant
            return exp.error();
        }

        // Set parent
        for (const auto &contribute : std::as_const(contributes)) {
            contribute->_impl->package = pd;
        }

        // Add to package's data space
        for (const auto &contribute : std::as_const(contributes)) {
            pd->moduleSpecs[contribute->_impl->category][contribute->_impl->id] = contribute;
        }

        const auto &removePending = [this, pd]
        {
            const auto it = pendingPackages.find(pd->id);
            auto &versionSet = it->second;
            versionSet.erase(pd->version);
            if (versionSet.empty()) {
                pendingPackages.erase(it);
            }
        };

        // Check duplications
        do {
            std::unique_lock lock(su_mtx);
            auto &pkgMap = loadedPackageMap;

            // Helper: handle duplicate detection — sets error and returns early
            auto handleDuplicate = [&](Error &&error) -> PackageData * {
                pd->err = std::move(error);
                resourcePackages.insert(pd);
                return pd;
            };

            // Check if a package with same id and version but different path is loaded
            if (auto it = pkgMap.idIndexes.find(pd->id); it != pkgMap.idIndexes.end()) {
                const auto &versionMap = it->second;
                if (auto it2 = versionMap.find(pd->version); it2 != versionMap.end()) {
                    auto pkg = *it2->second;
                    return handleDuplicate({
                        Error::FileSystemError,
                        stdc::formatN("duplicated package %1[%2] in %3 is loaded.", pd->id, pd->version.toString(),
                                      pkg.spec->path),
                    });
                }
            }

            // Check pending list
            if (auto it = pendingPackages.find(pd->id); it != pendingPackages.end()) {
                const auto &versionMap = it->second;
                if (auto it2 = versionMap.find(pd->version); it2 != versionMap.end()) {
                    return handleDuplicate({
                        Error::DependencyError,
                        stdc::formatN("recursive dependency chain detected: package %1[%2] in %3 is being loaded.",
                                      pd->id, pd->version.toString(), it2->second),
                    });
                }
            }

            pendingPackages[pd->id][pd->version] = pd->path;
        }
        while (false);

        // Refresh dependency cache if needed
        if (packagePathsDirty) {
            std::unique_lock lock(su_mtx);
            for (const auto &[ctxKey, _] : contextPackagePaths)
                refreshPackageIndexes(ctxKey);
        }

        // Load dependencies
        llvm::SmallVector<PackageData *> dependencies;
        auto closeDependencies = [&dependencies, this]
        {
            for (auto it = dependencies.rbegin(); it != dependencies.rend(); ++it) {
                std::ignore = close(*it);
            }
        };

        // Initialize
        {
            Error error1;
            bool failed = false;
            int i = 0;
            for (; i < contributes.size(); ++i) {
                const auto &contribute = contributes[i];
                const auto &cateName = contribute->_impl->category;
                auto it = categories.find(cateName);
                if (it == categories.end()) {
                    error1 = {
                        Error::NotImplementedError,
                        stdc::formatN(".category %1 not found.", cateName),
                    };
                    failed = true;
                    break;
                }
                const auto &cc = it->second;
                if (auto exp = cc->loadSpec(contribute, ModuleSpec::Initialized); !exp) {
                    error1 = exp.error();
                    i--;
                    failed = true;
                    break;
                }
                contribute->_impl->state = ModuleSpec::Initialized;
            }

            if (failed) {
                // Delete
                for (; i >= 0; --i) {
                    const auto &contribute = contributes[i];
                    const auto &cc = categories.at(contribute->_impl->category);
                    std::ignore = cc->loadSpec(contribute, ModuleSpec::Deleted);
                    contribute->_impl->state = ModuleSpec::Deleted;
                }

                closeDependencies();
                pd->err = error1;

                std::unique_lock lock(su_mtx);
                removePending();
                resourcePackages.insert(pd);
                return pd;
            }
        }

        // Get ready
        {
            Error error1;
            bool failed = false;
            int i = 0;
            for (; i < contributes.size(); ++i) {
                const auto &contribute = contributes[i];
                const auto &cc = categories.at(contribute->_impl->category);
                if (auto exp = cc->loadSpec(contribute, ModuleSpec::Ready); !exp) {
                    error1 = exp.error();
                    i--;
                    failed = true;
                    break;
                }
                contribute->_impl->state = ModuleSpec::Ready;
            }

            if (failed) {
                // Finish
                for (; i >= 0; --i) {
                    const auto &contribute = contributes[i];
                    const auto &cc = categories.at(contribute->_impl->category);
                    std::ignore = cc->loadSpec(contribute, ModuleSpec::Finished);
                    contribute->_impl->state = ModuleSpec::Finished;
                }

                // Delete
                for (i = static_cast<int>(contributes.size()) - 1; i >= 0; i--) {
                    const auto &contribute = contributes[i];
                    const auto &cc = categories.at(contribute->_impl->category);
                    std::ignore = cc->loadSpec(contribute, ModuleSpec::Deleted);
                    contribute->_impl->state = ModuleSpec::Deleted;
                }

                closeDependencies();
                pd->err = error1;

                std::unique_lock lock(su_mtx);
                removePending();
                resourcePackages.insert(pd);
                return pd;
            }
        }

        pd->loaded = true;

        // Add to link map
        {
            LoadedPackageBlock pkg;
            pkg.spec = pd;
            pkg.ref = 1;
            pkg.contributes = std::move(contributes);
            pkg.linked = std::move(dependencies);

            std::unique_lock lock(su_mtx);
            removePending();
            auto &[packages, pathIndexes, idIndexes, pointerIndexes] = loadedPackageMap;
            auto it = packages.insert(packages.end(), pkg);
            pathIndexes[pd->path] = it;
            idIndexes[pd->id][pd->version] = it;
            pointerIndexes[pd] = it;
        }
        return pd;
    }

    bool PackageManager::Impl::close(PackageData *spec) {
        if (!spec->loaded) {
            std::unique_lock lock(su_mtx);
            const auto it = resourcePackages.find(spec);
            if (it == resourcePackages.end()) {
                return false;
            }

            resourcePackages.erase(it);
            delete spec;
            return true;
        }

        LoadedPackageBlock pkgToClose;
        {
            std::unique_lock lock(su_mtx);
            auto &[packages, pathIndexes, idIndexes, pointerIndexes] = loadedPackageMap;
            const auto it = pointerIndexes.find(spec);
            if (it == pointerIndexes.end()) {
                return false;
            }

            const auto it1 = it->second;
            auto &pkg = *it1;
            pkg.ref--;
            if (pkg.ref != 0) {
                return true;
            }
            pkgToClose = std::move(pkg);

            packages.erase(it1);
            pointerIndexes.erase(it);
            pathIndexes.erase(spec->path);

            // Remove id indexes
            const auto it2 = idIndexes.find(spec->id);
            auto &versionMap = it2->second;
            versionMap.erase(spec->version);
            if (versionMap.empty()) {
                idIndexes.erase(it2);
            }
        }

        // Finish and delete
        {
            // Finish
            for (auto it = pkgToClose.contributes.rbegin(); it != pkgToClose.contributes.rend(); ++it) {
                const auto &contribute = *it;
                const auto &cc = categories.at(contribute->_impl->category);
                std::ignore = cc->loadSpec(contribute, ModuleSpec::Finished);
                contribute->_impl->state = ModuleSpec::Finished;
            }

            // Delete
            for (auto it = pkgToClose.contributes.rbegin(); it != pkgToClose.contributes.rend(); ++it) {
                const auto &contribute = *it;
                const auto &cc = categories.at(contribute->_impl->category);
                std::ignore = cc->loadSpec(contribute, ModuleSpec::Deleted);
                contribute->_impl->state = ModuleSpec::Deleted;
            }
        }

        // Unload dependencies
        for (auto it = pkgToClose.linked.rbegin(); it != pkgToClose.linked.rend(); ++it) {
            std::ignore = close(*it);
        }

        delete spec;
        return true;
    }

    void PackageManager::Impl::closeAllLoadedPackages() {
        while (!loadedPackageMap.packages.empty()) {
            const auto spec = loadedPackageMap.packages.back().spec;
            std::ignore = close(spec);
        }
    }

    void PackageManager::Impl::refreshPackageIndexes(const ContextKey &ctxKey) {
        auto &cachedIndexes = contextCachedIndexes[ctxKey];
        cachedIndexes.clear();
        auto pathsIt = contextPackagePaths.find(ctxKey);
        if (pathsIt == contextPackagePaths.end())
            return;
        for (const auto &path : std::as_const(pathsIt->second)) {
            if (!fs::is_directory(path)) {
                continue;
            }
            for (const auto &entry : fs::directory_iterator(path)) {
                const auto filename = entry.path().filename();
                if (!entry.is_directory()) {
                    continue;
                }

                JsonObject obj;
                if (auto exp = PackageData::readDesc(entry.path()); !exp) {
                    continue;
                } else {
                    obj = exp.take();
                }

                // Search id, version, compatVersion
                std::string id_;
                stdc::VersionNumber version_;
                stdc::VersionNumber compatVersion_;

                // id
                {
                    auto it = obj.find("id");
                    if (it == obj.end()) {
                        continue;
                    }
                    id_ = it->second.toString();
                    if (!ModuleLocator::isValidLocator(id_)) {
                        continue;
                    }
                }
                // version
                {
                    auto it = obj.find("version");
                    if (it == obj.end()) {
                        continue;
                    }
                    version_ = stdc::VersionNumber::fromString(it->second.toString());
                }
                // compatVersion
                {
                    if (auto it = obj.find("compatVersion"); it != obj.end()) {
                        compatVersion_ = stdc::VersionNumber::fromString(it->second.toString());
                    } else {
                        compatVersion_ = version_;
                    }
                }

                // Store
                cachedIndexes[id_][version_] = {
                    fs::canonical(entry.path()),
                    compatVersion_,
                };
            }
        }

        packagePathsDirty = false;
    }

    PackageManager::PackageManager() : PluginFactory(*new Impl(this)) {}

    PackageManager::~PackageManager() = default;

    ModuleCategory *PackageManager::category(const std::string_view &name) const {
        __stdc_impl_t;
        const auto it = impl.categories.find(name);
        if (it == impl.categories.end()) {
            return nullptr;
        }
        return it->second;
    }

    bool PackageManager::checkDependencies() {
        // Collect all module metadata across all contexts
        std::vector<ModuleMetadata> allModuleInfos;
        for (const auto &[ctxKey, _] : _impl->contextPackagePaths) {
            auto moduleInfos = this->getModuleMetadatas(ctxKey);
            allModuleInfos.insert(allModuleInfos.end(), moduleInfos.begin(), moduleInfos.end());
        }

        if (allModuleInfos.empty()) {

            MgrLog.langCoreCritical("Dependency resolution failed: No modules found. Cannot load packages.");

            MgrLog.langCoreCritical("Possible causes:");

            MgrLog.langCoreCritical("  1. package.json files are missing or corrupted");

            MgrLog.langCoreCritical("  2. Module definitions in package.json are invalid");

            MgrLog.langCoreCritical("  3. Package paths are incorrect");

            return false;
        }


        // Level 兼容性检查（Strict 模式：不兼容插件拒绝加载）
        LevelCompatibilityChecker::LevelConfig levelConfig;
        levelConfig.currentLevel = _impl->currentLevel;
        levelConfig.minimumLevel = _impl->minimumLevel;
        levelConfig.maximumLevel = _impl->maximumLevel;

        // §14.14 fix: collect ALL incompatible modules before returning, not just the first
        bool hasIncompatible = false;
        for (const auto &info : allModuleInfos) {
            auto checkResult = LevelCompatibilityChecker::checkCorePlugin(info.level, levelConfig);

            if (!checkResult.isCompatible) {
                MgrLog.langCoreCritical("Level Compatibility Check Failed:");
                MgrLog.langCoreCritical("  Module: %1:%2", info.packageId, info.moduleId.c_str());
                MgrLog.langCoreCritical("  Module Level: %1", std::to_string(info.level));
                MgrLog.langCoreCritical("  System Level Range: %1-%2", std::to_string(_impl->minimumLevel),
                                        std::to_string(levelConfig.getEffectiveMaximumLevel()));
                MgrLog.langCoreCritical("  Details: %1", checkResult.message);
                MgrLog.langCoreCritical("  Suggestion: %1", checkResult.suggestion);

                // 保存错误信息到列表
                _impl->dependencyErrors.push_back(
                    stdc::formatN("Module %1:%2 (Level %3) not compatible with system (Level %4-%5): %6",
                                  info.packageId, info.moduleId.c_str(), std::to_string(info.level),
                                  std::to_string(_impl->minimumLevel),
                                  std::to_string(levelConfig.getEffectiveMaximumLevel()),
                                  checkResult.message));

                MgrLog.langCoreCritical("Strict compatibility mode: rejecting incompatible module.");

                hasIncompatible = true;
            }
        }

        if (hasIncompatible) {
            return false;
        }


        // §14.22 fix: clear dependency graph before adding modules to ensure
        // repeated calls don't accumulate stale data
        _impl->dependencyGraph.clear();

        for (const auto &info : allModuleInfos)

            _impl->dependencyGraph.addModule(info);


        if (!_impl->dependencyGraph.buildGraph()) {

            MgrLog.langCoreCritical("Failed to build dependency graph due to missing dependencies");

            MgrLog.langCoreCritical("Please check that all dependencies are correctly declared in package.json");

            MgrLog.langCoreCritical("and that the required plugin packages are available.");

            return false;
        }


        if (const auto cycles = _impl->dependencyGraph.findCycles(); !cycles.empty()) {

            MgrLog.langCoreCritical("Found %1 dependency cycle(s) - circular dependencies detected:", cycles.size());

            for (size_t i = 0; i < cycles.size(); ++i) {

                MgrLog.langCoreCritical("Cycle %1:", i + 1);

                for (const auto &module : cycles[i]) {

                    MgrLog.langCoreCritical("  - %1:%2 %3", module.packageId, module.moduleId, module.version);
                }
            }

            MgrLog.langCoreCritical("Circular dependencies are not allowed. Please modify dependency declarations.");

            return false;
        }

        return true;
    }

    std::vector<PackageInitializationPlan> PackageManager::getPackageInitializationOrder() {
        if (!this->checkDependencies()) {
            MgrLog.langCoreCritical(
                "Failed to determine package initialization order due to dependency or compatibility issues");
            return {};
        }
        return _impl->dependencyGraph.getPackageInitializationOrder();
    }

    std::vector<std::string> PackageManager::getDependencyErrors() const {
        __stdc_impl_t;
        std::shared_lock lock(impl.su_mtx);
        return impl.dependencyErrors;
    }

    Expected<void> PackageManager::addPackagePath(const std::string &context, const std::filesystem::path &path) {
        return addPackagePath(context, {}, path);
    }

    Expected<void> PackageManager::addPackagePath(const std::string &context, const stdc::VersionNumber &version,
                                                   const std::filesystem::path &path) {
        if (auto exp = ContextUtils::validateContextName(context); !exp)
            return exp.error();

        __stdc_impl_t;
        if (!fs::exists(path) || !fs::is_directory(path)) {
            return Error(Error::FileSystemError, stdc::formatN("Package path does not exist or is not a directory: %1", path));
        }

        auto canonical = fs::canonical(path);
        ContextKey ctxKey(context, version);

        std::unique_lock lock(impl.su_mtx);
        auto &paths = impl.contextPackagePaths[ctxKey];
        // Check duplicate
        for (const auto &existing : paths) {
            if (existing == canonical) {
                MgrLog.langCoreDebug("Duplicate package path skipped for context '%1': %2", ctxKey.toString(), canonical);
                return {};
            }
        }
        paths.push_back(canonical);
        impl.packagePathsDirty = true;
        return {};
    }

    Expected<void> PackageManager::setPackagePaths(const std::string &context,
                                                   const std::vector<std::filesystem::path> &paths) {
        return setPackagePaths(context, {}, paths);
    }

    Expected<void> PackageManager::setPackagePaths(const std::string &context, const stdc::VersionNumber &version,
                                                   const std::vector<std::filesystem::path> &paths) {
        if (auto exp = ContextUtils::validateContextName(context); !exp)
            return exp.error();

        __stdc_impl_t;
        ContextKey ctxKey(context, version);
        std::unique_lock lock(impl.su_mtx);
        auto &ctxPaths = impl.contextPackagePaths[ctxKey];
        ctxPaths.clear();
        for (const auto &path : paths) {
            if (!fs::is_directory(path)) {
                continue;
            }
            ctxPaths.push_back(fs::canonical(path));
        }
        impl.packagePathsDirty = true;
        return {};
    }

    std::vector<std::filesystem::path> PackageManager::packagePaths(const std::string &context) const {
        return packagePaths(context, {});
    }

    std::vector<std::filesystem::path> PackageManager::packagePaths(const std::string &context,
                                                                     const stdc::VersionNumber &version) const {
        __stdc_impl_t;
        ContextKey ctxKey(context, version);
        std::shared_lock lock(impl.su_mtx);
        auto it = impl.contextPackagePaths.find(ctxKey);
        if (it == impl.contextPackagePaths.end())
            return {};
        return {it->second.begin(), it->second.end()};
    }

    std::vector<std::string> PackageManager::contexts() const {
        __stdc_impl_t;
        std::shared_lock lock(impl.su_mtx);
        std::vector<std::string> result;
        for (const auto &[ctxKey, _] : impl.contextPackagePaths)
            result.push_back(ctxKey.context);
        return result;
    }

    std::vector<ContextKey> PackageManager::contextKeys() const {
        __stdc_impl_t;
        std::shared_lock lock(impl.su_mtx);
        std::vector<ContextKey> result;
        for (const auto &[ctxKey, _] : impl.contextPackagePaths)
            result.push_back(ctxKey);
        return result;
    }

    Expected<Package> PackageManager::open(const std::filesystem::path &path) {
        __stdc_impl_t;
        auto result = impl.open(path);
        if (!result) {
            return result.error();
        }
        return Package(result.get());
    }

    Package PackageManager::find(const std::string_view &id, const stdc::VersionNumber &version) const {
        __stdc_impl_t;
        std::shared_lock lock(impl.su_mtx);
        auto &pkgMap = impl.loadedPackageMap;
        const auto it = pkgMap.idIndexes.find(id);
        if (it == pkgMap.idIndexes.end()) {
            return {};
        }

        auto &versionMap = it->second;
        const auto it2 = versionMap.find(version);
        if (it2 == versionMap.end()) {
            return {};
        }
        return Package(it2->second->spec);
    }

    std::vector<Package> PackageManager::find(const std::string_view &id) const {
        __stdc_impl_t;
        std::shared_lock lock(impl.su_mtx);
        auto &pkgMap = impl.loadedPackageMap;
        const auto it = pkgMap.idIndexes.find(id);
        if (it == pkgMap.idIndexes.end()) {
            return {};
        }

        auto &versionMap = it->second;
        std::vector<Package> res;
        res.reserve(versionMap.size());
        for (const auto &[fst, snd] : versionMap) {
            res.push_back(Package(snd->spec));
        }
        return res;
    }

    std::vector<Package> PackageManager::packages() const {
        __stdc_impl_t;
        std::shared_lock lock(impl.su_mtx);
        auto &list = impl.loadedPackageMap.packages;

        std::vector<Package> res;
        res.reserve(list.size());
        for (const auto &item : list) {
            res.push_back(Package(item.spec));
        }
        return res;
    }

    bool PackageManager::loadPackagesInOrder() {
        const auto packageOrder = this->getPackageInitializationOrder();
        if (packageOrder.empty()) {
            MgrLog.langCoreCritical("Failed to determine package initialization order");
            return false;
        }

        int pkgSize = 0;
        int failedPkgCount = 0;

        for (const auto &packageInfo : packageOrder) {
            MgrLog.langCoreInfo("Loading package: %1 from %2", packageInfo.packageId, packageInfo.packagePath);

            auto exp = this->open(packageInfo.packagePath);
            if (!exp) {
                MgrLog.langCoreCritical("Failed to open package %1: %2", packageInfo.packageId, exp.error().message());
                failedPkgCount++;
                continue;
            }

            Package pkg = exp.take();
            if (!pkg.isLoaded()) {
                MgrLog.langCoreCritical("Failed to load package %1: %2", packageInfo.packageId, pkg.error().message());
                failedPkgCount++;
                continue;
            }

            pkgSize++;

            for (const auto &moduleInfo : packageInfo.initializationOrder) {
                if (auto taskExp = createModuleTask(moduleInfo, pkg)) {
                    MgrLog.langCoreInfo("  Created task for module: %1 (type: %2, class: %3)", moduleInfo.moduleId,
                                        moduleInfo.type, moduleInfo.iid);
                } else {
                    MgrLog.langCoreCritical("  Failed to create task for module: %1 (package=%2, type=%3, iid=%4): %5",
                                            moduleInfo.moduleId, moduleInfo.packageId, moduleInfo.type, moduleInfo.iid,
                                            taskExp.error().message());
                }
            }
        }
        MgrLog.langCoreInfo("Successfully loaded %1 packages.", pkgSize);
        if (failedPkgCount > 0) {
            MgrLog.langCoreCritical("%1 package(s) failed to load.", failedPkgCount);
            return false;
        }
        return true;
    }

    Expected<NO<Task>> PackageManager::createModuleTask(const ModuleMetadata &moduleInfo, const Package &pkg) const {
        const auto moduleSpec = pkg.moduleSpec(moduleInfo.type, moduleInfo.moduleId);
        if (!moduleSpec) {
            // 获取指定类型的所有模块 ID
            auto specs = pkg.moduleSpecs(moduleInfo.type);
            std::vector<std::string> availableIds;
            for (const auto *spec : specs) {
                availableIds.push_back(spec->id());
            }

            return Error(
                Error::FileSystemError,
                stdc::formatN(
                    "Module not found: package=%1, moduleId=%2, type=%3, iid=%4. Available modules in this package: %5",
                    moduleInfo.packageId, moduleInfo.moduleId, moduleInfo.type, moduleInfo.iid,
                    stdc::join(availableIds, ", ")));
        }

        // 使用完整的 iid 作为 pluginKey
        const auto taskPlugin = this->plugin<TaskPlugin>("org.openvpi.Task", moduleInfo.iid.c_str());
        if (!taskPlugin) {
            // 尝试获取同一 iid 下的所有可用插件
            auto allTaskPlugins = this->plugins("org.openvpi.Task");
            std::string availableKeys;
            for (const auto *plugin : allTaskPlugins) {
                if (!availableKeys.empty())
                    availableKeys += ", ";
                availableKeys += plugin->key();
            }

            return Error(Error::FileSystemError,
                         stdc::formatN("Failed to load FactoryPlugin: package=%1, module=%2/%3, type=%4, iid=%5. "
                                       "Expected plugin with iid='org.openvpi.Task' and key='%5'. "
                                       "Available plugins (iid=org.openvpi.Task): [%6]. "
                                       "Configuration file: %7",
                                       moduleInfo.packageId, moduleInfo.packageId, moduleInfo.moduleId, moduleInfo.type,
                                       moduleInfo.iid, availableKeys.empty() ? "none" : availableKeys,
                                       moduleInfo.configuration));
        }


        auto taskExp = taskPlugin->createTask(moduleSpec);
        if (!taskExp) {
            return Error(
                Error::RuntimeError,
                stdc::formatN(
                    "Failed to create task instance: package=%1, module=%2/%3, type=%4, iid=%5. "
                    "Error: %6. "
                    "Possible causes: 1) Plugin class not found, 2) Constructor failed, 3) Memory allocation failed",
                    moduleInfo.packageId, moduleInfo.packageId, moduleInfo.moduleId, moduleInfo.type, moduleInfo.iid,
                    taskExp.error().message()));
        }

        auto task = taskExp.take();

        auto initResult = task->initialize();
        if (!initResult) {
            return Error(Error::RuntimeError,
                         stdc::formatN("Failed to initialize task: package=%1, module=%2/%3, type=%4, iid=%5. "
                                       "Error: %6. "
                                       "Possible causes: 1) Initialization code failed, 2) Required resources not "
                                       "found, 3) Dependency plugin not loaded",
                                       moduleInfo.packageId, moduleInfo.packageId, moduleInfo.moduleId, moduleInfo.type,
                                       moduleInfo.iid, initResult.error().message()));
        }

        auto &ic = *this->category(moduleSpec->category());
        const auto fqid = ContextUtils::formatFqid(ContextKey(moduleInfo.context, moduleInfo.contextVersion), moduleSpec->id());
        ic.addObject(fqid, task);
        return task;
    }

    void PackageManager::registerCategoryFactory(ModuleCategory *(*fac)(PackageManager *)) {
        Impl::categoryFactories.push_back(fac);
    }

    Expected<JsonObject> readJsonFile(const std::filesystem::path &path) {
        const std::ifstream file(path);
        if (!file.is_open()) {
            return Error{
                Error::FileSystemError,
                stdc::formatN("%1: failed to open package manifest.", path),
            };
        }

        std::stringstream ss;
        ss << file.rdbuf();

        std::string error2;
        const auto root = JsonValue::fromJson(ss.str(), true, &error2);
        if (!error2.empty()) {
            return Error{
                Error::ConfigError,
                stdc::formatN("%1: Invalid package manifest format: %2.", path, error2),
            };
        }
        if (!root.isObject()) {
            return Error{
                Error::ConfigError,
                stdc::formatN("%1: Invalid package manifest format: not an object.", path),
            };
        }
        return root.toObject();
    }

    void PackageManager::collectModuleMetadata(const ContextKey &ctxKey, const std::string &packageId,
                                                const std::filesystem::path &packageDir,
                                                const JsonObject &modulesObj) {
        __stdc_impl_t;
        if (!fs::is_directory(packageDir)) {
            MgrLog.langCoreCritical("Invalid package path %1.", packageDir);
            return;
        }

        for (const auto &[moduleType, moduleArray] : modulesObj) {
            const auto &modules = moduleArray.toArray();

            for (const auto &moduleEntry : modules) {
                const auto &moduleObj = moduleEntry.toObject();
                ModuleMetadata info;
                info.context = ctxKey.context;
                info.contextVersion = ctxKey.version;
                info.packageId = packageId;
                info.packagePath = packageDir;
                info.type = moduleType;
                info.level = 0;

                extractModuleMetadataFromJson(packageId, moduleObj, info);

                // Validate moduleId doesn't contain ':'
                if (auto valExp = ContextUtils::validateModuleId(info.moduleId); !valExp) {
                    MgrLog.langCoreCritical("I-5: %1", valExp.error().message());
                    continue;
                }

                if (!info.configuration.empty()) {
                    if (std::filesystem::path configPath = packageDir / info.configuration;
                        fs::exists(configPath) && fs::is_regular_file(configPath)) {
                        try {
                            if (auto configExp = readJsonFile(configPath)) {
                                const auto &configObj = configExp.take();

                                if (auto verIt = configObj.find("$version"); verIt != configObj.end()) {
                                    info.version = verIt->second.toString();
                                }

                                if (auto levelIt = configObj.find("level"); levelIt != configObj.end()) {
                                    info.level = levelIt->second.toInt();
                                }
                            }
                        }
                        catch (const std::exception &e) {
                            std::string errorMsg = std::string("Failed to read module config at ") +
                                                  configPath.string() + ": " + e.what();
                            MgrLog.langCoreCritical(errorMsg);
                            std::unique_lock lock(impl.su_mtx);
                            impl.dependencyErrors.push_back(errorMsg);
                        }
                        catch (...) {
                            std::string errorMsg = std::string("Unknown exception reading module config at ") +
                                                  configPath.string();
                            MgrLog.langCoreCritical(errorMsg);
                            std::unique_lock lock(impl.su_mtx);
                            impl.dependencyErrors.push_back(errorMsg);
                        }
                    }
                }

                if (info.moduleId.empty() || info.iid.empty() || info.type.empty()) {
                    MgrLog.langCoreCritical("Module missing required fields in package: %1.", packageId);
                    continue;
                }

                auto &moduleInfoSet = impl.contextModuleInfoSets[ctxKey];
                auto &moduleInfos = impl.contextModuleInfos[ctxKey];

                if (auto [it, inserted] = moduleInfoSet.insert(info); !inserted) {
                    const ModuleMetadata &existing = *it;
                    std::ostringstream oss;
                    oss << "Error: Duplicate main module found!" << std::endl
                        << "  ModuleId: " << info.moduleId << std::endl
                        << "  Class: " << info.iid << std::endl
                        << "  Type: " << info.type << std::endl
                        << "  Level: " << info.level << std::endl
                        << "  Version: " << info.version << std::endl
                        << "  Configuration: " << (info.configuration.empty() ? "(empty)" : info.configuration)
                        << std::endl
                        << "  Context: " << (ctxKey.context.empty() ? "(default)" : ctxKey.toString()) << std::endl
                        << "  Existing location: Package=" << existing.packageId << " (v" << existing.version << ")"
                        << std::endl
                        << "  New location: Package=" << info.packageId << " (v" << info.version << ")" << std::endl;
                    MgrLog.langCoreCritical(oss.str());
                } else {
                    moduleInfos.push_back(info);
                }
            }
        }
    }

    void PackageManager::extractModuleMetadataFromJson(const std::string &packageId, const JsonObject &moduleEntry,
                                                       ModuleMetadata &info) {
        info.packageId = packageId;

        if (const auto moduleIdIt = moduleEntry.find("moduleId"); moduleIdIt != moduleEntry.end()) {
            info.moduleId = moduleIdIt->second.toString();
        }

        if (const auto classIt = moduleEntry.find("class"); classIt != moduleEntry.end()) {
            info.iid = classIt->second.toString();
        }

        if (const auto configIt = moduleEntry.find("configuration"); configIt != moduleEntry.end()) {
            info.configuration = configIt->second.toString();
        }

        if (const auto levelInit = moduleEntry.find("level"); levelInit != moduleEntry.end()) {
            info.level = levelInit->second.toInt();
        }

        if (const auto depsIt = moduleEntry.find("dependencies"); depsIt != moduleEntry.end()) {
            const auto &depsArray = depsIt->second.toArray();
            for (const auto &dep : depsArray) {
                const auto &depObj = dep.toObject();

                DependencyRequirement depRaw;

                if (auto depPackageIdIt = depObj.find("packageId"); depPackageIdIt != depObj.end()) {
                    depRaw.packageId = depPackageIdIt->second.toString();
                }

                if (auto depModuleIdIt = depObj.find("moduleId"); depModuleIdIt != depObj.end()) {
                    depRaw.moduleId = depModuleIdIt->second.toString();
                }

                if (auto levelIt = depObj.find("level"); levelIt != depObj.end()) {
                    depRaw.level = levelIt->second.toInt();
                }

                if (auto versionIt = depObj.find("version"); versionIt != depObj.end()) {
                    depRaw.versionRange = versionIt->second.toString();
                }

                if (!depRaw.packageId.empty() && !depRaw.moduleId.empty()) {
                    info.requirements.push_back(depRaw);
                }
            }
        }
    }

    std::vector<ModuleMetadata> PackageManager::getModuleMetadatas(const std::string &context) {
        return getModuleMetadatas(ContextKey(context));
    }

    std::vector<ModuleMetadata> PackageManager::getModuleMetadatas(const ContextKey &ctxKey) {
        __stdc_impl_t;
        std::unique_lock lock(impl.su_mtx);

        if (impl.dependencyResolutionSuccessful && !impl.contextModuleInfos[ctxKey].empty()) {
            return impl.contextModuleInfos[ctxKey];
        }

        impl.contextModuleInfos[ctxKey].clear();
        impl.contextModuleInfoSets[ctxKey].clear();
        impl.dependencyErrors.clear();
        impl.dependencyResolutionSuccessful = true;

        auto pathsIt = impl.contextPackagePaths.find(ctxKey);
        if (pathsIt == impl.contextPackagePaths.end())
            return {};

        std::vector<std::filesystem::path> uniquePaths;
        {
            std::unordered_set<std::string> seenPaths;
            for (const auto &path : pathsIt->second) {
                std::error_code ec;
                auto canonical = fs::canonical(path, ec);
                if (ec || !fs::exists(canonical) || !fs::is_directory(canonical)) {
                    continue;
                }
                if (std::string pathStr = canonical.string(); seenPaths.insert(pathStr).second) {
                    uniquePaths.push_back(canonical);
                }
            }
        }

        for (const auto &basePath : uniquePaths) {
            scanPackageDirectory(ctxKey, basePath);
        }

        printDiscoveryInfo(uniquePaths.size(), impl.contextModuleInfos[ctxKey].size());

        if (!impl.contextModuleInfos[ctxKey].empty()) {
            // For non-default contexts, pass default context modules as fallback
            std::vector<ModuleMetadata> fallback;
            if (!ctxKey.context.empty()) {
                auto defIt = impl.contextModuleInfos.find(ContextKey(""));
                if (defIt != impl.contextModuleInfos.end())
                    fallback = defIt->second;
            }
            if (!impl.resolveModuleDependencies(ctxKey, fallback))
                return {};
        }

        auto &moduleInfos = impl.contextModuleInfos[ctxKey];
        std::vector<ModuleMetadata> result;
        std::copy_if(moduleInfos.begin(), moduleInfos.end(), std::back_inserter(result),
                     [](const ModuleMetadata &info)
                     { return info.requirements.size() == info.resolvedDependencies.size(); });
        return result;
    }

    bool PackageManager::Impl::resolveModuleDependencies(const ContextKey &ctxKey,
                                                         const std::vector<ModuleMetadata> &fallbackModules) {
        DependencyResolver resolver;

        dependencyErrors.clear();
        dependencyResolutionSuccessful = true;

        auto &moduleInfos = contextModuleInfos[ctxKey];

        MgrLog.langCoreInfo("Starting module dependency resolution for context '%1'",
                            ctxKey.isDefault() ? "(default)" : ctxKey.toString());
        MgrLog.langCoreInfo("Module count: %1", moduleInfos.size());

        if (!resolver.resolveAllDependencies(moduleInfos, fallbackModules)) {
            dependencyResolutionSuccessful = false;
            dependencyErrors = resolver.getErrors();

            if (!dependencyErrors.empty()) {
                MgrLog.langCoreCritical("Dependency resolution failed");
                MgrLog.langCoreCritical("Cannot proceed with module initialization.");
                MgrLog.langCoreCritical("========================================");

                for (const auto &dependencyError : dependencyErrors) {
                    MgrLog.langCoreCritical(dependencyError);
                }

                MgrLog.langCoreCritical("========================================");
                MgrLog.langCoreCritical("%1 dependency errors found", dependencyErrors.size());
                return false;
            }

            return false;
        }

        moduleInfos = resolver.getResolvedModules();

        MgrLog.langCoreInfo("Dependency resolution successful, resolved modules: %1", moduleInfos.size());

        auto &moduleInfoSet = contextModuleInfoSets[ctxKey];
        moduleInfoSet.clear();
        moduleInfoSet.insert(moduleInfos.begin(), moduleInfos.end());

        return true;
    }

    void PackageManager::scanPackageDirectory(const ContextKey &ctxKey, const std::filesystem::path &basePath) {
        if (!fs::exists(basePath) || !fs::is_directory(basePath)) {
            return;
        }

        for (const auto &entry : fs::directory_iterator(basePath)) {
            if (!entry.is_directory()) {
                continue;
            }

            if (auto descPath = entry.path() / "package.json";
                !fs::exists(descPath) || !fs::is_regular_file(descPath)) {
                continue;
            }

            processPackageJson(ctxKey, entry.path());
        }
    }

    void PackageManager::processPackageJson(const ContextKey &ctxKey, const std::filesystem::path &packageDir) {
        const auto &descPath = packageDir / _TSTR("package.json");
        auto exp = PackageData::readDesc(descPath);
        if (!exp)
            return;

        JsonObject obj = exp.take();

        const auto idIt = obj.find("packageId");
        if (idIt == obj.end()) {
            return;
        }
        const std::string id_ = idIt->second.toString();

        if (const auto modulesIt = obj.find("modules"); modulesIt != obj.end()) {
            const auto &modulesObj = modulesIt->second.toObject();
            this->collectModuleMetadata(ctxKey, id_, descPath.parent_path(), modulesObj);
        }
    }

    void PackageManager::printDiscoveryInfo(const size_t pathCount, const size_t moduleCount) {
        if (moduleCount == 0) {
            MgrLog.langCoreCritical(
                "NO MODULES FOUND:\nNo modules were discovered in the package paths.\n"
                "Scanned %1 package path(s).", pathCount);
        } else {
            MgrLog.langCoreInfo("MODULE DISCOVERY COMPLETE - Scanned %1 package path(s) - Discovered %2 module(s)",
                                pathCount, moduleCount);
        }
    }
} // namespace LangCore
