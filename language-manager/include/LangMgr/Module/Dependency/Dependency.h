#ifndef LANGMGR_DEPENDENCY_H
#define LANGMGR_DEPENDENCY_H

#include <LangMgr/LangMgrGlobal.h>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace LangMgr
{
    struct DependencyRaw {
        std::string packageId;
        std::string moduleId;
        int level = -1;
        std::string versionRange = "999.999.999";

        DependencyRaw() = default;
        DependencyRaw(std::string packageId, std::string moduleId, int level = -1, std::string versionRange = "") :
            packageId(std::move(packageId)), moduleId(std::move(moduleId)), level(std::move(level)),
            versionRange(std::move(versionRange)) {}

        bool operator==(const DependencyRaw &other) const;
        std::string key() const;
    };

    struct DependencyInfo {
        std::string packageId;
        std::string moduleId;
        int level = -1;
        std::string version;

        DependencyInfo() = default;
        DependencyInfo(std::string packageId, std::string moduleId, int level = -1, std::string version = "") :
            packageId(std::move(packageId)), moduleId(std::move(moduleId)), level(level), version(std::move(version)) {}

        bool operator==(const DependencyInfo &other) const;
        std::string key() const;
    };

    struct ModuleInfo {
        std::string packageId;
        std::string moduleId;
        std::string type;
        std::string iid;
        std::string configuration;
        std::filesystem::path packagePath;
        std::string version;
        int level = 0;
        std::vector<DependencyRaw> dependenciesRaw;
        std::vector<DependencyInfo> dependencies;

        bool isSameMainModule(const ModuleInfo &other) const {
            return moduleId == other.moduleId && iid == other.iid && type == other.type &&
                configuration == other.configuration && level == other.level;
        }

        bool operator==(const ModuleInfo &other) const {
            return packageId == other.packageId && moduleId == other.moduleId && iid == other.iid &&
                type == other.type && configuration == other.configuration && version == other.version &&
                level == other.level;
        }

        std::string key() const {
            return packageId + ":" + moduleId + ":" + version + ":" + iid + ":" + type + ":" + configuration + ":" +
                std::to_string(level) + ":" + std::to_string(level);
        }

        std::string uniqueKey() const {
            return packageId + ":" + moduleId + ":" + iid + ":" + type + ":" + std::to_string(level);
        }

        struct MainModuleHash {
            size_t operator()(const ModuleInfo &info) const {
                const size_t h1 = std::hash<std::string>()(info.moduleId);
                const size_t h2 = std::hash<std::string>()(info.iid);
                const size_t h3 = std::hash<std::string>()(info.type);
                const size_t h4 = std::hash<std::string>()(info.configuration);
                const size_t h5 = std::hash<int>()(info.level);
                const size_t h6 = std::hash<int>()(info.level);
                return h1 ^ h2 << 1 ^ h3 << 2 ^ h4 << 3 ^ h5 << 4 ^ h6 << 5;
            }
        };

        struct MainModuleEqual {
            bool operator()(const ModuleInfo &a, const ModuleInfo &b) const { return a.isSameMainModule(b); }
        };
    };

    struct VersionSelectionResult {
        bool success = false;
        std::string selectedVersion;
        std::string error;
        std::vector<std::string> candidates;
        std::vector<std::string> filteredCandidates;
    };

    struct PackageInitInfo {
        std::string packageId;
        std::filesystem::path packagePath;
        std::vector<ModuleInfo> modules;
        std::vector<ModuleInfo> initializationOrder;
    };

    class LANGMGR_EXPORT Dependency {
    public:
        Dependency();
        ~Dependency();

        Dependency(const Dependency &) = delete;
        Dependency &operator=(const Dependency &) = delete;
        Dependency(Dependency &&) noexcept;
        Dependency &operator=(Dependency &&) noexcept;

        bool addModule(const ModuleInfo &module) const;
        bool validate() const;
        void clear() const;

        std::vector<std::vector<ModuleInfo>> getCycles() const;
        std::vector<ModuleInfo> getAllModules() const;
        std::vector<ModuleInfo> getModulesByUniqueKey(const std::string &uniqueKey) const;

        static VersionSelectionResult
        selectBestVersion(const std::vector<std::string> &allVersions, const std::string &versionRange,
                          int targetLevel = -1, const std::unordered_map<std::string, int> &versionToLevel = {});

        std::vector<PackageInitInfo> getPackageInitializationOrder() const;

    private:
        class Impl;
        std::unique_ptr<Impl> _impl;
    };

    void LANGMGR_EXPORT printModule(const ModuleInfo &module);
    void LANGMGR_EXPORT printDependency(const DependencyInfo &dep);
    void LANGMGR_EXPORT printModuleList(const std::vector<ModuleInfo> &modules, const std::string &title = "");
    void LANGMGR_EXPORT printPackageInitOrder(const std::vector<PackageInitInfo> &packageOrder,
                                              const std::string &title = "");
} // namespace LangMgr
#endif
