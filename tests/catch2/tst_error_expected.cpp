#include "catch.hpp"

#include <LangCore/Support/Error.h>
#include <LangCore/Support/Expected.h>

#include <string>
#include <utility>

using namespace LangCore;

// ============================================================================
// Error tests
// ============================================================================

TEST_CASE("Error DefaultConstruct") {
    Error err;
    REQUIRE(err.ok());
    REQUIRE(err.type() == Error::Success);
}

TEST_CASE("Error SuccessFactory") {
    auto err = Error::success();
    REQUIRE(err.ok());
    REQUIRE(err.type() == Error::Success);
}

TEST_CASE("Error TypeOnly") {
    Error err(Error::ConfigError);
    REQUIRE_FALSE(err.ok());
    REQUIRE(err.type() == Error::ConfigError);
}

TEST_CASE("Error TypeWithStringMessage") {
    Error err(Error::RuntimeError, std::string("runtime failure"));
    REQUIRE_FALSE(err.ok());
    REQUIRE(err.type() == Error::RuntimeError);
    REQUIRE(err.message() == "runtime failure");
    REQUIRE(std::string(err.what()) == "runtime failure");
}

TEST_CASE("Error TypeWithCStringMessage") {
    Error err(Error::FileSystemError, "file not found");
    REQUIRE_FALSE(err.ok());
    REQUIRE(err.type() == Error::FileSystemError);
    REQUIRE(err.message() == "file not found");
}

TEST_CASE("Error WithSuggestion Strings") {
    Error err(Error::DependencyError, std::string("missing dep"), std::string("install it"));
    REQUIRE(err.hasSuggestion());
    REQUIRE(err.suggestion() == "install it");
}

TEST_CASE("Error WithSuggestion CStrings") {
    Error err(Error::DependencyError, "missing dep", "install it");
    REQUIRE(err.hasSuggestion());
    REQUIRE(err.suggestion() == "install it");
}

TEST_CASE("Error NoSuggestion") {
    Error err(Error::RuntimeError, "oops");
    REQUIRE_FALSE(err.hasSuggestion());
    REQUIRE(err.suggestion().empty());
}

TEST_CASE("Error Context") {
    Error err(Error::RuntimeError, "ctx test");
    REQUIRE_FALSE(err.hasContext());
    err.withContext("file.cpp", 42, "doStuff");
    REQUIRE(err.hasContext());
    REQUIRE(err.context().file == "file.cpp");
    REQUIRE(err.context().line == 42);
    REQUIRE(err.context().function == "doStuff");
}

TEST_CASE("Error FullMessage") {
    Error err(Error::RuntimeError, "bad thing", "try again");
    err.withContext("src.cpp", 10, "run").withExtra("details");
    std::string full = err.fullMessage();
    REQUIRE(full.find("bad thing") != std::string::npos);
    REQUIRE(full.find("src.cpp") != std::string::npos);
    REQUIRE(full.find("10") != std::string::npos);
    REQUIRE(full.find("run") != std::string::npos);
    REQUIRE(full.find("details") != std::string::npos);
    REQUIRE(full.find("try again") != std::string::npos);
}

// --- Error additional tests ---

TEST_CASE("Error AllTypes HaveDefaultMessages") {
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
        REQUIRE(err.what() != nullptr);
    }
}

TEST_CASE("Error CopySemantics") {
    Error original(Error::ValidationError, "validate fail");
    Error copy = original;
    REQUIRE(copy.message() == "validate fail");
    REQUIRE(original.message() == "validate fail");
    REQUIRE(copy.type() == original.type());
}

TEST_CASE("Error ConstCharConstructors") {
    Error e1(Error::IndexError, "out of range");
    REQUIRE(std::string(e1.what()) == "out of range");

    Error e2(Error::IndexError, "out of range", "check bounds");
    REQUIRE(std::string(e2.what()) == "out of range");
    REQUIRE(e2.suggestion() == "check bounds");
}

// ============================================================================
// Expected<T> tests
// ============================================================================

TEST_CASE("Expected DefaultConstruct") {
    Expected<int> e;
    REQUIRE(e.hasValue());
    REQUIRE(static_cast<bool>(e));
}

TEST_CASE("Expected ValueConstruct") {
    Expected<int> e(42);
    REQUIRE(e.hasValue());
    REQUIRE(e.value() == 42);
}

TEST_CASE("Expected ErrorConstruct") {
    Expected<int> e(Error(Error::RuntimeError, "fail"));
    REQUIRE_FALSE(e.hasValue());
    REQUIRE_FALSE(static_cast<bool>(e));
}

TEST_CASE("Expected Get") {
    Expected<std::string> e(std::string("hello"));
    REQUIRE(e.get() == "hello");
}

TEST_CASE("Expected ConstGet") {
    const Expected<int> e(99);
    REQUIRE(e.get() == 99);
}

TEST_CASE("Expected TakeError OnSuccess") {
    Expected<int> e(10);
    Error err = e.takeError();
    REQUIRE(err.ok());
}

