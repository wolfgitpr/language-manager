#include "catch.hpp"

#include <LangCore/Base/LangCommon.h>
#include <LangCore/Module/Dependency/DependencyGraph.h>
#include <LangCore/Module/Dependency/DependencyResolver.h>
#include <LangCore/Support/ContextUtils.h>

using namespace LangCore;

// ============================================================================
// ContextKey tests
// ============================================================================

TEST_CASE("contextKey_default") {
    ContextKey key;
    REQUIRE(key.isDefault());
    REQUIRE_FALSE(key.isVersioned());
    REQUIRE(key.toString() == "(default)");
}

TEST_CASE("contextKey_unversioned") {
    ContextKey key("SingerA");
    REQUIRE_FALSE(key.isDefault());
    REQUIRE_FALSE(key.isVersioned());
    REQUIRE(key.toString() == "SingerA");
}

TEST_CASE("contextKey_versioned") {
    ContextKey key("SingerA", stdc::VersionNumber(2, 0, 0));
    REQUIRE_FALSE(key.isDefault());
    REQUIRE(key.isVersioned());
    // VersionNumber(2,0,0).toString() = "2.0" (trailing zeros stripped)
    REQUIRE(key.toString() == "SingerA@2.0");
}

TEST_CASE("contextKey_ordering") {
    ContextKey a("SingerA", stdc::VersionNumber(1, 0, 0));
    ContextKey b("SingerA", stdc::VersionNumber(2, 0, 0));
    ContextKey c("SingerB");
    REQUIRE(a < b);
    REQUIRE(b < c);
    REQUIRE_FALSE(a == b);
}

TEST_CASE("contextKey_equality") {
    ContextKey a("SingerA", stdc::VersionNumber(1, 0, 0));
    ContextKey b("SingerA", stdc::VersionNumber(1, 0, 0));
    ContextKey c("SingerA");
    REQUIRE(a == b);
    REQUIRE(a != c);
}

// ============================================================================
// FQID with version
// ============================================================================

TEST_CASE("fqid_format_versioned") {
    ContextKey key("SingerA", stdc::VersionNumber(2, 0, 0));
    auto fqid = ContextUtils::formatFqid(key, "g2p-cmn-custom");
    // Should contain context@version:moduleId
    REQUIRE(fqid.find("SingerA@") != std::string::npos);
    REQUIRE(fqid.find(":g2p-cmn-custom") != std::string::npos);
}

TEST_CASE("fqid_format_unversioned") {
    ContextKey key("SingerA");
    auto fqid = ContextUtils::formatFqid(key, "g2p-cmn-custom");
    REQUIRE(fqid == "SingerA:g2p-cmn-custom");
}

TEST_CASE("fqid_format_default") {
    ContextKey key;
    auto fqid = ContextUtils::formatFqid(key, "g2p-cmn");
    REQUIRE(fqid == "g2p-cmn");
}

TEST_CASE("fqid_parse_versioned") {
    auto result = ContextUtils::parseFqid("SingerA@2.0.0:g2p-cmn");
    REQUIRE(result.context == "SingerA");
    REQUIRE_FALSE(result.version.isEmpty());
    REQUIRE(result.version.major() == 2);
    REQUIRE(result.moduleId == "g2p-cmn");
}

TEST_CASE("fqid_parse_unversioned") {
    auto result = ContextUtils::parseFqid("SingerA:g2p-cmn");
    REQUIRE(result.context == "SingerA");
    REQUIRE(result.version.isEmpty());
    REQUIRE(result.moduleId == "g2p-cmn");
}

TEST_CASE("fqid_roundtrip_versioned") {
    ContextKey key("SingerA", stdc::VersionNumber(1, 2, 3));
    auto fqid = ContextUtils::formatFqid(key, "g2p-cmn");
    auto parsed = ContextUtils::parseFqid(fqid);
    REQUIRE(parsed.context == "SingerA");
    REQUIRE(parsed.version.major() == 1);
    REQUIRE(parsed.version.minor() == 2);
    REQUIRE(parsed.version.patch() == 3);
    REQUIRE(parsed.moduleId == "g2p-cmn");
}

// ============================================================================
// G2pInput / G2pRes with contextVersion
// ============================================================================

TEST_CASE("g2pInput_withVersion") {
    G2pInput input("你好", "g2p-cmn", "SingerA", stdc::VersionNumber(1, 0, 0));
    REQUIRE(input.context == "SingerA");
    REQUIRE_FALSE(input.contextVersion.isEmpty());
    REQUIRE(input.contextVersion.major() == 1);
}

TEST_CASE("g2pInput_withoutVersion_backward_compat") {
    G2pInput input("hello", "g2p-eng", "SingerA");
    REQUIRE(input.context == "SingerA");
    REQUIRE(input.contextVersion.isEmpty());
}

TEST_CASE("g2pInput_default_backward_compat") {
    G2pInput input("hello", "g2p-eng");
    REQUIRE(input.context == "");
    REQUIRE(input.contextVersion.isEmpty());
}

TEST_CASE("g2pRes_withVersion") {
    G2pRes res("hello", "eng", "SingerA", stdc::VersionNumber(2, 0, 0), "hh ah l ow");
    REQUIRE(res.context == "SingerA");
    REQUIRE(res.contextVersion.major() == 2);
    REQUIRE(res.pronunciation == "hh ah l ow");
}

TEST_CASE("g2pRes_legacy_constructor") {
    G2pRes res("hello", "eng", "SingerA", "hh ah l ow");
    REQUIRE(res.context == "SingerA");
    REQUIRE(res.contextVersion.isEmpty());
    REQUIRE(res.pronunciation == "hh ah l ow");
}

// ============================================================================
// ModuleMetadata with contextVersion
// ============================================================================

