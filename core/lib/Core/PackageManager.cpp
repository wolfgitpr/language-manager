#include <LangCore/Core/PackageManager.h>

#include "Package_p.h"

#include <fstream>
#include <iostream>
#include <mutex>
#include <set>

#include <stdcorelib/path.h>
#include <stdcorelib/pimpl.h>
#include <stdcorelib/stlextra/algorithms.h>

#include <LangCore/Module/Dependency/DependencyResolver.h>
#include <LangCore/Package/Package.h>
#include <LangCore/Support/Expected.h>
#include <LangCore/Support/JSON.h>

#include "Module_p.h"
#include "PackageManager_p.h"
#include "PluginFactory_p.h"
#include "TaskFactoryPlugin.h"

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
                Error::FileNotOpen,
                stdc::formatN(R"(invalid package path "%1")", path),
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
            Error error1;

            // Check if a package with same id and version but different path is loaded
            {
                if (auto it = pkgMap.idIndexes.find(pd->id); it != pkgMap.idIndexes.end()) {
                    const auto &versionMap = it->second;
                    if (auto it2 = versionMap.find(pd->version); it2 != versionMap.end()) {
                        auto pkg = *it2->second;
                        error1 = {
                            Error::FileDuplicated,
                            stdc::formatN(R"(duplicated package "%1[%2]" in "%3" is loaded)", pd->id,
                                          pd->version.toString(), pkg.spec->path),
                        };
                        goto out_dup;
                    }
                }
            }

            // Check pending list
            {
                if (auto it = pendingPackages.find(pd->id); it != pendingPackages.end()) {
                    const auto &versionMap = it->second;
                    if (auto it2 = versionMap.find(pd->version); it2 != versionMap.end()) {
                        error1 = {
                            Error::RecursiveDependency,
                            stdc::formatN(
                                R"(recursive dependency chain detected: package "%1[%2]" in %3 is being loaded)",
                                pd->id, pd->version.toString(), it2->second),
                        };
                        goto out_dup;
                    }
                }
            }

            pendingPackages[pd->id][pd->version] = pd->path;
            break;

        out_dup:
            pd->err = error1;
            resourcePackages.insert(pd);
            return pd;
        }
        while (false);

        // Refresh dependency cache if needed
        if (packagePathsDirty) {
            std::unique_lock lock(su_mtx);
            refreshPackageIndexes();
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
                        Error::FeatureNotSupported,
                        stdc::formatN(R"(category "%1" not found)", cateName),
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

    void PackageManager::Impl::refreshPackageIndexes() {
        cachedPackageIndexesMap.clear();
        for (const auto &path : std::as_const(packagePaths)) {
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
                cachedPackageIndexesMap[id_][version_] = {
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

    PackageManager *PackageManager::instance() {
        static PackageManager instance;
        return &instance;
    }

    bool PackageManager::checkDependencies() {
        const auto moduleInfos = this->getModuleMetadatas();

        if (moduleInfos.empty()) {
            std::cerr << "Dependency resolution failed. Cannot load packages." << std::endl;
            return false;
        }

        for (const auto &info : moduleInfos) {
            if (!_impl->dependencyGraph.addModule(info)) {
                std::cerr << "Failed to add module to graph: " << info.packageId << ":" << info.moduleId << std::endl;
            }
        }

        if (!_impl->dependencyGraph.buildGraph()) {
            std::cerr << "Failed to build dependency graph due to missing dependencies" << std::endl;
            return false;
        }

        if (const auto cycles = _impl->dependencyGraph.findCycles(); !cycles.empty()) {
            std::cout << "Found " << cycles.size() << " dependency cycle(s):" << std::endl;
            for (size_t i = 0; i < cycles.size(); ++i) {
                std::cout << "Cycle " << (i + 1) << ":" << std::endl;
                for (const auto &module : cycles[i]) {
                    std::cout << "  " << module.packageId << ":" << module.moduleId << " v" << module.version
                              << std::endl;
                }
            }
            std::cerr << "Cannot load packages." << std::endl;
            return false;
        }
        return true;
    }

    std::vector<PackageInitializationPlan> PackageManager::getPackageInitializationOrder() {
        this->checkDependencies();
        return _impl->dependencyGraph.getPackageInitializationOrder();
    }

    void PackageManager::addPackagePaths(const stdc::array_view<std::filesystem::path> paths) {
        __stdc_impl_t;
        std::unique_lock lock(impl.su_mtx);
        for (const auto &path : paths) {
            if (!fs::is_directory(path)) {
                continue;
            }
            impl.packagePaths.push_back(fs::canonical(path));
            impl.packagePathsDirty = true;
        }
    }

    void PackageManager::setPackagePaths(const stdc::array_view<std::filesystem::path> paths) {
        __stdc_impl_t;
        std::unique_lock lock(impl.su_mtx);
        impl.packagePaths.clear();
        for (const auto &path : paths) {
            if (!fs::is_directory(path)) {
                continue;
            }
            impl.packagePaths.push_back(fs::canonical(path));
            if (!impl.packagePathsDirty) {
                impl.packagePathsDirty = true;
            }
        }
    }

    std::vector<std::filesystem::path> PackageManager::packagePaths() const {
        __stdc_impl_t;
        std::shared_lock lock(impl.su_mtx);
        return {impl.packagePaths.begin(), impl.packagePaths.end()};
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
            return Package();
        }

        auto &versionMap = it->second;
        const auto it2 = versionMap.find(version);
        if (it2 == versionMap.end()) {
            return Package();
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
            std::cerr << "Failed to determine package initialization order" << std::endl;
            return false;
        }

        std::set<std::string> iidHistory;
        auto &ic = *this->category("engine");
        for (const auto &packageInfo : packageOrder) {
            for (const auto &moduleInfo : packageInfo.modules) {
                if (iidHistory.find(moduleInfo.iid) != iidHistory.end())
                    continue;
                const auto taskFactoryPlugin = this->plugin<TaskFactoryPlugin>(moduleInfo.iid.c_str());
                if (!taskFactoryPlugin) {
                    std::cerr << "Failed to load FactoryPlugin: " << moduleInfo.iid << std::endl;
                    return false;
                }
                const auto &task = taskFactoryPlugin->create();
                ic.addObject(moduleInfo.iid, task);
                iidHistory.insert(moduleInfo.iid);
            }
        }

        int pkgSize = 0;

        for (const auto &packageInfo : packageOrder) {
            std::cout << "Loading package: " << packageInfo.packageId << " from " << packageInfo.packagePath
                      << std::endl;

            auto exp = this->open(packageInfo.packagePath);
            if (!exp) {
                std::cerr << "Failed to open package " << packageInfo.packageId << ": " << exp.error().message()
                          << std::endl;
                continue;
            }

            Package pkg = exp.take();
            if (!pkg.isLoaded()) {
                std::cerr << "Failed to load package " << packageInfo.packageId << ": " << pkg.error().message()
                          << std::endl;
                continue;
            }

            pkgSize++;

            for (const auto &moduleInfo : packageInfo.initializationOrder) {
                if (auto taskExp = createModuleTask(moduleInfo, pkg)) {
                    std::cout << "  Created task for module: " << moduleInfo.moduleId << " (type: " << moduleInfo.type
                              << ", class: " << moduleInfo.iid << ")" << std::endl;
                } else {
                    std::cerr << "  Failed to create task for module: " << moduleInfo.moduleId << ": "
                              << taskExp.error().message() << std::endl;
                }
            }
        }
        std::cout << "\nSuccessfully loaded " << pkgSize << " packages" << std::endl;
        return true;
    }

    Expected<NO<Task>> PackageManager::createModuleTask(const ModuleMetadata &moduleInfo, const Package &pkg) const {
        const auto moduleSpec = pkg.moduleSpec(moduleInfo.type, moduleInfo.moduleId);
        if (!moduleSpec) {
            return Error(Error::FileNotFound,
                         stdc::formatN("Module %1 not found in package %2", moduleInfo.moduleId, pkg.id()));
        }

        const auto &moduleCategory = *this->category("engine");
        const auto taskFactory = moduleCategory.getFirstObject(moduleInfo.iid).as<TaskFactory>();
        if (!taskFactory) {
            return Error(Error::InterpreterNotFound, stdc::formatN("%1 task Engine not found", moduleSpec->id()));
        }

        const auto runtimeOptions =
            NO<TaskRuntimeOptions>::create(moduleSpec->id(), moduleSpec->className(), moduleSpec->apiLevel());

        auto taskExp = taskFactory->createTask(moduleSpec, runtimeOptions);
        if (!taskExp) {
            return Error(Error::InvalidArgument, stdc::formatN("Failed to create task: %1", taskExp.error().message()));
        }

        auto task = taskExp.take();

        const auto initArgs =
            NO<TaskInitArgs>::create(moduleSpec->id(), moduleSpec->className(), moduleSpec->apiLevel());
        if (const auto exp = task->initialize(initArgs); !exp) {
            return Error(Error::InvalidArgument, stdc::formatN("Failed to initialize task: %1", exp.error().message()));
        }

        auto &ic = *this->category(moduleSpec->category().c_str());
        ic.addObject(moduleSpec->id(), task);
        return task;
    }

    void PackageManager::registerCategoryFactory(ModuleCategory *(*fac)(PackageManager *)) {
        Impl::categoryFactories.push_back(fac);
    }

    Expected<JsonObject> readJsonFile(const std::filesystem::path &path) {
        const std::ifstream file(path);
        if (!file.is_open()) {
            return Error{
                Error::FileNotOpen,
                stdc::formatN(R"("%1": failed to open package manifest)", path),
            };
        }

        std::stringstream ss;
        ss << file.rdbuf();

        std::string error2;
        const auto root = JsonValue::fromJson(ss.str(), true, &error2);
        if (!error2.empty()) {
            return Error{
                Error::InvalidFormat,
                stdc::formatN(R"("%1": invalid package manifest format: %2)", path, error2),
            };
        }
        if (!root.isObject()) {
            return Error{
                Error::InvalidFormat,
                stdc::formatN(R"("%1": invalid package manifest format: not an object)", path),
            };
        }
        return root.toObject();
    }

    void PackageManager::collectModuleMetadata(const std::string &packageId, const std::string &packageVersion,
                                               const std::filesystem::path &packageDir, const JsonObject &modulesObj) {
        __stdc_impl_t;
        if (!fs::is_directory(packageDir)) {
            std::cerr << stdc::formatN(R"(invalid package path "%1")", packageDir).c_str() << std::endl;
        }

        for (const auto &[moduleType, moduleArray] : modulesObj) {
            const auto &modules = moduleArray.toArray();

            for (const auto &moduleEntry : modules) {
                const auto &moduleObj = moduleEntry.toObject();
                ModuleMetadata info;
                info.packageId = packageId;
                info.packagePath = packageDir;
                info.type = moduleType;
                info.level = 0;

                extractModuleMetadataFromJson(packageId, packageVersion, moduleObj, info);

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
                        catch (...) {
                        }
                    }
                }

                if (info.moduleId.empty() || info.iid.empty() || info.type.empty()) {
                    std::cerr << std::endl
                              << "Warning:" << std::endl
                              << "Module missing required fields in package " << packageId << std::endl;
                    continue;
                }

                if (auto [it, inserted] = impl.moduleInfoSet.insert(info); !inserted) {
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
                        << "  Existing location: Package=" << existing.packageId << " (v" << existing.version << ")"
                        << std::endl
                        << "  New location: Package=" << info.packageId << " (v" << info.version << ")" << std::endl;
                    std::cerr << oss.str() << std::endl;
                } else {
                    impl.moduleInfos.push_back(info);
                }
            }
        }
    }

    void PackageManager::extractModuleMetadataFromJson(const std::string &packageId, const std::string &packageVersion,
                                                       const JsonObject &moduleEntry, ModuleMetadata &info) {
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

    std::vector<ModuleMetadata> PackageManager::getModuleMetadatas() {
        __stdc_impl_t;
        std::shared_lock lock(impl.su_mtx);

        if (impl.dependencyResolutionSuccessful && !impl.moduleInfos.empty()) {
            return impl.moduleInfos;
        }

        impl.moduleInfos.clear();
        impl.moduleInfoSet.clear();
        impl.dependencyErrors.clear();
        impl.dependencyResolutionSuccessful = true;

        std::vector<std::filesystem::path> uniquePaths;
        {
            std::unordered_set<std::string> seenPaths;
            for (const auto &path : impl.packagePaths) {
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
            scanPackageDirectory(basePath);
        }

        printDiscoveryInfo(uniquePaths.size(), impl.moduleInfos.size());

        if (!impl.moduleInfos.empty()) {
            impl.resolveModuleDependencies();
        }

        std::vector<ModuleMetadata> result;
        std::copy_if(impl.moduleInfos.begin(), impl.moduleInfos.end(), std::back_inserter(result),
                     [](const ModuleMetadata &info)
                     { return info.requirements.size() == info.resolvedDependencies.size(); });
        return result;
    }

    bool PackageManager::Impl::resolveModuleDependencies() {
        DependencyResolver resolver;

        dependencyErrors.clear();
        dependencyResolutionSuccessful = true;

        std::cout << "\nStarting module dependency resolution" << std::endl;
        std::cout << "Module count: " << moduleInfos.size() << std::endl;

        if (!resolver.resolveAllDependencies(moduleInfos)) {
            dependencyResolutionSuccessful = false;
            dependencyErrors = resolver.getErrors();

            if (!dependencyErrors.empty()) {
                std::cerr << "\nDependency resolution failed" << std::endl;
                std::cerr << "Cannot proceed with module initialization." << std::endl;
                std::cerr << "========================================" << std::endl;

                for (size_t i = 0; i < dependencyErrors.size(); ++i) {
                    std::cerr << dependencyErrors[i];
                    if (i < dependencyErrors.size() - 1) {
                        std::cerr << std::endl;
                    }
                }

                std::cerr << "========================================" << std::endl;
                std::cerr << dependencyErrors.size() << " dependency errors found" << std::endl;
            }

            return false;
        }

        moduleInfos = resolver.getResolvedModules();

        std::cout << "\nDependency resolution successful" << std::endl;
        std::cout << "Successfully resolved modules: " << moduleInfos.size() << std::endl;

        moduleInfoSet.clear();
        moduleInfoSet.insert(moduleInfos.begin(), moduleInfos.end());

        return true;
    }

    void PackageManager::scanPackageDirectory(const std::filesystem::path &basePath) {
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

            processPackageJson(entry.path());
        }
    }

    void PackageManager::processPackageJson(const std::filesystem::path &packageDir) {
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

        const auto versionIt = obj.find("version");
        if (versionIt == obj.end()) {
            return;
        }
        const stdc::VersionNumber version_ = stdc::VersionNumber::fromString(versionIt->second.toString());

        if (const auto modulesIt = obj.find("modules"); modulesIt != obj.end()) {
            const auto &modulesObj = modulesIt->second.toObject();
            this->collectModuleMetadata(id_, version_.toString(), descPath.parent_path(), modulesObj);
        }
    }

    void PackageManager::printDiscoveryInfo(const size_t pathCount, const size_t moduleCount) {
        __stdc_impl_t;
        if (moduleCount == 0) {
            std::cout << "\n" << std::endl;
            std::cout << "================================================================================"
                      << std::endl;
            std::cout << "⚠️  NO MODULES FOUND" << std::endl;
            std::cout << "--------------------------------------------------------------------------------"
                      << std::endl;
            std::cout << "No modules were discovered in the package paths." << std::endl;
            std::cout << "Package paths searched:" << std::endl;
            for (const auto &path : impl.packagePaths) {
                std::cout << "  - " << path.string() << std::endl;
            }
            std::cout << "================================================================================"
                      << std::endl;
        } else {
            std::cout << "\n" << std::endl;
            std::cout << "================================================================================"
                      << std::endl;
            std::cout << "📦  MODULE DISCOVERY COMPLETE" << std::endl;
            std::cout << "--------------------------------------------------------------------------------"
                      << std::endl;
            std::cout << "Scanned " << pathCount << " package path(s)" << std::endl;
            std::cout << "Discovered " << moduleCount << " module(s)" << std::endl;
            std::cout << "================================================================================"
                      << std::endl;
        }
    }
} // namespace LangCore
