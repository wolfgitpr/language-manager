#ifndef LANGMGR_DEPENDENCY_P_H
#define LANGMGR_DEPENDENCY_P_H

#include <memory>
#include <unordered_map>
#include <vector>

#include <LangMgr/Module/Dependency/Dependency.h>

namespace LangMgr
{
    namespace DependencyConstants
    {
        constexpr int MAX_DEPTH = 10;
        constexpr int MAX_CYCLE_COUNT = 20;
        constexpr int MAX_TOTAL_CYCLES = 50;
    } // namespace DependencyConstants

    class Dependency::Impl {
    public:
        Impl();
        ~Impl();

        bool addModule(const ModuleInfo &module);
        bool validate() const;
        void clear();

        std::vector<std::vector<ModuleInfo>> getCycles() const;
        std::vector<ModuleInfo> getAllModules() const;
        std::vector<PackageInitInfo> getPackageInitializationOrder() const;

    private:
        struct GraphNode;
        enum class VisitState { UNVISITED, VISITING, VISITED };

        using NodeMap = std::unordered_map<std::string, std::shared_ptr<GraphNode>>;
        using MainModuleMap = std::unordered_map<ModuleInfo, std::vector<std::shared_ptr<GraphNode>>,
                                                 ModuleInfo::MainModuleHash, ModuleInfo::MainModuleEqual>;

        struct GraphNode {
            ModuleInfo module;
            std::vector<std::shared_ptr<GraphNode>> neighbors;
            explicit GraphNode(ModuleInfo mod) : module(std::move(mod)) {}
        };

        std::shared_ptr<GraphNode> getOrCreateNode(const ModuleInfo &module);
        std::vector<std::shared_ptr<GraphNode>> findNodesByDependency(const DependencyInfo &dep);

        static bool hasCycleDFS(const std::shared_ptr<GraphNode> &node,
                                std::unordered_map<std::string, VisitState> &visited, int depth);

        static void findCyclesLimited(const std::shared_ptr<GraphNode> &startNode,
                                      std::unordered_map<std::string, VisitState> &visited,
                                      std::vector<std::shared_ptr<GraphNode>> &path,
                                      std::vector<std::vector<ModuleInfo>> &allCycles, int depth, int &cyclesFound);

        std::vector<std::string> getPackageTopologicalOrder() const;
        std::vector<ModuleInfo> getGlobalModuleInitializationOrder() const;

        NodeMap nodeMap;
        MainModuleMap mainModuleMap;
    };
} // namespace LangMgr
#endif
