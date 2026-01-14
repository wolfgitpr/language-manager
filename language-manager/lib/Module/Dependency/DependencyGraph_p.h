#ifndef LANGMGR_DEPENDENCY_P_H
#define LANGMGR_DEPENDENCY_P_H

#include <memory>
#include <stack>
#include <unordered_map>
#include <vector>

#include <LangMgr/Module/Dependency/DependencyGraph.h>

namespace LangMgr
{
    class DependencyGraph::Impl {
    public:
        Impl();
        ~Impl();

        bool addModule(const ModuleMetadata &module);
        bool buildGraph();
        void clear();

        std::vector<std::vector<ModuleMetadata>> getCycles() const;
        std::vector<ModuleMetadata> getAllModules() const;
        std::vector<PackageInitializationPlan> getPackageInitializationOrder() const;

    private:
        struct GraphNode;

        struct TarjanState {
            int index = -1;
            int lowlink = -1;
            bool onStack = false;
        };

        using NodeMap = std::unordered_map<std::string, std::shared_ptr<GraphNode>>;
        using MainModuleMap = std::unordered_map<ModuleMetadata, std::vector<std::shared_ptr<GraphNode>>,
                                                 ModuleMetadata::MainModuleHash, ModuleMetadata::MainModuleEqual>;

        struct GraphNode {
            ModuleMetadata module;
            std::vector<std::shared_ptr<GraphNode>> neighbors;
            explicit GraphNode(ModuleMetadata mod) : module(std::move(mod)) {}
        };

        std::shared_ptr<GraphNode> getOrCreateNode(const ModuleMetadata &module);
        std::vector<std::shared_ptr<GraphNode>> findNodesByDependency(const ResolvedDependency &dep);

        static void strongConnect(const std::shared_ptr<GraphNode> &node,
                                  std::unordered_map<std::string, TarjanState> &state,
                                  std::stack<std::shared_ptr<GraphNode>> &stack, int &index,
                                  std::vector<std::vector<std::shared_ptr<GraphNode>>> &sccs);

        std::vector<std::string> getPackageTopologicalOrder() const;
        std::vector<ModuleMetadata> getGlobalModuleInitializationOrder() const;

        NodeMap nodeMap;
        MainModuleMap mainModuleMap;
        bool graphBuilt = false;
    };
} // namespace LangMgr

#endif
