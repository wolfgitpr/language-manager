#include "LanguageEngine_p.h"
#include "LanguageManager.h"

#include <iostream>
#include <mutex>

#include <stdcorelib/path.h>
#include <stdcorelib/pimpl.h>
#include <stdcorelib/stlextra/algorithms.h>

#include "JSON.h"
#include "U32Str.h"

#include "Contribute_p.h"
#include "PackageRef_p.h"

namespace fs = std::filesystem;

namespace LangMgr
{
    llvm::SmallVector<ContribCategory *(*)(LanguageManager *)> LanguageManager::Impl::categoryFactories;

    LanguageManager::Impl::Impl(LanguageManager *decl) : PluginFactory::Impl(decl) {
        for (const auto &factory : categoryFactories) {
            const auto category = factory(decl);
            categories[std::string(category->name())] = category;
            cateKeyMap[std::string(category->key())] = category;
        }
    }

    LanguageManager::Impl::~Impl() {
        closeAllLoadedPackages();
        stdc::delete_all(categories);

        for (const auto &[id, factory] : g2ps) {
            if (factory) {
                delete factory;
            }
        }
        g2ps.clear();
    }

    std::pair<std::string, std::string> LanguageManager::Impl::extractConfig(const std::string &g2pId) {
        const auto firstColonIndex = g2pId.find(':');

        if (firstColonIndex == std::string::npos) {
            return {g2pId, "0"};
        }

        std::string beforeColon = g2pId.substr(0, firstColonIndex);
        std::string afterColon = g2pId.substr(firstColonIndex + 1);

        try {
            if (const int value = std::stoi(afterColon); value < 0) {
                return {beforeColon, "0"};
            }
            return {beforeColon, afterColon};
        }
        catch (const std::exception &) {
            return {beforeColon, "0"};
        }
    }

    std::vector<IG2pFactory *>
    LanguageManager::Impl::priorityG2ps(const std::vector<std::string> &priorityG2pIds) const {
        std::vector<std::string> order = defaultG2pOrder;

        std::vector<IG2pFactory *> result;
        for (const auto &g2pId : priorityG2pIds) {
            const auto it = g2ps.find(g2pId);
            if (it == g2ps.end())
                continue;
            result.push_back(it->second);
        }

        for (const auto &id : order) {
            const auto it = g2ps.find(id);
            if (it == g2ps.end())
                continue;

            bool add = true;
            for (const auto &g2p : result) {
                if (const auto [g2pType, configId] = extractConfig(g2p->id()); g2pType == it->second->id()) {
                    add = false;
                    break;
                }
            }
            if (add)
                result.push_back(it->second);
        }
        return result;
    }

    Expected<PackageData *> LanguageManager::Impl::open(const std::filesystem::path &path, bool noLoad) {
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
        llvm::SmallVector<ContribSpec *> contributes;

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
            pd->contributes[contribute->_impl->category][contribute->_impl->id] = contribute;
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
        auto searchDependencies = [this](const std::string &id,
                                         const stdc::VersionNumber &version) -> llvm::SmallVector<fs::path>
        {
            llvm::SmallVector<fs::path> res;
            const auto it = cachedPackageIndexesMap.find(id);
            if (it == cachedPackageIndexesMap.end()) {
                return {};
            }

            // Search precise version
            const auto &versionMap = it->second;
            {
                if (const auto it2 = versionMap.find(version); it2 != versionMap.end()) {
                    res.emplace_back(it2->second.path);
                }
            }

            // Test from high version to low version
            for (auto it2 = versionMap.rbegin(); it2 != versionMap.rend(); ++it2) {
                if (it2->first < version) {
                    break;
                }
                if (const auto &[path, compatVersion] = it2->second; compatVersion <= version) {
                    res.emplace_back(it2->second.path);
                }
            }
            return res;
        };
        do {
            Error error1;
            for (const auto &dep : std::as_const(pd->dependencies)) {

                // Try to load all matched packages
                fs::path foundPath;
                auto depPaths = searchDependencies(dep.id, dep.version);
                for (auto it = depPaths.rbegin(); it != depPaths.rend(); ++it) {
                    const auto &depPath = *it;

                    // Test
                    auto depPkg = open(depPath, true);
                    if (!depPkg) {
                        continue; // ignore
                    }
                    std::ignore = close(depPkg.get());
                    foundPath = depPath;
                    break;
                }

                if (foundPath.empty()) {
                    if (!dep.required) {
                        continue; // ignore
                    }

                    // Not found
                    error1 = {
                        Error::FileNotFound,
                        stdc::formatN(R"(required package "%1[%2]" not found)", dep.id, dep.version.toString()),
                    };
                    goto out_deps;
                }

                {
                    // Load
                    auto depPkg = open(foundPath, false);
                    if (!depPkg) {
                        error1 = {
                            Error::FileNotOpen,
                            stdc::formatN(R"(required package "%1[%2]" not valid: %3)", dep.id, dep.version.toString(),
                                          depPkg.error().message()),
                        };
                        goto out_deps;
                    }

                    auto depSpec = depPkg.get();
                    if (!depSpec->loaded) {
                        error1 = {
                            Error::FileNotOpen,
                            stdc::formatN(R"(required package "%1[%2]" not loaded: %3)", dep.id, dep.version.toString(),
                                          depSpec->err.message()),
                        };
                        std::ignore = close(depSpec);
                        goto out_deps;
                    }
                    dependencies.push_back(depSpec);
                }
            }
            break;

        out_deps:
            closeDependencies();
            pd->err = error1;

            std::unique_lock lock(su_mtx);
            removePending();
            resourcePackages.insert(pd);
            return pd;
        }
        while (false);

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
                if (auto exp = cc->loadSpec(contribute, ContribSpec::Initialized); !exp) {
                    error1 = exp.error();
                    i--;
                    failed = true;
                    break;
                }
                contribute->_impl->state = ContribSpec::Initialized;
            }

