#include "Manager.h"

#include <fstream>

#include "Manager_p.h"

#include <iostream>
#include <mutex>
#include <set>

#include <stdcorelib/path.h>
#include <stdcorelib/pimpl.h>
#include <stdcorelib/stlextra/algorithms.h>

#include <LangMgr/Module/Dependency/DependencyResolver.h>
#include <LangMgr/Module/Dependency/VersionUtils.h>

#include <LangMgr/Support/Expected.h>
#include <LangMgr/Support/JSON.h>
#include <LangMgr/Task/Task.h>

#include "Module_p.h"
#include "Package_p.h"

namespace fs = std::filesystem;

namespace LangMgr
{
    llvm::SmallVector<ModuleCategory *(*)(Manager *)> Manager::Impl::categoryFactories;

    Manager::Impl::Impl(Manager *decl) : PluginFactory::Impl(decl) {
        for (const auto &factory : categoryFactories) {
            const auto category = factory(decl);
            categories[std::string(category->name())] = category;
            cateKeyMap[std::string(category->key())] = category;
        }
    }

    Manager::Impl::~Impl() {
        closeAllLoadedPackages();
        stdc::delete_all(categories);
    }

    std::vector<NO<Task>> Manager::Impl::priorityTaggers(const std::vector<std::string> &priorityTaggerIds) const {
        const std::vector<std::string> order = defaultTaggerOrder;

        std::vector<NO<Task>> result;
        for (const auto &g2pId : priorityTaggerIds) {
            const auto it = taggers.find(g2pId);
            if (it == taggers.end())
                continue;
            result.push_back(it->second);
        }

        for (const auto &id : order) {
            if (std::find(priorityTaggerIds.begin(), priorityTaggerIds.end(), id) != priorityTaggerIds.end())
                continue;

            const auto it = taggers.find(id);
            if (it == taggers.end())
                continue;
            result.push_back(it->second);
        }
        return result;
    }

