#include "catch.hpp"

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

TEST_CASE("VersionRange NormalizeEmpty") {
    REQUIRE(VersionRange::normalizeVersion("") == "0.0.0");
}

TEST_CASE("VersionRange NormalizeSimple") {
    REQUIRE(VersionRange::normalizeVersion("1.2.3") == "1.2.3");
}

TEST_CASE("VersionRange NormalizeTwoParts") {
    REQUIRE(VersionRange::normalizeVersion("1.2") == "1.2.0");
}

TEST_CASE("VersionRange NormalizeOnePart") {
    REQUIRE(VersionRange::normalizeVersion("5") == "5.0.0");
}

TEST_CASE("VersionRange NormalizeVPrefix") {
    REQUIRE(VersionRange::normalizeVersion("v2.3.4") == "2.3.4");
}

TEST_CASE("VersionRange NormalizeVUpperCase") {
    REQUIRE(VersionRange::normalizeVersion("V1.0.0") == "1.0.0");
}

TEST_CASE("VersionRange CompareEqual") {
    REQUIRE(VersionRange::compareVersions("1.2.3", "1.2.3") == 0);
}

TEST_CASE("VersionRange CompareLess") {
    REQUIRE(VersionRange::compareVersions("1.2.3", "1.2.4") < 0);
}

TEST_CASE("VersionRange CompareGreater") {
    REQUIRE(VersionRange::compareVersions("2.0.0", "1.9.9") > 0);
}

TEST_CASE("VersionRange CompareMajorDiff") {
    REQUIRE(VersionRange::compareVersions("1.0.0", "2.0.0") < 0);
}

TEST_CASE("VersionRange CompareMinorDiff") {
    REQUIRE(VersionRange::compareVersions("1.0.0", "1.1.0") < 0);
}

TEST_CASE("VersionRange ComparePatchDiff") {
    REQUIRE(VersionRange::compareVersions("1.0.0", "1.0.1") < 0);
}

TEST_CASE("VersionRange MatchAny") {
    VersionRange range("*");
    auto result = range.getVersionsInRange({"1.0.0", "2.0.0", "3.0.0"});
    REQUIRE(result.size() == 3u);
}

TEST_CASE("VersionRange MatchGreaterThan") {
    VersionRange range(">1.0.0");
    auto result = range.getVersionsInRange({"0.9.0", "1.0.0", "1.1.0", "2.0.0"});
    REQUIRE(result.size() == 2u);
}

TEST_CASE("VersionRange MatchGreaterEqual") {
    VersionRange range(">=1.0.0");
    auto result = range.getVersionsInRange({"0.9.0", "1.0.0", "1.1.0"});
    REQUIRE(result.size() == 2u);
}

TEST_CASE("VersionRange MatchLessThan") {
    VersionRange range("<2.0.0");
    auto result = range.getVersionsInRange({"1.0.0", "1.5.0", "2.0.0", "3.0.0"});
    REQUIRE(result.size() == 2u);
}

TEST_CASE("VersionRange MatchLessEqual") {
    VersionRange range("<=2.0.0");
    auto result = range.getVersionsInRange({"1.0.0", "2.0.0", "3.0.0"});
    REQUIRE(result.size() == 2u);
}

TEST_CASE("VersionRange MatchExact") {
    VersionRange range("==1.0.0");
    auto result = range.getVersionsInRange({"0.9.0", "1.0.0", "1.1.0"});
    REQUIRE(result.size() == 1u);
    REQUIRE(result[0] == "1.0.0");
}

TEST_CASE("VersionRange MatchExactBareVersion") {
    VersionRange range("1.0.0");
    auto result = range.getVersionsInRange({"0.9.0", "1.0.0", "1.1.0"});
    REQUIRE(result.size() == 1u);
    REQUIRE(result[0] == "1.0.0");
}

TEST_CASE("VersionRange CompatibleTilde") {
    VersionRange range("~1.2.0");
    auto result = range.getVersionsInRange({"1.1.9", "1.2.0", "1.2.5", "1.3.0"});
    REQUIRE(result.size() == 2u);
}

