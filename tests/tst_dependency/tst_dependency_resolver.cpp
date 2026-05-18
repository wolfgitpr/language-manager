#include "tst_framework.h"

#include <LangCore/Module/Dependency/DependencyResolver.h>
#include <LangCore/Module/Dependency/DependencyGraph.h>
#include <LangCore/Module/Dependency/LevelCompatibilityChecker.h>
#include <LangCore/Module/Dependency/VersionUtils.h>

using namespace LangCore;

static ModuleMetadata makeModule(const std::string &pkgId, const std::string &modId, const std::string &version,
                                 int level, const std::string &type = "g2p", const std::string &iid = "",
                                 const std::string &config = "") {
    ModuleMetadata m;
    m.packageId = pkgId;
    m.moduleId = modId;
    m.version = version;
    m.level = level;
    m.type = type;
    m.iid = iid;
    m.configuration = config;
    return m;
}

static DependencyRequirement makeReq(const std::string &pkgId, const std::string &modId, int level = -1,
                                     const std::string &versionRange = "*") {
    DependencyRequirement req;
    req.packageId = pkgId;
    req.moduleId = modId;
    req.level = level;
    req.versionRange = versionRange;
    return req;
}

TEST_CASE(Resolver_CrossPackage_Simple) {
    DependencyResolver resolver;
    auto main = makeModule("pkg-main", "main", "1.0.0", 1);
    auto dep = makeModule("pkg-dep", "dep", "1.0.0", 1);
    main.requirements.push_back(makeReq("pkg-dep", "dep", -1, "*"));

    std::vector<ModuleMetadata> mods = {main, dep};
    ASSERT_TRUE(resolver.resolveAllDependencies(mods));
    auto resolved = resolver.getResolvedModules();
    ASSERT_EQ(resolved.size(), 2u);
}

TEST_CASE(Resolver_CrossPackage_Missing) {
    DependencyResolver resolver;
    auto main = makeModule("pkg-main", "main", "1.0.0", 1);
    main.requirements.push_back(makeReq("pkg-missing", "dep", -1, "*"));

    std::vector<ModuleMetadata> mods = {main};
    ASSERT_FALSE(resolver.resolveAllDependencies(mods));
    ASSERT_GT(resolver.getErrors().size(), 0u);
}

TEST_CASE(Resolver_DiamondDep_SharedDependency) {
    DependencyResolver resolver;
    auto main = makeModule("pkg-main", "main", "1.0.0", 1);
    auto depA = makeModule("pkg-a", "depA", "1.0.0", 1);
    auto depB = makeModule("pkg-b", "depB", "1.0.0", 1);
    auto common = makeModule("pkg-common", "common", "1.0.0", 1);

    main.requirements.push_back(makeReq("pkg-a", "depA", -1, "*"));
    main.requirements.push_back(makeReq("pkg-b", "depB", -1, "*"));
    depA.requirements.push_back(makeReq("pkg-common", "common", -1, "*"));
    depB.requirements.push_back(makeReq("pkg-common", "common", -1, "*"));

    std::vector<ModuleMetadata> mods = {main, depA, depB, common};
    ASSERT_TRUE(resolver.resolveAllDependencies(mods));
    auto resolved = resolver.getResolvedModules();
    ASSERT_EQ(resolved.size(), 4u);
}

TEST_CASE(Resolver_TransitiveDep_Chain) {
    DependencyResolver resolver;
    auto modA = makeModule("pkgA", "modA", "1.0.0", 1);
    auto modB = makeModule("pkgB", "modB", "1.0.0", 1);
    auto modC = makeModule("pkgC", "modC", "1.0.0", 1);
    auto modD = makeModule("pkgD", "modD", "1.0.0", 1);

    modA.requirements.push_back(makeReq("pkgB", "modB", -1, "*"));
    modB.requirements.push_back(makeReq("pkgC", "modC", -1, "*"));
    modC.requirements.push_back(makeReq("pkgD", "modD", -1, "*"));

    std::vector<ModuleMetadata> mods = {modA, modB, modC, modD};
    ASSERT_TRUE(resolver.resolveAllDependencies(mods));
    auto resolved = resolver.getResolvedModules();
    ASSERT_EQ(resolved.size(), 4u);
}

TEST_CASE(Resolver_MissingTransitiveDep) {
    DependencyResolver resolver;
    auto modA = makeModule("pkgA", "modA", "1.0.0", 1);
    auto modB = makeModule("pkgB", "modB", "1.0.0", 1);
    modA.requirements.push_back(makeReq("pkgB", "modB", -1, "*"));
    modB.requirements.push_back(makeReq("pkgC", "modX", -1, "*"));

    std::vector<ModuleMetadata> mods = {modA, modB};
    ASSERT_FALSE(resolver.resolveAllDependencies(mods));
}

