#include "catch.hpp"

#include <filesystem>

#include <LangCore/Core/PackageManager.h>
#include <LangCore/Support/ContextUtils.h>

using namespace LangCore;

// ============================================================================
// Routing two-level decision (G2pRouteResolver).
// See docs/host-integration/03-host-integration-contract.md §7.4.
//
// The full G2pRouteResolver lives in the host (ds-editor-lite), not in
// LangCore. These tests verify the LangCore PRIMITIVES that the routing
// decision relies on:
//   - PackageManager::packagePaths(context): whether a voicebank context has
//     registered G2P packages (determines voicebank vs official routing).
//   - ContextKey construction: the routing context key passed to convert().
//   - ContextUtils::formatFqid: FQID for the chosen routing context.
//
// Host-side concerns (resolutionState, language lookup, g2pId validity) are
// NOT testable here — they live in ds-editor-lite and require L3 tests.
// ============================================================================

// Simulate the host's two-level routing decision using LangCore state.
// Mirrors G2pRouteResolver::resolve(singerInfo, language):
//   - singer g2pPackagePaths non-empty → context = singerId (voicebank)
//   - singer g2pPackagePaths empty    → context = "" (official default)
static ContextKey resolveRoute(const PackageManager &mgr, const std::string &singerId) {
    // Level 1: voicebank context has packages?
    if (!mgr.packagePaths(singerId).empty()) {
        return ContextKey(singerId);
    }
    // Level 2: fallback to official default context
    return ContextKey("");
}

// R-C1: Singer with registered G2P packages → route to voicebank context.
TEST_CASE("rc1_singerWithPackages_routesToVoicebank") {
    PackageManager mgr;
    const auto tempDir = std::filesystem::temp_directory_path();
    mgr.addPackagePath("SingerA", tempDir);

    auto ctxKey = resolveRoute(mgr, "SingerA");
    REQUIRE(ctxKey.context == "SingerA");
    REQUIRE_FALSE(ctxKey.isDefault());
}

// R-C2: Singer without G2P packages → route to official default context.
TEST_CASE("rc2_singerWithoutPackages_routesToOfficial") {
    PackageManager mgr;
    // Register default context only; SingerA has no packages
    mgr.addPackagePath("", std::filesystem::temp_directory_path());

    auto ctxKey = resolveRoute(mgr, "SingerA");
    REQUIRE(ctxKey.context == "");
    REQUIRE(ctxKey.isDefault());
}

// R-C2b: No contexts registered at all → still routes to default (empty).
TEST_CASE("rc2b_noContextsRegistered_routesToOfficialDefault") {
    PackageManager mgr;

    auto ctxKey = resolveRoute(mgr, "SingerA");
    REQUIRE(ctxKey.isDefault());
}

// Versioned voicebank context: packagePaths with version distinguishes
// different versions of the same voicebank.
TEST_CASE("rc3_versionedVoicebank_packagePaths") {
    PackageManager mgr;
    const auto tempDir = std::filesystem::temp_directory_path();
    mgr.addPackagePath("SingerA", stdc::VersionNumber(1, 0, 0), tempDir);

    // Versioned query returns the registered path
    auto paths = mgr.packagePaths("SingerA", stdc::VersionNumber(1, 0, 0));
    REQUIRE_FALSE(paths.empty());

    // Unversioned query returns empty (only versioned registered)
    auto unversionedPaths = mgr.packagePaths("SingerA");
    REQUIRE(unversionedPaths.empty());
}

// Routing decision produces correct FQID for the chosen context.
// Voicebank context → "SingerA:g2pId"; default context → "g2pId" (bare).
TEST_CASE("rc4_routingContext_producesCorrectFqid") {
    PackageManager mgr;
    const auto tempDir = std::filesystem::temp_directory_path();

    // Voicebank routing → FQID with context prefix
    mgr.addPackagePath("SingerA", tempDir);
    auto voicebankCtx = resolveRoute(mgr, "SingerA");
    REQUIRE(ContextUtils::formatFqid(voicebankCtx, "g2p-cmn") == "SingerA:g2p-cmn");

    // Official routing → bare FQID (no prefix)
    PackageManager mgr2;
    auto officialCtx = resolveRoute(mgr2, "SingerA");
    REQUIRE(ContextUtils::formatFqid(officialCtx, "g2p-cmn") == "g2p-cmn");
}

// contexts() lists all registered context names for routing inspection.
TEST_CASE("rc5_contexts_listsRegisteredContexts") {
    PackageManager mgr;
    const auto tempDir = std::filesystem::temp_directory_path();
    mgr.addPackagePath("", tempDir);
    mgr.addPackagePath("SingerA", tempDir);
    mgr.addPackagePath("SingerB", tempDir);

    auto ctxs = mgr.contexts();
    // Default + SingerA + SingerB
    REQUIRE(ctxs.size() == 3u);

    bool hasDefault = false, hasA = false, hasB = false;
    for (const auto &c : ctxs) {
        if (c.empty()) hasDefault = true;
        if (c == "SingerA") hasA = true;
        if (c == "SingerB") hasB = true;
    }
    REQUIRE(hasDefault);
    REQUIRE(hasA);
    REQUIRE(hasB);
}

// contextKeys() provides versioned context keys for routing.
TEST_CASE("rc6_contextKeys_includesVersionedContexts") {
    PackageManager mgr;
    const auto tempDir = std::filesystem::temp_directory_path();
    mgr.addPackagePath("", tempDir);
    mgr.addPackagePath("SingerA", stdc::VersionNumber(2, 0, 0), tempDir);

    auto keys = mgr.contextKeys();
    REQUIRE(keys.size() == 2u);

    bool hasDefault = false, hasVersionedA = false;
    for (const auto &k : keys) {
        if (k.isDefault()) hasDefault = true;
        if (k.context == "SingerA" && k.isVersioned()) hasVersionedA = true;
    }
    REQUIRE(hasDefault);
    REQUIRE(hasVersionedA);
}
