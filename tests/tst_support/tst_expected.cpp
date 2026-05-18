#include "tst_framework.h"

#include <LangCore/Support/Expected.h>
#include <LangCore/Support/Error.h>

#include <string>
#include <vector>
#include <memory>

using namespace LangCore;

TEST_CASE(Expected_DefaultConstructible_Int) {
    Expected<int> e;
    ASSERT_TRUE(e.hasValue());
    ASSERT_EQ(e.take(), 0);
}

TEST_CASE(Expected_ValueConstructor) {
    Expected<int> e(42);
    ASSERT_TRUE(e.hasValue());
    ASSERT_EQ(e.take(), 42);
}

TEST_CASE(Expected_ErrorConstructor) {
    Expected<int> e(Error(Error::RuntimeError, "test error"));
    ASSERT_FALSE(e.hasValue());
    ASSERT_STREQ(e.error().message().c_str(), "test error");
}

TEST_CASE(Expected_CannotConstructFromSuccessError) {
    Expected<int> e(42);
    ASSERT_TRUE(e.hasValue());
    auto err = e.takeError();
    ASSERT_TRUE(err.ok());
}

TEST_CASE(Expected_BoolConversion_Value) {
    Expected<int> e(10);
    ASSERT_TRUE(static_cast<bool>(e));
}

TEST_CASE(Expected_BoolConversion_Error) {
    Expected<int> e(Error(Error::ConfigError, ""));
    ASSERT_FALSE(static_cast<bool>(e));
}

TEST_CASE(Expected_GetValue) {
    Expected<int> e(100);
    ASSERT_EQ(e.get(), 100);
}

TEST_CASE(Expected_DereferenceOperator) {
    Expected<int> e(7);
    ASSERT_EQ(*e, 7);
}

TEST_CASE(Expected_ArrowOperator_String) {
    Expected<std::string> e(std::string("hello"));
    ASSERT_EQ(e->size(), 5u);
}

TEST_CASE(Expected_Take_MoveValue) {
    Expected<std::string> e(std::string("hello"));
    ASSERT_TRUE(e.hasValue());
    std::string val = e.take();
    ASSERT_STREQ(val.c_str(), "hello");
}

TEST_CASE(Expected_ValueOr_HasValue) {
    Expected<int> e(42);
    ASSERT_EQ(e.valueOr(99), 42);
}

TEST_CASE(Expected_ValueOr_NoValue) {
    Expected<int> e(Error(Error::RuntimeError, ""));
    ASSERT_EQ(e.valueOr(99), 99);
}

TEST_CASE(Expected_MoveConstructor_Value) {
    Expected<std::string> e1(std::string("hello"));
    Expected<std::string> e2(std::move(e1));
    ASSERT_TRUE(e2.hasValue());
    ASSERT_STREQ(e2.take().c_str(), "hello");
}

TEST_CASE(Expected_MoveConstructor_Error) {
    Expected<int> e1(Error(Error::ConfigError, "test"));
    Expected<int> e2(std::move(e1));
    ASSERT_FALSE(e2.hasValue());
    ASSERT_STREQ(e2.error().message().c_str(), "test");
}

TEST_CASE(Expected_MoveAssignment_Value) {
    Expected<std::string> e1(std::string("hello"));
    Expected<std::string> e2;
    e2 = std::move(e1);
    ASSERT_TRUE(e2.hasValue());
    ASSERT_STREQ(e2.take().c_str(), "hello");
}

TEST_CASE(Expected_MoveAssignment_Error) {
    Expected<int> e1(Error(Error::RuntimeError, "moved"));
    Expected<int> e2;
    e2 = std::move(e1);
    ASSERT_FALSE(e2.hasValue());
    ASSERT_STREQ(e2.error().message().c_str(), "moved");
}

TEST_CASE(Expected_CrossType_MoveConstruct) {
    Expected<int> e1(42);
    Expected<long> e2(std::move(e1));
    ASSERT_TRUE(e2.hasValue());
    ASSERT_EQ(e2.take(), 42L);
}

