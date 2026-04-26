#ifndef LANGCORE_VERSION_UTILS_H
#define LANGCORE_VERSION_UTILS_H

#include <string>
#include <vector>

#include <LangCore/Module/Dependency/DependencyGraph.h>

namespace LangCore
{
    struct ResolutionResult {
        bool success = false;
        std::string resolvedVersion;
        std::string error;
        std::vector<std::string> candidates;
        std::vector<std::string> versionsInRange;
        std::vector<std::string> allPackageModules;
        std::string requestedPackageId;
        std::string requestedModuleId;
        std::string versionRange;
        int requestedLevel = -1;
        std::string configPath;
        int resolvedLevel = -1;
    };

    class LANGCORE_EXPORT VersionRange {
    public:
        enum class Op { LESS, LESS_EQUAL, GREATER, GREATER_EQUAL, EQUAL, COMPATIBLE, ANY, HYPHEN_RANGE };

        struct Constraint {
            Op op;
            std::string version;
            std::string version2;

            bool matches(const std::string &testVersion) const;
            std::string toString() const;
            bool isMoreSpecificThan(const Constraint &other) const;
        };

        VersionRange() = default;
        explicit VersionRange(const std::string &rangeStr);

        static std::string normalizeVersion(const std::string &version);
        static int compareVersions(const std::string &v1, const std::string &v2);

        std::vector<std::string> getVersionsInRange(const std::vector<std::string> &availableVersions) const;

        std::string toString() const;

    private:
        std::vector<Constraint> constraints_;
        static Constraint parseConstraint(const std::string &constraintStr);
    };

    class LANGCORE_EXPORT VersionResolver {
    public:
        static ResolutionResult resolveDependency(const std::vector<ModuleMetadata> &allModules,
                                                  const DependencyRequirement &dependency,
                                                  const ModuleMetadata &requestingModule);

        static std::string selectHighestVersion(const std::vector<std::string> &versions);
    };

} // namespace LangCore

#endif