    Expected<PackageData *> Manager::Impl::open(const std::filesystem::path &path, bool noLoad) {
        __stdc_decl_t;
        auto canonicalPath = stdc::path::canonical(path);
        if (canonicalPath.empty() || !fs::is_directory(canonicalPath)) {
            return Error{
                Error::FileNotOpen,
                stdc::formatN(R"(invalid package path "%1")", path),
            };
        }

        // Check package path
        if (!noLoad) {
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
        llvm::SmallVector<ModuleDefinition *> contributes;

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

        if (noLoad) {
            std::unique_lock lock(su_mtx);
            resourcePackages.insert(pd);
            return pd;
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
                if (auto exp = cc->loadDefinition(contribute, ModuleDefinition::Initialized); !exp) {
                    error1 = exp.error();
                    i--;
                    failed = true;
                    break;
                }
                contribute->_impl->state = ModuleDefinition::Initialized;
            }

            if (failed) {
                // Delete
                for (; i >= 0; --i) {
                    const auto &contribute = contributes[i];
                    const auto &cc = categories.at(contribute->_impl->category);
                    std::ignore = cc->loadDefinition(contribute, ModuleDefinition::Deleted);
                    contribute->_impl->state = ModuleDefinition::Deleted;
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
                if (auto exp = cc->loadDefinition(contribute, ModuleDefinition::Ready); !exp) {
                    error1 = exp.error();
                    i--;
                    failed = true;
                    break;
                }
                contribute->_impl->state = ModuleDefinition::Ready;
            }

            if (failed) {
                // Finish
                for (; i >= 0; --i) {
                    const auto &contribute = contributes[i];
                    const auto &cc = categories.at(contribute->_impl->category);
                    std::ignore = cc->loadDefinition(contribute, ModuleDefinition::Finished);
                    contribute->_impl->state = ModuleDefinition::Finished;
                }

                // Delete
                for (i = static_cast<int>(contributes.size()) - 1; i >= 0; i--) {
                    const auto &contribute = contributes[i];
                    const auto &cc = categories.at(contribute->_impl->category);
                    std::ignore = cc->loadDefinition(contribute, ModuleDefinition::Deleted);
                    contribute->_impl->state = ModuleDefinition::Deleted;
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

    bool Manager::Impl::close(PackageData *spec) {
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
                std::ignore = cc->loadDefinition(contribute, ModuleDefinition::Finished);
                contribute->_impl->state = ModuleDefinition::Finished;
            }

            // Delete
            for (auto it = pkgToClose.contributes.rbegin(); it != pkgToClose.contributes.rend(); ++it) {
                const auto &contribute = *it;
                const auto &cc = categories.at(contribute->_impl->category);
                std::ignore = cc->loadDefinition(contribute, ModuleDefinition::Deleted);
                contribute->_impl->state = ModuleDefinition::Deleted;
            }
        }

        // Unload dependencies
        for (auto it = pkgToClose.linked.rbegin(); it != pkgToClose.linked.rend(); ++it) {
            std::ignore = close(*it);
        }

        delete spec;
        return true;
    }

    void Manager::Impl::closeAllLoadedPackages() {
        while (!loadedPackageMap.packages.empty()) {
            const auto spec = loadedPackageMap.packages.back().spec;
            std::ignore = close(spec);
        }
    }

    void Manager::Impl::refreshPackageIndexes() {
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

    Manager::Manager() : PluginFactory(*new Impl(this)) {}

    Manager::~Manager() = default;

    ModuleCategory *Manager::category(const std::string_view &name) const {
        __stdc_impl_t;
        const auto it = impl.categories.find(name);
        if (it == impl.categories.end()) {
            return nullptr;
        }
        return it->second;
    }

    Manager *Manager::instance() {
        static Manager obj;
        return &obj;
    }

    bool Manager::initialize(std::string &errMsg) {
        __stdc_impl_t;
        impl.initialized = true;
        return true;
    }

    bool Manager::initialized() const {
        __stdc_impl_t;
        return impl.initialized;
    }

    Expected<NO<Task>> Manager::tagger(const std::string &id) const {
        __stdc_impl_t;
        const auto it = impl.taggers.find(id);
        if (it == impl.taggers.end()) {
            std::cerr << "LangMgr::Manager::tagger(): factory does not exist:" << id << std::endl;
            return Expected<NO<Task>>();
        }
        return it->second;
    }

    std::vector<NO<Task>> Manager::taggers() const {
        __stdc_impl_t;
        std::vector<NO<Task>> result;
        for (auto [id, tagger] : impl.taggers)
            result.push_back(tagger);
        return result;
    }

    std::vector<std::string> Manager::defaultOrder() const {
        __stdc_impl_t;
        return impl.defaultTaggerOrder;
    }

    void Manager::setDefaultOrder(const std::vector<std::string> &order) {
        __stdc_impl_t;
        impl.defaultTaggerOrder = order;
    }

    std::vector<TaggerRes> Manager::split(const std::string &input,
                                          const std::vector<std::string> &priorityTaggerIds) const {
        __stdc_impl_t;
        // const auto &taggersList = impl.priorityTaggers(priorityTaggerIds);
        // std::vector result = {TaggerRes(utf8strToU32str(input))};
        // for (const auto &tagger : taggersList)
        //     result = tagger->start(result);
        // return result;
        return {};
    }

    void Manager::convert(const std::vector<TaggerRes *> &input) const {
        // __stdc_impl_t;
        // std::map<std::string, std::vector<int>> indexMap;
        // std::map<std::string, std::vector<std::u32string>> lyricMap;
        //
        // for (int i = 0; i < input.size(); ++i) {
        //     const TaggerRes *note = input.at(i);
        //     indexMap[note->g2pId].push_back(i);
        //     lyricMap[note->g2pId].push_back(note->lyric);
        // }
        //
        // for (const auto &[taggerId, indices] : indexMap) {
        //     const auto &rawLyrics = lyricMap[taggerId];
        //     auto [taggerType, configId] = impl.extractConfig(taggerId);
        //
        //     auto g2pFactory = this->tagger(taggerId);
        //     if (!g2pFactory)
        //         g2pFactory = this->tagger("unknown");
        //
        //     const auto &tempRes = g2pFactory->convert(rawLyrics);
        //     for (int i = 0; i < tempRes.size(); i++) {
        //         const auto &index = indices[i];
        //         input[index]->error = tempRes[i].error;
        //         input[index]->syllable = tempRes[i].syllable;
        //         input[index]->candidates = tempRes[i].candidates;
        //     }
        // }
    }

    std::vector<std::string> Manager::tag(const std::vector<std::string> &input,
                                          const std::vector<std::string> &priorityTaggerIds,
                                          const std::vector<std::string> &reservedTokens) const {
        // __stdc_impl_t;
        // const auto &taggersList = impl.priorityTaggers(priorityTaggerIds);
        // std::vector<TaggerRes *> inputNote;
        // for (const auto &lyric : input) {
        //     inputNote.push_back(new TaggerRes(utf8strToU32str(lyric)));
        // }
        //
        // for (const auto &tagger : taggersList)
        //     tagger->correct(inputNote);
        //
        // std::vector<std::string> result;
        // for (const auto &note : inputNote)
        //     result.push_back(note->language);
        //
        // for (const auto note : inputNote) {
        //     delete note;
        // }
        //
        // return result;
        return {};
    }

    void Manager::addPackagePaths(const stdc::array_view<std::filesystem::path> paths) {
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

    void Manager::setPackagePaths(const stdc::array_view<std::filesystem::path> paths) {
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

    std::vector<std::filesystem::path> Manager::packagePaths() const {
        __stdc_impl_t;
        std::shared_lock lock(impl.su_mtx);
        return {impl.packagePaths.begin(), impl.packagePaths.end()};
    }

    Expected<Package> Manager::open(const std::filesystem::path &path, const bool noLoad) {
        __stdc_impl_t;
        auto result = impl.open(path, noLoad);
        if (!result) {
            return result.error();
        }
        return Package(result.get());
    }

    Package Manager::find(const std::string_view &id, const stdc::VersionNumber &version) const {
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

    std::vector<Package> Manager::find(const std::string_view &id) const {
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

    std::vector<Package> Manager::packages() const {
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

    void Manager::registerCategoryFactory(ModuleCategory *(*fac)(Manager *)) { Impl::categoryFactories.push_back(fac); }

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

    void Manager::collectModuleInfo(const std::string &packageId, const std::string &packageVersion,
                                    const std::filesystem::path &packageDir, const JsonObject &modulesObj) {
        __stdc_impl_t;
        if (!fs::is_directory(packageDir)) {
            std::cerr << stdc::formatN(R"(invalid package path "%1")", packageDir).c_str() << std::endl;
        }

        for (const auto &[moduleType, moduleArray] : modulesObj) {
            const auto &modules = moduleArray.toArray();

            for (const auto &moduleEntry : modules) {
                const auto &moduleObj = moduleEntry.toObject();
                ModuleInfo info;
                info.packageId = packageId;
                info.packagePath = packageDir;
                info.type = moduleType;
                info.level = 0;

                extractModuleInfoFromJson(packageId, packageVersion, moduleObj, info);

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
                    std::cerr << "Warning: Module missing required fields in package " << packageId << std::endl;
                    continue;
                }

                if (auto [it, inserted] = impl.moduleInfoSet.insert(info); !inserted) {
                    const ModuleInfo &existing = *it;
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

    void Manager::extractModuleInfoFromJson(const std::string &packageId, const std::string &packageVersion,
                                            const JsonObject &moduleEntry, ModuleInfo &info) {
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

                DependencyRaw depRaw;

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
                    info.dependenciesRaw.push_back(depRaw);
                }
            }
        }
    }

    std::vector<ModuleInfo> Manager::getModuleInfos() {
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

        return impl.moduleInfos;
    }

    bool Manager::Impl::resolveModuleDependencies() {
        DependencyResolver resolver;

        dependencyErrors.clear();
        dependencyResolutionSuccessful = true;

        if (!resolver.resolveAllDependencies(moduleInfos)) {
            dependencyResolutionSuccessful = false;
            dependencyErrors = resolver.getErrors();

            if (!dependencyErrors.empty()) {
                std::cerr << "\n⚠️  DEPENDENCY RESOLUTION FAILED" << std::endl;
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

        moduleInfoSet.clear();
        moduleInfoSet.insert(moduleInfos.begin(), moduleInfos.end());

        return true;
    }

    void Manager::scanPackageDirectory(const std::filesystem::path &basePath) {
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

    void Manager::processPackageJson(const std::filesystem::path &packageDir) {
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
            this->collectModuleInfo(id_, version_.toString(), descPath.parent_path(), modulesObj);
        }
    }

    void Manager::printDiscoveryInfo(const size_t pathCount, const size_t moduleCount) {
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
} // namespace LangMgr
