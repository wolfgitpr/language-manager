#ifndef LANGMGR_VERSION_UTILS_H
#define LANGMGR_VERSION_UTILS_H

#include <map>
#include <regex>
#include <string>
#include <vector>

namespace LangMgr
{
    struct DependencyRaw;
    struct ModuleInfo;

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

    class VersionRange {
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

        bool matches(const std::string &version) const;
        static std::string normalizeVersion(const std::string &version);
        static int compareVersions(const std::string &v1, const std::string &v2);

        std::vector<std::string> getVersionsInRange(const std::vector<std::string> &availableVersions) const;
        std::string getBestMatch(const std::vector<std::string> &availableVersions) const;
        bool isValid() const { return !constraints_.empty(); }

        std::string toString() const;

    private:
        std::vector<Constraint> constraints_;
        static Constraint parseConstraint(const std::string &constraintStr);
    };

    class VersionResolver {
    public:
        static ResolutionResult resolveDependency(const std::vector<ModuleInfo> &allModules,
                                                  const DependencyRaw &dependency, const ModuleInfo &requestingModule);

        static bool checkVersionConflicts(const std::vector<ModuleInfo> &modules, std::vector<std::string> &conflicts);

        static std::string selectBestVersionInRange(const std::vector<std::string> &versions,
                                                    const VersionRange &range);

        static std::vector<std::string> filterByApiLevel(const std::vector<std::string> &versions, int targetApiLevel,
                                                         const std::map<std::string, int> &versionToApiLevel);

        static std::string selectHighestVersion(const std::vector<std::string> &versions);

        static bool checkCompatibility(const std::string &version1, const std::string &version2,
                                       const std::string &compatibilityRule = "~");

        static std::vector<std::string> getPackageModules(const std::vector<ModuleInfo> &allModules,
                                                          const std::string &packageId);
    };

} // namespace LangMgr
#endif
