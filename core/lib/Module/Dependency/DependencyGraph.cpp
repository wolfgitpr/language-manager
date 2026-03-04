#include <LangCore/Module/Dependency/DependencyGraph.h>
#include <LangCore/Module/Dependency/VersionUtils.h>
#include "DependencyGraph_p.h"

#include <queue>
#include <set>
#include <unordered_set>
#include <utility>

#include <LangCore/Core/ManagerLogger.h>

namespace LangCore
{
    bool DependencyRequirement::operator==(const DependencyRequirement &other) const {
        return packageId == other.packageId && moduleId == other.moduleId && level == other.level &&
            versionRange == other.versionRange;
    }

    std::string DependencyRequirement::key() const { return packageId + ":" + moduleId + ":" + std::to_string(level); }

    bool ResolvedDependency::operator==(const ResolvedDependency &other) const {
        return packageId == other.packageId && moduleId == other.moduleId && level == other.level &&
            version == other.version;
    }

    std::string ResolvedDependency::key() const {
        return packageId + ":" + moduleId + ":" + version + ":" + std::to_string(level);
    }

    bool ModuleMetadata::isSameMainModule(const ModuleMetadata &other) const {
        return moduleId == other.moduleId && iid == other.iid && type == other.type &&
            configuration == other.configuration && level == other.level;
    }

    bool ModuleMetadata::operator==(const ModuleMetadata &other) const {
        return packageId == other.packageId && moduleId == other.moduleId && iid == other.iid && type == other.type &&
            configuration == other.configuration && version == other.version && level == other.level;
    }

    std::string ModuleMetadata::key() const {
        return packageId + ":" + moduleId + ":" + version + ":" + iid + ":" + type + ":" + configuration + ":" +
            std::to_string(level);
    }

    std::string ModuleMetadata::uniqueKey() const {
        return packageId + ":" + moduleId + ":" + iid + ":" + type + ":" + std::to_string(level);
    }

    size_t ModuleMetadata::MainModuleHash::operator()(const ModuleMetadata &info) const {
        const size_t h1 = std::hash<std::string>()(info.moduleId);
        const size_t h2 = std::hash<std::string>()(info.iid);
        const size_t h3 = std::hash<std::string>()(info.type);
        const size_t h4 = std::hash<std::string>()(info.configuration);
        const size_t h5 = std::hash<int>()(info.level);
        const size_t h6 = std::hash<int>()(info.level);
        return h1 ^ h2 << 1 ^ h3 << 2 ^ h4 << 3 ^ h5 << 4 ^ h6 << 5;
    }

    bool ModuleMetadata::MainModuleEqual::operator()(const ModuleMetadata &a, const ModuleMetadata &b) const {
        return a.isSameMainModule(b);
    }

    DependencyGraph::Impl::Impl() = default;
    DependencyGraph::Impl::~Impl() = default;

    bool DependencyGraph::Impl::addModule(const ModuleMetadata &module) {
        const auto node = getOrCreateNode(module);
        return true;
    }

    bool DependencyGraph::Impl::buildGraph() {
        if (graphBuilt) {
            MgrLog.langCoreCritical("Warning: Graph already built");
            return true;
        }

        bool success = true;

        for (const auto &[key, node] : nodeMap) {
            for (const auto &dep : node->module.resolvedDependencies) {
                auto depNodes = findNodesByDependency(dep);

                if (depNodes.empty()) {
                    MgrLog.langCoreCritical("Error: Cannot add module %1:%2 (v%3, level %4)", node->module.packageId,
                                            node->module.moduleId, node->module.version, node->module.level);
                    MgrLog.langCoreCritical("  Missing dependency: %1:%2 (v%3, level %4)", dep.packageId, dep.moduleId,
                                            dep.version, dep.level);

                    MgrLog.langCoreCritical("  Available modules in graph:");
                    for (const auto &[existingKey, existingNode] : nodeMap) {
                        MgrLog.langCoreCritical("    - %1:%2 (v%3, level %4", existingNode->module.packageId,
                                                existingNode->module.moduleId, existingNode->module.version,
                                                existingNode->module.level);
                    }

                    success = false;
                    continue;
                }

                for (const auto &depNode : depNodes) {
                    if (depNode != node) {
                        bool alreadyExists = false;
                        for (const auto &neighbor : node->neighbors) {
                            if (neighbor == depNode) {
                                alreadyExists = true;
                                break;
                            }
                        }

                        if (!alreadyExists) {
                            node->neighbors.push_back(depNode);
                        }
                    }
                }
            }
        }

        graphBuilt = success;
        return success;
    }

    std::shared_ptr<DependencyGraph::Impl::GraphNode>
    DependencyGraph::Impl::getOrCreateNode(const ModuleMetadata &module) {
        const auto key = module.key();
        if (const auto it = nodeMap.find(key); it != nodeMap.end())
            return it->second;

        auto node = std::make_shared<GraphNode>(module);
        nodeMap[key] = node;
        mainModuleMap[module].push_back(node);
        return node;
    }

