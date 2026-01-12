#include <LangMgr/Module/Dependency/Dependency.h>
#include <LangMgr/Module/Dependency/VersionUtils.h>
#include "Dependency_p.h"

#include <iostream>
#include <queue>
#include <set>
#include <unordered_set>
#include <utility>

namespace fs = std::filesystem;

namespace LangMgr
{
    bool DependencyRaw::operator==(const DependencyRaw &other) const {
        return packageId == other.packageId && moduleId == other.moduleId && level == other.level &&
            versionRange == other.versionRange;
    }

    std::string DependencyRaw::key() const { return packageId + ":" + moduleId + ":" + std::to_string(level); }

    bool DependencyInfo::operator==(const DependencyInfo &other) const {
        return packageId == other.packageId && moduleId == other.moduleId && level == other.level &&
            version == other.version;
    }

    std::string DependencyInfo::key() const {
        return packageId + ":" + moduleId + ":" + version + ":" + std::to_string(level);
    }

    Dependency::Impl::Impl() = default;
    Dependency::Impl::~Impl() = default;

    bool Dependency::Impl::addModule(const ModuleInfo &module) {
        const auto node = getOrCreateNode(module);
        for (const auto &dep : module.dependencies) {
            if (auto depNodes = findNodesByDependency(dep); depNodes.empty()) {
                std::cerr << "Warning: Module " << module.packageId << ":" << module.moduleId << " depends on "
                          << dep.packageId << ":" << dep.moduleId << " version " << dep.version << " (Level "
                          << dep.level << ") but no suitable version found in the dependency graph." << std::endl;
                std::vector<std::string> alternativeModules;
                for (const auto &[key, n] : nodeMap) {
                    if (n->module.packageId == dep.packageId && n->module.moduleId == dep.moduleId) {
                        alternativeModules.push_back("v" + n->module.version +
                                                     " (Level:" + std::to_string(n->module.level) + ")");
                    }
                }

                if (!alternativeModules.empty()) {
                    std::cerr << "  Available versions of " << dep.packageId << ":" << dep.moduleId << ": ";
                    for (size_t i = 0; i < alternativeModules.size(); ++i) {
                        if (i > 0)
                            std::cerr << ", ";
                        std::cerr << alternativeModules[i];
                    }
                    std::cerr << std::endl;
                }
                return false;
            }
        }
        return true;
    }

    std::shared_ptr<Dependency::Impl::GraphNode> Dependency::Impl::getOrCreateNode(const ModuleInfo &module) {
        const auto key = module.key();
        if (const auto it = nodeMap.find(key); it != nodeMap.end())
            return it->second;

        auto node = std::make_shared<GraphNode>(module);
        nodeMap[key] = node;
        mainModuleMap[module].push_back(node);
        return node;
    }

    void Dependency::Impl::clear() {
        nodeMap.clear();
        mainModuleMap.clear();
    }

