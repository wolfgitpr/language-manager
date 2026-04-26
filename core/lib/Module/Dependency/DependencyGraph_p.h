#ifndef LANGCORE_DEPENDENCY_P_H
#define LANGCORE_DEPENDENCY_P_H

#include <memory>
#include <unordered_map>
#include <vector>

#include <LangCore/Module/Dependency/DependencyGraph.h>

namespace LangCore
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
        struct GraphNode {
            ModuleMetadata module;
            std::vector<std::shared_ptr<GraphNode>> neighbors;
            explicit GraphNode(ModuleMetadata mod) : module(std::move(mod)) {}
        };

        using NodeMap = std::unordered_map<std::string, std::shared_ptr<GraphNode>>;
        using MainModuleMap = std::unordered_map<ModuleMetadata, std::vector<std::shared_ptr<GraphNode>>,
                                                 ModuleMetadata::MainModuleHash, ModuleMetadata::MainModuleEqual>;

        std::shared_ptr<GraphNode> getOrCreateNode(const ModuleMetadata &module);
        std::vector<std::shared_ptr<GraphNode>> findNodesByDependency(const ResolvedDependency &dep);

        std::vector<std::string> getPackageTopologicalOrder() const;

        /// Kahn's topological sort. Returns sorted modules, or empty if cycle detected.
        /// If a cycle is detected and \p cycleMembers is non-null, the modules in the
        /// cycle are written there.
        std::vector<ModuleMetadata> getGlobalModuleInitializationOrder(
            std::vector<ModuleMetadata> *cycleMembers = nullptr) const;

        NodeMap nodeMap;
        MainModuleMap mainModuleMap;
        bool graphBuilt = false;
    };
} // namespace LangCore

#endif