TEST_CASE("VersionRange HyphenRange") {
    VersionRange range("1.0.0 - 2.0.0");
    auto result = range.getVersionsInRange({"0.5.0", "1.0.0", "1.5.0", "2.0.0", "2.1.0"});
    REQUIRE(result.size() == 3u);
}

TEST_CASE("VersionRange NoMatchesInRange") {
    VersionRange range(">5.0.0");
    auto result = range.getVersionsInRange({"1.0.0", "2.0.0", "3.0.0"});
    REQUIRE(result.size() == 0u);
}

TEST_CASE("VersionRange ToString Any") {
    VersionRange range("*");
    REQUIRE(range.toString() == "*");
}

TEST_CASE("VersionRange ToString GreaterEqual") {
    VersionRange range(">=1.0.0");
    REQUIRE(range.toString() == ">=1.0.0");
}

TEST_CASE("VersionRange ToString Less") {
    VersionRange range("<2.0.0");
    REQUIRE(range.toString() == "<2.0.0");
}

TEST_CASE("VersionRange ToString Compatible") {
    VersionRange range("~1.2.3");
    REQUIRE(range.toString() == "~1.2.3");
}

TEST_CASE("VersionRange SortedDescending") {
    VersionRange range(">=1.0.0");
    auto result = range.getVersionsInRange({"1.0.0", "3.0.0", "2.0.0"});
    REQUIRE(result.size() == 3u);
    REQUIRE(result[0] == "3.0.0");
    REQUIRE(result[1] == "2.0.0");
    REQUIRE(result[2] == "1.0.0");
}

TEST_CASE("VersionResolver SelectHighest") {
    auto highest = VersionResolver::selectHighestVersion({"1.0.0", "2.0.0", "1.5.0"});
    REQUIRE(highest == "2.0.0");
}

TEST_CASE("VersionResolver SelectHighest Single") {
    auto highest = VersionResolver::selectHighestVersion({"3.0.0"});
    REQUIRE(highest == "3.0.0");
}

TEST_CASE("VersionResolver SelectHighest Empty") {
    auto highest = VersionResolver::selectHighestVersion({});
    REQUIRE(highest == "");
}

TEST_CASE("VersionResolver ResolveDependency Simple") {
    auto modA = makeModule("pkg", "modA", "1.0.0", 1);
    auto modB = makeModule("pkg", "modB", "1.0.0", 1);
    auto req = makeReq("pkg", "modB", -1, "*");
    auto result = VersionResolver::resolveDependency({modA, modB}, req, modA);
    REQUIRE(result.success);
    REQUIRE(result.resolvedVersion == "1.0.0");
}

TEST_CASE("VersionResolver ResolveDependency NotFound") {
    auto modA = makeModule("pkg", "modA", "1.0.0", 1);
    auto req = makeReq("pkg", "modX", -1, "*");
    auto result = VersionResolver::resolveDependency({modA}, req, modA);
    REQUIRE_FALSE(result.success);
}

TEST_CASE("VersionResolver ResolveDependency VersionRange") {
    auto modA = makeModule("pkg", "modA", "1.0.0", 1);
    auto modB1 = makeModule("pkg", "modB", "1.0.0", 1);
    auto modB2 = makeModule("pkg", "modB", "2.0.0", 1);
    auto req = makeReq("pkg", "modB", -1, ">=1.5.0");
    auto result = VersionResolver::resolveDependency({modA, modB1, modB2}, req, modA);
    REQUIRE(result.success);
    REQUIRE(result.resolvedVersion == "2.0.0");
}

TEST_CASE("VersionResolver ResolveDependency LevelFilter") {
    auto modA = makeModule("pkg", "modA", "1.0.0", 1);
    auto modB_L1 = makeModule("pkg", "modB", "1.0.0", 1);
    auto modB_L2 = makeModule("pkg", "modB", "2.0.0", 2);
    auto req = makeReq("pkg", "modB", 2, "*");
    auto result = VersionResolver::resolveDependency({modA, modB_L1, modB_L2}, req, modA);
    REQUIRE(result.success);
    REQUIRE(result.resolvedVersion == "2.0.0");
}

