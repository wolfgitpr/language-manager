#include <LangMgr/Module/Dependency/DependencyResolver.h>
#include <LangMgr/Module/Dependency/VersionUtils.h>

namespace LangMgr
{
    void DependencyResolver::buildIndex(const std::vector<ModuleInfo> &modules) {
        moduleIndex_.clear();
        for (auto &module : const_cast<std::vector<ModuleInfo> &>(modules)) {
            std::string key = module.packageId + "::" + module.moduleId;
            moduleIndex_[key].push_back(&module);
        }
    }

    bool DependencyResolver::selectBestModules(std::vector<ModuleInfo> &modules) {
        std::unordered_map<std::string, ModuleInfo *> bestModules;
        std::vector<ModuleInfo> selected;

        for (auto &module : modules) {
            std::string uniqueKey = module.uniqueKey();

            if (auto it = bestModules.find(uniqueKey); it == bestModules.end()) {
                bestModules[uniqueKey] = &module;
            } else {
                if (VersionRange::compareVersions(module.version, it->second->version) > 0) {
                    bestModules[uniqueKey] = &module;
                }
            }
        }

        modules.erase(std::remove_if(modules.begin(), modules.end(),
                                     [&bestModules](const ModuleInfo &module)
                                     {
                                         const std::string uniqueKey = module.uniqueKey();
                                         const auto it = bestModules.find(uniqueKey);
                                         return it == bestModules.end() || it->second != &module;
                                     }),
                      modules.end());

        return true;
    }

    bool DependencyResolver::resolveAllDependencies(std::vector<ModuleInfo> &modules) {
        clear();

        if (!selectBestModules(modules)) {
            return false;
        }

        buildIndex(modules);

        const std::vector<ModuleInfo> allModulesForResolution = modules;

        for (auto &module : modules) {
            if (!resolveModuleDependencies(module, allModulesForResolution)) {
                return false;
            }
        }

        resolvedModules_ = modules;
        return errors_.empty();
    }

    bool DependencyResolver::resolveModuleDependencies(ModuleInfo &module, const std::vector<ModuleInfo> &allModules) {
        module.dependencies.clear();

        for (const auto &depRaw : module.dependenciesRaw) {
            // 从全局模块列表中查找匹配的模块
            std::vector<ModuleInfo> candidates;
            for (const auto &candidate : allModules) {
                if (candidate.packageId == depRaw.packageId && candidate.moduleId == depRaw.moduleId) {
                    candidates.push_back(candidate);
                }
            }

            // 如果没有找到任何候选模块，报告错误
            if (candidates.empty()) {
                std::ostringstream oss;
                oss << "[ERROR] Dependency not found" << std::endl;
                oss << "  Requesting module: " << module.packageId << "::" << module.moduleId << " (v" << module.version
                    << ", level " << module.level << ")" << std::endl;
                oss << "  Required: " << depRaw.packageId << "::" << depRaw.moduleId << " (level "
                    << (depRaw.level == -1 ? "any" : std::to_string(depRaw.level))
                    << ", version: " << (depRaw.versionRange.empty() ? "any" : depRaw.versionRange) << ")" << std::endl;

                errors_.push_back(oss.str());
                continue;
            }

            // 调用版本解析器
            auto result = VersionResolver::resolveDependency(candidates, depRaw, module);

            if (!result.success) {
                errors_.push_back(result.error);
                continue;
            }

            DependencyInfo resolvedDep;
            resolvedDep.packageId = depRaw.packageId;
            resolvedDep.moduleId = depRaw.moduleId;
            resolvedDep.level = result.resolvedLevel;
            resolvedDep.version = result.resolvedVersion;

            module.dependencies.push_back(resolvedDep);
        }

        return errors_.empty();
    }

    void DependencyResolver::clear() {
        resolvedModules_.clear();
        errors_.clear();
        moduleIndex_.clear();
    }
} // namespace LangMgr
