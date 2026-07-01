#include "catch.hpp"

#include <LangCore/Base/NamedObject.h>
#include <LangCore/Core/PackageManager.h>
#include <LangCore/Module/Dependency/DependencyResolver.h>
#include <LangCore/Module/Dependency/DependencyGraph.h>
#include <LangCore/Support/ContextUtils.h>

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

// Two contexts each have a module with the same moduleId but different packages.
// Resolving each independently should succeed.
TEST_CASE("isolate_sameModuleId") {
    auto modA = makeModule("SingerA", "singerA-custom", "g2p-cmn-custom");
    auto modB = makeModule("SingerB", "singerB-custom", "g2p-cmn-custom");

    // Resolve SingerA's modules (no deps, no fallback)
    std::vector<LangCore::ModuleMetadata> modulesA = {modA};
    LangCore::DependencyResolver resolverA;
    REQUIRE(resolverA.resolveAllDependencies(modulesA));
    REQUIRE(resolverA.getResolvedModules().size() == 1u);

    // Resolve SingerB's modules
    std::vector<LangCore::ModuleMetadata> modulesB = {modB};
    LangCore::DependencyResolver resolverB;
    REQUIRE(resolverB.resolveAllDependencies(modulesB));
    REQUIRE(resolverB.getResolvedModules().size() == 1u);

    // Both have same moduleId but different context
    REQUIRE(resolverA.getResolvedModules()[0].moduleId == "g2p-cmn-custom");
    REQUIRE(resolverB.getResolvedModules()[0].moduleId == "g2p-cmn-custom");
    REQUIRE(resolverA.getResolvedModules()[0].context == "SingerA");
    REQUIRE(resolverB.getResolvedModules()[0].context == "SingerB");
}

// SingerA's module depends on a module from SingerB's package.
// Without fallback, SingerA can't see SingerB's modules → fails.
TEST_CASE("isolate_noCrossDep") {
    auto modA = makeModule("SingerA", "singerA-custom", "g2p-A");
    LangCore::DependencyRequirement dep;
    dep.packageId = "singerB-custom";
    dep.moduleId = "g2p-B";
    dep.level = 1;
    dep.versionRange = "*";
    modA.requirements.push_back(dep);

    std::vector<LangCore::ModuleMetadata> modules = {modA};
    LangCore::DependencyResolver resolver;
    REQUIRE_FALSE(resolver.resolveAllDependencies(modules));
    REQUIRE(resolver.getErrors().size() > 0u);
}

// SingerC depends on a default-context module. Fallback provides it.
TEST_CASE("fallback_toDefault") {
    auto defaultMod = makeModule("", "cmn-official", "g2p-cmn-official");

    auto singerCMod = makeModule("SingerC", "singerC-custom", "g2p-cmn-enhanced");
    LangCore::DependencyRequirement dep;
    dep.packageId = "cmn-official";
    dep.moduleId = "g2p-cmn-official";
    dep.level = 1;
    dep.versionRange = "*";
    singerCMod.requirements.push_back(dep);

    std::vector<LangCore::ModuleMetadata> modules = {singerCMod};
    std::vector<LangCore::ModuleMetadata> fallback = {defaultMod};

    LangCore::DependencyResolver resolver;
    REQUIRE(resolver.resolveAllDependencies(modules, fallback));
    REQUIRE(resolver.getResolvedModules().size() == 1u);
}

// Local module is preferred over fallback when both match.
// Both singerA's modules list and default fallback have the same packageId::moduleId.
// Resolver searches `modules` first, so SingerA's v2.0 wins over default's v1.0.
TEST_CASE("fallback_preferLocal") {
    auto defaultMod = makeModule("", "shared-pkg", "g2p-cmn-x", "1.0.0");
    auto localDep = makeModule("SingerA", "shared-pkg", "g2p-cmn-x", "2.0.0");

    auto mainMod = makeModule("SingerA", "singerA-pkg", "g2p-main");
    LangCore::DependencyRequirement dep;
    dep.packageId = "shared-pkg";
    dep.moduleId = "g2p-cmn-x";
    dep.level = 1;
    dep.versionRange = "*";
    mainMod.requirements.push_back(dep);

    // SingerA's modules include both g2p-main and the local g2p-cmn-x
    std::vector<LangCore::ModuleMetadata> modules = {mainMod, localDep};
    std::vector<LangCore::ModuleMetadata> fallback = {defaultMod};

    LangCore::DependencyResolver resolver;
    REQUIRE(resolver.resolveAllDependencies(modules, fallback));

    // Verify g2p-main resolved, and its dependency points to v2.0.0 (local)
    bool foundMain = false;
    for (const auto &m : resolver.getResolvedModules()) {
        if (m.moduleId == "g2p-main") {
            foundMain = true;
            REQUIRE(m.resolvedDependencies.size() == 1u);
            REQUIRE(m.resolvedDependencies[0].version == "2.0.0");
        }
    }
    REQUIRE(foundMain);
}

