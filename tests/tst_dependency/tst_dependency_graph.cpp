#include "catch.hpp"

#include <LangCore/Module/Dependency/DependencyGraph.h>
#include <LangCore/Module/Dependency/LevelCompatibilityChecker.h>

#include <string>
#include <vector>

using namespace LangCore;

static ModuleMetadata makeModule(const std::string &pkgId, const std::string &modId, const std::string &version,
                                 int level) {
    ModuleMetadata m;
    m.packageId = pkgId;
    m.moduleId = modId;
    m.version = version;
    m.level = level;
    return m;
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
// Dependency graph architecture tests (18 tests)
// ============================================================================

TEST_CASE("GraphModule AddSingleModule") {
    DependencyGraph graph;
    graph.addModule(makeModule("pkg-a", "mod-a", "1.0.0", 1));
    REQUIRE(graph.buildGraph());
    auto all = graph.getAllModules();
    REQUIRE(all.size() == 1u);
}

TEST_CASE("GraphModule AddMultipleModules") {
    DependencyGraph graph;
    graph.addModule(makeModule("pkg-a", "mod-a", "1.0.0", 1));
    graph.addModule(makeModule("pkg-b", "mod-b", "1.0.0", 1));
    REQUIRE(graph.buildGraph());
    auto all = graph.getAllModules();
    REQUIRE(all.size() == 2u);
}

TEST_CASE("GraphModule SingleModuleInitOrder") {
    DependencyGraph graph;
    graph.addModule(makeModule("pkg-a", "mod-a", "1.0.0", 1));
    REQUIRE(graph.buildGraph());
    auto plans = graph.getPackageInitializationOrder();
    REQUIRE(plans.size() == 1u);
    REQUIRE(plans[0].packageId == "pkg-a");
    REQUIRE(plans[0].initializationOrder.size() == 1u);
}

TEST_CASE("GraphModule NoCycleCheckSimple") {
    DependencyGraph graph;
    graph.addModule(makeModule("pkg-a", "mod-a", "1.0.0", 1));
    graph.addModule(makeModule("pkg-b", "mod-b", "1.0.0", 1));
    REQUIRE(graph.buildGraph());
    auto cycles = graph.findCycles();
    REQUIRE(cycles.size() == 0u);
}

TEST_CASE("GraphModule CycleDetection") {
    DependencyGraph graph;
    auto m1 = makeModule("pkg-a", "mod-a", "1.0.0", 1);
    auto m2 = makeModule("pkg-b", "mod-b", "1.0.0", 1);
    m1.resolvedDependencies.push_back(makeResDep("pkg-b", "mod-b", "1.0.0", 1));
    m2.resolvedDependencies.push_back(makeResDep("pkg-a", "mod-a", "1.0.0", 1));
    graph.addModule(m1);
    graph.addModule(m2);
    REQUIRE(graph.buildGraph());
    auto cycles = graph.findCycles();
    REQUIRE(cycles.size() > 0u);
}

TEST_CASE("GraphModule InitOrder LinearChain") {
    DependencyGraph graph;
    auto m1 = makeModule("pkg-a", "mod-a", "1.0.0", 1);
    auto m2 = makeModule("pkg-b", "mod-b", "1.0.0", 1);
    m2.resolvedDependencies.push_back(makeResDep("pkg-a", "mod-a", "1.0.0", 1));
    graph.addModule(m1);
    graph.addModule(m2);
    REQUIRE(graph.buildGraph());
    auto plans = graph.getPackageInitializationOrder();
    REQUIRE(plans.size() == 2u);
    // m1 (pkg-a) must come before m2 (pkg-b)
    REQUIRE(plans[0].packageId == "pkg-a");
    REQUIRE(plans[1].packageId == "pkg-b");
}

TEST_CASE("GraphModule InitOrder DiamondDepTopoSort") {
    DependencyGraph graph;
    auto base = makeModule("pkg-base", "base", "1.0.0", 1);
    auto midA = makeModule("pkg-mid-a", "mid-a", "1.0.0", 1);
    auto midB = makeModule("pkg-mid-b", "mid-b", "1.0.0", 1);
    auto top = makeModule("pkg-top", "top", "1.0.0", 1);
    midA.resolvedDependencies.push_back(makeResDep("pkg-base", "base", "1.0.0", 1));
    midB.resolvedDependencies.push_back(makeResDep("pkg-base", "base", "1.0.0", 1));
    top.resolvedDependencies.push_back(makeResDep("pkg-mid-a", "mid-a", "1.0.0", 1));
    top.resolvedDependencies.push_back(makeResDep("pkg-mid-b", "mid-b", "1.0.0", 1));
    graph.addModule(base);
    graph.addModule(midA);
    graph.addModule(midB);
    graph.addModule(top);
    REQUIRE(graph.buildGraph());
    auto plans = graph.getPackageInitializationOrder();
    REQUIRE(plans.size() == 4u);
    REQUIRE(plans[0].packageId == "pkg-base");
    // mid-a and mid-b can be in any order but must come after base
    REQUIRE(plans[3].packageId == "pkg-top");
}

TEST_CASE("GraphModule ClearAndReuseModule") {
    DependencyGraph graph;
    graph.addModule(makeModule("pkg-a", "mod-a", "1.0.0", 1));
    REQUIRE(graph.buildGraph());
    auto all1 = graph.getAllModules();
    REQUIRE(all1.size() == 1u);
    graph.clear();
    graph.addModule(makeModule("pkg-b", "mod-b", "1.0.0", 1));
    REQUIRE(graph.buildGraph());
    auto all2 = graph.getAllModules();
    REQUIRE(all2.size() == 1u);
    REQUIRE(all2[0].packageId == "pkg-b");
}

TEST_CASE("GraphModule ModuleKeyUniqueness") {
    DependencyGraph graph;
    graph.addModule(makeModule("pkg-a", "mod-a", "1.0.0", 1));
    graph.addModule(makeModule("pkg-a", "mod-a", "1.0.0", 1));
    REQUIRE(graph.buildGraph());
    auto all = graph.getAllModules();
    REQUIRE(all.size() == 1u);
}

TEST_CASE("GraphModule MultiLevelModule") {
    DependencyGraph graph;
    graph.addModule(makeModule("pkg", "mod", "1.0.0", 1));
    graph.addModule(makeModule("pkg", "mod", "2.0.0", 2));
    REQUIRE(graph.buildGraph());
    auto all = graph.getAllModules();
    REQUIRE(all.size() == 2u);
}

TEST_CASE("GraphModule MultiLevelInitOrder") {
    DependencyGraph graph;
    auto l1 = makeModule("pkg", "mod", "1.0.0", 1);
    auto l2 = makeModule("pkg", "mod", "2.0.0", 2);
    l2.resolvedDependencies.push_back(makeResDep("pkg", "mod", "1.0.0", 1));
    graph.addModule(l1);
    graph.addModule(l2);
    REQUIRE(graph.buildGraph());
    auto plans = graph.getPackageInitializationOrder();
    REQUIRE(plans.size() == 1u);
    REQUIRE(plans[0].initializationOrder.size() == 2u);
    REQUIRE(plans[0].initializationOrder[0].level == 1);
    REQUIRE(plans[0].initializationOrder[1].level == 2);
}

TEST_CASE("GraphModule CycleInMultiLevel") {
    DependencyGraph graph;
    auto l1 = makeModule("pkg", "mod", "1.0.0", 1);
    auto l2 = makeModule("pkg", "mod", "2.0.0", 2);
    l1.resolvedDependencies.push_back(makeResDep("pkg", "mod", "2.0.0", 2));
    l2.resolvedDependencies.push_back(makeResDep("pkg", "mod", "1.0.0", 1));
    graph.addModule(l1);
    graph.addModule(l2);
    REQUIRE(graph.buildGraph());
    auto cycles = graph.findCycles();
    REQUIRE(cycles.size() > 0u);
}

TEST_CASE("GraphModule BatchAddInitOrder") {
    DependencyGraph graph;
    graph.addModule(makeModule("pkg-a", "mod-a", "1.0.0", 1));
    graph.addModule(makeModule("pkg-b", "mod-b", "1.0.0", 1));
    graph.addModule(makeModule("pkg-c", "mod-c", "1.0.0", 1));
    REQUIRE(graph.buildGraph());
    auto all = graph.getAllModules();
    REQUIRE(all.size() == 3u);
    auto plans = graph.getPackageInitializationOrder();
    REQUIRE(plans.size() == 3u);
}

TEST_CASE("GraphModule GraphAfterClear") {
    DependencyGraph graph;
    graph.addModule(makeModule("pkg-a", "mod-a", "1.0.0", 1));
    REQUIRE(graph.buildGraph());
    graph.clear();
    graph.addModule(makeModule("pkg-b", "mod-b", "1.0.0", 1));
    REQUIRE(graph.buildGraph());
    auto all = graph.getAllModules();
    REQUIRE(all.size() == 1u);
    REQUIRE(all[0].packageId == "pkg-b");
}

TEST_CASE("GraphModule EdgeCaseEmptyGraph") {
    DependencyGraph graph;
    REQUIRE(graph.buildGraph());
    auto plans = graph.getPackageInitializationOrder();
    REQUIRE(plans.size() == 0u);
}

TEST_CASE("GraphModule SingleModuleNoDeps") {
    DependencyGraph graph;
    graph.addModule(makeModule("pkg-solo", "mod-solo", "1.0.0", 1));
    REQUIRE(graph.buildGraph());
    auto plans = graph.getPackageInitializationOrder();
    REQUIRE(plans.size() == 1u);
    REQUIRE(plans[0].packageId == "pkg-solo");
}

TEST_CASE("GraphModule LevelCompatibilityCheck Result") {
    LevelCompatibilityChecker::LevelConfig cfg(2, 1, 3);
    auto result = LevelCompatibilityChecker::checkCorePlugin(2, cfg);
    REQUIRE(result.isCompatible);

    auto failResult = LevelCompatibilityChecker::checkCorePlugin(10, cfg);
    REQUIRE_FALSE(failResult.isCompatible);
}

TEST_CASE("GraphModule EdgeCasesSelfDependencyResolved") {
    DependencyGraph graph;
    auto m = makeModule("pkg", "self", "1.0.0", 1);
    m.resolvedDependencies.push_back(makeResDep("pkg", "self", "1.0.0", 1));
    graph.addModule(m);
    REQUIRE(graph.buildGraph());
    auto all = graph.getAllModules();
    REQUIRE(all.size() == 1u);
}