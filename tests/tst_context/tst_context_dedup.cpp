#include "catch.hpp"

#include <vector>

#include <LangCore/Module/Dependency/DependencyGraph.h>
#include <LangCore/Module/Dependency/DependencyResolver.h>

static LangCore::ModuleMetadata makeModule(const std::string &context, const std::string &pkgId,
                                            const std::string &modId, const std::string &version = "1.0.0",
                                            int level = 1, const std::string &type = "g2p",
                                            const std::string &iid = "g2p.mock.MockG2p") {
    LangCore::ModuleMetadata m;
    m.context = context;
    m.packageId = pkgId;
    m.moduleId = modId;
    m.version = version;
    m.level = level;
    m.type = type;
    m.iid = iid;
    return m;
}

// Same (context, moduleId, iid, type, config, level) from two different packages → isSameMainModule.
TEST_CASE("dedup_sameQuad") {
    auto a = makeModule("SingerA", "pkgA", "g2p-cmn");
    auto b = makeModule("SingerA", "pkgB", "g2p-cmn");
    REQUIRE(a.isSameMainModule(b));
}

// Same main module fields but different version → isSameMainModule ignores version → still same.
TEST_CASE("dedup_diffVersion") {
    auto a = makeModule("SingerA", "pkgA", "g2p-cmn", "1.0.0");
    auto b = makeModule("SingerA", "pkgA", "g2p-cmn", "2.0.0");
    REQUIRE(a.isSameMainModule(b));
}

// Same (context, moduleId, version, level) but different iid → NOT same main module.
TEST_CASE("dedup_diffIid") {
    auto a = makeModule("SingerA", "pkgA", "g2p-cmn", "1.0.0", 1, "g2p", "g2p.impl.A");
    auto b = makeModule("SingerA", "pkgA", "g2p-cmn", "1.0.0", 1, "g2p", "g2p.impl.B");
    REQUIRE_FALSE(a.isSameMainModule(b));
}

// Same (context, moduleId, version, iid) but different level → NOT same main module.
TEST_CASE("dedup_diffLevel") {
    auto a = makeModule("SingerA", "pkgA", "g2p-cmn", "1.0.0", 1);
    auto b = makeModule("SingerA", "pkgA", "g2p-cmn", "1.0.0", 2);
    REQUIRE_FALSE(a.isSameMainModule(b));
}

// Different contexts with identical (moduleId, iid, level, version) → NOT same main module.
TEST_CASE("dedup_crossContext_noDedup") {
    auto a = makeModule("SingerA", "pkgA", "g2p-cmn");
    auto b = makeModule("SingerB", "pkgA", "g2p-cmn");
    REQUIRE_FALSE(a.isSameMainModule(b));
}

// Default context ("") dedup works the same way.
TEST_CASE("dedup_defaultContext") {
    auto a = makeModule("", "pkgA", "g2p-cmn");
    auto b = makeModule("", "pkgB", "g2p-cmn");
    REQUIRE(a.isSameMainModule(b));
}

// Verify that different type breaks dedup even if everything else matches.
TEST_CASE("dedup_diffType") {
    auto a = makeModule("SingerA", "pkgA", "mod-x", "1.0.0", 1, "g2p", "impl.A");
    auto b = makeModule("SingerA", "pkgA", "mod-x", "1.0.0", 1, "dict", "impl.A");
    REQUIRE_FALSE(a.isSameMainModule(b));
}

// Verify operator== includes packageId and version (stricter than isSameMainModule).
TEST_CASE("dedup_operatorEq_includesPackageAndVersion") {
    auto a = makeModule("SingerA", "pkgA", "g2p-cmn", "1.0.0");
    auto b = makeModule("SingerA", "pkgA", "g2p-cmn", "1.0.0");
    REQUIRE(a == b);

    auto c = makeModule("SingerA", "pkgA", "g2p-cmn", "2.0.0");
    REQUIRE_FALSE(a == c);

    auto d = makeModule("SingerA", "pkgB", "g2p-cmn", "1.0.0");
    REQUIRE_FALSE(a == d);
}

// selectBestModules is tested indirectly via resolveAllDependencies:
// Two modules with same (packageId, moduleId, level) but different versions —
// the resolver's selectBestModules keeps only the highest version.
TEST_CASE("dedup_selectBest_viaResolver") {
    auto v1 = makeModule("SingerA", "pkgA", "g2p-cmn", "1.0.0");
    auto v2 = makeModule("SingerA", "pkgA", "g2p-cmn", "2.0.0");

    std::vector<LangCore::ModuleMetadata> modules = {v1, v2};
    LangCore::DependencyResolver resolver;
    REQUIRE(resolver.resolveAllDependencies(modules));

    // Only the best version should survive
    const auto &resolved = resolver.getResolvedModules();
    REQUIRE(resolved.size() == 1u);
    REQUIRE(resolved[0].version == "2.0.0");
}