    void DependencyGraph::Impl::clear() {
        nodeMap.clear();
        mainModuleMap.clear();
    }

    std::vector<std::string> DependencyGraph::Impl::getPackageTopologicalOrder() const {
        std::unordered_map<std::string, std::set<std::string>> packageGraph;
        std::unordered_map<std::string, int> packageInDegree;

        for (const auto &[_, node] : nodeMap) {
            auto pkgId = node->module.packageId;
            packageGraph[pkgId] = {};
            packageInDegree[pkgId] = 0;
        }

        for (const auto &[_, node] : nodeMap) {
            auto srcPkg = node->module.packageId;
            for (const auto &dep : node->module.resolvedDependencies) {
                if (dep.packageId != srcPkg && packageGraph.count(dep.packageId)) {
                    if (packageGraph[srcPkg].insert(dep.packageId).second) {
                        packageInDegree[dep.packageId]++;
                    }
                }
            }
        }

        std::vector<std::string> result;
        std::queue<std::string> zeroInDegreePackages;

        for (const auto &[pkgId, degree] : packageInDegree) {
            if (degree == 0)
                zeroInDegreePackages.push(pkgId);
        }

        while (!zeroInDegreePackages.empty()) {
            auto currentPkg = zeroInDegreePackages.front();
            zeroInDegreePackages.pop();
            result.push_back(currentPkg);

            for (const auto &neighbor : packageGraph[currentPkg]) {
                if (--packageInDegree[neighbor] == 0)
                    zeroInDegreePackages.push(neighbor);
            }
        }

        if (result.size() != packageInDegree.size()) {
            MgrLog.langCoreCritical("Error: Package dependency cycle detected!");
            return {};
        }
        return result;
    }

    std::vector<PackageInitializationPlan> DependencyGraph::Impl::getPackageInitializationOrder() const {
        const auto globalOrder = getGlobalModuleInitializationOrder();
        if (globalOrder.empty())
            return {};

        std::unordered_map<std::string, PackageInitializationPlan> packageMap;
        for (const auto &module : globalOrder) {
            auto &[packageId, packagePath, modules, initializationOrder] = packageMap[module.packageId];
            packageId = module.packageId;
            packagePath = module.packagePath;
            initializationOrder.push_back(module);

            bool found = false;
            for (const auto &existing : modules) {
                if (existing.key() == module.key()) {
                    found = true;
                    break;
                }
            }
            if (!found)
                modules.push_back(module);
        }

        std::unordered_set<std::string> seenPackages;
        std::vector<PackageInitializationPlan> sortedResult;
        sortedResult.reserve(packageMap.size());

        for (const auto &module : globalOrder) {
            if (seenPackages.insert(module.packageId).second) {
                sortedResult.push_back(packageMap[module.packageId]);
            }
        }

        return sortedResult;
    }

    void DependencyGraph::Impl::strongConnect(const std::shared_ptr<GraphNode> &node,
                                              std::unordered_map<std::string, TarjanState> &state,
                                              std::stack<std::shared_ptr<GraphNode>> &stack, int &index,
                                              std::vector<std::vector<std::shared_ptr<GraphNode>>> &sccs) {
        const std::string nodeKey = node->module.key();
        auto &nodeState = state[nodeKey];

        nodeState.index = index;
        nodeState.lowlink = index;
        index++;
        stack.push(node);
        nodeState.onStack = true;

        for (const auto &neighbor : node->neighbors) {
            const std::string neighborKey = neighbor->module.key();
            auto &neighborState = state[neighborKey];

            if (neighborState.index == -1) {
                strongConnect(neighbor, state, stack, index, sccs);
                nodeState.lowlink = std::min(nodeState.lowlink, neighborState.lowlink);
            } else if (neighborState.onStack) {
                nodeState.lowlink = std::min(nodeState.lowlink, neighborState.index);
            }
        }

        if (nodeState.lowlink == nodeState.index) {
            std::vector<std::shared_ptr<GraphNode>> scc;
            std::shared_ptr<GraphNode> w;

            do {
                w = stack.top();
                stack.pop();
                state[w->module.key()].onStack = false;
                scc.push_back(w);
            }
            while (w != node);

            if (scc.size() > 1) {
                sccs.push_back(scc);
            }
        }
    }