            if (failed) {
                // Delete
                for (; i >= 0; --i) {
                    const auto &contribute = contributes[i];
                    const auto &cc = categories.at(contribute->_impl->category);
                    std::ignore = cc->loadSpec(contribute, ContribSpec::Deleted);
                    contribute->_impl->state = ContribSpec::Deleted;
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
                if (auto exp = cc->loadSpec(contribute, ContribSpec::Ready); !exp) {
                    error1 = exp.error();
                    i--;
                    failed = true;
                    break;
                }
                contribute->_impl->state = ContribSpec::Ready;
            }

            if (failed) {
                // Finish
                for (; i >= 0; --i) {
                    const auto &contribute = contributes[i];
                    const auto &cc = categories.at(contribute->_impl->category);
                    std::ignore = cc->loadSpec(contribute, ContribSpec::Finished);
                    contribute->_impl->state = ContribSpec::Finished;
                }

                // Delete
                for (i = static_cast<int>(contributes.size()) - 1; i >= 0; i--) {
                    const auto &contribute = contributes[i];
                    const auto &cc = categories.at(contribute->_impl->category);
                    std::ignore = cc->loadSpec(contribute, ContribSpec::Deleted);
                    contribute->_impl->state = ContribSpec::Deleted;
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

    bool LanguageManager::Impl::close(PackageData *spec) {
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
                std::ignore = cc->loadSpec(contribute, ContribSpec::Finished);
                contribute->_impl->state = ContribSpec::Finished;
            }

            // Delete
            for (auto it = pkgToClose.contributes.rbegin(); it != pkgToClose.contributes.rend(); ++it) {
                const auto &contribute = *it;
                const auto &cc = categories.at(contribute->_impl->category);
                std::ignore = cc->loadSpec(contribute, ContribSpec::Deleted);
                contribute->_impl->state = ContribSpec::Deleted;
            }
        }

        // Unload dependencies
        for (auto it = pkgToClose.linked.rbegin(); it != pkgToClose.linked.rend(); ++it) {
            std::ignore = close(*it);
        }

        delete spec;
        return true;
    }

    void LanguageManager::Impl::closeAllLoadedPackages() {
        while (!loadedPackageMap.packages.empty()) {
            const auto spec = loadedPackageMap.packages.back().spec;
            std::ignore = close(spec);
        }
    }

    void LanguageManager::Impl::refreshPackageIndexes() {
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
                    if (!ContribLocator::isValidLocator(id_)) {
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

    LanguageManager::LanguageManager() : PluginFactory(*new Impl(this)) {}

    LanguageManager::~LanguageManager() = default;

    ContribCategory *LanguageManager::category(const std::string_view &name) const {
        __stdc_impl_t;
        const auto it = impl.categories.find(name);
        if (it == impl.categories.end()) {
            return nullptr;
        }
        return it->second;
    }

    LanguageManager *LanguageManager::instance() {
        static LanguageManager obj;
        return &obj;
    }

    bool LanguageManager::initialize(std::string &errMsg) {
        __stdc_impl_t;
        impl.initialized = true;
        return true;
    }

    bool LanguageManager::initialized() const {
        __stdc_impl_t;
        return impl.initialized;
    }

    IG2pFactory *LanguageManager::g2p(const std::string &id) const {
        __stdc_impl_t;
        const auto it = impl.g2ps.find(id);
        if (it == impl.g2ps.end()) {
            std::cerr << "LangMgr::LanguageManager::g2p(): factory does not exist:" << id << std::endl;
            return nullptr;
        }
        return it->second;
    }

    std::vector<IG2pFactory *> LanguageManager::g2ps() const {
        __stdc_impl_t;
        std::vector<IG2pFactory *> result;
        for (const auto &[id, factory] : impl.g2ps) {
            result.push_back(factory);
        }
        return result;
    }

    bool LanguageManager::addG2p(IG2pFactory *factory) {
        __stdc_impl_t;
        if (!factory) {
            std::cerr << "LangMgr::LanguageManager::addG2p(): trying to add null factory" << std::endl;
            return false;
        }
        if (impl.g2ps.find(factory->id()) != impl.g2ps.end()) {
            std::cerr << "LangMgr::LanguageManager::addG2p(): trying to add duplicated factory:" << factory->id()
                      << std::endl;
            return false;
        }
        impl.g2ps[factory->id()] = factory;
        return true;
    }

    bool LanguageManager::removeG2p(const IG2pFactory *factory) {
        if (factory == nullptr) {
            std::cerr << "LangMgr::LanguageManager::removeG2p(): trying to remove null factory" << std::endl;
            return false;
        }
        return removeG2p(factory->id());
    }

    bool LanguageManager::removeG2p(const std::string &id) {
        __stdc_impl_t;
        const auto it = impl.g2ps.find(id);
        if (it == impl.g2ps.end()) {
            std::cerr << "LangMgr::LanguageManager::removeG2p(): factory does not exist:" << id << std::endl;
            return false;
        }
        impl.g2ps.erase(it);
        return true;
    }

    void LanguageManager::clearG2ps() {
        __stdc_impl_t;
        impl.g2ps.clear();
    }

    std::vector<std::string> LanguageManager::defaultOrder() const {
        __stdc_impl_t;
        return impl.defaultG2pOrder;
    }

    void LanguageManager::setDefaultOrder(const std::vector<std::string> &order) {
        __stdc_impl_t;
        impl.defaultG2pOrder = order;
    }

    std::vector<LangNote> LanguageManager::split(const std::string &input,
                                                const std::vector<std::string> &priorityG2pIds) const {
        __stdc_impl_t;
        const auto &g2psList = impl.priorityG2ps(priorityG2pIds);
        std::vector result = {LangNote(utf8strToU32str(input))};
        for (const auto &g2p : g2psList)
            result = g2p->split(result);
        return result;
    }

    void LanguageManager::correct(const std::vector<LangNote *> &input, const std::vector<std::string> &priorityG2pIds,
                                 const std::vector<std::string> &reservedTokens) const {
        __stdc_impl_t;
        const auto &g2psList = impl.priorityG2ps(priorityG2pIds);
        for (const auto &g2p : g2psList)
            g2p->correct(input);
    }

    void LanguageManager::convert(const std::vector<LangNote *> &input) const {
        __stdc_impl_t;
        std::map<std::string, std::vector<int>> indexMap;
        std::map<std::string, std::vector<std::u32string>> lyricMap;

        for (int i = 0; i < input.size(); ++i) {
            const LangNote *note = input.at(i);
            indexMap[note->g2pId].push_back(i);
            lyricMap[note->g2pId].push_back(note->lyric);
        }

        for (const auto &[g2pId, indices] : indexMap) {
            const auto &rawLyrics = lyricMap[g2pId];
            auto [g2pType, configId] = impl.extractConfig(g2pId);

            auto g2pFactory = this->g2p(g2pId);
            if (g2pFactory == nullptr) {
                g2pFactory = this->g2p("unknown");
            }

            const auto &tempRes = g2pFactory->convert(rawLyrics);
            for (int i = 0; i < tempRes.size(); i++) {
                const auto &index = indices[i];
                input[index]->error = tempRes[i].error;
                input[index]->syllable = tempRes[i].syllable;
                input[index]->candidates = tempRes[i].candidates;
            }
        }
    }

    std::string LanguageManager::analysis(const std::string &input, const std::vector<std::string> &priorityG2pIds,
                                         const std::vector<std::string> &reservedTokens) const {
        __stdc_impl_t;
        static std::vector<std::string> keywords = {"AP", "SP"};
        if (std::find(keywords.begin(), keywords.end(), input) != keywords.end() ||
            std::find(reservedTokens.begin(), reservedTokens.end(), input) != reservedTokens.end())
            return {"reserved-token"};

        const auto &g2psList = impl.priorityG2ps(priorityG2pIds);

        for (const auto &g2p : g2psList) {
            if (const auto language = g2p->analysis(utf8strToU32str(input)); language != "unknown")
                return language;
        }
        return "unknown";
    }

    std::vector<std::string> LanguageManager::analysis(const std::vector<std::string> &input,
                                                      const std::vector<std::string> &priorityG2pIds,
                                                      const std::vector<std::string> &reservedTokens) const {
        __stdc_impl_t;
        const auto &g2psList = impl.priorityG2ps(priorityG2pIds);
        std::vector<LangNote *> inputNote;
        for (const auto &lyric : input) {
            inputNote.push_back(new LangNote(utf8strToU32str(lyric)));
        }

        for (const auto &g2p : g2psList)
            g2p->correct(inputNote);

        std::vector<std::string> result;
        for (const auto &note : inputNote)
            result.push_back(note->language);

        for (const auto note : inputNote) {
            delete note;
        }

        return result;
    }

    void LanguageManager::addPackagePaths(const stdc::array_view<std::filesystem::path> paths) {
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

    void LanguageManager::setPackagePaths(const stdc::array_view<std::filesystem::path> paths) {
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

    std::vector<std::filesystem::path> LanguageManager::packagePaths() const {
        __stdc_impl_t;
        std::shared_lock lock(impl.su_mtx);
        return {impl.packagePaths.begin(), impl.packagePaths.end()};
    }

    Expected<PackageRef> LanguageManager::open(const std::filesystem::path &path, const bool noLoad) {
        __stdc_impl_t;
        auto result = impl.open(path, noLoad);
        if (!result) {
            return result.error();
        }
        return PackageRef(result.get());
    }

    PackageRef LanguageManager::find(const std::string_view &id, const stdc::VersionNumber &version) const {
        __stdc_impl_t;
        std::shared_lock lock(impl.su_mtx);
        auto &pkgMap = impl.loadedPackageMap;
        const auto it = pkgMap.idIndexes.find(id);
        if (it == pkgMap.idIndexes.end()) {
            return PackageRef();
        }

        auto &versionMap = it->second;
        const auto it2 = versionMap.find(version);
        if (it2 == versionMap.end()) {
            return PackageRef();
        }
        return PackageRef(it2->second->spec);
    }

    std::vector<PackageRef> LanguageManager::find(const std::string_view &id) const {
        __stdc_impl_t;
        std::shared_lock lock(impl.su_mtx);
        auto &pkgMap = impl.loadedPackageMap;
        const auto it = pkgMap.idIndexes.find(id);
        if (it == pkgMap.idIndexes.end()) {
            return {};
        }

        auto &versionMap = it->second;
        std::vector<PackageRef> res;
        res.reserve(versionMap.size());
        for (const auto &[fst, snd] : versionMap) {
            res.push_back(PackageRef(snd->spec));
        }
        return res;
    }

    std::vector<PackageRef> LanguageManager::packages() const {
        __stdc_impl_t;
        std::shared_lock lock(impl.su_mtx);
        auto &list = impl.loadedPackageMap.packages;

        std::vector<PackageRef> res;
        res.reserve(list.size());
        for (const auto &item : list) {
            res.push_back(PackageRef(item.spec));
        }
        return res;
    }

    void LanguageManager::registerCategoryFactory(ContribCategory *(*fac)(LanguageManager *)) {
        Impl::categoryFactories.push_back(fac);
    }

} // namespace LangMgr