    std::vector<std::string> Dependency::Impl::getPackageTopologicalOrder() const {
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
            std::cerr
                << "Error: Package dependency cycle detected! Some packages cannot be initialized in proper order."
                << std::endl;
            return {};
        }
        return result;
    }

    std::vector<ModuleInfo> Dependency::Impl::getGlobalModuleInitializationOrder() const {
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
                    if (otherNode->module.packageId == dep.packageId && otherNode->module.moduleId == dep.moduleId &&
                        otherNode->module.level == dep.level) {
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
            std::cerr << "Error: Global module dependency cycle detected! Cannot resolve initialization order."
                      << std::endl;
            return {};
        }
        return order;
    }

    std::vector<PackageInitInfo> Dependency::Impl::getPackageInitializationOrder() const {
        const auto globalOrder = getGlobalModuleInitializationOrder();
        if (globalOrder.empty())
            return {};

        std::unordered_map<std::string, PackageInitInfo> packageMap;
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
        std::vector<PackageInitInfo> sortedResult;
        sortedResult.reserve(packageMap.size());

        for (const auto &module : globalOrder) {
            if (seenPackages.insert(module.packageId).second) {
                sortedResult.push_back(packageMap[module.packageId]);
            }
        }

        return sortedResult;
    }

    bool Dependency::Impl::validate() const {
        std::unordered_map<std::string, VisitState> visited;
        for (const auto &[key, _] : nodeMap)
            visited[key] = VisitState::UNVISITED;

        for (const auto &[key, node] : nodeMap) {
            if (visited[key] == VisitState::UNVISITED && hasCycleDFS(node, visited, 0))
                return false;
        }
        return true;
    }

    bool Dependency::Impl::hasCycleDFS(const std::shared_ptr<GraphNode> &node,
                                       std::unordered_map<std::string, VisitState> &visited, const int depth) {
        if (depth > DependencyConstants::MAX_DEPTH)
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

    std::vector<std::vector<ModuleInfo>> Dependency::Impl::getCycles() const {
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
                if (cyclesFound >= DependencyConstants::MAX_TOTAL_CYCLES)
                    break;
            }
        }

        if (allCycles.size() > DependencyConstants::MAX_CYCLE_COUNT) {
            allCycles.resize(DependencyConstants::MAX_CYCLE_COUNT);
        }
        return allCycles;
    }

    void Dependency::Impl::findCyclesLimited(const std::shared_ptr<GraphNode> &startNode,
                                             std::unordered_map<std::string, VisitState> &visited,
                                             std::vector<std::shared_ptr<GraphNode>> &path,
                                             std::vector<std::vector<ModuleInfo>> &allCycles, const int depth,
                                             int &cyclesFound) {
        if (depth > DependencyConstants::MAX_DEPTH || cyclesFound >= DependencyConstants::MAX_TOTAL_CYCLES)
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

                    if (!duplicate && allCycles.size() < DependencyConstants::MAX_CYCLE_COUNT)
                        allCycles.push_back(std::move(cycle));
                }
            } else if (visited[neighborKey] == VisitState::UNVISITED) {
                findCyclesLimited(neighbor, visited, path, allCycles, depth + 1, cyclesFound);
                if (cyclesFound >= DependencyConstants::MAX_TOTAL_CYCLES)
                    break;
            }
        }

        path.pop_back();
        visited[nodeKey] = VisitState::VISITED;
    }

    std::vector<ModuleInfo> Dependency::Impl::getAllModules() const {
        std::vector<ModuleInfo> modules;
        modules.reserve(nodeMap.size());
        for (const auto &[_, node] : nodeMap)
            modules.push_back(node->module);
        return modules;
    }

    std::vector<std::shared_ptr<Dependency::Impl::GraphNode>>
    Dependency::Impl::findNodesByDependency(const DependencyInfo &dep) {
        std::vector<std::shared_ptr<GraphNode>> result;
        for (auto &[_, node] : nodeMap) {
            if (node->module.packageId == dep.packageId && node->module.moduleId == dep.moduleId &&
                node->module.level == dep.level) {
                result.push_back(node);
            }
        }
        return result;
    }

    Dependency::Dependency() : _impl(std::make_unique<Impl>()) {}
    Dependency::~Dependency() = default;
    Dependency::Dependency(Dependency &&) noexcept = default;
    Dependency &Dependency::operator=(Dependency &&) noexcept = default;

    bool Dependency::addModule(const ModuleInfo &module) const { return _impl->addModule(module); }

    void Dependency::clear() const { _impl->clear(); }

    bool Dependency::validate() const { return _impl->validate(); }

    std::vector<std::vector<ModuleInfo>> Dependency::getCycles() const { return _impl->getCycles(); }

    std::vector<ModuleInfo> Dependency::getAllModules() const { return _impl->getAllModules(); }

    std::vector<PackageInitInfo> Dependency::getPackageInitializationOrder() const {
        return _impl->getPackageInitializationOrder();
    }

    std::vector<ModuleInfo> Dependency::getModulesByUniqueKey(const std::string &uniqueKey) const {
        std::vector<ModuleInfo> result;
        const auto allModules = getAllModules();

        for (const auto &module : allModules) {
            if (module.uniqueKey() == uniqueKey) {
                result.push_back(module);
            }
        }

        std::sort(result.begin(), result.end(), [](const ModuleInfo &a, const ModuleInfo &b)
                  { return VersionRange::compareVersions(a.version, b.version) > 0; });

        return result;
    }

    VersionSelectionResult Dependency::selectBestVersion(const std::vector<std::string> &allVersions,
                                                         const std::string &versionRange, int targetLevel,
                                                         const std::unordered_map<std::string, int> &versionToLevel) {

        VersionSelectionResult result;

        if (allVersions.empty()) {
            result.error = "No versions available for selection";
            return result;
        }

        result.candidates = allVersions;

        VersionRange range(versionRange);
        if (!range.isValid() && !versionRange.empty() && versionRange != "*") {
            result.error = "Invalid version range format: '" + versionRange + "'";
            return result;
        }

        auto versionsInRange = range.getVersionsInRange(allVersions);
        if (versionsInRange.empty()) {
            result.error = "No versions found matching the range '" + versionRange + "'";
            return result;
        }

        result.filteredCandidates = versionsInRange;

        if (targetLevel != -1) {
            std::vector<std::string> filteredVersions;
            for (const auto &version : versionsInRange) {
                if (auto it = versionToLevel.find(version); it != versionToLevel.end() && it->second == targetLevel) {
                    filteredVersions.push_back(version);
                }
            }

            if (filteredVersions.empty()) {
                std::string levelStr = std::to_string(targetLevel);
                result.error = "No versions found matching Level " + levelStr + " within range '" + versionRange + "'";
                return result;
            }
            versionsInRange = filteredVersions;
        }

        std::string bestVersion = range.getBestMatch(versionsInRange);
        if (bestVersion.empty()) {
            result.error = "Failed to select best version from candidates";
            return result;
        }

        result.success = true;
        result.selectedVersion = bestVersion;

        if (auto it = versionToLevel.find(bestVersion); it != versionToLevel.end()) {
            result.filteredCandidates = versionsInRange;
        }

        return result;
    }

    void printModule(const ModuleInfo &module) {
        std::cout << module.packageId << ":" << module.moduleId << " [class=" << module.iid
                  << ", level=" << module.level << ", version=" << module.version << ", config=" << module.configuration
                  << ", path=" << module.packagePath.string() << "]";
    }

    void printDependency(const DependencyInfo &dep) {
        std::cout << dep.packageId << ":" << dep.moduleId << "(level=" << dep.level << ", level=" << dep.level
                  << ", version=" << dep.version << ")";
    }

    void printModuleList(const std::vector<ModuleInfo> &modules, const std::string &title) {
        if (!title.empty())
            std::cout << title << " (" << modules.size() << " modules):\n";

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
            std::cout << title << " (" << packageOrder.size() << " packages):\n";

        for (size_t pkgIdx = 0; pkgIdx < packageOrder.size(); ++pkgIdx) {
            const auto &[packageId, packagePath, modules, initializationOrder] = packageOrder[pkgIdx];
            std::cout << "package " << pkgIdx + 1 << ": " << packageId << " (path: " << packagePath.string() << ")\n";
            std::cout << "  includes " << modules.size() << " modules\n";
            if (!initializationOrder.empty()) {
                std::cout << "  InitializationOrder:\n";
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
