#include "tst_framework.h"

#include <LangCore/Base/LangCommon.h>
#include <LangCore/Module/Dependency/DependencyGraph.h>
#include <LangCore/Module/Dependency/DependencyResolver.h>
#include <LangCore/Support/ContextUtils.h>

using namespace LangCore;

// ============================================================================
// ContextKey tests
// ============================================================================

TEST_CASE(contextKey_default) {
    ContextKey key;
    ASSERT_TRUE(key.isDefault());
    ASSERT_FALSE(key.isVersioned());
    ASSERT_STREQ(key.toString().c_str(), "(default)");
}

TEST_CASE(contextKey_unversioned) {
    ContextKey key("SingerA");
    ASSERT_FALSE(key.isDefault());
    ASSERT_FALSE(key.isVersioned());
    ASSERT_STREQ(key.toString().c_str(), "SingerA");
}

TEST_CASE(contextKey_versioned) {
    ContextKey key("SingerA", stdc::VersionNumber(2, 0, 0));
    ASSERT_FALSE(key.isDefault());
    ASSERT_TRUE(key.isVersioned());
    // VersionNumber(2,0,0).toString() = "2.0" (trailing zeros stripped)
    ASSERT_STREQ(key.toString().c_str(), "SingerA@2.0");
}

TEST_CASE(contextKey_ordering) {
    ContextKey a("SingerA", stdc::VersionNumber(1, 0, 0));
    ContextKey b("SingerA", stdc::VersionNumber(2, 0, 0));
    ContextKey c("SingerB");
    ASSERT_TRUE(a < b);
    ASSERT_TRUE(b < c);
    ASSERT_FALSE(a == b);
}

TEST_CASE(contextKey_equality) {
    ContextKey a("SingerA", stdc::VersionNumber(1, 0, 0));
    ContextKey b("SingerA", stdc::VersionNumber(1, 0, 0));
    ContextKey c("SingerA");
    ASSERT_TRUE(a == b);
    ASSERT_TRUE(a != c);
}

// ============================================================================
// FQID with version
// ============================================================================

TEST_CASE(fqid_format_versioned) {
    ContextKey key("SingerA", stdc::VersionNumber(2, 0, 0));
    auto fqid = ContextUtils::formatFqid(key, "g2p-cmn-custom");
    // Should contain context@version:moduleId
    ASSERT_TRUE(fqid.find("SingerA@") != std::string::npos);
    ASSERT_TRUE(fqid.find(":g2p-cmn-custom") != std::string::npos);
}

TEST_CASE(fqid_format_unversioned) {
    ContextKey key("SingerA");
    auto fqid = ContextUtils::formatFqid(key, "g2p-cmn-custom");
    ASSERT_STREQ(fqid.c_str(), "SingerA:g2p-cmn-custom");
}

TEST_CASE(fqid_format_default) {
    ContextKey key;
    auto fqid = ContextUtils::formatFqid(key, "g2p-cmn");
    ASSERT_STREQ(fqid.c_str(), "g2p-cmn");
}

TEST_CASE(fqid_parse_versioned) {
    auto result = ContextUtils::parseFqid("SingerA@2.0.0:g2p-cmn");
    ASSERT_STREQ(result.context.c_str(), "SingerA");
    ASSERT_FALSE(result.version.isEmpty());
    ASSERT_EQ(result.version.major(), 2);
    ASSERT_STREQ(result.moduleId.c_str(), "g2p-cmn");
}

TEST_CASE(fqid_parse_unversioned) {
    auto result = ContextUtils::parseFqid("SingerA:g2p-cmn");
    ASSERT_STREQ(result.context.c_str(), "SingerA");
    ASSERT_TRUE(result.version.isEmpty());
    ASSERT_STREQ(result.moduleId.c_str(), "g2p-cmn");
}

TEST_CASE(fqid_roundtrip_versioned) {
    ContextKey key("SingerA", stdc::VersionNumber(1, 2, 3));
    auto fqid = ContextUtils::formatFqid(key, "g2p-cmn");
    auto parsed = ContextUtils::parseFqid(fqid);
    ASSERT_STREQ(parsed.context.c_str(), "SingerA");
    ASSERT_EQ(parsed.version.major(), 1);
    ASSERT_EQ(parsed.version.minor(), 2);
    ASSERT_EQ(parsed.version.patch(), 3);
    ASSERT_STREQ(parsed.moduleId.c_str(), "g2p-cmn");
}

// ============================================================================
// G2pInput / G2pRes with contextVersion
// ============================================================================

TEST_CASE(g2pInput_withVersion) {
    G2pInput input("你好", "g2p-cmn", "SingerA", stdc::VersionNumber(1, 0, 0));
    ASSERT_STREQ(input.context.c_str(), "SingerA");
    ASSERT_FALSE(input.contextVersion.isEmpty());
    ASSERT_EQ(input.contextVersion.major(), 1);
}

TEST_CASE(g2pInput_withoutVersion_backward_compat) {
    G2pInput input("hello", "g2p-eng", "SingerA");
    ASSERT_STREQ(input.context.c_str(), "SingerA");
    ASSERT_TRUE(input.contextVersion.isEmpty());
}

TEST_CASE(g2pInput_default_backward_compat) {
    G2pInput input("hello", "g2p-eng");
    ASSERT_STREQ(input.context.c_str(), "");
    ASSERT_TRUE(input.contextVersion.isEmpty());
}

