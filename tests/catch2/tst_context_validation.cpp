#include "catch.hpp"

#include <filesystem>

#include <LangCore/Base/LangCommon.h>
#include <LangCore/Core/PackageManager.h>
#include <LangCore/Module/Dependency/DependencyGraph.h>
#include <LangCore/Module/Dependency/DependencyResolver.h>
#include <LangCore/Support/ContextUtils.h>
#include <LangCore/Support/Expected.h>

using namespace LangCore;

// ============================================================================
// R-8: Default context cannot have a version
// ============================================================================

TEST_CASE("r8_addPackagePath_defaultWithVersion_rejected") {
    PackageManager mgr;
    auto result = mgr.addPackagePath("", stdc::VersionNumber(1, 0, 0), "nonexistent/path");
    REQUIRE_FALSE(result.hasValue());
    REQUIRE(result.error().type() == Error::ValidationError);
}

TEST_CASE("r8_setPackagePaths_defaultWithVersion_rejected") {
    PackageManager mgr;
    auto result = mgr.setPackagePaths("", stdc::VersionNumber(2, 0, 0),
                                       {"nonexistent/path1", "nonexistent/path2"});
    REQUIRE_FALSE(result.hasValue());
    REQUIRE(result.error().type() == Error::ValidationError);
}

TEST_CASE("r8_addPackagePath_defaultNoVersion_accepted") {
    PackageManager mgr;
    auto result = mgr.addPackagePath("", "nonexistent/path");
    // Should fail on filesystem, not on validation
    REQUIRE_FALSE(result.hasValue());
    REQUIRE(result.error().type() != Error::ValidationError);
}

TEST_CASE("r8_addPackagePath_nonDefaultWithVersion_accepted") {
    PackageManager mgr;
    auto result = mgr.addPackagePath("SingerA", stdc::VersionNumber(1, 0, 0), "nonexistent/path");
    // Should fail on filesystem, not on validation
    REQUIRE_FALSE(result.hasValue());
    REQUIRE(result.error().type() != Error::ValidationError);
}

// ============================================================================
// selectBestModules: cross-contextVersion dedup (方案C)
// ============================================================================