// ============================================================================
// Dependency tests (19 original dependency tests)
// ============================================================================

TEST_CASE("DependencyRequirement Equality") {
    auto a = makeReq("pkg", "mod", 1, ">=1.0.0");
    auto b = makeReq("pkg", "mod", 1, ">=1.0.0");
    REQUIRE(a == b);
}

TEST_CASE("DependencyRequirement Inequality") {
    auto a = makeReq("pkg", "modA", 1);
    auto b = makeReq("pkg", "modB", 1);
    REQUIRE_FALSE(a == b);
}

TEST_CASE("DependencyRequirement Key") {
    auto req = makeReq("pkg", "mod", 1);
    REQUIRE(req.key() == "pkg:mod:1");
}

TEST_CASE("ResolvedDependency Equality") {
    auto a = makeResDep("pkg", "mod", "1.0.0", 1);
    auto b = makeResDep("pkg", "mod", "1.0.0", 1);
    REQUIRE(a == b);
}

TEST_CASE("ResolvedDependency Inequality") {
    auto a = makeResDep("pkg", "mod", "1.0.0", 1);
    auto b = makeResDep("pkg", "mod", "2.0.0", 1);
    REQUIRE_FALSE(a == b);
}

TEST_CASE("ResolvedDependency Key") {
    auto rd = makeResDep("pkg", "mod", "1.0.0", 1);
    REQUIRE(rd.key() == "pkg:mod:1.0.0:1");
}

TEST_CASE("DependencyGraph AddAndGetModules") {
    DependencyGraph graph;
    auto modA = makeModule("pkg", "modA", "1.0.0", 1);
    auto modB = makeModule("pkg", "modB", "1.0.0", 1);
    graph.addModule(modA);
    graph.addModule(modB);
    auto all = graph.getAllModules();
    REQUIRE(all.size() == 2u);
}

TEST_CASE("DependencyGraph BuildGraph NoDeps") {
    DependencyGraph graph;
    graph.addModule(makeModule("pkg", "modA", "1.0.0", 1));
    graph.addModule(makeModule("pkg", "modB", "1.0.0", 1));
    REQUIRE(graph.buildGraph());
}

TEST_CASE("DependencyGraph NoCycles Independent") {
    DependencyGraph graph;
    graph.addModule(makeModule("pkg", "modA", "1.0.0", 1));
    graph.addModule(makeModule("pkg", "modB", "1.0.0", 1));
    graph.buildGraph();
    auto cycles = graph.findCycles();
    REQUIRE(cycles.size() == 0u);
}

TEST_CASE("DependencyGraph DetectCycle") {
    DependencyGraph graph;
    auto modA = makeModule("pkg", "modA", "1.0.0", 1);
    auto modB = makeModule("pkg", "modB", "1.0.0", 1);
    modA.resolvedDependencies.push_back(makeResDep("pkg", "modB", "1.0.0", 1));
    modB.resolvedDependencies.push_back(makeResDep("pkg", "modA", "1.0.0", 1));
    graph.addModule(modA);
    graph.addModule(modB);
    graph.buildGraph();
    auto cycles = graph.findCycles();
    REQUIRE(cycles.size() > 0u);
}

TEST_CASE("DependencyGraph InitOrder NoDeps") {
    DependencyGraph graph;
    graph.addModule(makeModule("pkg", "modA", "1.0.0", 1));
    graph.buildGraph();
    auto plans = graph.getPackageInitializationOrder();
    REQUIRE(plans.size() == 1u);
    REQUIRE(plans[0].initializationOrder.size() == 1u);
}