TEST_CASE(g2pRes_withVersion) {
    G2pRes res("hello", "eng", "SingerA", stdc::VersionNumber(2, 0, 0), "hh ah l ow");
    ASSERT_STREQ(res.context.c_str(), "SingerA");
    ASSERT_EQ(res.contextVersion.major(), 2);
    ASSERT_STREQ(res.pronunciation.c_str(), "hh ah l ow");
}

TEST_CASE(g2pRes_legacy_constructor) {
    G2pRes res("hello", "eng", "SingerA", "hh ah l ow");
    ASSERT_STREQ(res.context.c_str(), "SingerA");
    ASSERT_TRUE(res.contextVersion.isEmpty());
    ASSERT_STREQ(res.pronunciation.c_str(), "hh ah l ow");
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
TEST_CASE(dedup_diffContextVersion) {
    auto a = makeVersionedModule("SingerA", stdc::VersionNumber(1, 0, 0), "pkgA", "g2p-cmn");
    auto b = makeVersionedModule("SingerA", stdc::VersionNumber(2, 0, 0), "pkgA", "g2p-cmn");
    ASSERT_FALSE(a.isSameMainModule(b));
}

// Same contextVersion → same main module (version ignored in isSameMainModule).
TEST_CASE(dedup_sameContextVersion) {
    auto a = makeVersionedModule("SingerA", stdc::VersionNumber(1, 0, 0), "pkgA", "g2p-cmn", "1.0.0");
    auto b = makeVersionedModule("SingerA", stdc::VersionNumber(1, 0, 0), "pkgB", "g2p-cmn", "2.0.0");
    ASSERT_TRUE(a.isSameMainModule(b));
}

// contextVersion included in key().
TEST_CASE(key_includesContextVersion) {
    auto a = makeVersionedModule("SingerA", stdc::VersionNumber(1, 0, 0), "pkgA", "g2p-cmn");
    auto b = makeVersionedModule("SingerA", stdc::VersionNumber(2, 0, 0), "pkgA", "g2p-cmn");
    ASSERT_TRUE(a.key() != b.key());
}

// No contextVersion → key same as before (backward compat).
TEST_CASE(key_noContextVersion) {
    auto a = makeVersionedModule("SingerA", {}, "pkgA", "g2p-cmn");
    ASSERT_TRUE(a.key().find("@") == std::string::npos);
}

// operator== includes contextVersion.
TEST_CASE(equality_diffContextVersion) {
    auto a = makeVersionedModule("SingerA", stdc::VersionNumber(1, 0, 0), "pkgA", "g2p-cmn");
    auto b = makeVersionedModule("SingerA", stdc::VersionNumber(2, 0, 0), "pkgA", "g2p-cmn");
    ASSERT_FALSE(a == b);
}

// ============================================================================
// Dependency resolver: versioned contexts are independent
// ============================================================================

TEST_CASE(isolate_diffContextVersion) {
    // SingerA v1.0 and SingerA v2.0 each have same moduleId
    auto modV1 = makeVersionedModule("SingerA", stdc::VersionNumber(1, 0, 0),
                                      "singerA-v1", "g2p-cmn-custom");
    auto modV2 = makeVersionedModule("SingerA", stdc::VersionNumber(2, 0, 0),
                                      "singerA-v2", "g2p-cmn-custom");

    // Resolve each independently
    std::vector<ModuleMetadata> modulesV1 = {modV1};
    DependencyResolver resolverV1;
    ASSERT_TRUE(resolverV1.resolveAllDependencies(modulesV1));
    ASSERT_EQ(resolverV1.getResolvedModules().size(), 1u);

    std::vector<ModuleMetadata> modulesV2 = {modV2};
    DependencyResolver resolverV2;
    ASSERT_TRUE(resolverV2.resolveAllDependencies(modulesV2));
    ASSERT_EQ(resolverV2.getResolvedModules().size(), 1u);

    // Both resolved but with different contextVersions
    ASSERT_EQ(resolverV1.getResolvedModules()[0].contextVersion.major(), 1);
    ASSERT_EQ(resolverV2.getResolvedModules()[0].contextVersion.major(), 2);
}

// Versioned context can fall back to default context for dependencies
TEST_CASE(fallback_versionedToDefault) {
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
    ASSERT_TRUE(resolver.resolveAllDependencies(modules, fallback));
    ASSERT_EQ(resolver.getResolvedModules().size(), 1u);
}

// Two versioned contexts with same moduleId resolved independently via selectBest
TEST_CASE(dedup_selectBest_versionedContext) {
    auto v1_old = makeVersionedModule("SingerA", stdc::VersionNumber(1, 0, 0),
                                       "pkgA", "g2p-cmn", "1.0.0");
    auto v1_new = makeVersionedModule("SingerA", stdc::VersionNumber(1, 0, 0),
                                       "pkgA", "g2p-cmn", "2.0.0");

    std::vector<ModuleMetadata> modules = {v1_old, v1_new};
    DependencyResolver resolver;
    ASSERT_TRUE(resolver.resolveAllDependencies(modules));

    const auto &resolved = resolver.getResolvedModules();
    ASSERT_EQ(resolved.size(), 1u);
    ASSERT_STREQ(resolved[0].version.c_str(), "2.0.0");
    ASSERT_EQ(resolved[0].contextVersion.major(), 1);
}
