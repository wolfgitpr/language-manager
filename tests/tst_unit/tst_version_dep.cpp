#include "tst_framework.h"

#include <LangCore/Module/Dependency/DependencyGraph.h>
#include <LangCore/Module/Dependency/DependencyResolver.h>
#include <LangCore/Module/Dependency/LevelCompatibilityChecker.h>
#include <LangCore/Module/Dependency/VersionUtils.h>

using namespace LangCore;

// ============================================================================
// Helpers
// ============================================================================

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

static ResolvedDependency makeResDep(const std::string &pkgId, const std::string &modId, const std::string &version,
                                     int level) {
    ResolvedDependency rd;
    rd.packageId = pkgId;
    rd.moduleId = modId;
    rd.version = version;
    rd.level = level;
    return rd;
}

// ============================================================================
// VersionRange — normalizeVersion (33 original version tests)
// ============================================================================

TEST_CASE(VersionRange_NormalizeEmpty) {
    ASSERT_STREQ(VersionRange::normalizeVersion("").c_str(), "0.0.0");
}

TEST_CASE(VersionRange_NormalizeSimple) {
    ASSERT_STREQ(VersionRange::normalizeVersion("1.2.3").c_str(), "1.2.3");
}

TEST_CASE(VersionRange_NormalizeTwoParts) {
    ASSERT_STREQ(VersionRange::normalizeVersion("1.2").c_str(), "1.2.0");
}

TEST_CASE(VersionRange_NormalizeOnePart) {
    ASSERT_STREQ(VersionRange::normalizeVersion("5").c_str(), "5.0.0");
}

TEST_CASE(VersionRange_NormalizeVPrefix) {
    ASSERT_STREQ(VersionRange::normalizeVersion("v2.3.4").c_str(), "2.3.4");
}

TEST_CASE(VersionRange_NormalizeVUpperCase) {
    ASSERT_STREQ(VersionRange::normalizeVersion("V1.0.0").c_str(), "1.0.0");
}

TEST_CASE(VersionRange_CompareEqual) {
    ASSERT_EQ(VersionRange::compareVersions("1.2.3", "1.2.3"), 0);
}

TEST_CASE(VersionRange_CompareLess) {
    ASSERT_LT(VersionRange::compareVersions("1.2.3", "1.2.4"), 0);
}

TEST_CASE(VersionRange_CompareGreater) {
    ASSERT_GT(VersionRange::compareVersions("2.0.0", "1.9.9"), 0);
}

TEST_CASE(VersionRange_CompareMajorDiff) {
    ASSERT_LT(VersionRange::compareVersions("1.0.0", "2.0.0"), 0);
}

TEST_CASE(VersionRange_CompareMinorDiff) {
    ASSERT_LT(VersionRange::compareVersions("1.0.0", "1.1.0"), 0);
}

TEST_CASE(VersionRange_ComparePatchDiff) {
    ASSERT_LT(VersionRange::compareVersions("1.0.0", "1.0.1"), 0);
}

TEST_CASE(VersionRange_MatchAny) {
    VersionRange range("*");
    auto result = range.getVersionsInRange({"1.0.0", "2.0.0", "3.0.0"});
    ASSERT_EQ(result.size(), 3u);
}

TEST_CASE(VersionRange_MatchGreaterThan) {
    VersionRange range(">1.0.0");
    auto result = range.getVersionsInRange({"0.9.0", "1.0.0", "1.1.0", "2.0.0"});
    ASSERT_EQ(result.size(), 2u);
}

TEST_CASE(VersionRange_MatchGreaterEqual) {
    VersionRange range(">=1.0.0");
    auto result = range.getVersionsInRange({"0.9.0", "1.0.0", "1.1.0"});
    ASSERT_EQ(result.size(), 2u);
}

TEST_CASE(VersionRange_MatchLessThan) {
    VersionRange range("<2.0.0");
    auto result = range.getVersionsInRange({"1.0.0", "1.5.0", "2.0.0", "3.0.0"});
    ASSERT_EQ(result.size(), 2u);
}

TEST_CASE(VersionRange_MatchLessEqual) {
    VersionRange range("<=2.0.0");
    auto result = range.getVersionsInRange({"1.0.0", "2.0.0", "3.0.0"});
    ASSERT_EQ(result.size(), 2u);
}

TEST_CASE(VersionRange_MatchExact) {
    VersionRange range("==1.0.0");
    auto result = range.getVersionsInRange({"0.9.0", "1.0.0", "1.1.0"});
    ASSERT_EQ(result.size(), 1u);
    ASSERT_STREQ(result[0].c_str(), "1.0.0");
}