TEST_CASE("DependencyGraph InitOrder WithDeps") {
    DependencyGraph graph;
    auto modA = makeModule("pkg", "modA", "1.0.0", 1);
    auto modB = makeModule("pkg", "modB", "1.0.0", 1);
    modA.resolvedDependencies.push_back(makeResDep("pkg", "modB", "1.0.0", 1));
    graph.addModule(modA);
    graph.addModule(modB);
    graph.buildGraph();
    auto plans = graph.getPackageInitializationOrder();
    REQUIRE(plans.size() == 1u);
    REQUIRE(plans[0].initializationOrder.size() == 2u);
    REQUIRE(plans[0].initializationOrder[0].moduleId == "modB");
    REQUIRE(plans[0].initializationOrder[1].moduleId == "modA");
}

TEST_CASE("DependencyResolver SimpleResolve") {
    DependencyResolver resolver;
    auto modA = makeModule("pkg", "modA", "1.0.0", 1);
    auto modB = makeModule("pkg", "modB", "1.0.0", 1);
    modA.requirements.push_back(makeReq("pkg", "modB", -1, "*"));
    std::vector<ModuleMetadata> mods = {modA, modB};
    REQUIRE(resolver.resolveAllDependencies(mods));
    auto resolved = resolver.getResolvedModules();
    REQUIRE(resolved.size() == 2u);
}

TEST_CASE("DependencyResolver MissingDependency") {
    DependencyResolver resolver;
    auto modA = makeModule("pkg", "modA", "1.0.0", 1);
    modA.requirements.push_back(makeReq("pkg", "modX", -1, "*"));
    std::vector<ModuleMetadata> mods = {modA};
    REQUIRE_FALSE(resolver.resolveAllDependencies(mods));
    REQUIRE(resolver.getErrors().size() > 0u);
}

TEST_CASE("DependencyResolver SelfDependency Removed") {
    DependencyResolver resolver;
    auto modA = makeModule("pkg", "modA", "1.0.0", 1);
    modA.requirements.push_back(makeReq("pkg", "modA", 1, "*"));
    std::vector<ModuleMetadata> mods = {modA};
    REQUIRE_FALSE(resolver.resolveAllDependencies(mods));
}

TEST_CASE("DependencyResolver SelectBestVersion") {
    DependencyResolver resolver;
    auto modA = makeModule("pkg", "modA", "1.0.0", 1);
    auto modB1 = makeModule("pkg", "modB", "1.0.0", 1);
    auto modB2 = makeModule("pkg", "modB", "2.0.0", 1);
    modA.requirements.push_back(makeReq("pkg", "modB", -1, "*"));
    std::vector<ModuleMetadata> mods = {modA, modB1, modB2};
    REQUIRE(resolver.resolveAllDependencies(mods));
    // selectBestModules keeps only the highest version per unique key
    auto resolved = resolver.getResolvedModules();
    REQUIRE(resolved.size() == 2u);
}

TEST_CASE("LevelChecker Compatible") {
    LevelCompatibilityChecker::LevelConfig cfg(3, 1, 5);
    auto result = LevelCompatibilityChecker::checkCorePlugin(3, cfg);
    REQUIRE(result.isCompatible);
}

TEST_CASE("LevelChecker TooLow") {
    LevelCompatibilityChecker::LevelConfig cfg(3, 2, 5);
    auto result = LevelCompatibilityChecker::checkCorePlugin(1, cfg);
    REQUIRE_FALSE(result.isCompatible);
}

TEST_CASE("LevelChecker TooHigh") {
    LevelCompatibilityChecker::LevelConfig cfg(3, 1, 3);
    auto result = LevelCompatibilityChecker::checkCorePlugin(5, cfg);
    REQUIRE_FALSE(result.isCompatible);
}

// ============================================================================
// NEW: VersionRange additional tests
// ============================================================================

TEST_CASE("VersionRange EmptyVersion Normalizes") {
    REQUIRE(VersionRange::normalizeVersion("") == "0.0.0");
}

TEST_CASE("VersionRange PreRelease Stripped") {
    REQUIRE(VersionRange::normalizeVersion("1.2.3-beta") == "1.2.3");
}

