#include "tst_framework.h"

#include <LangCore/Support/Error.h>
#include <LangCore/Support/Expected.h>

#include <string>
#include <utility>

using namespace LangCore;

// ============================================================================
// Error tests
// ============================================================================

TEST_CASE(Error_DefaultConstruct) {
    Error err;
    ASSERT_TRUE(err.ok());
    ASSERT_EQ(err.type(), Error::Success);
}

TEST_CASE(Error_SuccessFactory) {
    auto err = Error::success();
    ASSERT_TRUE(err.ok());
    ASSERT_EQ(err.type(), Error::Success);
}

TEST_CASE(Error_TypeOnly) {
    Error err(Error::ConfigError);
    ASSERT_FALSE(err.ok());
    ASSERT_EQ(err.type(), Error::ConfigError);
}

TEST_CASE(Error_TypeWithStringMessage) {
    Error err(Error::RuntimeError, std::string("runtime failure"));
    ASSERT_FALSE(err.ok());
    ASSERT_EQ(err.type(), Error::RuntimeError);
    ASSERT_STREQ(err.message().c_str(), "runtime failure");
    ASSERT_STREQ(err.what(), "runtime failure");
}

TEST_CASE(Error_TypeWithCStringMessage) {
    Error err(Error::FileSystemError, "file not found");
    ASSERT_FALSE(err.ok());
    ASSERT_EQ(err.type(), Error::FileSystemError);
    ASSERT_STREQ(err.message().c_str(), "file not found");
}

TEST_CASE(Error_WithSuggestion_Strings) {
    Error err(Error::DependencyError, std::string("missing dep"), std::string("install it"));
    ASSERT_TRUE(err.hasSuggestion());
    ASSERT_STREQ(err.suggestion().c_str(), "install it");
}

TEST_CASE(Error_WithSuggestion_CStrings) {
    Error err(Error::DependencyError, "missing dep", "install it");
    ASSERT_TRUE(err.hasSuggestion());
    ASSERT_STREQ(err.suggestion().c_str(), "install it");
}

TEST_CASE(Error_NoSuggestion) {
    Error err(Error::RuntimeError, "oops");
    ASSERT_FALSE(err.hasSuggestion());
    ASSERT_TRUE(err.suggestion().empty());
}

TEST_CASE(Error_Context) {
    Error err(Error::RuntimeError, "ctx test");
    ASSERT_FALSE(err.hasContext());
    err.withContext("file.cpp", 42, "doStuff");
    ASSERT_TRUE(err.hasContext());
    ASSERT_STREQ(err.context().file.c_str(), "file.cpp");
    ASSERT_EQ(err.context().line, 42);
    ASSERT_STREQ(err.context().function.c_str(), "doStuff");
}

TEST_CASE(Error_FullMessage) {
    Error err(Error::RuntimeError, "bad thing", "try again");
    err.withContext("src.cpp", 10, "run").withExtra("details");
    std::string full = err.fullMessage();
    ASSERT_TRUE(full.find("bad thing") != std::string::npos);
    ASSERT_TRUE(full.find("src.cpp") != std::string::npos);
    ASSERT_TRUE(full.find("10") != std::string::npos);
    ASSERT_TRUE(full.find("run") != std::string::npos);
    ASSERT_TRUE(full.find("details") != std::string::npos);
    ASSERT_TRUE(full.find("try again") != std::string::npos);
}

// --- Error additional tests ---

TEST_CASE(Error_AllTypes_HaveDefaultMessages) {
    Error::Type types[] = {
        Error::Success,
        Error::ConfigError,
        Error::FileSystemError,
        Error::DependencyError,
        Error::RuntimeError,
        Error::NotImplementedError,
        Error::InitializationError,
        Error::ValidationError,
        Error::NullPointerError,
        Error::IndexError,
        Error::TimeoutError,
    };
    for (auto t : types) {
        Error err(t);
        ASSERT_TRUE(err.what() != nullptr);
    }
}

TEST_CASE(Error_CopySemantics) {
    Error original(Error::ValidationError, "validate fail");
    Error copy = original; // NOLINT
    ASSERT_STREQ(copy.message().c_str(), "validate fail");
    ASSERT_STREQ(original.message().c_str(), "validate fail");
    ASSERT_EQ(copy.type(), original.type());
}

TEST_CASE(Error_ConstCharConstructors) {
    Error e1(Error::IndexError, "out of range");
    ASSERT_STREQ(e1.what(), "out of range");

    Error e2(Error::IndexError, "out of range", "check bounds");
    ASSERT_STREQ(e2.what(), "out of range");
    ASSERT_STREQ(e2.suggestion().c_str(), "check bounds");
}

