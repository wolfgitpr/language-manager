#ifndef LANGMGR_DEPENDENCY_RESOLVER_H
#define LANGMGR_DEPENDENCY_RESOLVER_H

#include <sstream>
#include <unordered_map>
#include <vector>

#include <LangMgr/Module/Dependency/Dependency.h>
#include <LangMgr/Module/Dependency/VersionUtils.h>

namespace LangMgr
{
    class DependencyResolver {
    public:
        DependencyResolver() = default;

        bool resolveAllDependencies(std::vector<ModuleInfo> &modules);
        bool resolveModuleDependencies(ModuleInfo &module, const std::vector<ModuleInfo> &allModules);

        const std::vector<std::string> &getErrors() const { return errors_; }
        const std::vector<ModuleInfo> &getResolvedModules() const { return resolvedModules_; }

        void clear();

    private:
        static bool selectBestModules(std::vector<ModuleInfo> &modules);
        void buildIndex(const std::vector<ModuleInfo> &modules);

        std::vector<ModuleInfo> resolvedModules_;
        std::vector<std::string> errors_;
        std::unordered_map<std::string, std::vector<ModuleInfo *>> moduleIndex_;
    };
} // namespace LangMgr

#endif
