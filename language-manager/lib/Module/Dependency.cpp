#include <LangMgr/Module/Dependency.h>

#include <algorithm>
#include <iostream>
#include <queue>
#include <set>
#include <unordered_set>
#include <utility>

namespace fs = std::filesystem;

namespace LangMgr
{
    bool DependencyInfo::operator==(const DependencyInfo &other) const {
        return packageId == other.packageId && moduleId == other.moduleId;
    }

    std::string DependencyInfo::key() const { return packageId + ":" + moduleId; }

    struct Dependency::GraphNode {
        ModuleInfo module;
        std::vector<std::shared_ptr<GraphNode>> neighbors;
        explicit GraphNode(ModuleInfo mod) : module(std::move(mod)) {}
    };

    Dependency::Dependency() = default;
    Dependency::~Dependency() = default;

    std::shared_ptr<Dependency::GraphNode> Dependency::getOrCreateNode(const ModuleInfo &module) {
        const auto key = module.key();
        if (const auto it = nodeMap.find(key); it != nodeMap.end())
            return it->second;

        auto node = std::make_shared<GraphNode>(module);
        nodeMap[key] = node;
        mainModuleMap[module].push_back(node);
        return node;
    }

    bool Dependency::addModule(const ModuleInfo &module) {
        const auto node = getOrCreateNode(module);
        for (const auto &dep : module.dependencies) {
            for (const auto &depNode : findNodesByDependency(dep))
                node->neighbors.push_back(depNode);
        }
        return true;
    }