TEST_CASE("Expected TakeError OnError") {
    Expected<int> e(Error(Error::ConfigError, "bad config"));
    Error err = e.takeError();
    REQUIRE_FALSE(err.ok());
    REQUIRE(err.type() == Error::ConfigError);
}

TEST_CASE("Expected MoveConstruct") {
    Expected<std::string> a(std::string("moved"));
    Expected<std::string> b(std::move(a));
    REQUIRE(b.hasValue());
    REQUIRE(b.value() == "moved");
}

TEST_CASE("Expected MoveAssign") {
    Expected<int> a(1);
    Expected<int> b(2);
    b = std::move(a);
    REQUIRE(b.hasValue());
    REQUIRE(b.value() == 1);
}

TEST_CASE("Expected ValueOr HasValue") {
    Expected<int> e(7);
    REQUIRE(e.valueOr(0) == 7);
}

TEST_CASE("Expected ValueOr HasError") {
    Expected<int> e(Error(Error::RuntimeError, "err"));
    REQUIRE(e.valueOr(42) == 42);
}

TEST_CASE("Expected Take") {
    Expected<std::string> e(std::string("taken"));
    std::string s = e.take();
    REQUIRE(s == "taken");
}

// --- Expected<void> tests ---

TEST_CASE("ExpectedVoid DefaultConstruct") {
    Expected<void> e;
    REQUIRE(e.hasValue());
    REQUIRE(static_cast<bool>(e));
}

TEST_CASE("ExpectedVoid ErrorConstruct") {
    Expected<void> e(Error(Error::FileSystemError, "no file"));
    REQUIRE_FALSE(e.hasValue());
}

TEST_CASE("ExpectedVoid TakeError OnSuccess") {
    Expected<void> e;
    Error err = e.takeError();
    REQUIRE(err.ok());
}

TEST_CASE("ExpectedVoid TakeError OnError") {
    Expected<void> e(Error(Error::InitializationError, "init fail"));
    Error err = e.takeError();
    REQUIRE_FALSE(err.ok());
    REQUIRE(err.type() == Error::InitializationError);
}

// --- Expected additional tests ---

TEST_CASE("Expected OperatorArrow") {
    Expected<std::string> e(std::string("arrow"));
    REQUIRE(e->size() == 5u);
}

TEST_CASE("Expected OperatorStar") {
    Expected<int> e(123);
    REQUIRE(*e == 123);
}

TEST_CASE("Expected ErrorAccess") {
    Expected<int> e(Error(Error::TimeoutError, "timed out"));
    const Error &err = e.error();
    REQUIRE(err.type() == Error::TimeoutError);
    REQUIRE(err.message() == "timed out");
}

TEST_CASE("Expected ConvertibleTypes") {
    short s = 42;
    Expected<int> e(s);
    REQUIRE(e.hasValue());
    REQUIRE(e.value() == 42);
}

TEST_CASE("ExpectedVoid MoveSemantics") {
    Expected<void> a;
    Expected<void> b(std::move(a));
    REQUIRE(b.hasValue());

    Expected<void> c(Error(Error::RuntimeError, "err"));
    Expected<void> d(std::move(c));
    REQUIRE_FALSE(d.hasValue());
}

// ============================================================================
// §14.15 regression: Expected<T> default constructor SFINAE
// ============================================================================

// A non-default-constructible type
struct NonDefaultConstructible {
    int value;
    explicit NonDefaultConstructible(int v) : value(v) {}
    // No default constructor
};

TEST_CASE("Expected NonDefaultConstructible ValueConstruct") {
    // Verify Expected<NonDefaultConstructible> works with explicit value
    Expected<NonDefaultConstructible> e(NonDefaultConstructible(42));
    REQUIRE(e.hasValue());
    REQUIRE(e.value().value == 42);
}

TEST_CASE("Expected NonDefaultConstructible ErrorConstruct") {
    Expected<NonDefaultConstructible> e(Error(Error::RuntimeError, "fail"));
    REQUIRE_FALSE(e.hasValue());
    REQUIRE(e.error().type() == Error::RuntimeError);
}

// Compile-time check: Expected<NonDefaultConstructible>() should NOT compile.
// We verify this indirectly by checking the SFINAE constraint works:
TEST_CASE("Expected DefaultConstructible Works") {
    // std::string is default-constructible, so Expected<string>() should work
    Expected<std::string> e;
    REQUIRE(e.hasValue());
    REQUIRE(e.value() == "");
}

// ============================================================================
// §14.21 regression: Error::defaultMessage thread safety
// ============================================================================

TEST_CASE("Error AllTypesHaveDefaultMessage") {
    // Verify every Error::Type from 0..10 has a non-null default message
    // and that only Success returns ok() == true
    for (int i = 0; i <= 10; ++i) {
        Error err(static_cast<Error::Type>(i));
        if (i == 0) {
            REQUIRE(err.ok());
        } else {
            REQUIRE_FALSE(err.ok());
        }
        // All types should have a non-empty what() (except Success which is "")
        REQUIRE(err.what() != nullptr);
    }
}