TEST_CASE(Resolver_VersionConflict_NoCompatibleVersion) {
    DependencyResolver resolver;
    auto main = makeModule("pkg-main", "main", "1.0.0", 1);
    auto dep1 = makeModule("pkg-dep", "dep", "1.0.0", 1);

    main.requirements.push_back(makeReq("pkg-dep", "dep", -1, ">=2.0.0"));

    std::vector<ModuleMetadata> mods = {main, dep1};
    ASSERT_FALSE(resolver.resolveAllDependencies(mods));
}

TEST_CASE(Resolver_LevelCompatibility_ExactMatch) {
    LevelCompatibilityChecker::LevelConfig cfg(2, 1, 3);
    auto result = LevelCompatibilityChecker::checkCorePlugin(2, cfg);
    ASSERT_TRUE(result.isCompatible);
}

TEST_CASE(Resolver_LevelCompatibility_AtMinimum) {
    LevelCompatibilityChecker::LevelConfig cfg(2, 1, 3);
    auto result = LevelCompatibilityChecker::checkCorePlugin(1, cfg);
    ASSERT_TRUE(result.isCompatible);
}

TEST_CASE(Resolver_LevelCompatibility_AtMaximum) {
    LevelCompatibilityChecker::LevelConfig cfg(2, 1, 3);
    auto result = LevelCompatibilityChecker::checkCorePlugin(3, cfg);
    ASSERT_TRUE(result.isCompatible);
}

TEST_CASE(Resolver_LevelCompatibility_BelowMinimum) {
    LevelCompatibilityChecker::LevelConfig cfg(2, 2, 5);
    auto result = LevelCompatibilityChecker::checkCorePlugin(1, cfg);
    ASSERT_FALSE(result.isCompatible);
}

TEST_CASE(Resolver_LevelCompatibility_AboveMaximum) {
    LevelCompatibilityChecker::LevelConfig cfg(2, 1, 3);
    auto result = LevelCompatibilityChecker::checkCorePlugin(5, cfg);
    ASSERT_FALSE(result.isCompatible);
}

TEST_CASE(Resolver_SelectBest_DedupByLevel) {
    DependencyResolver resolver;
    auto modA_L1 = makeModule("pkg", "modA", "1.0.0", 1);
    auto modA_L2 = makeModule("pkg", "modA", "2.0.0", 2);

    std::vector<ModuleMetadata> mods = {modA_L1, modA_L2};
    ASSERT_TRUE(resolver.resolveAllDependencies(mods));
    auto resolved = resolver.getResolvedModules();
    ASSERT_EQ(resolved.size(), 2u);
}

TEST_CASE(Resolver_VersionResolver_PackageDistinctVersion) {
    auto modA = makeModule("pkg", "modA", "1.0.0", 1);
    auto modB_v1 = makeModule("pkg", "modB", "1.0.0", 1);
    auto modB_v2 = makeModule("pkg", "modB", "3.0.0", 1);
    auto req = makeReq("pkg", "modB", -1, ">=2.0.0");
    auto result = VersionResolver::resolveDependency({modA, modB_v1, modB_v2}, req, modA);
    ASSERT_TRUE(result.success);
    ASSERT_STREQ(result.resolvedVersion.c_str(), "3.0.0");
}

TEST_CASE(Resolver_EmptyModuleList) {
    DependencyResolver resolver;
    std::vector<ModuleMetadata> mods;
    ASSERT_TRUE(resolver.resolveAllDependencies(mods));
    auto resolved = resolver.getResolvedModules();
    ASSERT_EQ(resolved.size(), 0u);
}

TEST_CASE(Resolver_ModuleWithNoDependencies) {
    DependencyResolver resolver;
    auto mod = makeModule("pkg", "standalone", "1.0.0", 1);

    std::vector<ModuleMetadata> mods = {mod};
    ASSERT_TRUE(resolver.resolveAllDependencies(mods));
    auto resolved = resolver.getResolvedModules();
    ASSERT_EQ(resolved.size(), 1u);
    ASSERT_STREQ(resolved[0].moduleId.c_str(), "standalone");
}

TEST_CASE(Resolver_LevelChecker_CompleteIncompatibilityAll) {
    LevelCompatibilityChecker::LevelConfig cfg;
    cfg.currentLevel = 2;
    cfg.minimumLevel = 2;
    cfg.maximumLevel = 2;

    ASSERT_TRUE(LevelCompatibilityChecker::checkCorePlugin(2, cfg).isCompatible);
    ASSERT_FALSE(LevelCompatibilityChecker::checkCorePlugin(1, cfg).isCompatible);
    ASSERT_FALSE(LevelCompatibilityChecker::checkCorePlugin(3, cfg).isCompatible);
}