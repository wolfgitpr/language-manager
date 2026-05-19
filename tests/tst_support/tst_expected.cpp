#include "catch.hpp"

#include <LangCore/Support/Expected.h>
#include <LangCore/Support/Error.h>

#include <string>
#include <vector>
#include <memory>

using namespace LangCore;

TEST_CASE("Expected DefaultConstructible Int") {
    Expected<int> e;
    REQUIRE(e.hasValue());
    REQUIRE(e.take() == 0);
}

TEST_CASE("Expected ValueConstructor") {
    Expected<int> e(42);
    REQUIRE(e.hasValue());
    REQUIRE(e.take() == 42);
}

TEST_CASE("Expected ErrorConstructor") {
    Expected<int> e(Error(Error::RuntimeError, "test error"));
    REQUIRE_FALSE(e.hasValue());
    REQUIRE(e.error().message() == "test error");
}

TEST_CASE("Expected CannotConstructFromSuccessError") {
    Expected<int> e(42);
    REQUIRE(e.hasValue());
    auto err = e.takeError();
    REQUIRE(err.ok());
}

TEST_CASE("Expected BoolConversion Value") {
    Expected<int> e(10);
    REQUIRE(static_cast<bool>(e));
}

TEST_CASE("Expected BoolConversion Error") {
    Expected<int> e(Error(Error::ConfigError, ""));
    REQUIRE_FALSE(static_cast<bool>(e));
}

TEST_CASE("Expected GetValue") {
    Expected<int> e(100);
    REQUIRE(e.get() == 100);
}

TEST_CASE("Expected DereferenceOperator") {
    Expected<int> e(7);
    REQUIRE(*e == 7);
}

TEST_CASE("Expected ArrowOperator String") {
    Expected<std::string> e(std::string("hello"));
    REQUIRE(e->size() == 5u);
}

TEST_CASE("Expected Take MoveValue") {
    Expected<std::string> e(std::string("hello"));
    REQUIRE(e.hasValue());
    std::string val = e.take();
    REQUIRE(val == "hello");
}

TEST_CASE("Expected ValueOr HasValue") {
    Expected<int> e(42);
    REQUIRE(e.valueOr(99) == 42);
}

TEST_CASE("Expected ValueOr NoValue") {
    Expected<int> e(Error(Error::RuntimeError, ""));
    REQUIRE(e.valueOr(99) == 99);
}

TEST_CASE("Expected MoveConstructor Value") {
    Expected<std::string> e1(std::string("hello"));
    Expected<std::string> e2(std::move(e1));
    REQUIRE(e2.hasValue());
    REQUIRE(e2.take() == "hello");
}

TEST_CASE("Expected MoveConstructor Error") {
    Expected<int> e1(Error(Error::ConfigError, "test"));
    Expected<int> e2(std::move(e1));
    REQUIRE_FALSE(e2.hasValue());
    REQUIRE(e2.error().message() == "test");
}

TEST_CASE("Expected MoveAssignment Value") {
    Expected<std::string> e1(std::string("hello"));
    Expected<std::string> e2;
    e2 = std::move(e1);
    REQUIRE(e2.hasValue());
    REQUIRE(e2.take() == "hello");
}

TEST_CASE("Expected MoveAssignment Error") {
    Expected<int> e1(Error(Error::RuntimeError, "moved"));
    Expected<int> e2;
    e2 = std::move(e1);
    REQUIRE_FALSE(e2.hasValue());
    REQUIRE(e2.error().message() == "moved");
}

TEST_CASE("Expected CrossType MoveConstruct") {
    Expected<int> e1(42);
    Expected<long> e2(std::move(e1));
    REQUIRE(e2.hasValue());
    REQUIRE(e2.take() == 42L);
}

TEST_CASE("Expected VectorOfString") {
    std::vector<std::string> expected_vec = {"a", "b", "c"};
    Expected<std::vector<std::string>> e(std::move(expected_vec));
    REQUIRE(e.hasValue());
    REQUIRE(e->size() == 3u);
}

TEST_CASE("Expected UniquePtr") {
    auto ptr = std::make_unique<int>(42);
    Expected<std::unique_ptr<int>> e(std::move(ptr));
    REQUIRE(e.hasValue());
    REQUIRE(*e.take() == 42);
}

TEST_CASE("Expected ConstGet") {
    const Expected<int> e(55);
    REQUIRE(e.get() == 55);
}

TEST_CASE("Expected ConstDereference") {
    const Expected<int> e(33);
    REQUIRE(*e == 33);
}

TEST_CASE("Expected ConstArrow") {
    const Expected<std::string> e(std::string("world"));
    REQUIRE(e->size() == 5u);
}

TEST_CASE("Expected Void DefaultConstruct") {
    Expected<void> e;
    REQUIRE(e.hasValue());
}

TEST_CASE("Expected Void ErrorConstruct") {
    Expected<void> e(Error(Error::RuntimeError, "void error"));
    REQUIRE_FALSE(e.hasValue());
    REQUIRE(e.error().message() == "void error");
}

TEST_CASE("Expected Void BoolConversion") {
    Expected<void> e;
    REQUIRE(static_cast<bool>(e));
}

TEST_CASE("Expected Void BoolConversion Error") {
    Expected<void> e(Error(Error::ConfigError, ""));
    REQUIRE_FALSE(static_cast<bool>(e));
}

TEST_CASE("Expected Void MoveConstruct Value") {
    Expected<void> e1;
    Expected<void> e2(std::move(e1));
    REQUIRE(e2.hasValue());
}

TEST_CASE("Expected Void MoveConstruct Error") {
    Expected<void> e1(Error(Error::RuntimeError, "moved void"));
    Expected<void> e2(std::move(e1));
    REQUIRE_FALSE(e2.hasValue());
    REQUIRE(e2.error().message() == "moved void");
}

TEST_CASE("Expected Void MoveAssignment Value") {
    Expected<void> e1;
    Expected<void> e2(Error(Error::ConfigError, ""));
    e2 = std::move(e1);
    REQUIRE(e2.hasValue());
}

TEST_CASE("Expected Void MoveAssignment Error") {
    Expected<void> e1(Error(Error::RuntimeError, "assigned error"));
    Expected<void> e2;
    e2 = std::move(e1);
    REQUIRE_FALSE(e2.hasValue());
    REQUIRE(e2.error().message() == "assigned error");
}

TEST_CASE("Expected Void TakeError Value") {
    Expected<void> e;
    auto err = e.takeError();
    REQUIRE(err.ok());
}

TEST_CASE("Expected Void TakeError Error") {
    Expected<void> e(Error(Error::ValidationError, "checked"));
    auto err = e.takeError();
    REQUIRE(err.message() == "checked");
}

TEST_CASE("Expected ValueOr ConstRef") {
    Expected<std::string> e(Error(Error::ConfigError, "no value"));
    std::string defaultVal = "fallback";
    REQUIRE(e.valueOr(defaultVal) == "fallback");
}

TEST_CASE("Expected ValueOr Rvalue") {
    Expected<std::string> e(Error(Error::RuntimeError, "no value"));
    REQUIRE(std::move(e).valueOr(std::string("fallback")) == "fallback");
}

TEST_CASE("Expected Error ConstRef") {
    Expected<int> e(Error(Error::ConfigError, "const error get"));
    const auto &err = e.error();
    REQUIRE(err.message() == "const error get");
}