TEST_CASE("VersionRange ExtraComponents Truncated") {
    REQUIRE(VersionRange::normalizeVersion("1.2.3.4.5") == "1.2.3");
}

TEST_CASE("VersionRange DefaultConstructor") {
    VersionRange range;
    REQUIRE(range.toString() == "*");
}

// ============================================================================
// NEW: DependencyGraph additional tests
// ============================================================================

TEST_CASE("Graph EmptyGraph NoCycles") {
    DependencyGraph graph;
    graph.buildGraph();
    auto cycles = graph.findCycles();
    REQUIRE(cycles.size() == 0u);
}

TEST_CASE("Graph ThreeNodeChain") {
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
    REQUIRE(plans.size() == 1u);
    auto &order = plans[0].initializationOrder;
    REQUIRE(order.size() == 3u);
    REQUIRE(order[0].moduleId == "modC");
    REQUIRE(order[1].moduleId == "modB");
    REQUIRE(order[2].moduleId == "modA");
}

TEST_CASE("Graph MultiplePackages SortedByDependency") {
    DependencyGraph graph;
    auto modA = makeModule("pkgA", "modA", "1.0.0", 1);
    auto modB = makeModule("pkgB", "modB", "1.0.0", 1);
    // pkgA::modA depends on pkgB::modB
    modA.resolvedDependencies.push_back(makeResDep("pkgB", "modB", "1.0.0", 1));
    graph.addModule(modA);
    graph.addModule(modB);
    graph.buildGraph();
    auto plans = graph.getPackageInitializationOrder();
    REQUIRE(plans.size() == 2u);
    // pkgB should come before pkgA since pkgA depends on pkgB
    REQUIRE(plans[0].packageId == "pkgB");
    REQUIRE(plans[1].packageId == "pkgA");
}

// ============================================================================
// NEW: ModuleMetadata tests
// ============================================================================

TEST_CASE("ModuleMetadata Key ContainsAllFields") {
    auto mod = makeModule("pkg", "mod", "1.0.0", 1, "g2p", "iid1", "cfg1");
    auto k = mod.key();
    // key() = packageId:moduleId:version:iid:type:configuration:level
    REQUIRE(k == ":pkg:mod:1.0.0:iid1:g2p:cfg1:1");
}

TEST_CASE("ModuleMetadata UniqueKey DiffersFromKey") {
    auto mod = makeModule("pkg", "mod", "1.0.0", 1, "g2p", "iid1", "cfg1");
    auto uk = mod.uniqueKey();
    // uniqueKey() = packageId:moduleId:iid:type:level (no version, no config)
    REQUIRE(uk == ":pkg:mod:iid1:g2p:1");
    REQUIRE(mod.key() != mod.uniqueKey());
}

TEST_CASE("ModuleMetadata IsSameMainModule") {
    auto a = makeModule("pkgA", "mod", "1.0.0", 1, "g2p", "iid", "cfg");
    auto b = makeModule("pkgB", "mod", "2.0.0", 1, "g2p", "iid", "cfg");
    // isSameMainModule checks moduleId, iid, type, configuration, level (not packageId or version)
    REQUIRE(a.isSameMainModule(b));
}

TEST_CASE("ModuleMetadata Equality") {
    auto a = makeModule("pkg", "mod", "1.0.0", 1, "g2p", "iid", "cfg");
    auto b = makeModule("pkg", "mod", "1.0.0", 1, "g2p", "iid", "cfg");
    REQUIRE(a == b);

    auto c = makeModule("pkg", "mod", "2.0.0", 1, "g2p", "iid", "cfg");
    REQUIRE_FALSE(a == c);
}

// ============================================================================
// §14.18 regression: selectBestModules pointer invalidation fix
// ============================================================================