static ModuleMetadata makeVersionedModule(const std::string &context,
                                           const stdc::VersionNumber &contextVer,
                                           const std::string &pkgId,
                                           const std::string &modId,
                                           const std::string &version = "1.0.0",
                                           int level = 1,
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

// Two modules with same (packageId, moduleId, level) but different contextVersion
// must NOT be deduplicated — each versioned context keeps its own best version.
TEST_CASE("selectBest_crossContextVersion_noDedup") {
    auto v1mod = makeVersionedModule("SingerA", stdc::VersionNumber(1, 0, 0),
                                      "singerA-pkg", "g2p-cmn-custom", "1.0.0");
    auto v2mod = makeVersionedModule("SingerA", stdc::VersionNumber(2, 0, 0),
                                      "singerA-pkg", "g2p-cmn-custom", "1.0.0");

    std::vector<ModuleMetadata> modules = {v1mod, v2mod};
    DependencyResolver resolver;
    REQUIRE(resolver.resolveAllDependencies(modules));

    const auto &resolved = resolver.getResolvedModules();
    REQUIRE(resolved.size() == 2u);
}

// Same contextVersion but different module versions → selectBest keeps highest.
TEST_CASE("selectBest_sameContextVersion_selectsHighest") {
    auto v1old = makeVersionedModule("SingerA", stdc::VersionNumber(1, 0, 0),
                                      "singerA-pkg", "g2p-cmn-custom", "1.0.0");
    auto v1new = makeVersionedModule("SingerA", stdc::VersionNumber(1, 0, 0),
                                      "singerA-pkg", "g2p-cmn-custom", "2.0.0");

    std::vector<ModuleMetadata> modules = {v1old, v1new};
    DependencyResolver resolver;
    REQUIRE(resolver.resolveAllDependencies(modules));

    const auto &resolved = resolver.getResolvedModules();
    REQUIRE(resolved.size() == 1u);
    REQUIRE(resolved[0].version == "2.0.0");
}

// Different context names, same module identity → no dedup.
TEST_CASE("selectBest_crossContextName_noDedup") {
    auto singerAMod = makeVersionedModule("SingerA", stdc::VersionNumber(1, 0, 0),
                                           "pkg", "g2p-cmn-custom", "1.0.0");
    auto singerBMod = makeVersionedModule("SingerB", stdc::VersionNumber(1, 0, 0),
                                           "pkg", "g2p-cmn-custom", "1.0.0");

    std::vector<ModuleMetadata> modules = {singerAMod, singerBMod};
    DependencyResolver resolver;
    REQUIRE(resolver.resolveAllDependencies(modules));

    const auto &resolved = resolver.getResolvedModules();
    REQUIRE(resolved.size() == 2u);
}

// Versioned and unversioned same context name → no dedup (different ContextKey).
TEST_CASE("selectBest_versionedVsUnversioned_noDedup") {
    auto vMod = makeVersionedModule("SingerA", stdc::VersionNumber(1, 0, 0),
                                     "pkg", "g2p-cmn-custom", "1.0.0");
    auto uMod = makeVersionedModule("SingerA", {},
                                     "pkg", "g2p-cmn-custom", "1.0.0");

    std::vector<ModuleMetadata> modules = {vMod, uMod};
    DependencyResolver resolver;
    REQUIRE(resolver.resolveAllDependencies(modules));

    const auto &resolved = resolver.getResolvedModules();
    REQUIRE(resolved.size() == 2u);
}

// ============================================================================
// Cross-contextVersion conflict scenario: same voicebank, different versions,
// each with its own g2p that depends on different official modules
// ============================================================================

TEST_CASE("conflict_multiVersion_g2p_withDeps") {
    // SingerA v1.0 depends on default cmn-official v1.0
    auto defaultMod = makeVersionedModule("", {}, "cmn-official", "g2p-cmn-official", "1.0.0");
    auto singerV1 = makeVersionedModule("SingerA", stdc::VersionNumber(1, 0, 0),
                                         "singerA-v1", "g2p-cmn-custom", "1.0.0");
    DependencyRequirement dep1;
    dep1.packageId = "cmn-official";
    dep1.moduleId = "g2p-cmn-official";
    dep1.level = 1;
    dep1.versionRange = "*";
    singerV1.requirements.push_back(dep1);

    // SingerA v2.0 also depends on default cmn-official
    auto singerV2 = makeVersionedModule("SingerA", stdc::VersionNumber(2, 0, 0),
                                         "singerA-v2", "g2p-cmn-custom", "2.0.0");
    DependencyRequirement dep2;
    dep2.packageId = "cmn-official";
    dep2.moduleId = "g2p-cmn-official";
    dep2.level = 1;
    dep2.versionRange = "*";
    singerV2.requirements.push_back(dep2);

    // Resolve v1.0 context with fallback to default
    {
        std::vector<ModuleMetadata> modules = {singerV1};
        std::vector<ModuleMetadata> fallback = {defaultMod};
        DependencyResolver resolver;
        REQUIRE(resolver.resolveAllDependencies(modules, fallback));
        REQUIRE(resolver.getResolvedModules().size() == 1u);
    }

    // Resolve v2.0 context with fallback to default
    {
        std::vector<ModuleMetadata> modules = {singerV2};
        std::vector<ModuleMetadata> fallback = {defaultMod};
        DependencyResolver resolver;
        REQUIRE(resolver.resolveAllDependencies(modules, fallback));
        REQUIRE(resolver.getResolvedModules().size() == 1u);
    }
}

// SingerA v2.0's G2p mistakenly depends on SingerA v1.0's G2p (cross-context).
// Must fail since cross-context dependencies are not allowed.
TEST_CASE("conflict_crossVersionDep_rejected") {
    auto singerV1 = makeVersionedModule("SingerA", stdc::VersionNumber(1, 0, 0),
                                         "singerA-v1", "g2p-cmn-custom", "1.0.0");

    auto singerV2 = makeVersionedModule("SingerA", stdc::VersionNumber(2, 0, 0),
                                         "singerA-v2", "g2p-cmn-enhanced", "2.0.0");
    DependencyRequirement dep;
    dep.packageId = "singerA-v1";
    dep.moduleId = "g2p-cmn-custom";
    dep.level = 1;
    dep.versionRange = "*";
    singerV2.requirements.push_back(dep);

    // SingerA@v2.0 context only has singerV2 module.
    // singerV1 is in a different ContextKey, so it won't be found here.
    std::vector<ModuleMetadata> modules = {singerV2};
    DependencyResolver resolver;
    REQUIRE_FALSE(resolver.resolveAllDependencies(modules));
}

// ============================================================================
// ContextUtils edge cases
// ============================================================================

TEST_CASE("contextKey_versionBoundary") {
    // Max version components
    ContextKey key1("SingerA", stdc::VersionNumber(999, 999, 999));
    REQUIRE(key1.isVersioned());
}

TEST_CASE("contextKey_orderingWithVersion") {
    ContextKey a("SingerA", stdc::VersionNumber(1, 0, 0));
    ContextKey b("SingerA", stdc::VersionNumber(1, 19, 0));
    ContextKey c("SingerA", stdc::VersionNumber(2, 0, 0));
    ContextKey d("SingerA");
    ContextKey e("SingerB");

    REQUIRE(a < b);
    REQUIRE(b < c);
    REQUIRE(d < a); // unversioned < versioned with same context name
    REQUIRE(d < e);
}

TEST_CASE("contextKey_differentContextSameVersion") {
    ContextKey a("SingerA", stdc::VersionNumber(1, 0, 0));
    ContextKey b("SingerB", stdc::VersionNumber(1, 0, 0));
    REQUIRE_FALSE(a == b);
    REQUIRE(a < b);
}

TEST_CASE("fqid_parse_versionEdgeCases") {
    // Version with components
    auto r1 = ContextUtils::parseFqid("SingerA@1.2.3:g2p-cmn");
    REQUIRE(r1.context == "SingerA");
    REQUIRE(r1.version.major() == 1);
    REQUIRE(r1.version.minor() == 2);
    REQUIRE(r1.version.patch() == 3);
    REQUIRE(r1.moduleId == "g2p-cmn");

    // Context with dots (legal)
    auto r2 = ContextUtils::parseFqid("v1.0_test:g2p-cmn");
    REQUIRE(r2.context == "v1.0_test");
    REQUIRE(r2.moduleId == "g2p-cmn");

    // Context with '@' in version part only
    auto r3 = ContextUtils::parseFqid("SingerA@1.0:g2p-cmn");
    REQUIRE(r3.context == "SingerA");
    REQUIRE(r3.version.major() == 1);
    REQUIRE(r3.version.minor() == 0);
}

// ============================================================================
// Context name validation edge cases
// ============================================================================

TEST_CASE("validateContextName_boundaries") {
    // Single char
    REQUIRE(ContextUtils::validateContextName("A").hasValue());
    REQUIRE(ContextUtils::validateContextName("0").hasValue());
    REQUIRE(ContextUtils::validateContextName("_").hasValue());
    REQUIRE(ContextUtils::validateContextName(".").hasValue());
    REQUIRE(ContextUtils::validateContextName("-").hasValue());

    // Max length
    std::string maxName(128, 'A');
    REQUIRE(ContextUtils::validateContextName(maxName).hasValue());

    // One over max
    std::string overName(129, 'A');
    REQUIRE_FALSE(ContextUtils::validateContextName(overName).hasValue());
}

TEST_CASE("validateContextName_invalidChars") {
    REQUIRE_FALSE(ContextUtils::validateContextName("name with space").hasValue());
    REQUIRE_FALSE(ContextUtils::validateContextName("a@b").hasValue());
    REQUIRE_FALSE(ContextUtils::validateContextName("a:b").hasValue());
    REQUIRE_FALSE(ContextUtils::validateContextName("a;b").hasValue());
    REQUIRE_FALSE(ContextUtils::validateContextName("a'b").hasValue());
    REQUIRE_FALSE(ContextUtils::validateContextName("a\"b").hasValue());
    REQUIRE_FALSE(ContextUtils::validateContextName("a[b").hasValue());
    REQUIRE_FALSE(ContextUtils::validateContextName("a]b").hasValue());
}

// ============================================================================
// contextState() / failedContexts() observability API (Task 1.4)
// L1 unit tests: Pending / NotRegistered / empty failedContexts
// Ready / Failed states require Manager::initialize() → L3 integration tests (Task 1.5)
// ============================================================================

TEST_CASE("contextState_unregisteredContext_returnsNotRegistered") {
    PackageManager mgr;
    // 未调用 addPackagePath，context 不在 contextPackagePaths 中
    ContextKey unregisteredCtx("SingerX", stdc::VersionNumber(1, 0, 0));
    REQUIRE(mgr.contextState(unregisteredCtx) == ContextState::NotRegistered);
}

TEST_CASE("contextState_defaultUnregistered_returnsNotRegistered") {
    PackageManager mgr;
    // 默认 context 未注册
    ContextKey defaultCtx("");
    REQUIRE(mgr.contextState(defaultCtx) == ContextState::NotRegistered);
}

TEST_CASE("contextState_registeredButNotInitialized_returnsPending") {
    PackageManager mgr;
    // 注册一个声库 context（使用 temp 目录作为合法路径）
    const auto tempDir = std::filesystem::temp_directory_path();
    auto exp = mgr.addPackagePath("SingerA", stdc::VersionNumber(1, 0, 0), tempDir);
    REQUIRE(exp.hasValue());

    // 已注册但未 initialize() → Pending
    ContextKey ctxKey("SingerA", stdc::VersionNumber(1, 0, 0));
    REQUIRE(mgr.contextState(ctxKey) == ContextState::Pending);
}

TEST_CASE("contextState_defaultContextRegistered_returnsPending") {
    PackageManager mgr;
    const auto tempDir = std::filesystem::temp_directory_path();
    auto exp = mgr.addPackagePath("", tempDir);
    REQUIRE(exp.hasValue());

    // 默认 context 已注册但未 initialize() → Pending
    ContextKey defaultCtx("");
    REQUIRE(mgr.contextState(defaultCtx) == ContextState::Pending);
}

TEST_CASE("contextState_wrongVersion_returnsNotRegistered") {
    PackageManager mgr;
    const auto tempDir = std::filesystem::temp_directory_path();
    // 注册 SingerA@1.0.0
    auto exp = mgr.addPackagePath("SingerA", stdc::VersionNumber(1, 0, 0), tempDir);
    REQUIRE(exp.hasValue());

    // 查询 SingerA@2.0.0（未注册的版本）→ NotRegistered
    ContextKey wrongVersionCtx("SingerA", stdc::VersionNumber(2, 0, 0));
    REQUIRE(mgr.contextState(wrongVersionCtx) == ContextState::NotRegistered);
}

TEST_CASE("contextState_wrongContextName_returnsNotRegistered") {
    PackageManager mgr;
    const auto tempDir = std::filesystem::temp_directory_path();
    auto exp = mgr.addPackagePath("SingerA", stdc::VersionNumber(1, 0, 0), tempDir);
    REQUIRE(exp.hasValue());

    // 查询 SingerB（不同的 context 名）→ NotRegistered
    ContextKey wrongNameCtx("SingerB", stdc::VersionNumber(1, 0, 0));
    REQUIRE(mgr.contextState(wrongNameCtx) == ContextState::NotRegistered);
}

TEST_CASE("failedContexts_noInitialization_returnsEmpty") {
    PackageManager mgr;
    // 未调用 initialize()，contextStates 为空 → failedContexts 返回空
    const auto tempDir = std::filesystem::temp_directory_path();
    mgr.addPackagePath("SingerA", stdc::VersionNumber(1, 0, 0), tempDir);
    mgr.addPackagePath("SingerB", stdc::VersionNumber(1, 0, 0), tempDir);

    auto failed = mgr.failedContexts();
    REQUIRE(failed.empty());
}

TEST_CASE("contextState_multipleContextsMixedRegistration") {
    PackageManager mgr;
    const auto tempDir = std::filesystem::temp_directory_path();
    // 注册 SingerA@1.0.0 和默认 context
    mgr.addPackagePath("SingerA", stdc::VersionNumber(1, 0, 0), tempDir);
    mgr.addPackagePath("", tempDir);

    // 已注册的 → Pending
    REQUIRE(mgr.contextState(ContextKey("SingerA", stdc::VersionNumber(1, 0, 0))) == ContextState::Pending);
    REQUIRE(mgr.contextState(ContextKey("")) == ContextState::Pending);

    // 未注册的 → NotRegistered
    REQUIRE(mgr.contextState(ContextKey("SingerB", stdc::VersionNumber(1, 0, 0))) == ContextState::NotRegistered);
    REQUIRE(mgr.contextState(ContextKey("SingerA", stdc::VersionNumber(2, 0, 0))) == ContextState::NotRegistered);
}