TEST_CASE(VersionRange_MatchExactBareVersion) {
    VersionRange range("1.0.0");
    auto result = range.getVersionsInRange({"0.9.0", "1.0.0", "1.1.0"});
    ASSERT_EQ(result.size(), 1u);
    ASSERT_STREQ(result[0].c_str(), "1.0.0");
}

TEST_CASE(VersionRange_CompatibleTilde) {
    VersionRange range("~1.2.0");
    auto result = range.getVersionsInRange({"1.1.9", "1.2.0", "1.2.5", "1.3.0"});
    ASSERT_EQ(result.size(), 2u);
}

TEST_CASE(VersionRange_HyphenRange) {
    VersionRange range("1.0.0 - 2.0.0");
    auto result = range.getVersionsInRange({"0.5.0", "1.0.0", "1.5.0", "2.0.0", "2.1.0"});
    ASSERT_EQ(result.size(), 3u);
}

TEST_CASE(VersionRange_NoMatchesInRange) {
    VersionRange range(">5.0.0");
    auto result = range.getVersionsInRange({"1.0.0", "2.0.0", "3.0.0"});
    ASSERT_EQ(result.size(), 0u);
}

TEST_CASE(VersionRange_ToString_Any) {
    VersionRange range("*");
    ASSERT_STREQ(range.toString().c_str(), "*");
}

TEST_CASE(VersionRange_ToString_GreaterEqual) {
    VersionRange range(">=1.0.0");
    ASSERT_STREQ(range.toString().c_str(), ">=1.0.0");
}

TEST_CASE(VersionRange_ToString_Less) {
    VersionRange range("<2.0.0");
    ASSERT_STREQ(range.toString().c_str(), "<2.0.0");
}

TEST_CASE(VersionRange_ToString_Compatible) {
    VersionRange range("~1.2.3");
    ASSERT_STREQ(range.toString().c_str(), "~1.2.3");
}

TEST_CASE(VersionRange_SortedDescending) {
    VersionRange range(">=1.0.0");
    auto result = range.getVersionsInRange({"1.0.0", "3.0.0", "2.0.0"});
    ASSERT_EQ(result.size(), 3u);
    ASSERT_STREQ(result[0].c_str(), "3.0.0");
    ASSERT_STREQ(result[1].c_str(), "2.0.0");
    ASSERT_STREQ(result[2].c_str(), "1.0.0");
}

TEST_CASE(VersionResolver_SelectHighest) {
    auto highest = VersionResolver::selectHighestVersion({"1.0.0", "2.0.0", "1.5.0"});
    ASSERT_STREQ(highest.c_str(), "2.0.0");
}

TEST_CASE(VersionResolver_SelectHighest_Single) {
    auto highest = VersionResolver::selectHighestVersion({"3.0.0"});
    ASSERT_STREQ(highest.c_str(), "3.0.0");
}

TEST_CASE(VersionResolver_SelectHighest_Empty) {
    auto highest = VersionResolver::selectHighestVersion({});
    ASSERT_STREQ(highest.c_str(), "");
}

TEST_CASE(VersionResolver_ResolveDependency_Simple) {
    auto modA = makeModule("pkg", "modA", "1.0.0", 1);
    auto modB = makeModule("pkg", "modB", "1.0.0", 1);
    auto req = makeReq("pkg", "modB", -1, "*");
    auto result = VersionResolver::resolveDependency({modA, modB}, req, modA);
    ASSERT_TRUE(result.success);
    ASSERT_STREQ(result.resolvedVersion.c_str(), "1.0.0");
}

TEST_CASE(VersionResolver_ResolveDependency_NotFound) {
    auto modA = makeModule("pkg", "modA", "1.0.0", 1);
    auto req = makeReq("pkg", "modX", -1, "*");
    auto result = VersionResolver::resolveDependency({modA}, req, modA);
    ASSERT_FALSE(result.success);
}

TEST_CASE(VersionResolver_ResolveDependency_VersionRange) {
    auto modA = makeModule("pkg", "modA", "1.0.0", 1);
    auto modB1 = makeModule("pkg", "modB", "1.0.0", 1);
    auto modB2 = makeModule("pkg", "modB", "2.0.0", 1);
    auto req = makeReq("pkg", "modB", -1, ">=1.5.0");
    auto result = VersionResolver::resolveDependency({modA, modB1, modB2}, req, modA);
    ASSERT_TRUE(result.success);
    ASSERT_STREQ(result.resolvedVersion.c_str(), "2.0.0");
}

