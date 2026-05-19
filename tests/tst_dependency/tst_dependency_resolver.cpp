#include "catch.hpp"

#include <LangCore/Module/Dependency/DependencyResolver.h>
#include <LangCore/Module/Dependency/DependencyGraph.h>

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
// Comprehensive resolver tests (15 tests)
// ============================================================================

TEST_CASE("Resolver NoDepsResolve") {
    DependencyResolver resolver;
    std::vector<ModuleMetadata> modules;
    modules.push_back(makeModule("pkg-a", "mod-a", "1.0.0", 1));
    REQUIRE(resolver.resolveAllDependencies(modules));
    auto resolved = resolver.getResolvedModules();
    REQUIRE(resolved.size() == 1u);
}

TEST_CASE("Resolver SingleDependencyResolve") {
    DependencyResolver resolver;
    std::vector<ModuleMetadata> modules;
    auto m1 = makeModule("pkg-a", "mod-a", "1.0.0", 1);
    auto m2 = makeModule("pkg-b", "mod-b", "1.0.0", 1);
    m1.requirements.push_back(makeReq("pkg-b", "mod-b"));
    modules.push_back(m1);
    modules.push_back(m2);
    REQUIRE(resolver.resolveAllDependencies(modules));
    auto resolved = resolver.getResolvedModules();
    REQUIRE(resolved.size() == 2u);
}

TEST_CASE("Resolver MissingDependencyFail") {
    DependencyResolver resolver;
    std::vector<ModuleMetadata> modules;
    auto m1 = makeModule("pkg-a", "mod-a", "1.0.0", 1);
    m1.requirements.push_back(makeReq("pkg-missing", "mod-missing"));
    modules.push_back(m1);
    REQUIRE_FALSE(resolver.resolveAllDependencies(modules));
    REQUIRE(resolver.getErrors().size() > 0u);
}

TEST_CASE("Resolver SelfDependencyEvict") {
    DependencyResolver resolver;
    std::vector<ModuleMetadata> modules;
    auto m1 = makeModule("pkg-a", "mod-a", "1.0.0", 1);
    m1.requirements.push_back(makeReq("pkg-a", "mod-a", 1));
    modules.push_back(m1);
    REQUIRE_FALSE(resolver.resolveAllDependencies(modules));
}

TEST_CASE("Resolver ChainDependencyResolve") {
    DependencyResolver resolver;
    std::vector<ModuleMetadata> modules;
    auto m1 = makeModule("pkg-a", "mod-a", "1.0.0", 1);
    auto m2 = makeModule("pkg-b", "mod-b", "1.0.0", 1);
    auto m3 = makeModule("pkg-c", "mod-c", "1.0.0", 1);
    m1.requirements.push_back(makeReq("pkg-b", "mod-b"));
    m2.requirements.push_back(makeReq("pkg-c", "mod-c"));
    modules.push_back(m1);
    modules.push_back(m2);
    modules.push_back(m3);
    REQUIRE(resolver.resolveAllDependencies(modules));
    auto resolved = resolver.getResolvedModules();
    REQUIRE(resolved.size() == 3u);
}

TEST_CASE("Resolver VersionRangeMatching") {
    DependencyResolver resolver;
    std::vector<ModuleMetadata> modules;
    auto m1 = makeModule("pkg-a", "mod-a", "1.0.0", 1);
    auto m2 = makeModule("pkg-b", "mod-b", "2.0.0", 1);
    m1.requirements.push_back(makeReq("pkg-b", "mod-b", -1, ">=1.5.0"));
    modules.push_back(m1);
    modules.push_back(m2);
    REQUIRE(resolver.resolveAllDependencies(modules));
    auto resolved = resolver.getResolvedModules();
    REQUIRE(resolved.size() == 2u);
}

TEST_CASE("Resolver VersionRangeFail") {
    DependencyResolver resolver;
    std::vector<ModuleMetadata> modules;
    auto m1 = makeModule("pkg-a", "mod-a", "1.0.0", 1);
    auto m2 = makeModule("pkg-b", "mod-b", "1.0.0", 1);
    m1.requirements.push_back(makeReq("pkg-b", "mod-b", -1, ">=2.0.0"));
    modules.push_back(m1);
    modules.push_back(m2);
    REQUIRE_FALSE(resolver.resolveAllDependencies(modules));
}

TEST_CASE("Resolver MultipleVersionSelectBest") {
    DependencyResolver resolver;
    std::vector<ModuleMetadata> modules;
    auto m1 = makeModule("pkg-a", "mod-a", "1.0.0", 1);
    auto m2_v1 = makeModule("pkg-b", "mod-b", "1.0.0", 1);
    auto m2_v3 = makeModule("pkg-b", "mod-b", "3.0.0", 1);
    auto m2_v2 = makeModule("pkg-b", "mod-b", "2.0.0", 1);
    m1.requirements.push_back(makeReq("pkg-b", "mod-b"));
    modules.push_back(m1);
    modules.push_back(m2_v1);
    modules.push_back(m2_v3);
    modules.push_back(m2_v2);
    REQUIRE(resolver.resolveAllDependencies(modules));
    auto resolved = resolver.getResolvedModules();
    REQUIRE(resolved.size() == 2u);
    // Best version (highest) should be selected
    for (const auto &m : resolved) {
        if (m.moduleId == "mod-b") {
            REQUIRE(m.version == "3.0.0");
        }
    }
}