// SingerA depends on SingerB's module. Neither SingerB nor default has it → fails.
TEST_CASE("noFallback_crossContext") {
    auto modA = makeModule("SingerA", "singerA-custom", "g2p-A");
    LangCore::DependencyRequirement dep;
    dep.packageId = "singerB-custom";
    dep.moduleId = "g2p-B";
    dep.level = 1;
    dep.versionRange = "*";
    modA.requirements.push_back(dep);

    std::vector<LangCore::ModuleMetadata> modules = {modA};
    std::vector<LangCore::ModuleMetadata> emptyFallback;

    LangCore::DependencyResolver resolver;
    REQUIRE_FALSE(resolver.resolveAllDependencies(modules, emptyFallback));
}

// Default context module is visible to SingerA via fallback.
TEST_CASE("defaultContext_globalVisibility") {
    auto defaultMod = makeModule("", "cmn-official", "g2p-cmn-official");

    auto singerAMod = makeModule("SingerA", "singerA-pkg", "g2p-main");
    LangCore::DependencyRequirement dep;
    dep.packageId = "cmn-official";
    dep.moduleId = "g2p-cmn-official";
    dep.level = 1;
    dep.versionRange = "*";
    singerAMod.requirements.push_back(dep);

    std::vector<LangCore::ModuleMetadata> modules = {singerAMod};
    std::vector<LangCore::ModuleMetadata> fallback = {defaultMod};

    LangCore::DependencyResolver resolver;
    REQUIRE(resolver.resolveAllDependencies(modules, fallback));
    REQUIRE(resolver.getResolvedModules().size() == 1u);
}

// Default context depends on SingerA's module. No fallback → fails.
// Default can't see other contexts' modules.
TEST_CASE("otherContext_notVisibleToDefault") {
    auto singerAMod = makeModule("SingerA", "singerA-custom", "g2p-A");

    auto defaultMod = makeModule("", "default-pkg", "g2p-default");
    LangCore::DependencyRequirement dep;
    dep.packageId = "singerA-custom";
    dep.moduleId = "g2p-A";
    dep.level = 1;
    dep.versionRange = "*";
    defaultMod.requirements.push_back(dep);

    // Default context modules only — SingerA's module is NOT in modules or fallback
    std::vector<LangCore::ModuleMetadata> modules = {defaultMod};
    LangCore::DependencyResolver resolver;
    REQUIRE_FALSE(resolver.resolveAllDependencies(modules));
}

// ============================================================================
// S5: ModelStep FQID two-level lookup (Decision D4)
// Tests the ObjectPool lookup pattern used by ModelStep::configure() without
// requiring the ChainG2p plugin. Mirrors the exact lookup logic:
//   1. formatFqid(ctxKey, g2pId) → try "context:g2pId"
//   2. if not found && !ctxKey.isDefault() → try bare "g2pId" (default fallback)
// See ModelStep.cpp:53-60
// ============================================================================

using LangCore::ContextKey;
using LangCore::ContextUtils;
using LangCore::NamedObject;
using LangCore::NO;
using LangCore::ObjectPool;
using LangCore::PackageManager;

// Simulate the ModelStep FQID two-level lookup against a real ObjectPool.
// Returns the found object (or null) exactly as ModelStep::configure() would.
static NO<NamedObject> modelStepFqidLookup(const ObjectPool *cate, const ContextKey &ctxKey,
                                          const std::string &g2pId) {
    const auto fqid = ContextUtils::formatFqid(ctxKey, g2pId);
    auto obj = cate->getFirstObject(fqid);
    if (!obj && !ctxKey.isDefault()) {
        // 声库 context 找不到 → 回退默认 context（裸 id = 默认 context 的 FQID）
        obj = cate->getFirstObject(g2pId);
    }
    return obj;
}

// S5-C1: g2pId only in default context, private context does not have it.
// FQID lookup falls back to default context (bare id), succeeds.
TEST_CASE("s5_c1_defaultFallback_succeeds") {
    PackageManager mgr;
    auto *g2pCate = mgr.category("g2p");
    REQUIRE(g2pCate != nullptr);

    // Register only in default context (bare id)
    auto defaultObj = NO<NamedObject>::create("default-g2p");
    g2pCate->addObject("g2p-model", defaultObj);

    // Private context lookup → fallback to default
    ContextKey privateCtx("SingerA");
    auto found = modelStepFqidLookup(g2pCate, privateCtx, "g2p-model");
    REQUIRE(found);
    REQUIRE(found->objectName() == "default-g2p");
}