TEST_CASE(Expected_VectorOfString) {
    std::vector<std::string> expected_vec = {"a", "b", "c"};
    Expected<std::vector<std::string>> e(std::move(expected_vec));
    ASSERT_TRUE(e.hasValue());
    ASSERT_EQ(e->size(), 3u);
}

TEST_CASE(Expected_UniquePtr) {
    auto ptr = std::make_unique<int>(42);
    Expected<std::unique_ptr<int>> e(std::move(ptr));
    ASSERT_TRUE(e.hasValue());
    ASSERT_EQ(*e.take(), 42);
}

TEST_CASE(Expected_ConstGet) {
    const Expected<int> e(55);
    ASSERT_EQ(e.get(), 55);
}

TEST_CASE(Expected_ConstDereference) {
    const Expected<int> e(33);
    ASSERT_EQ(*e, 33);
}

TEST_CASE(Expected_ConstArrow) {
    const Expected<std::string> e(std::string("world"));
    ASSERT_EQ(e->size(), 5u);
}

TEST_CASE(Expected_Void_DefaultConstruct) {
    Expected<void> e;
    ASSERT_TRUE(e.hasValue());
}

TEST_CASE(Expected_Void_ErrorConstruct) {
    Expected<void> e(Error(Error::RuntimeError, "void error"));
    ASSERT_FALSE(e.hasValue());
    ASSERT_STREQ(e.error().message().c_str(), "void error");
}

TEST_CASE(Expected_Void_BoolConversion) {
    Expected<void> e;
    ASSERT_TRUE(static_cast<bool>(e));
}

TEST_CASE(Expected_Void_BoolConversion_Error) {
    Expected<void> e(Error(Error::ConfigError, ""));
    ASSERT_FALSE(static_cast<bool>(e));
}

TEST_CASE(Expected_Void_MoveConstruct_Value) {
    Expected<void> e1;
    Expected<void> e2(std::move(e1));
    ASSERT_TRUE(e2.hasValue());
}

TEST_CASE(Expected_Void_MoveConstruct_Error) {
    Expected<void> e1(Error(Error::RuntimeError, "moved void"));
    Expected<void> e2(std::move(e1));
    ASSERT_FALSE(e2.hasValue());
    ASSERT_STREQ(e2.error().message().c_str(), "moved void");
}

TEST_CASE(Expected_Void_MoveAssignment_Value) {
    Expected<void> e1;
    Expected<void> e2(Error(Error::ConfigError, ""));
    e2 = std::move(e1);
    ASSERT_TRUE(e2.hasValue());
}

TEST_CASE(Expected_Void_MoveAssignment_Error) {
    Expected<void> e1(Error(Error::RuntimeError, "assigned error"));
    Expected<void> e2;
    e2 = std::move(e1);
    ASSERT_FALSE(e2.hasValue());
    ASSERT_STREQ(e2.error().message().c_str(), "assigned error");
}

TEST_CASE(Expected_Void_TakeError_Value) {
    Expected<void> e;
    auto err = e.takeError();
    ASSERT_TRUE(err.ok());
}

TEST_CASE(Expected_Void_TakeError_Error) {
    Expected<void> e(Error(Error::ValidationError, "checked"));
    auto err = e.takeError();
    ASSERT_STREQ(err.message().c_str(), "checked");
}

TEST_CASE(Expected_ValueOr_ConstRef) {
    Expected<std::string> e(Error(Error::ConfigError, "no value"));
    std::string defaultVal = "fallback";
    ASSERT_STREQ(e.valueOr(defaultVal).c_str(), "fallback");
}

TEST_CASE(Expected_ValueOr_Rvalue) {
    Expected<std::string> e(Error(Error::RuntimeError, "no value"));
    ASSERT_STREQ(std::move(e).valueOr(std::string("fallback")).c_str(), "fallback");
}

TEST_CASE(Expected_Error_ConstRef) {
    Expected<int> e(Error(Error::ConfigError, "const error get"));
    const auto &err = e.error();
    ASSERT_STREQ(err.message().c_str(), "const error get");
}