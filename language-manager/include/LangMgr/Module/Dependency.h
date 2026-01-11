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

    struct DependencyInfo {
        std::string packageId;
        std::string moduleId;
        int level = -1;

        DependencyInfo() = default;
        DependencyInfo(std::string packageId, std::string moduleId, const int level = -1) :
            packageId(std::move(packageId)), moduleId(std::move(moduleId)), level(level) {}

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
        std::vector<DependencyInfo> dependencies;

        bool isSameMainModule(const ModuleInfo &other) const {
            return moduleId == other.moduleId && iid == other.iid && type == other.type &&
                configuration == other.configuration;
        }

        bool operator==(const ModuleInfo &other) const {
            return packageId == other.packageId && moduleId == other.moduleId && iid == other.iid &&
                type == other.type && configuration == other.configuration;
        }

        std::string key() const { return packageId + ":" + moduleId + ":" + iid + ":" + type + ":" + configuration; }

        struct MainModuleHash {
            size_t operator()(const ModuleInfo &info) const {
                const size_t h1 = std::hash<std::string>()(info.moduleId);
                const size_t h2 = std::hash<std::string>()(info.iid);
                const size_t h3 = std::hash<std::string>()(info.type);
                const size_t h4 = std::hash<std::string>()(info.configuration);
                return h1 ^ h2 << 1 ^ h3 << 2 ^ h4 << 3;
            }
        };

        struct MainModuleEqual {
            bool operator()(const ModuleInfo &a, const ModuleInfo &b) const { return a.isSameMainModule(b); }
        };
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
        Dependency(Dependency &&) = default;
        Dependency &operator=(Dependency &&) = default;

        bool addModule(const ModuleInfo &module);
        bool validate();
        std::vector<std::vector<ModuleInfo>> getCycles();

        std::vector<std::vector<ModuleInfo>> getMainModules() const;
        std::vector<ModuleInfo> findModulesByMainModule(const ModuleInfo &mainModule) const;

        size_t getModuleCount() const;
        size_t getMainModuleCount() const;
        size_t getDependencyCount() const;
        std::vector<ModuleInfo> getAllModules() const;

        void clear();

        std::vector<PackageInitInfo> getPackageInitializationOrder() const;
        std::vector<ModuleInfo> getGlobalModuleInitializationOrder() const;
        std::vector<std::pair<std::string, std::filesystem::path>> getPackageDependencyOrder() const;

    private:
        struct GraphNode;

        enum class VisitState { UNVISITED, VISITING, VISITED };

        static constexpr int MAX_DEPTH = 10;
        static constexpr int MAX_CYCLE_COUNT = 20;
        static constexpr int MAX_TOTAL_CYCLES = 50;

        std::shared_ptr<GraphNode> getOrCreateNode(const ModuleInfo &module);
        std::vector<std::shared_ptr<GraphNode>> findNodesByDependency(const DependencyInfo &dep);

        static bool hasCycleDFS(const std::shared_ptr<GraphNode> &node,
                                std::unordered_map<std::string, VisitState> &visited, int depth);

        static void findCyclesLimited(const std::shared_ptr<GraphNode> &startNode,
                                      std::unordered_map<std::string, VisitState> &visited,
                                      std::vector<std::shared_ptr<GraphNode>> &path,
                                      std::vector<std::vector<ModuleInfo>> &allCycles, int depth, int &cyclesFound);

        std::vector<ModuleInfo> getReverseTopologicalOrder() const;
        std::vector<ModuleInfo> getInitializationOrderInternal() const;

        std::vector<ModuleInfo> getPackageModuleOrder(const std::string &packageId) const;
        std::vector<std::string> getPackageTopologicalOrder() const;

        std::unordered_map<std::string, std::shared_ptr<GraphNode>> nodeMap;
        std::unordered_map<ModuleInfo, std::vector<std::shared_ptr<GraphNode>>, ModuleInfo::MainModuleHash,
                           ModuleInfo::MainModuleEqual>
            mainModuleMap;
    };

    void printModule(const ModuleInfo &module);
    void printDependency(const DependencyInfo &dep);
    void printModuleList(const std::vector<ModuleInfo> &modules, const std::string &title = "");
    void printPackageInitOrder(const std::vector<PackageInitInfo> &packageOrder, const std::string &title = "");
} // namespace LangMgr
#endif // LANGMGR_DEPENDENCY_H