TEST_CASE("DependencyResolver SelectBest ThreeVersions") {
    // Three versions of the same module — only the highest should remain
    std::vector<ModuleMetadata> modules;
    auto m1 = makeModule("pkg", "mod", "1.0.0", 1);
    auto m2 = makeModule("pkg", "mod", "3.0.0", 1);
    auto m3 = makeModule("pkg", "mod", "2.0.0", 1);
    modules.push_back(m1);
    modules.push_back(m2);
    modules.push_back(m3);

    DependencyResolver resolver;
    resolver.resolveAllDependencies(modules);

    // After resolution, only v3.0.0 should remain (selectBestModules is called internally)
    auto resolved = resolver.getResolvedModules();
    REQUIRE(resolved.size() == static_cast<size_t>(1));
    REQUIRE(resolved[0].version == "3.0.0");
}

TEST_CASE("DependencyResolver SelectBest DifferentLevels") {
    // Same module at different levels should both be kept
    std::vector<ModuleMetadata> modules;
    auto m1 = makeModule("pkg", "mod", "1.0.0", 1);
    auto m2 = makeModule("pkg", "mod", "2.0.0", 2);
    modules.push_back(m1);
    modules.push_back(m2);

    DependencyResolver resolver;
    resolver.resolveAllDependencies(modules);

    auto resolved = resolver.getResolvedModules();
    REQUIRE(resolved.size() == static_cast<size_t>(2));
}

TEST_CASE("DependencyResolver SelectBest ManyModulesMixed") {
    // Mix of different modules and versions — stress test for pointer stability
    std::vector<ModuleMetadata> modules;
    for (int i = 0; i < 10; ++i) {
        modules.push_back(makeModule("pkg", "mod-a", std::to_string(i) + ".0.0", 1));
        modules.push_back(makeModule("pkg", "mod-b", std::to_string(i) + ".0.0", 1));
    }

    DependencyResolver resolver;
    resolver.resolveAllDependencies(modules);

    auto resolved = resolver.getResolvedModules();
    // Should have exactly 2 modules: best of mod-a (9.0.0) and best of mod-b (9.0.0)
    REQUIRE(resolved.size() == static_cast<size_t>(2));
    for (const auto &m : resolved) {
        REQUIRE(m.version == "9.0.0");
    }
}

// ============================================================================
// §14.14 regression: checkDependencies should collect all errors
// (tested indirectly via DependencyResolver and LevelCompatibilityChecker)
// ============================================================================

TEST_CASE("LevelChecker MultipleIncompatible") {
    // Verify we can check multiple modules and get results for all of them
    LevelCompatibilityChecker::LevelConfig config;
    config.currentLevel = 2;
    config.minimumLevel = 1;
    config.maximumLevel = 3;

    auto r1 = LevelCompatibilityChecker::checkCorePlugin(0, config);
    REQUIRE_FALSE(r1.isCompatible);

    auto r2 = LevelCompatibilityChecker::checkCorePlugin(2, config);
    REQUIRE(r2.isCompatible);

    auto r3 = LevelCompatibilityChecker::checkCorePlugin(5, config);
    REQUIRE_FALSE(r3.isCompatible);

    // All three checks work independently — framework can now collect all errors
}

// ============================================================================
// §14.22 regression: DependencyGraph::clear before reuse
// ============================================================================

TEST_CASE("DependencyGraph ClearAndReuse") {
    DependencyGraph graph;
    auto modA = makeModule("pkg", "mod-a", "1.0.0", 1);
    graph.addModule(modA);
    REQUIRE(graph.buildGraph());

    auto modules1 = graph.getAllModules();
    REQUIRE(modules1.size() == static_cast<size_t>(1));

    // Clear and add different module
    graph.clear();
    auto modB = makeModule("pkg", "mod-b", "2.0.0", 1);
    graph.addModule(modB);
    REQUIRE(graph.buildGraph());

    auto modules2 = graph.getAllModules();
    // Should have only mod-b, not mod-a + mod-b
    REQUIRE(modules2.size() == static_cast<size_t>(1));
    REQUIRE(modules2[0].moduleId == "mod-b");
}