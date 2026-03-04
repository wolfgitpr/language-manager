#ifndef LANGCORE_DEPENDENCY_RESOLVER_H
#define LANGCORE_DEPENDENCY_RESOLVER_H

#include <unordered_map>
#include <vector>

#include <LangCore/Module/Dependency/DependencyGraph.h>

namespace LangCore
{
    class DependencyResolver {
    public:
        DependencyResolver() = default;

        bool resolveAllDependencies(std::vector<ModuleMetadata> &modules);
        const std::vector<std::string> &getErrors() const { return errors_; }
        const std::vector<ModuleMetadata> &getResolvedModules() const { return resolvedModules_; }

        void clear();

    private:
        static void selectBestModules(std::vector<ModuleMetadata> &modules);
        void buildIndex(const std::vector<ModuleMetadata> &modules);

        std::vector<ModuleMetadata> resolvedModules_;
        std::vector<std::string> errors_;
        std::unordered_map<std::string, std::vector<ModuleMetadata *>> moduleIndex_;
    };
} // namespace LangCore

#endif
