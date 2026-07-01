#include "catch.hpp"

#include <filesystem>

#include <LangCore/Core/Manager.h>
#include <LangCore/Support/ContextUtils.h>
#include <LangCore/Support/Expected.h>

using namespace LangCore;

// ============================================================================
// Manager initialization constraints (L-1 ~ L-4) and idempotency guard.
// See docs/host-integration/03-host-integration-contract.md §3-4.
//
// L1 scope: Tests that do NOT require successful initialize() (which needs real
// G2P packages + ONNX runtime). The following require L3 (tst_langCore):
//   - I-C1 Ready state after successful init
//   - I-C3 idempotent no-op on SUCCESSFUL second call (returns AlreadyInitialized)
//   - I-C5 initialized()==true after successful init
// ============================================================================

// Creates a clean, isolated subdirectory under temp_directory_path().
// Tests that register the DEFAULT context ("") trigger directory scanning during
// initialize() (via refreshPackageIndexes). Using temp_directory_path() directly
// can hit foreign files (e.g. IDE socket files) that cause directory_entry::status
// to throw. This helper guarantees a clean, empty directory.
static std::filesystem::path makeCleanTempDir() {
    const auto path = std::filesystem::temp_directory_path() / "langmgr_test_init";
    std::error_code ec;
    std::filesystem::remove_all(path, ec);
    std::filesystem::create_directories(path, ec);
    return path;
}

// I-C4: initialized() returns false before any initialize() call.
TEST_CASE("ic4_initialized_false_before_initialize") {
    Manager mgr;
    REQUIRE_FALSE(mgr.initialized());
}

// I-C2: addPackagePath after a FAILED initialize() still works.
// A failed initialize() (no packages found) does not lock the manager —
// the idempotency guard only activates after SUCCESSFUL completion.
// (L-3 "runtime addPackagePath is ignored" applies only post-successful-init.)
TEST_CASE("ic2_addPackagePath_afterFailedInit_stillWorks") {
    Manager mgr;
    const auto tempDir = std::filesystem::temp_directory_path();

    // initialize() with no registered default context → fails
    auto result = mgr.initialize();
    REQUIRE_FALSE(result.hasValue());
    REQUIRE_FALSE(mgr.initialized());

    // addPackagePath still works after failed init
    auto addResult = mgr.addPackagePath("SingerA", tempDir);
    REQUIRE(addResult.hasValue());

    // The newly registered context shows as Pending
    REQUIRE(mgr.contextState(ContextKey("SingerA")) == ContextState::Pending);
}

// Failed initialize() does not set initialized=true, allowing retry.
// This verifies the guard is "success-gated", not "call-gated".
TEST_CASE("ic_failedInit_allowsRetry_notBlocked") {
    Manager mgr;
    const auto tempDir = std::filesystem::temp_directory_path();
    mgr.addPackagePath("SingerA", tempDir);

    // First initialize() fails (temp dir has no G2P packages)
    auto result1 = mgr.initialize();
    REQUIRE_FALSE(result1.hasValue());
    REQUIRE_FALSE(mgr.initialized());

    // Second call is NOT blocked (initialized is still false) — re-runs
    auto result2 = mgr.initialize();
    REQUIRE_FALSE(result2.hasValue());
    REQUIRE_FALSE(mgr.initialized());
}

// initialize() with no registered default context returns InitializationError.
// (Ord-1: Default context initialization failed: no modules found)
TEST_CASE("ic_initialize_noDefaultContext_returnsInitError") {
    Manager mgr;
    auto result = mgr.initialize();
    REQUIRE_FALSE(result.hasValue());
    REQUIRE(result.error().type() == Error::InitializationError);
    REQUIRE_FALSE(mgr.initialized());
}

// initialize() with an empty default context (registered path, no packages)
// returns InitializationError.
// Uses a clean isolated temp dir: the default context triggers directory scanning
// during initialize(), so temp_directory_path() may hit inaccessible foreign files.
TEST_CASE("ic_initialize_emptyDefaultContext_returnsInitError") {
    Manager mgr;
    const auto cleanDir = makeCleanTempDir();
    mgr.addPackagePath("", cleanDir);

    auto result = mgr.initialize();
    REQUIRE_FALSE(result.hasValue());
    REQUIRE(result.error().type() == Error::InitializationError);
    REQUIRE_FALSE(mgr.initialized());
}

// ContextState transition: NotRegistered → Pending (via addPackagePath).
// Ready/Failed transitions require successful/failed initialize() with real
// packages → L3 integration tests.
TEST_CASE("ic_contextState_notRegistered_to_pending") {
    Manager mgr;
    const auto tempDir = std::filesystem::temp_directory_path();

    // Before registration: NotRegistered
    REQUIRE(mgr.contextState(ContextKey("SingerA")) == ContextState::NotRegistered);

    // After registration: Pending
    mgr.addPackagePath("SingerA", tempDir);
    REQUIRE(mgr.contextState(ContextKey("SingerA")) == ContextState::Pending);

    // Unregistered version of same context: NotRegistered
    REQUIRE(mgr.contextState(ContextKey("SingerA", stdc::VersionNumber(1, 0, 0))) ==
            ContextState::NotRegistered);
}

// After failed initialize(), registered contexts remain Pending (not Failed).
// initialize() returns early at default-context phase, so non-default contexts
// are never reached → they stay Pending (registered but unprocessed).
// Uses a clean isolated temp dir: the default context triggers directory scanning
// during initialize(), so temp_directory_path() may hit inaccessible foreign files.
TEST_CASE("ic_failedInit_leavesRegisteredContextsPending") {
    Manager mgr;
    const auto cleanDir = makeCleanTempDir();
    mgr.addPackagePath("", cleanDir);
    mgr.addPackagePath("SingerA", cleanDir);

    // initialize() fails at default context (no packages)
    auto result = mgr.initialize();
    REQUIRE_FALSE(result.hasValue());

    // Both contexts remain Pending (never processed)
    REQUIRE(mgr.contextState(ContextKey("")) == ContextState::Pending);
    REQUIRE(mgr.contextState(ContextKey("SingerA")) == ContextState::Pending);

    // failedContexts is empty (no context reached Failed state)
    REQUIRE(mgr.failedContexts().empty());
}

// failedContexts() returns empty before initialize() processes any context.
TEST_CASE("ic_failedContexts_empty_beforeInit") {
    Manager mgr;
    const auto tempDir = std::filesystem::temp_directory_path();
    mgr.addPackagePath("SingerA", tempDir);
    mgr.addPackagePath("SingerB", tempDir);

    REQUIRE(mgr.failedContexts().empty());
}