TEST_CASE(VersionResolver_ResolveDependency_LevelFilter) {
    auto modA = makeModule("pkg", "modA", "1.0.0", 1);
    auto modB_L1 = makeModule("pkg", "modB", "1.0.0", 1);
    auto modB_L2 = makeModule("pkg", "modB", "2.0.0", 2);
    auto req = makeReq("pkg", "modB", 2, "*");
    auto result = VersionResolver::resolveDependency({modA, modB_L1, modB_L2}, req, modA);
    ASSERT_TRUE(result.success);
    ASSERT_STREQ(result.resolvedVersion.c_str(), "2.0.0");
}

// ============================================================================
// Dependency tests (19 original dependency tests)
// ============================================================================

TEST_CASE(DependencyRequirement_Equality) {
    auto a = makeReq("pkg", "mod", 1, ">=1.0.0");
    auto b = makeReq("pkg", "mod", 1, ">=1.0.0");
    ASSERT_TRUE(a == b);
}

TEST_CASE(DependencyRequirement_Inequality) {
    auto a = makeReq("pkg", "modA", 1);
    auto b = makeReq("pkg", "modB", 1);
    ASSERT_FALSE(a == b);
}

TEST_CASE(DependencyRequirement_Key) {
    auto req = makeReq("pkg", "mod", 1);
    ASSERT_STREQ(req.key().c_str(), "pkg:mod:1");
}

TEST_CASE(ResolvedDependency_Equality) {
    auto a = makeResDep("pkg", "mod", "1.0.0", 1);
    auto b = makeResDep("pkg", "mod", "1.0.0", 1);
    ASSERT_TRUE(a == b);
}

TEST_CASE(ResolvedDependency_Inequality) {
    auto a = makeResDep("pkg", "mod", "1.0.0", 1);
    auto b = makeResDep("pkg", "mod", "2.0.0", 1);
    ASSERT_FALSE(a == b);
}

TEST_CASE(ResolvedDependency_Key) {
    auto rd = makeResDep("pkg", "mod", "1.0.0", 1);
    ASSERT_STREQ(rd.key().c_str(), "pkg:mod:1.0.0:1");
}

TEST_CASE(DependencyGraph_AddAndGetModules) {
    DependencyGraph graph;
    auto modA = makeModule("pkg", "modA", "1.0.0", 1);
    auto modB = makeModule("pkg", "modB", "1.0.0", 1);
    graph.addModule(modA);
    graph.addModule(modB);
    auto all = graph.getAllModules();
    ASSERT_EQ(all.size(), 2u);
}

TEST_CASE(DependencyGraph_BuildGraph_NoDeps) {
    DependencyGraph graph;
    graph.addModule(makeModule("pkg", "modA", "1.0.0", 1));
    graph.addModule(makeModule("pkg", "modB", "1.0.0", 1));
    ASSERT_TRUE(graph.buildGraph());
}

TEST_CASE(DependencyGraph_NoCycles_Independent) {
    DependencyGraph graph;
    graph.addModule(makeModule("pkg", "modA", "1.0.0", 1));
    graph.addModule(makeModule("pkg", "modB", "1.0.0", 1));
    graph.buildGraph();
    auto cycles = graph.findCycles();
    ASSERT_EQ(cycles.size(), 0u);
}

TEST_CASE(DependencyGraph_DetectCycle) {
    DependencyGraph graph;
    auto modA = makeModule("pkg", "modA", "1.0.0", 1);
    auto modB = makeModule("pkg", "modB", "1.0.0", 1);
    modA.resolvedDependencies.push_back(makeResDep("pkg", "modB", "1.0.0", 1));
    modB.resolvedDependencies.push_back(makeResDep("pkg", "modA", "1.0.0", 1));
    graph.addModule(modA);
    graph.addModule(modB);
    graph.buildGraph();
    auto cycles = graph.findCycles();
    ASSERT_GT(cycles.size(), 0u);
}

TEST_CASE(DependencyGraph_InitOrder_NoDeps) {
    DependencyGraph graph;
    graph.addModule(makeModule("pkg", "modA", "1.0.0", 1));
    graph.buildGraph();
    auto plans = graph.getPackageInitializationOrder();
    ASSERT_EQ(plans.size(), 1u);
    ASSERT_EQ(plans[0].initializationOrder.size(), 1u);
}