    std::vector<std::string> Dependency::getPackageTopologicalOrder() const {
        std::unordered_map<std::string, std::set<std::string>> packageGraph;
        std::unordered_map<std::string, int> packageInDegree;

        for (const auto &[_, node] : nodeMap) {
            auto pkgId = node->module.packageId;
            packageGraph[pkgId] = {};
            packageInDegree[pkgId] = 0;
        }

        for (const auto &[_, node] : nodeMap) {
            auto srcPkg = node->module.packageId;
            for (const auto &dep : node->module.dependencies) {
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
            std::cerr << "Warning: Package dependency cycle detected!\n";
            return {};
        }
        return result;
    }

    std::vector<ModuleInfo> Dependency::getGlobalModuleInitializationOrder() const {
        std::unordered_map<std::string, std::set<std::string>> graph;
        std::unordered_map<std::string, int> inDegree;
        std::unordered_map<std::string, const GraphNode *> nodeMapById;

        for (const auto &[key, node] : nodeMap) {
            nodeMapById[key] = node.get();
            graph[key] = {};
            inDegree[key] = 0;
        }

        for (const auto &[key, node] : nodeMap) {
            for (const auto &dep : node->module.dependencies) {
                std::string depKey;
                for (const auto &[otherKey, otherNode] : nodeMap) {
                    if (otherNode->module.packageId == dep.packageId && otherNode->module.moduleId == dep.moduleId) {
                        depKey = otherKey;
                        break;
                    }
                }
                if (!depKey.empty() && depKey != key && graph[depKey].insert(key).second)
                    inDegree[key]++;
            }
        }

        std::vector<ModuleInfo> order;
        std::queue<const GraphNode *> zeroInDegreeNodes;

        for (const auto &[key, degree] : inDegree) {
            if (degree == 0)
                zeroInDegreeNodes.push(nodeMapById[key]);
        }

        while (!zeroInDegreeNodes.empty()) {
            auto node = zeroInDegreeNodes.front();
            zeroInDegreeNodes.pop();
            order.push_back(node->module);

            for (const auto &neighborKey : graph[node->module.key()]) {
                if (--inDegree[neighborKey] == 0)
                    zeroInDegreeNodes.push(nodeMapById[neighborKey]);
            }
        }

        if (order.size() != nodeMapById.size()) {
            std::cerr << "Warning: Global module dependency cycle detected!\n";
            return {};
        }
        return order;
    }

    std::vector<PackageInitInfo> Dependency::getPackageInitializationOrder() const {
        const auto globalOrder = getGlobalModuleInitializationOrder();
        if (globalOrder.empty())
            return {};

        std::unordered_map<std::string, PackageInitInfo> packageMap;
        for (const auto &module : globalOrder) {
            auto &[packageId, packagePath, modules, initOrder] = packageMap[module.packageId];
            packageId = module.packageId;
            packagePath = module.packagePath;
            initOrder.push_back(module);

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
        std::vector<PackageInitInfo> sortedResult;
        sortedResult.reserve(packageMap.size());

        for (const auto &module : globalOrder) {
            if (seenPackages.insert(module.packageId).second) {
                sortedResult.push_back(packageMap[module.packageId]);
            }
        }

        return sortedResult;
    }

    std::vector<ModuleInfo> Dependency::getPackageModuleOrder(const std::string &packageId) const {
        std::unordered_map<std::string, std::shared_ptr<GraphNode>> subNodeMap;
        std::unordered_map<std::string, int> inDegree;
        std::queue<std::shared_ptr<GraphNode>> zeroInDegreeNodes;

        for (const auto &[key, node] : nodeMap) {
            if (node->module.packageId == packageId) {
                subNodeMap[key] = node;
                inDegree[key] = 0;
            }
        }

        for (const auto &[key, node] : subNodeMap) {
            for (const auto &neighbor : node->neighbors) {
                if (subNodeMap.count(neighbor->module.key()))
                    inDegree[neighbor->module.key()]++;
            }
        }

        for (const auto &[key, node] : subNodeMap) {
            if (inDegree[key] == 0)
                zeroInDegreeNodes.push(node);
        }

        std::vector<ModuleInfo> order;
        while (!zeroInDegreeNodes.empty()) {
            const auto node = zeroInDegreeNodes.front();
            zeroInDegreeNodes.pop();
            order.push_back(node->module);

            for (const auto &neighbor : node->neighbors) {
                if (auto neighborKey = neighbor->module.key();
                    subNodeMap.count(neighborKey) && --inDegree[neighborKey] == 0) {
                    zeroInDegreeNodes.push(neighbor);
                }
            }
        }

        if (order.size() != subNodeMap.size()) {
            std::cerr << "Warning: Module dependency cycle detected in package " << packageId << "\n";
            return {};
        }
        return order;
    }

    std::vector<std::pair<std::string, fs::path>> Dependency::getPackageDependencyOrder() const {
        const auto packageOrder = getPackageTopologicalOrder();
        std::vector<std::pair<std::string, fs::path>> result;
        result.reserve(packageOrder.size());

        std::unordered_map<std::string, fs::path> packagePaths;
        for (const auto &[_, node] : nodeMap)
            packagePaths[node->module.packageId] = node->module.packagePath;

        for (const auto &pkgId : packageOrder) {
            if (auto it = packagePaths.find(pkgId); it != packagePaths.end())
                result.emplace_back(pkgId, it->second);
        }
        return result;
    }

    bool Dependency::validate() {
        std::unordered_map<std::string, VisitState> visited;
        for (const auto &[key, _] : nodeMap)
            visited[key] = VisitState::UNVISITED;

        for (const auto &[key, node] : nodeMap) {
            if (visited[key] == VisitState::UNVISITED && hasCycleDFS(node, visited, 0))
                return false;
        }
        return true;
    }

    std::vector<std::vector<ModuleInfo>> Dependency::getCycles() {
        std::vector<std::vector<ModuleInfo>> allCycles;
        if (validate())
            return allCycles;

        std::unordered_map<std::string, VisitState> visited;
        for (const auto &[key, _] : nodeMap)
            visited[key] = VisitState::UNVISITED;

        int cyclesFound = 0;
        for (const auto &[key, node] : nodeMap) {
            if (visited[key] == VisitState::UNVISITED) {
                std::vector<std::shared_ptr<GraphNode>> path;
                findCyclesLimited(node, visited, path, allCycles, 0, cyclesFound);
                if (cyclesFound >= MAX_TOTAL_CYCLES)
                    break;
            }
        }

        if (allCycles.size() > MAX_CYCLE_COUNT) {
            allCycles.resize(MAX_CYCLE_COUNT);
        }
        return allCycles;
    }

    std::vector<std::vector<ModuleInfo>> Dependency::getMainModules() const {
        std::vector<std::vector<ModuleInfo>> result;
        result.reserve(mainModuleMap.size());

        for (const auto &[mainModule, nodes] : mainModuleMap) {
            std::vector<ModuleInfo> modules;
            modules.reserve(nodes.size());
            for (const auto &node : nodes)
                modules.push_back(node->module);
            result.push_back(std::move(modules));
        }
        return result;
    }

    std::vector<ModuleInfo> Dependency::findModulesByMainModule(const ModuleInfo &mainModule) const {
        if (const auto it = mainModuleMap.find(mainModule); it != mainModuleMap.end()) {
            std::vector<ModuleInfo> result;
            result.reserve(it->second.size());
            for (const auto &node : it->second)
                result.push_back(node->module);
            return result;
        }
        return {};
    }

    size_t Dependency::getModuleCount() const { return nodeMap.size(); }
    size_t Dependency::getMainModuleCount() const { return mainModuleMap.size(); }

    size_t Dependency::getDependencyCount() const {
        size_t count = 0;
        for (const auto &[_, node] : nodeMap)
            count += node->neighbors.size();
        return count;
    }

    std::vector<ModuleInfo> Dependency::getAllModules() const {
        std::vector<ModuleInfo> modules;
        modules.reserve(nodeMap.size());
        for (const auto &[_, node] : nodeMap)
            modules.push_back(node->module);
        return modules;
    }

    void Dependency::clear() {
        nodeMap.clear();
        mainModuleMap.clear();
    }

    std::vector<std::shared_ptr<Dependency::GraphNode>> Dependency::findNodesByDependency(const DependencyInfo &dep) {
        std::vector<std::shared_ptr<GraphNode>> result;
        for (auto &[_, node] : nodeMap) {
            if (node->module.packageId == dep.packageId && node->module.moduleId == dep.moduleId)
                result.push_back(node);
        }
        return result;
    }

    bool Dependency::hasCycleDFS(const std::shared_ptr<GraphNode> &node,
                                 std::unordered_map<std::string, VisitState> &visited, const int depth) {
        if (depth > MAX_DEPTH)
            return false;

        const auto nodeKey = node->module.key();
        visited[nodeKey] = VisitState::VISITING;

        for (auto &neighbor : node->neighbors) {
            auto neighborKey = neighbor->module.key();
            if (visited[neighborKey] == VisitState::VISITING)
                return true;
            if (visited[neighborKey] == VisitState::UNVISITED) {
                if (hasCycleDFS(neighbor, visited, depth + 1))
                    return true;
            }
        }

        visited[nodeKey] = VisitState::VISITED;
        return false;
    }

    void Dependency::findCyclesLimited(const std::shared_ptr<GraphNode> &startNode,
                                       std::unordered_map<std::string, VisitState> &visited,
                                       std::vector<std::shared_ptr<GraphNode>> &path,
                                       std::vector<std::vector<ModuleInfo>> &allCycles, const int depth,
                                       int &cyclesFound) {
        if (depth > MAX_DEPTH || cyclesFound >= MAX_TOTAL_CYCLES)
            return;

        const auto nodeKey = startNode->module.key();
        visited[nodeKey] = VisitState::VISITING;
        path.push_back(startNode);

        for (auto &neighbor : startNode->neighbors) {
            auto neighborKey = neighbor->module.key();
            if (visited[neighborKey] == VisitState::VISITING) {
                cyclesFound++;
                auto it = std::find_if(path.begin(), path.end(),
                                       [&neighborKey](const auto &node) { return node->module.key() == neighborKey; });

                if (it != path.end()) {
                    std::vector<ModuleInfo> cycle;
                    for (auto iter = it; iter != path.end(); ++iter)
                        cycle.push_back((*iter)->module);
                    cycle.push_back(neighbor->module);

                    bool duplicate = false;
                    for (const auto &existingCycle : allCycles) {
                        if (existingCycle.size() == cycle.size() &&
                            std::equal(cycle.begin(), cycle.end(), existingCycle.begin())) {
                            duplicate = true;
                            break;
                        }
                    }

                    if (!duplicate && allCycles.size() < MAX_CYCLE_COUNT)
                        allCycles.push_back(std::move(cycle));
                }
            } else if (visited[neighborKey] == VisitState::UNVISITED) {
                findCyclesLimited(neighbor, visited, path, allCycles, depth + 1, cyclesFound);
                if (cyclesFound >= MAX_TOTAL_CYCLES)
                    break;
            }
        }

        path.pop_back();
        visited[nodeKey] = VisitState::VISITED;
    }

    void printModule(const ModuleInfo &module) {
        std::cout << module.packageId << ":" << module.moduleId << " [class=" << module.iid
                  << ", config=" << module.configuration << ", path=" << module.packagePath.string() << "]";
    }

    void printDependency(const DependencyInfo &dep) {
        std::cout << dep.packageId << ":" << dep.moduleId << "(level=" << dep.level << ")";
    }

    void printModuleList(const std::vector<ModuleInfo> &modules, const std::string &title) {
        if (!title.empty())
            std::cout << title << " (" << modules.size() << "个模块):\n";

        for (size_t i = 0; i < modules.size(); ++i) {
            std::cout << "  " << i + 1 << ". ";
            printModule(modules[i]);
            std::cout << "\n";
        }

        if (!modules.empty())
            std::cout << "\n";
    }

    void printPackageInitOrder(const std::vector<PackageInitInfo> &packageOrder, const std::string &title) {
        if (!title.empty())
            std::cout << title << " (" << packageOrder.size() << "个包):\n";

        for (size_t pkgIdx = 0; pkgIdx < packageOrder.size(); ++pkgIdx) {
            const auto &[packageId, packagePath, modules, initializationOrder] = packageOrder[pkgIdx];
            std::cout << "包 " << pkgIdx + 1 << ": " << packageId << " (路径: " << packagePath.string() << ")\n";
            std::cout << "  包含 " << modules.size() << " 个模块\n";
            if (!initializationOrder.empty()) {
                std::cout << "  初始化顺序:\n";
                for (size_t modIdx = 0; modIdx < initializationOrder.size(); ++modIdx) {
                    const auto &mod = initializationOrder[modIdx];
                    std::cout << "    " << modIdx + 1 << ". ";
                    printModule(mod);
                    std::cout << "\n";
                }
            }
            std::cout << "\n";
        }
    }
} // namespace LangMgr