// ============================================================================
// Expected<T> tests
// ============================================================================

TEST_CASE(Expected_DefaultConstruct) {
    Expected<int> e;
    ASSERT_TRUE(e.hasValue());
    ASSERT_TRUE(static_cast<bool>(e));
}

TEST_CASE(Expected_ValueConstruct) {
    Expected<int> e(42);
    ASSERT_TRUE(e.hasValue());
    ASSERT_EQ(e.value(), 42);
}

TEST_CASE(Expected_ErrorConstruct) {
    Expected<int> e(Error(Error::RuntimeError, "fail"));
    ASSERT_FALSE(e.hasValue());
    ASSERT_FALSE(static_cast<bool>(e));
}

TEST_CASE(Expected_Get) {
    Expected<std::string> e(std::string("hello"));
    ASSERT_STREQ(e.get().c_str(), "hello");
}

TEST_CASE(Expected_ConstGet) {
    const Expected<int> e(99);
    ASSERT_EQ(e.get(), 99);
}

TEST_CASE(Expected_TakeError_OnSuccess) {
    Expected<int> e(10);
    Error err = e.takeError();
    ASSERT_TRUE(err.ok());
}

TEST_CASE(Expected_TakeError_OnError) {
    Expected<int> e(Error(Error::ConfigError, "bad config"));
    Error err = e.takeError();
    ASSERT_FALSE(err.ok());
    ASSERT_EQ(err.type(), Error::ConfigError);
}

TEST_CASE(Expected_MoveConstruct) {
    Expected<std::string> a(std::string("moved"));
    Expected<std::string> b(std::move(a));
    ASSERT_TRUE(b.hasValue());
    ASSERT_STREQ(b.value().c_str(), "moved");
}

TEST_CASE(Expected_MoveAssign) {
    Expected<int> a(1);
    Expected<int> b(2);
    b = std::move(a);
    ASSERT_TRUE(b.hasValue());
    ASSERT_EQ(b.value(), 1);
}

TEST_CASE(Expected_ValueOr_HasValue) {
    Expected<int> e(7);
    ASSERT_EQ(e.valueOr(0), 7);
}

TEST_CASE(Expected_ValueOr_HasError) {
    Expected<int> e(Error(Error::RuntimeError, "err"));
    ASSERT_EQ(e.valueOr(42), 42);
}

TEST_CASE(Expected_Take) {
    Expected<std::string> e(std::string("taken"));
    std::string s = e.take();
    ASSERT_STREQ(s.c_str(), "taken");
}

// --- Expected<void> tests ---

TEST_CASE(ExpectedVoid_DefaultConstruct) {
    Expected<void> e;
    ASSERT_TRUE(e.hasValue());
    ASSERT_TRUE(static_cast<bool>(e));
}

TEST_CASE(ExpectedVoid_ErrorConstruct) {
    Expected<void> e(Error(Error::FileSystemError, "no file"));
    ASSERT_FALSE(e.hasValue());
}

TEST_CASE(ExpectedVoid_TakeError_OnSuccess) {
    Expected<void> e;
    Error err = e.takeError();
    ASSERT_TRUE(err.ok());
}

TEST_CASE(ExpectedVoid_TakeError_OnError) {
    Expected<void> e(Error(Error::InitializationError, "init fail"));
    Error err = e.takeError();
    ASSERT_FALSE(err.ok());
    ASSERT_EQ(err.type(), Error::InitializationError);
}

// --- Expected additional tests ---

TEST_CASE(Expected_OperatorArrow) {
    Expected<std::string> e(std::string("arrow"));
    ASSERT_EQ(e->size(), 5u);
}

TEST_CASE(Expected_OperatorStar) {
    Expected<int> e(123);
    ASSERT_EQ(*e, 123);
}

TEST_CASE(Expected_ErrorAccess) {
    Expected<int> e(Error(Error::TimeoutError, "timed out"));
    const Error &err = e.error();
    ASSERT_EQ(err.type(), Error::TimeoutError);
    ASSERT_STREQ(err.message().c_str(), "timed out");
}

TEST_CASE(Expected_ConvertibleTypes) {
    short s = 42;
    Expected<int> e(s);
    ASSERT_TRUE(e.hasValue());
    ASSERT_EQ(e.value(), 42);
}

TEST_CASE(ExpectedVoid_MoveSemantics) {
    Expected<void> a;
    Expected<void> b(std::move(a));
    ASSERT_TRUE(b.hasValue());

    Expected<void> c(Error(Error::RuntimeError, "err"));
    Expected<void> d(std::move(c));
    ASSERT_FALSE(d.hasValue());
}