TEST_CASE(DependencyGraph_InitOrder_WithDeps) {
    DependencyGraph graph;
    auto modA = makeModule("pkg", "modA", "1.0.0", 1);
    auto modB = makeModule("pkg", "modB", "1.0.0", 1);
    modA.resolvedDependencies.push_back(makeResDep("pkg", "modB", "1.0.0", 1));
    graph.addModule(modA);
    graph.addModule(modB);
    graph.buildGraph();
    auto plans = graph.getPackageInitializationOrder();
    ASSERT_EQ(plans.size(), 1u);
    ASSERT_EQ(plans[0].initializationOrder.size(), 2u);
    ASSERT_STREQ(plans[0].initializationOrder[0].moduleId.c_str(), "modB");
    ASSERT_STREQ(plans[0].initializationOrder[1].moduleId.c_str(), "modA");
}

TEST_CASE(DependencyResolver_SimpleResolve) {
    DependencyResolver resolver;
    auto modA = makeModule("pkg", "modA", "1.0.0", 1);
    auto modB = makeModule("pkg", "modB", "1.0.0", 1);
    modA.requirements.push_back(makeReq("pkg", "modB", -1, "*"));
    std::vector<ModuleMetadata> mods = {modA, modB};
    ASSERT_TRUE(resolver.resolveAllDependencies(mods));
    auto resolved = resolver.getResolvedModules();
    ASSERT_EQ(resolved.size(), 2u);
}

TEST_CASE(DependencyResolver_MissingDependency) {
    DependencyResolver resolver;
    auto modA = makeModule("pkg", "modA", "1.0.0", 1);
    modA.requirements.push_back(makeReq("pkg", "modX", -1, "*"));
    std::vector<ModuleMetadata> mods = {modA};
    ASSERT_FALSE(resolver.resolveAllDependencies(mods));
    ASSERT_GT(resolver.getErrors().size(), 0u);
}

TEST_CASE(DependencyResolver_SelfDependency_Removed) {
    DependencyResolver resolver;
    auto modA = makeModule("pkg", "modA", "1.0.0", 1);
    modA.requirements.push_back(makeReq("pkg", "modA", 1, "*"));
    std::vector<ModuleMetadata> mods = {modA};
    ASSERT_FALSE(resolver.resolveAllDependencies(mods));
}

TEST_CASE(DependencyResolver_SelectBestVersion) {
    DependencyResolver resolver;
    auto modA = makeModule("pkg", "modA", "1.0.0", 1);
    auto modB1 = makeModule("pkg", "modB", "1.0.0", 1);
    auto modB2 = makeModule("pkg", "modB", "2.0.0", 1);
    modA.requirements.push_back(makeReq("pkg", "modB", -1, "*"));
    std::vector<ModuleMetadata> mods = {modA, modB1, modB2};
    ASSERT_TRUE(resolver.resolveAllDependencies(mods));
    // selectBestModules keeps only the highest version per unique key
    auto resolved = resolver.getResolvedModules();
    ASSERT_EQ(resolved.size(), 2u);
}

TEST_CASE(LevelChecker_Compatible) {
    LevelCompatibilityChecker::LevelConfig cfg(3, 1, 5);
    auto result = LevelCompatibilityChecker::checkCorePlugin(3, cfg);
    ASSERT_TRUE(result.isCompatible);
}

TEST_CASE(LevelChecker_TooLow) {
    LevelCompatibilityChecker::LevelConfig cfg(3, 2, 5);
    auto result = LevelCompatibilityChecker::checkCorePlugin(1, cfg);
    ASSERT_FALSE(result.isCompatible);
}

TEST_CASE(LevelChecker_TooHigh) {
    LevelCompatibilityChecker::LevelConfig cfg(3, 1, 3);
    auto result = LevelCompatibilityChecker::checkCorePlugin(5, cfg);
    ASSERT_FALSE(result.isCompatible);
}

// ============================================================================
// NEW: VersionRange additional tests
// ============================================================================

TEST_CASE(VersionRange_EmptyVersion_Normalizes) {
    ASSERT_STREQ(VersionRange::normalizeVersion("").c_str(), "0.0.0");
}

TEST_CASE(VersionRange_PreRelease_Stripped) {
    ASSERT_STREQ(VersionRange::normalizeVersion("1.2.3-beta").c_str(), "1.2.3");
}

TEST_CASE(VersionRange_ExtraComponents_Truncated) {
    ASSERT_STREQ(VersionRange::normalizeVersion("1.2.3.4.5").c_str(), "1.2.3");
}

