#include "DependencyResolver.h"
#include <algorithm>
#include <iostream>
#include <sstream>

#include <LangCore/Module/Dependency/DependencyGraph.h>
#include <LangCore/Module/Dependency/VersionUtils.h>

namespace LangCore
{
    bool DependencyResolver::resolveAllDependencies(std::vector<ModuleMetadata> &modules) {
        clear();

        modules.erase(std::remove_if(modules.begin(), modules.end(),
                                     [this](const ModuleMetadata &module)
                                     {
                                         for (const auto &depRaw : module.requirements) {
                                             if (depRaw.packageId == module.packageId &&
                                                 depRaw.moduleId == module.moduleId && depRaw.level == module.level) {
                                                 std::ostringstream oss;
                                                 oss << "[ERROR] Removed module " << module.packageId
                                                     << "::" << module.moduleId << ": level " << module.level
                                                     << " because it depends on itself!" << std::endl;
                                                 errors_.push_back(oss.str());
                                                 return true;
                                             }
                                         }
                                         return false;
                                     }),
                      modules.end());

        if (!selectBestModules(modules)) {
            return false;
        }

        buildIndex(modules);

        std::unordered_map<std::string, bool> resolvedStatus;

        for (auto &module : modules) {
            resolvedStatus[module.key()] = module.resolvedDependencies.size() == module.requirements.size();
        }

        size_t maxIterations = modules.size() * 2;
        for (size_t iteration = 0; iteration < maxIterations; ++iteration) {
            bool progressMade = false;

            for (auto &module : modules) {
                std::string moduleKey = module.key();

                if (resolvedStatus[moduleKey]) {
                    continue;
                }

                bool canResolve = true;
                std::vector<ResolvedDependency> newResolvedDeps;

                for (const auto &depRaw : module.requirements) {
                    bool alreadyResolved = false;
                    for (const auto &existingDep : module.resolvedDependencies) {
                        if (existingDep.packageId == depRaw.packageId && existingDep.moduleId == depRaw.moduleId) {
                            alreadyResolved = true;
                            newResolvedDeps.push_back(existingDep);
                            break;
                        }
                    }

                    if (alreadyResolved) {
                        continue;
                    }

                    std::vector<ModuleMetadata> candidates;
                    for (const auto &candidate : modules) {
                        if (candidate.packageId == depRaw.packageId && candidate.moduleId == depRaw.moduleId) {
                            candidates.push_back(candidate);
                        }
                    }

                    if (candidates.empty()) {
                        std::ostringstream oss;
                        oss << "[ERROR] Direct dependency missing: " << depRaw.packageId << "::" << depRaw.moduleId
                            << " required by module: " << module.packageId << "::" << module.moduleId << std::endl;
                        oss << "  Requesting module: " << module.packageId << "::" << module.moduleId << " (v"
                            << module.version << ", level " << module.level << ")" << std::endl;
                        oss << "  Required: " << depRaw.packageId << "::" << depRaw.moduleId << " (level "
                            << (depRaw.level == -1 ? "any" : std::to_string(depRaw.level))
                            << ", version: " << (depRaw.versionRange.empty() ? "any" : depRaw.versionRange) << ")"
                            << std::endl;

                        errors_.push_back(oss.str());
                        canResolve = false;
                        break;
                    }

                    auto result = VersionResolver::resolveDependency(candidates, depRaw, module);

                    if (!result.success) {
                        errors_.push_back(result.error);
                        canResolve = false;
                        break;
                    }

                    ResolvedDependency resolvedDep;
                    resolvedDep.packageId = depRaw.packageId;
                    resolvedDep.moduleId = depRaw.moduleId;
                    resolvedDep.level = result.resolvedLevel;
                    resolvedDep.version = result.resolvedVersion;
                    newResolvedDeps.push_back(resolvedDep);
                }

                if (canResolve) {
                    module.resolvedDependencies = newResolvedDeps;
                    resolvedStatus[moduleKey] = true;
                    progressMade = true;
                }
            }

            if (!progressMade) {
                std::vector<std::string> unresolvedModules;
                for (const auto &module : modules) {
                    if (!resolvedStatus[module.key()]) {
                        unresolvedModules.push_back(module.packageId + "::" + module.moduleId);
                    }
                }

                if (!unresolvedModules.empty()) {
                    std::ostringstream oss;
                    oss << "[ERROR] Cannot resolve dependencies for " << unresolvedModules.size()
                        << " module(s):" << std::endl;
                    for (const auto &modName : unresolvedModules) {
                        oss << "  - " << modName << std::endl;
                    }
                    oss << "Possible circular dependencies or missing dependencies." << std::endl;
                    errors_.push_back(oss.str());
                }
                break;
            }

            bool allDone = true;
            for (const auto &module : modules) {
                if (!resolvedStatus[module.key()]) {
                    allDone = false;
                    break;
                }
            }

            if (allDone) {
                break;
            }
        }

        for (const auto &module : modules) {
            if (resolvedStatus[module.key()]) {
                resolvedModules_.push_back(module);
            }
        }

        if (!errors_.empty()) {
            return false;
        }

        return true;
    }

    void DependencyResolver::buildIndex(const std::vector<ModuleMetadata> &modules) {
        moduleIndex_.clear();
        for (const auto &module : modules) {
            std::string key = module.packageId + "::" + module.moduleId + "::" + std::to_string(module.level);
            moduleIndex_[key].push_back(const_cast<ModuleMetadata *>(&module));
        }
    }

    bool DependencyResolver::selectBestModules(std::vector<ModuleMetadata> &modules) {
        std::unordered_map<std::string, ModuleMetadata *> bestModules;
        std::vector<ModuleMetadata> selected;

        for (auto &module : modules) {
            // 将level包含在唯一键中
            std::string uniqueKey = module.packageId + ":" + module.moduleId + ":" + std::to_string(module.level);

            if (auto it = bestModules.find(uniqueKey); it == bestModules.end()) {
                bestModules[uniqueKey] = &module;
            } else {
                if (VersionRange::compareVersions(module.version, it->second->version) > 0) {
                    bestModules[uniqueKey] = &module;
                }
            }
        }

        modules.erase(std::remove_if(modules.begin(), modules.end(),
                                     [&bestModules](const ModuleMetadata &module)
                                     {
                                         const std::string uniqueKey = module.packageId + ":" + module.moduleId + ":" +
                                             std::to_string(module.level);
                                         const auto it = bestModules.find(uniqueKey);
                                         return it == bestModules.end() || it->second != &module;
                                     }),
                      modules.end());

        return true;
    }

    void DependencyResolver::clear() {
        resolvedModules_.clear();
        errors_.clear();
        moduleIndex_.clear();
    }
} // namespace LangCore