    std::vector<std::vector<ModuleMetadata>> DependencyGraph::Impl::getCycles() const {
        std::vector<std::vector<ModuleMetadata>> allCycles;

        std::unordered_map<std::string, TarjanState> state;
        std::stack<std::shared_ptr<GraphNode>> stack;
        int index = 0;
        std::vector<std::vector<std::shared_ptr<GraphNode>>> sccs;

        for (const auto &[key, node] : nodeMap) {
            state[key] = TarjanState();
        }

        for (const auto &[key, node] : nodeMap) {
            if (state[key].index == -1) {
                strongConnect(node, state, stack, index, sccs);
            }
        }

        for (const auto &scc : sccs) {
            if (scc.size() > 1) {
                std::vector<ModuleMetadata> cycle;
                cycle.reserve(scc.size());
                for (const auto &node : scc) {
                    cycle.push_back(node->module);
                }
                allCycles.push_back(std::move(cycle));
            }
        }

        return allCycles;
    }

    std::vector<ModuleMetadata> DependencyGraph::Impl::getGlobalModuleInitializationOrder() const {
        if (auto cycles = getCycles(); !cycles.empty()) {
            MgrLog.langCoreCritical("Error: Global module dependency cycle detected!");

            for (size_t i = 0; i < cycles.size(); ++i) {
                const auto &cycle = cycles[i];
                MgrLog.langCoreCritical("Cycle %1 (%2 modules):", i + 1, cycle.size());
                for (size_t j = 0; j < cycle.size(); ++j) {
                    const auto &module = cycle[j];
                    MgrLog.langCoreCritical("  %1. %2:%3 (v%4, level %5)", j + 1, module.packageId, module.moduleId,
                                            module.version, module.level);
                }
            }
            return {};
        }

        std::unordered_map<std::string, std::set<std::string>> graph;
        std::unordered_map<std::string, int> inDegree;
        std::unordered_map<std::string, const GraphNode *> nodeMapById;

        for (const auto &[key, node] : nodeMap) {
            nodeMapById[key] = node.get();
            graph[key] = {};
            inDegree[key] = 0;
        }

        for (const auto &[key, node] : nodeMap) {
            for (const auto &dep : node->module.resolvedDependencies) {
                std::string depKey;
                for (const auto &[otherKey, otherNode] : nodeMap) {
                    if (otherNode->module.packageId == dep.packageId && otherNode->module.moduleId == dep.moduleId &&
                        otherNode->module.version == dep.version && otherNode->module.level == dep.level) {
                        depKey = otherKey;
                        break;
                    }
                }
                if (!depKey.empty() && depKey != key) {
                    if (graph[depKey].insert(key).second) {
                        inDegree[key]++;
                    }
                }
            }
        }

        std::vector<ModuleMetadata> order;
        std::queue<const GraphNode *> zeroInDegreeNodes;

        for (const auto &[key, degree] : inDegree) {
            if (degree == 0) {
                zeroInDegreeNodes.push(nodeMapById[key]);
            }
        }

        while (!zeroInDegreeNodes.empty()) {
            const auto node = zeroInDegreeNodes.front();
            zeroInDegreeNodes.pop();
            order.push_back(node->module);

            for (const auto &neighborKey : graph[node->module.key()]) {
                if (--inDegree[neighborKey] == 0) {
                    zeroInDegreeNodes.push(nodeMapById[neighborKey]);
                }
            }
        }

        if (order.size() != nodeMapById.size()) {
            return {};
        }

        return order;
    }

    std::vector<ModuleMetadata> DependencyGraph::Impl::getAllModules() const {
        std::vector<ModuleMetadata> modules;
        modules.reserve(nodeMap.size());
        for (const auto &[_, node] : nodeMap)
            modules.push_back(node->module);
        return modules;
    }

    std::vector<std::shared_ptr<DependencyGraph::Impl::GraphNode>>
    DependencyGraph::Impl::findNodesByDependency(const ResolvedDependency &dep) {
        std::vector<std::shared_ptr<GraphNode>> result;

        for (auto &[_, node] : nodeMap) {
            if (node->module.packageId == dep.packageId && node->module.moduleId == dep.moduleId &&
                node->module.level == dep.level && node->module.version == dep.version) {
                result.push_back(node);
            }
        }

        return result;
    }

    DependencyGraph::DependencyGraph() : _impl(std::make_unique<Impl>()) {}
    DependencyGraph::~DependencyGraph() = default;
    DependencyGraph::DependencyGraph(DependencyGraph &&) noexcept = default;
    DependencyGraph &DependencyGraph::operator=(DependencyGraph &&) noexcept = default;

    void DependencyGraph::addModule(const ModuleMetadata &module) const { _impl->addModule(module); }

    bool DependencyGraph::buildGraph() const { return _impl->buildGraph(); }

    void DependencyGraph::clear() const { _impl->clear(); }

    std::vector<std::vector<ModuleMetadata>> DependencyGraph::findCycles() const { return _impl->getCycles(); }

    std::vector<ModuleMetadata> DependencyGraph::getAllModules() const { return _impl->getAllModules(); }

    std::vector<PackageInitializationPlan> DependencyGraph::getPackageInitializationOrder() const {
        return _impl->getPackageInitializationOrder();
    }
} // namespace LangCore