TEST_CASE(VersionRange_DefaultConstructor) {
    VersionRange range;
    ASSERT_STREQ(range.toString().c_str(), "*");
}

// ============================================================================
// NEW: DependencyGraph additional tests
// ============================================================================

TEST_CASE(Graph_EmptyGraph_NoCycles) {
    DependencyGraph graph;
    graph.buildGraph();
    auto cycles = graph.findCycles();
    ASSERT_EQ(cycles.size(), 0u);
}

TEST_CASE(Graph_ThreeNodeChain) {
    DependencyGraph graph;
    auto modA = makeModule("pkg", "modA", "1.0.0", 1);
    auto modB = makeModule("pkg", "modB", "1.0.0", 1);
    auto modC = makeModule("pkg", "modC", "1.0.0", 1);
    modA.resolvedDependencies.push_back(makeResDep("pkg", "modB", "1.0.0", 1));
    modB.resolvedDependencies.push_back(makeResDep("pkg", "modC", "1.0.0", 1));
    graph.addModule(modA);
    graph.addModule(modB);
    graph.addModule(modC);
    graph.buildGraph();
    auto plans = graph.getPackageInitializationOrder();
    ASSERT_EQ(plans.size(), 1u);
    auto &order = plans[0].initializationOrder;
    ASSERT_EQ(order.size(), 3u);
    ASSERT_STREQ(order[0].moduleId.c_str(), "modC");
    ASSERT_STREQ(order[1].moduleId.c_str(), "modB");
    ASSERT_STREQ(order[2].moduleId.c_str(), "modA");
}

TEST_CASE(Graph_MultiplePackages_SortedByDependency) {
    DependencyGraph graph;
    auto modA = makeModule("pkgA", "modA", "1.0.0", 1);
    auto modB = makeModule("pkgB", "modB", "1.0.0", 1);
    // pkgA::modA depends on pkgB::modB
    modA.resolvedDependencies.push_back(makeResDep("pkgB", "modB", "1.0.0", 1));
    graph.addModule(modA);
    graph.addModule(modB);
    graph.buildGraph();
    auto plans = graph.getPackageInitializationOrder();
    ASSERT_EQ(plans.size(), 2u);
    // pkgB should come before pkgA since pkgA depends on pkgB
    ASSERT_STREQ(plans[0].packageId.c_str(), "pkgB");
    ASSERT_STREQ(plans[1].packageId.c_str(), "pkgA");
}

// ============================================================================
// NEW: ModuleMetadata tests
// ============================================================================

TEST_CASE(ModuleMetadata_Key_ContainsAllFields) {
    auto mod = makeModule("pkg", "mod", "1.0.0", 1, "g2p", "iid1", "cfg1");
    auto k = mod.key();
    // key() = packageId:moduleId:version:iid:type:configuration:level
    ASSERT_STREQ(k.c_str(), "pkg:mod:1.0.0:iid1:g2p:cfg1:1");
}

TEST_CASE(ModuleMetadata_UniqueKey_DiffersFromKey) {
    auto mod = makeModule("pkg", "mod", "1.0.0", 1, "g2p", "iid1", "cfg1");
    auto uk = mod.uniqueKey();
    // uniqueKey() = packageId:moduleId:iid:type:level (no version, no config)
    ASSERT_STREQ(uk.c_str(), "pkg:mod:iid1:g2p:1");
    ASSERT_TRUE(mod.key() != mod.uniqueKey());
}

TEST_CASE(ModuleMetadata_IsSameMainModule) {
    auto a = makeModule("pkgA", "mod", "1.0.0", 1, "g2p", "iid", "cfg");
    auto b = makeModule("pkgB", "mod", "2.0.0", 1, "g2p", "iid", "cfg");
    // isSameMainModule checks moduleId, iid, type, configuration, level (not packageId or version)
    ASSERT_TRUE(a.isSameMainModule(b));
}

TEST_CASE(ModuleMetadata_Equality) {
    auto a = makeModule("pkg", "mod", "1.0.0", 1, "g2p", "iid", "cfg");
    auto b = makeModule("pkg", "mod", "1.0.0", 1, "g2p", "iid", "cfg");
    ASSERT_TRUE(a == b);

    auto c = makeModule("pkg", "mod", "2.0.0", 1, "g2p", "iid", "cfg");
    ASSERT_FALSE(a == c);
}

// ModuleMetadata_Hash test removed: MainModuleHash::operator() is not exported from DLL