TEST_CASE("Resolver LevelFilteredDependency") {
    DependencyResolver resolver;
    std::vector<ModuleMetadata> modules;
    auto m1 = makeModule("pkg-a", "mod-a", "1.0.0", 1);
    auto m2 = makeModule("pkg-b", "mod-b", "2.0.0", 2);
    m1.requirements.push_back(makeReq("pkg-b", "mod-b", 2));
    modules.push_back(m1);
    modules.push_back(m2);
    REQUIRE(resolver.resolveAllDependencies(modules));
    auto resolved = resolver.getResolvedModules();
    REQUIRE(resolved.size() == 2u);
}

TEST_CASE("Resolver LevelFilteredFail") {
    DependencyResolver resolver;
    std::vector<ModuleMetadata> modules;
    auto m1 = makeModule("pkg-a", "mod-a", "1.0.0", 1);
    auto m2 = makeModule("pkg-b", "mod-b", "1.0.0", 1);
    m1.requirements.push_back(makeReq("pkg-b", "mod-b", 2)); // requires level 2, only level 1 available
    modules.push_back(m1);
    modules.push_back(m2);
    REQUIRE_FALSE(resolver.resolveAllDependencies(modules));
}

TEST_CASE("Resolver CrossPackageDependency") {
    DependencyResolver resolver;
    std::vector<ModuleMetadata> modules;
    auto m1 = makeModule("pkg-a", "mod-a", "1.0.0", 1);
    auto m2 = makeModule("pkg-b", "mod-b", "1.0.0", 1);
    auto m3 = makeModule("pkg-c", "mod-c", "1.0.0", 1);
    m1.requirements.push_back(makeReq("pkg-b", "mod-b"));
    m1.requirements.push_back(makeReq("pkg-c", "mod-c"));
    modules.push_back(m1);
    modules.push_back(m2);
    modules.push_back(m3);
    REQUIRE(resolver.resolveAllDependencies(modules));
    auto resolved = resolver.getResolvedModules();
    REQUIRE(resolved.size() == 3u);
}

TEST_CASE("Resolver DiamondDependencyPattern") {
    DependencyResolver resolver;
    std::vector<ModuleMetadata> modules;
    auto base = makeModule("pkg-base", "base", "1.0.0", 1);
    auto midA = makeModule("pkg-mid-a", "mid-a", "1.0.0", 1);
    auto midB = makeModule("pkg-mid-b", "mid-b", "1.0.0", 1);
    auto top = makeModule("pkg-top", "top", "1.0.0", 1);
    midA.requirements.push_back(makeReq("pkg-base", "base"));
    midB.requirements.push_back(makeReq("pkg-base", "base"));
    top.requirements.push_back(makeReq("pkg-mid-a", "mid-a"));
    top.requirements.push_back(makeReq("pkg-mid-b", "mid-b"));
    modules.push_back(base);
    modules.push_back(midA);
    modules.push_back(midB);
    modules.push_back(top);
    REQUIRE(resolver.resolveAllDependencies(modules));
    auto resolved = resolver.getResolvedModules();
    REQUIRE(resolved.size() == 4u);
}

TEST_CASE("Resolver GraphWithDependencies") {
    DependencyResolver resolver;
    std::vector<ModuleMetadata> modules;
    auto m1 = makeModule("pkg-a", "mod-a", "1.0.0", 1);
    auto m2 = makeModule("pkg-b", "mod-b", "1.0.0", 1);
    m1.requirements.push_back(makeReq("pkg-b", "mod-b"));
    modules.push_back(m1);
    modules.push_back(m2);
    REQUIRE(resolver.resolveAllDependencies(modules));
    auto resolved = resolver.getResolvedModules();
    REQUIRE(resolved.size() == 2u);
    // Verify graph construction works after resolution
    DependencyGraph graph;
    graph.addModule(resolved[0]);
    graph.addModule(resolved[1]);
    REQUIRE(graph.buildGraph());
}

TEST_CASE("Resolver EdgeCaseEmptyModuleList") {
    DependencyResolver resolver;
    std::vector<ModuleMetadata> modules;
    REQUIRE(resolver.resolveAllDependencies(modules));
    auto resolved = resolver.getResolvedModules();
    REQUIRE(resolved.size() == 0u);
}

TEST_CASE("Resolver EdgeCaseModuleWithoutRequirements") {
    DependencyResolver resolver;
    std::vector<ModuleMetadata> modules;
    modules.push_back(makeModule("pkg-only", "mod-only", "1.0.0", 1));
    REQUIRE(resolver.resolveAllDependencies(modules));
    auto resolved = resolver.getResolvedModules();
    REQUIRE(resolved.size() == 1u);
    REQUIRE(resolved[0].packageId == "pkg-only");
}