// S5-C2: g2pId only in private context.
// FQID lookup hits private context directly, no fallback needed.
TEST_CASE("s5_c2_privateOnly_succeeds") {
    PackageManager mgr;
    auto *g2pCate = mgr.category("g2p");
    REQUIRE(g2pCate != nullptr);

    // Register only in private context (FQID key)
    auto privateObj = NO<NamedObject>::create("private-g2p");
    g2pCate->addObject("SingerA:g2p-model", privateObj);

    ContextKey privateCtx("SingerA");
    auto found = modelStepFqidLookup(g2pCate, privateCtx, "g2p-model");
    REQUIRE(found);
    REQUIRE(found->objectName() == "private-g2p");
}

// S5-C3: g2pId in both contexts.
// Private context (FQID) is preferred over default (bare id).
TEST_CASE("s5_c3_bothPresent_prefersPrivate") {
    PackageManager mgr;
    auto *g2pCate = mgr.category("g2p");
    REQUIRE(g2pCate != nullptr);

    auto defaultObj = NO<NamedObject>::create("default-g2p");
    auto privateObj = NO<NamedObject>::create("private-g2p");
    g2pCate->addObject("g2p-model", defaultObj);           // default (bare)
    g2pCate->addObject("SingerA:g2p-model", privateObj);    // private (FQID)

    ContextKey privateCtx("SingerA");
    auto found = modelStepFqidLookup(g2pCate, privateCtx, "g2p-model");
    REQUIRE(found);
    REQUIRE(found->objectName() == "private-g2p");  // private preferred
}

// S5-C4: g2pId in neither context.
// Lookup fails (both FQID and bare id miss), returns null.
TEST_CASE("s5_c4_neitherPresent_notFound") {
    PackageManager mgr;
    auto *g2pCate = mgr.category("g2p");
    REQUIRE(g2pCate != nullptr);

    // Register unrelated objects
    g2pCate->addObject("other-id", NO<NamedObject>::create("other"));

    ContextKey privateCtx("SingerA");
    auto found = modelStepFqidLookup(g2pCate, privateCtx, "g2p-missing");
    REQUIRE_FALSE(found);
}

// S5-C5: default context lookup does not trigger fallback (isDefault() guard).
// When ctxKey is default, only bare id is tried (formatFqid returns bare id).
TEST_CASE("s5_c5_defaultContext_noFallbackBranch") {
    PackageManager mgr;
    auto *g2pCate = mgr.category("g2p");
    REQUIRE(g2pCate != nullptr);

    auto defaultObj = NO<NamedObject>::create("default-g2p");
    g2pCate->addObject("g2p-model", defaultObj);

    ContextKey defaultCtx;
    // formatFqid(defaultCtx, "g2p-model") == "g2p-model" (bare, no prefix)
    REQUIRE(ContextUtils::formatFqid(defaultCtx, "g2p-model") == "g2p-model");

    auto found = modelStepFqidLookup(g2pCate, defaultCtx, "g2p-model");
    REQUIRE(found);
    REQUIRE(found->objectName() == "default-g2p");
}

// S5-C6: versioned private context FQID lookup.
// FQID for a versioned context is "context@version:moduleId".
// Note: VersionNumber(1,0,0).toString() returns "1.0" (trailing .0 truncated),
// so the FQID is built dynamically to avoid hardcoding the canonical form.
TEST_CASE("s5_c6_versionedContext_fqidLookup") {
    PackageManager mgr;
    auto *g2pCate = mgr.category("g2p");
    REQUIRE(g2pCate != nullptr);

    // Use a version with non-zero patch so toString() yields the full form.
    // VersionNumber(2, 0, 5).toString() == "2.0.5"
    ContextKey versionedCtx("SingerA", stdc::VersionNumber(2, 0, 5));
    REQUIRE(versionedCtx.isVersioned());
    REQUIRE_FALSE(versionedCtx.isDefault());

    // Build the expected FQID dynamically from the context's own toString().
    const auto expectedFqid = ContextUtils::formatFqid(versionedCtx, "g2p-model");
    REQUIRE(expectedFqid == "SingerA@2.0.5:g2p-model");

    auto versionedObj = NO<NamedObject>::create("versioned-g2p");
    g2pCate->addObject(expectedFqid, versionedObj);

    auto found = modelStepFqidLookup(g2pCate, versionedCtx, "g2p-model");
    REQUIRE(found);
    REQUIRE(found->objectName() == "versioned-g2p");
}
