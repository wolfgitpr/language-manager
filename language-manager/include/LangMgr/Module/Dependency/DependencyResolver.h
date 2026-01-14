#ifndef LANGMGR_DEPENDENCY_RESOLVER_H
#define LANGMGR_DEPENDENCY_RESOLVER_H

#include <unordered_map>
#include <vector>

#include <LangMgr/Module/Dependency/DependencyGraph.h>

namespace LangMgr
{
    class DependencyResolver {
    public:
        DependencyResolver() = default;

        bool resolveAllDependencies(std::vector<ModuleMetadata> &modules);
        const std::vector<std::string> &getErrors() const { return errors_; }
        const std::vector<ModuleMetadata> &getResolvedModules() const { return resolvedModules_; }

        void clear();

    private:
        static bool selectBestModules(std::vector<ModuleMetadata> &modules);
        void buildIndex(const std::vector<ModuleMetadata> &modules);

        std::vector<ModuleMetadata> resolvedModules_;
        std::vector<std::string> errors_;
        std::unordered_map<std::string, std::vector<ModuleMetadata *>> moduleIndex_;
    };
} // namespace LangMgr

#endif
