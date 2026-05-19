#include "catch.hpp"

#include <LangCore/Support/Error.h>

using namespace LangCore;

TEST_CASE("Error DefaultConstructor") {
    Error e;
    REQUIRE(e.type() == Error::Success);
    REQUIRE(e.ok());
}

TEST_CASE("Error SuccessType") {
    Error e(Error::Success);
    REQUIRE(e.ok());
    REQUIRE(e.type() == Error::Success);
}

TEST_CASE("Error ConfigError") {
    Error e(Error::ConfigError, "invalid config");
    REQUIRE(e.type() == Error::ConfigError);
    REQUIRE_FALSE(e.ok());
    REQUIRE(e.message() == "invalid config");
}

TEST_CASE("Error FileSystemError") {
    Error e(Error::FileSystemError, "file not found");
    REQUIRE(e.type() == Error::FileSystemError);
    REQUIRE_FALSE(e.ok());
}

TEST_CASE("Error DependencyError") {
    Error e(Error::DependencyError, "cycle detected");
    REQUIRE(e.type() == Error::DependencyError);
    REQUIRE_FALSE(e.ok());
}

TEST_CASE("Error RuntimeError") {
    Error e(Error::RuntimeError, "unexpected state");
    REQUIRE(e.type() == Error::RuntimeError);
    REQUIRE_FALSE(e.ok());
}

TEST_CASE("Error NotImplementedError") {
    Error e(Error::NotImplementedError, "feature unavailable");
    REQUIRE(e.type() == Error::NotImplementedError);
    REQUIRE_FALSE(e.ok());
}

TEST_CASE("Error InitializationError") {
    Error e(Error::InitializationError, "init failed");
    REQUIRE(e.type() == Error::InitializationError);
    REQUIRE_FALSE(e.ok());
}

TEST_CASE("Error ValidationError") {
    Error e(Error::ValidationError, "validation failed");
    REQUIRE(e.type() == Error::ValidationError);
    REQUIRE_FALSE(e.ok());
}

TEST_CASE("Error NullPointerError") {
    Error e(Error::NullPointerError, "null pointer access");
    REQUIRE(e.type() == Error::NullPointerError);
    REQUIRE_FALSE(e.ok());
}

TEST_CASE("Error IndexError") {
    Error e(Error::IndexError, "index out of bounds");
    REQUIRE(e.type() == Error::IndexError);
    REQUIRE_FALSE(e.ok());
}

TEST_CASE("Error TimeoutError") {
    Error e(Error::TimeoutError, "operation timed out");
    REQUIRE(e.type() == Error::TimeoutError);
    REQUIRE_FALSE(e.ok());
}

TEST_CASE("Error TypeOnlyConstructor") {
    Error e(Error::ConfigError);
    REQUIRE(e.type() == Error::ConfigError);
    REQUIRE_FALSE(e.ok());
    REQUIRE_FALSE(e.message().empty());
}

TEST_CASE("Error CharPtrConstructor") {
    Error e(Error::RuntimeError, "test message");
    REQUIRE(e.message() == "test message");
}

TEST_CASE("Error WithSuggestion") {
    Error e(Error::FileSystemError, "not found", "check the path");
    REQUIRE(e.hasSuggestion());
    REQUIRE(e.suggestion() == "check the path");
}

TEST_CASE("Error WithoutSuggestion") {
    Error e(Error::ConfigError, "bad format");
    REQUIRE_FALSE(e.hasSuggestion());
}

TEST_CASE("Error WithContext ChainCall") {
    Error e(Error::RuntimeError, "error occurred");
    e.withContext("test.cpp", 42, "testFunc").withExtra("additional info");

    REQUIRE(e.hasContext());
    REQUIRE(e.context().file == "test.cpp");
    REQUIRE(e.context().line == 42);
    REQUIRE(e.context().function == "testFunc");
    REQUIRE(e.context().extra == "additional info");
}

TEST_CASE("Error WithContext OnlyFile") {
    Error e(Error::ConfigError, "config issue");
    e.withContext("config.cpp", 0, "");

    REQUIRE(e.hasContext());
    REQUIRE(e.context().file == "config.cpp");
    REQUIRE(e.context().line == 0);
}

TEST_CASE("Error SetContext Method") {
    Error e(Error::ValidationError, "invalid data");
    Error::Context ctx;
    ctx.file = "validator.cpp";
    ctx.line = 100;
    ctx.function = "validate";
    ctx.extra = "field=name";
    e.setContext(ctx);

    REQUIRE(e.hasContext());
    REQUIRE(e.context().file == "validator.cpp");
    REQUIRE(e.context().line == 100);
    REQUIRE(e.context().extra == "field=name");
}

TEST_CASE("Error NoContext") {
    Error e(Error::RuntimeError, "simple error");
    REQUIRE_FALSE(e.hasContext());
}

TEST_CASE("Error FullMessage Basic") {
    Error e(Error::ConfigError, "missing required field");
    auto msg = e.fullMessage();
    REQUIRE(msg == "missing required field");
}

TEST_CASE("Error FullMessage WithContext") {
    Error e(Error::RuntimeError, "process failed");
    e.withContext("worker.cpp", 55, "doWork");

    auto msg = e.fullMessage();
    REQUIRE(msg.find("at") != std::string::npos);
    REQUIRE(msg.find("worker.cpp:55") != std::string::npos);
    REQUIRE(msg.find("doWork") != std::string::npos);
}

TEST_CASE("Error FullMessage WithSuggestion") {
    Error e(Error::FileSystemError, "not found", "verify the path is correct");

    auto msg = e.fullMessage();
    REQUIRE(msg.find("suggestion:") != std::string::npos);
    REQUIRE(msg.find("verify the path is correct") != std::string::npos);
}

TEST_CASE("Error FullMessage ContextAndSuggestion") {
    Error e(Error::ValidationError, "value out of range");
    e.withContext("check.cpp", 10, "checkRange").withExtra("actual=999");

    auto msg = e.fullMessage();
    REQUIRE(msg.find("at") != std::string::npos);
    REQUIRE(msg.find("check.cpp:10") != std::string::npos);
    REQUIRE(msg.find("extra: actual=999") != std::string::npos);
}

TEST_CASE("Error What") {
    Error e(Error::RuntimeError, "test error message");
    REQUIRE(std::string(e.what()) == "test error message");
}

TEST_CASE("Error SuccessStatic") {
    auto e = Error::success();
    REQUIRE(e.ok());
    REQUIRE(e.type() == Error::Success);
}

TEST_CASE("Error AllElevenCodes Unique") {
    Error codes[] = {
        Error(Error::Success),
        Error(Error::ConfigError),
        Error(Error::FileSystemError),
        Error(Error::DependencyError),
        Error(Error::RuntimeError),
        Error(Error::NotImplementedError),
        Error(Error::InitializationError),
        Error(Error::ValidationError),
        Error(Error::NullPointerError),
        Error(Error::IndexError),
        Error(Error::TimeoutError),
    };

    REQUIRE(codes[0].type() == 0);
    REQUIRE(codes[1].type() == 1);
    REQUIRE(codes[10].type() == Error::TimeoutError);

    for (int i = 1; i <= 10; i++) {
        REQUIRE_FALSE(codes[i].ok());
    }
}