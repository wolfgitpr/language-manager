#include "tst_framework.h"

#include <LangCore/Module/Dependency/DependencyResolver.h>
#include <LangCore/Module/Dependency/DependencyGraph.h>

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
TEST_CASE(isolate_sameModuleId) {
    auto modA = makeModule("SingerA", "singerA-custom", "g2p-cmn-custom");
    auto modB = makeModule("SingerB", "singerB-custom", "g2p-cmn-custom");

    // Resolve SingerA's modules (no deps, no fallback)
    std::vector<LangCore::ModuleMetadata> modulesA = {modA};
    LangCore::DependencyResolver resolverA;
    ASSERT_TRUE(resolverA.resolveAllDependencies(modulesA));
    ASSERT_EQ(resolverA.getResolvedModules().size(), 1u);

    // Resolve SingerB's modules
    std::vector<LangCore::ModuleMetadata> modulesB = {modB};
    LangCore::DependencyResolver resolverB;
    ASSERT_TRUE(resolverB.resolveAllDependencies(modulesB));
    ASSERT_EQ(resolverB.getResolvedModules().size(), 1u);

    // Both have same moduleId but different context
    ASSERT_STREQ(resolverA.getResolvedModules()[0].moduleId.c_str(), "g2p-cmn-custom");
    ASSERT_STREQ(resolverB.getResolvedModules()[0].moduleId.c_str(), "g2p-cmn-custom");
    ASSERT_STREQ(resolverA.getResolvedModules()[0].context.c_str(), "SingerA");
    ASSERT_STREQ(resolverB.getResolvedModules()[0].context.c_str(), "SingerB");
}

// SingerA's module depends on a module from SingerB's package.
// Without fallback, SingerA can't see SingerB's modules → fails.
TEST_CASE(isolate_noCrossDep) {
    auto modA = makeModule("SingerA", "singerA-custom", "g2p-A");
    LangCore::DependencyRequirement dep;
    dep.packageId = "singerB-custom";
    dep.moduleId = "g2p-B";
    dep.level = 1;
    dep.versionRange = "*";
    modA.requirements.push_back(dep);

    std::vector<LangCore::ModuleMetadata> modules = {modA};
    LangCore::DependencyResolver resolver;
    ASSERT_FALSE(resolver.resolveAllDependencies(modules));
    ASSERT_GT(resolver.getErrors().size(), 0u);
}

// SingerC depends on a default-context module. Fallback provides it.
TEST_CASE(fallback_toDefault) {
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
    ASSERT_TRUE(resolver.resolveAllDependencies(modules, fallback));
    ASSERT_EQ(resolver.getResolvedModules().size(), 1u);
}

// Local module is preferred over fallback when both match.
// Both singerA's modules list and default fallback have the same packageId::moduleId.
// Resolver searches `modules` first, so SingerA's v2.0 wins over default's v1.0.
TEST_CASE(fallback_preferLocal) {
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
    ASSERT_TRUE(resolver.resolveAllDependencies(modules, fallback));

    // Verify g2p-main resolved, and its dependency points to v2.0.0 (local)
    bool foundMain = false;
    for (const auto &m : resolver.getResolvedModules()) {
        if (m.moduleId == "g2p-main") {
            foundMain = true;
            ASSERT_EQ(m.resolvedDependencies.size(), 1u);
            ASSERT_STREQ(m.resolvedDependencies[0].version.c_str(), "2.0.0");
        }
    }
    ASSERT_TRUE(foundMain);
}

// SingerA depends on SingerB's module. Neither SingerB nor default has it → fails.
TEST_CASE(noFallback_crossContext) {
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
    ASSERT_FALSE(resolver.resolveAllDependencies(modules, emptyFallback));
}

// Default context module is visible to SingerA via fallback.
TEST_CASE(defaultContext_globalVisibility) {
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
    ASSERT_TRUE(resolver.resolveAllDependencies(modules, fallback));
    ASSERT_EQ(resolver.getResolvedModules().size(), 1u);
}

// Default context depends on SingerA's module. No fallback → fails.
// Default can't see other contexts' modules.
TEST_CASE(otherContext_notVisibleToDefault) {
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
    ASSERT_FALSE(resolver.resolveAllDependencies(modules));
}