static ModuleMetadata makeVersionedModule(const std::string &context,
                                           const stdc::VersionNumber &contextVer,
                                           const std::string &pkgId, const std::string &modId,
                                           const std::string &version = "1.0.0", int level = 1,
                                           const std::string &type = "g2p",
                                           const std::string &iid = "g2p.mock.MockG2p") {
    ModuleMetadata m;
    m.context = context;
    m.contextVersion = contextVer;
    m.packageId = pkgId;
    m.moduleId = modId;
    m.version = version;
    m.level = level;
    m.type = type;
    m.iid = iid;
    return m;
}

// Same context name but different contextVersion → NOT same main module.
TEST_CASE("dedup_diffContextVersion") {
    auto a = makeVersionedModule("SingerA", stdc::VersionNumber(1, 0, 0), "pkgA", "g2p-cmn");
    auto b = makeVersionedModule("SingerA", stdc::VersionNumber(2, 0, 0), "pkgA", "g2p-cmn");
    REQUIRE_FALSE(a.isSameMainModule(b));
}

// Same contextVersion → same main module (version ignored in isSameMainModule).
TEST_CASE("dedup_sameContextVersion") {
    auto a = makeVersionedModule("SingerA", stdc::VersionNumber(1, 0, 0), "pkgA", "g2p-cmn", "1.0.0");
    auto b = makeVersionedModule("SingerA", stdc::VersionNumber(1, 0, 0), "pkgB", "g2p-cmn", "2.0.0");
    REQUIRE(a.isSameMainModule(b));
}

// contextVersion included in key().
TEST_CASE("key_includesContextVersion") {
    auto a = makeVersionedModule("SingerA", stdc::VersionNumber(1, 0, 0), "pkgA", "g2p-cmn");
    auto b = makeVersionedModule("SingerA", stdc::VersionNumber(2, 0, 0), "pkgA", "g2p-cmn");
    REQUIRE(a.key() != b.key());
}

// No contextVersion → key same as before (backward compat).
TEST_CASE("key_noContextVersion") {
    auto a = makeVersionedModule("SingerA", {}, "pkgA", "g2p-cmn");
    REQUIRE(a.key().find("@") == std::string::npos);
}

// operator== includes contextVersion.
TEST_CASE("equality_diffContextVersion") {
    auto a = makeVersionedModule("SingerA", stdc::VersionNumber(1, 0, 0), "pkgA", "g2p-cmn");
    auto b = makeVersionedModule("SingerA", stdc::VersionNumber(2, 0, 0), "pkgA", "g2p-cmn");
    REQUIRE_FALSE(a == b);
}

// ============================================================================
// Dependency resolver: versioned contexts are independent
// ============================================================================

TEST_CASE("isolate_diffContextVersion") {
    // SingerA v1.0 and SingerA v2.0 each have same moduleId
    auto modV1 = makeVersionedModule("SingerA", stdc::VersionNumber(1, 0, 0),
                                      "singerA-v1", "g2p-cmn-custom");
    auto modV2 = makeVersionedModule("SingerA", stdc::VersionNumber(2, 0, 0),
                                      "singerA-v2", "g2p-cmn-custom");

    // Resolve each independently
    std::vector<ModuleMetadata> modulesV1 = {modV1};
    DependencyResolver resolverV1;
    REQUIRE(resolverV1.resolveAllDependencies(modulesV1));
    REQUIRE(resolverV1.getResolvedModules().size() == 1u);

    std::vector<ModuleMetadata> modulesV2 = {modV2};
    DependencyResolver resolverV2;
    REQUIRE(resolverV2.resolveAllDependencies(modulesV2));
    REQUIRE(resolverV2.getResolvedModules().size() == 1u);

    // Both resolved but with different contextVersions
    REQUIRE(resolverV1.getResolvedModules()[0].contextVersion.major() == 1);
    REQUIRE(resolverV2.getResolvedModules()[0].contextVersion.major() == 2);
}

// Versioned context can fall back to default context for dependencies
TEST_CASE("fallback_versionedToDefault") {
    auto defaultMod = makeVersionedModule("", {}, "cmn-official", "g2p-cmn-official");

    auto singerMod = makeVersionedModule("SingerA", stdc::VersionNumber(1, 0, 0),
                                          "singerA-v1", "g2p-cmn-enhanced");
    DependencyRequirement dep;
    dep.packageId = "cmn-official";
    dep.moduleId = "g2p-cmn-official";
    dep.level = 1;
    dep.versionRange = "*";
    singerMod.requirements.push_back(dep);

    std::vector<ModuleMetadata> modules = {singerMod};
    std::vector<ModuleMetadata> fallback = {defaultMod};

    DependencyResolver resolver;
    REQUIRE(resolver.resolveAllDependencies(modules, fallback));
    REQUIRE(resolver.getResolvedModules().size() == 1u);
}

// Two versioned contexts with same moduleId resolved independently via selectBest
TEST_CASE("dedup_selectBest_versionedContext") {
    auto v1_old = makeVersionedModule("SingerA", stdc::VersionNumber(1, 0, 0),
                                       "pkgA", "g2p-cmn", "1.0.0");
    auto v1_new = makeVersionedModule("SingerA", stdc::VersionNumber(1, 0, 0),
                                       "pkgA", "g2p-cmn", "2.0.0");

    std::vector<ModuleMetadata> modules = {v1_old, v1_new};
    DependencyResolver resolver;
    REQUIRE(resolver.resolveAllDependencies(modules));

    const auto &resolved = resolver.getResolvedModules();
    REQUIRE(resolved.size() == 1u);
    REQUIRE(resolved[0].version == "2.0.0");
    REQUIRE(resolved[0].contextVersion.major() == 1);
}
