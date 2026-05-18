#include "tst_framework.h"

#include <LangCore/Support/Error.h>

using namespace LangCore;

TEST_CASE(Error_DefaultConstructor) {
    Error e;
    ASSERT_EQ(e.type(), Error::Success);
    ASSERT_TRUE(e.ok());
}

TEST_CASE(Error_SuccessType) {
    Error e(Error::Success);
    ASSERT_TRUE(e.ok());
    ASSERT_EQ(e.type(), Error::Success);
}

TEST_CASE(Error_ConfigError) {
    Error e(Error::ConfigError, "invalid config");
    ASSERT_EQ(e.type(), Error::ConfigError);
    ASSERT_FALSE(e.ok());
    ASSERT_STREQ(e.message().c_str(), "invalid config");
}

TEST_CASE(Error_FileSystemError) {
    Error e(Error::FileSystemError, "file not found");
    ASSERT_EQ(e.type(), Error::FileSystemError);
    ASSERT_FALSE(e.ok());
}

TEST_CASE(Error_DependencyError) {
    Error e(Error::DependencyError, "cycle detected");
    ASSERT_EQ(e.type(), Error::DependencyError);
    ASSERT_FALSE(e.ok());
}

TEST_CASE(Error_RuntimeError) {
    Error e(Error::RuntimeError, "unexpected state");
    ASSERT_EQ(e.type(), Error::RuntimeError);
    ASSERT_FALSE(e.ok());
}

TEST_CASE(Error_NotImplementedError) {
    Error e(Error::NotImplementedError, "feature unavailable");
    ASSERT_EQ(e.type(), Error::NotImplementedError);
    ASSERT_FALSE(e.ok());
}

TEST_CASE(Error_InitializationError) {
    Error e(Error::InitializationError, "init failed");
    ASSERT_EQ(e.type(), Error::InitializationError);
    ASSERT_FALSE(e.ok());
}

TEST_CASE(Error_ValidationError) {
    Error e(Error::ValidationError, "validation failed");
    ASSERT_EQ(e.type(), Error::ValidationError);
    ASSERT_FALSE(e.ok());
}

TEST_CASE(Error_NullPointerError) {
    Error e(Error::NullPointerError, "null pointer access");
    ASSERT_EQ(e.type(), Error::NullPointerError);
    ASSERT_FALSE(e.ok());
}

TEST_CASE(Error_IndexError) {
    Error e(Error::IndexError, "index out of bounds");
    ASSERT_EQ(e.type(), Error::IndexError);
    ASSERT_FALSE(e.ok());
}

TEST_CASE(Error_TimeoutError) {
    Error e(Error::TimeoutError, "operation timed out");
    ASSERT_EQ(e.type(), Error::TimeoutError);
    ASSERT_FALSE(e.ok());
}

TEST_CASE(Error_TypeOnlyConstructor) {
    Error e(Error::ConfigError);
    ASSERT_EQ(e.type(), Error::ConfigError);
    ASSERT_FALSE(e.ok());
    ASSERT_FALSE(e.message().empty());
}

TEST_CASE(Error_CharPtrConstructor) {
    Error e(Error::RuntimeError, "test message");
    ASSERT_STREQ(e.message().c_str(), "test message");
}

TEST_CASE(Error_WithSuggestion) {
    Error e(Error::FileSystemError, "not found", "check the path");
    ASSERT_TRUE(e.hasSuggestion());
    ASSERT_STREQ(e.suggestion().c_str(), "check the path");
}

TEST_CASE(Error_WithoutSuggestion) {
    Error e(Error::ConfigError, "bad format");
    ASSERT_FALSE(e.hasSuggestion());
}

TEST_CASE(Error_WithContext_ChainCall) {
    Error e(Error::RuntimeError, "error occurred");
    e.withContext("test.cpp", 42, "testFunc").withExtra("additional info");

    ASSERT_TRUE(e.hasContext());
    ASSERT_STREQ(e.context().file.c_str(), "test.cpp");
    ASSERT_EQ(e.context().line, 42);
    ASSERT_STREQ(e.context().function.c_str(), "testFunc");
    ASSERT_STREQ(e.context().extra.c_str(), "additional info");
}

TEST_CASE(Error_WithContext_OnlyFile) {
    Error e(Error::ConfigError, "config issue");
    e.withContext("config.cpp", 0, "");

    ASSERT_TRUE(e.hasContext());
    ASSERT_STREQ(e.context().file.c_str(), "config.cpp");
    ASSERT_EQ(e.context().line, 0);
}

TEST_CASE(Error_SetContext_Method) {
    Error e(Error::ValidationError, "invalid data");
    Error::Context ctx;
    ctx.file = "validator.cpp";
    ctx.line = 100;
    ctx.function = "validate";
    ctx.extra = "field=name";
    e.setContext(ctx);

    ASSERT_TRUE(e.hasContext());
    ASSERT_STREQ(e.context().file.c_str(), "validator.cpp");
    ASSERT_EQ(e.context().line, 100);
    ASSERT_STREQ(e.context().extra.c_str(), "field=name");
}

TEST_CASE(Error_NoContext) {
    Error e(Error::RuntimeError, "simple error");
    ASSERT_FALSE(e.hasContext());
}

TEST_CASE(Error_FullMessage_Basic) {
    Error e(Error::ConfigError, "missing required field");
    auto msg = e.fullMessage();
    ASSERT_STREQ(msg.c_str(), "missing required field");
}

TEST_CASE(Error_FullMessage_WithContext) {
    Error e(Error::RuntimeError, "process failed");
    e.withContext("worker.cpp", 55, "doWork");

    auto msg = e.fullMessage();
    ASSERT_TRUE(msg.find("at") != std::string::npos);
    ASSERT_TRUE(msg.find("worker.cpp:55") != std::string::npos);
    ASSERT_TRUE(msg.find("doWork") != std::string::npos);
}

TEST_CASE(Error_FullMessage_WithSuggestion) {
    Error e(Error::FileSystemError, "not found", "verify the path is correct");

    auto msg = e.fullMessage();
    ASSERT_TRUE(msg.find("suggestion:") != std::string::npos);
    ASSERT_TRUE(msg.find("verify the path is correct") != std::string::npos);
}

TEST_CASE(Error_FullMessage_ContextAndSuggestion) {
    Error e(Error::ValidationError, "value out of range");
    e.withContext("check.cpp", 10, "checkRange").withExtra("actual=999");

    auto msg = e.fullMessage();
    ASSERT_TRUE(msg.find("at") != std::string::npos);
    ASSERT_TRUE(msg.find("check.cpp:10") != std::string::npos);
    ASSERT_TRUE(msg.find("extra: actual=999") != std::string::npos);
}

TEST_CASE(Error_What) {
    Error e(Error::RuntimeError, "test error message");
    ASSERT_STREQ(e.what(), "test error message");
}

TEST_CASE(Error_SuccessStatic) {
    auto e = Error::success();
    ASSERT_TRUE(e.ok());
    ASSERT_EQ(e.type(), Error::Success);
}

TEST_CASE(Error_AllElevenCodes_Unique) {
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

    ASSERT_EQ(codes[0].type(), 0);
    ASSERT_EQ(codes[1].type(), 1);
    ASSERT_EQ(codes[10].type(), Error::TimeoutError);

    for (int i = 1; i <= 10; i++) {
        ASSERT_FALSE(codes[i].ok());
    }
}