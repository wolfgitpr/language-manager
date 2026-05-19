#include "catch.hpp"

#include <LangCore/Support/JSON.h>
#include <LangCore/Support/ConfigAccessor.h>

using namespace LangCore;

// ============================================================================
// JsonValue Tests (14 original + 8 new = 22)
// ============================================================================

TEST_CASE("JsonValue DefaultNull") {
    JsonValue v;
    REQUIRE(v.isNull());
    REQUIRE(v.type() == JsonValue::Null);
}

TEST_CASE("JsonValue Bool") {
    JsonValue v(true);
    REQUIRE(v.isBool());
    REQUIRE(v.toBool() == true);

    JsonValue v2(false);
    REQUIRE(v2.toBool() == false);
}

TEST_CASE("JsonValue Double") {
    JsonValue v(3.14);
    REQUIRE(v.isDouble());
    REQUIRE(v.isNumber());
    REQUIRE((v.toDouble() > 3.13 && v.toDouble() < 3.15));
}

TEST_CASE("JsonValue Int") {
    JsonValue v(static_cast<int64_t>(42));
    REQUIRE(v.isInt());
    REQUIRE(v.isNumber());
    REQUIRE(v.toInt() == 42);
}

TEST_CASE("JsonValue UInt") {
    JsonValue v(static_cast<uint64_t>(100));
    REQUIRE(v.isUInt());
    REQUIRE(v.isInt());
    REQUIRE(v.toUInt() == static_cast<uint64_t>(100));
}

TEST_CASE("JsonValue String") {
    JsonValue v(std::string("hello"));
    REQUIRE(v.isString());
    REQUIRE(v.toString() == "hello");
}

TEST_CASE("JsonValue CString") {
    JsonValue v("world");
    REQUIRE(v.isString());
    REQUIRE(v.toString() == "world");
}

TEST_CASE("JsonValue Array") {
    JsonArray arr;
    arr.push_back(JsonValue(1));
    arr.push_back(JsonValue(2));
    arr.push_back(JsonValue(3));
    JsonValue v(arr);
    REQUIRE(v.isArray());
    REQUIRE(v.toArray().size() == static_cast<size_t>(3));
    REQUIRE(v[0].toInt() == 1);
    REQUIRE(v[1].toInt() == 2);
    REQUIRE(v[2].toInt() == 3);
}

TEST_CASE("JsonValue Object") {
    JsonObject obj;
    obj["name"] = JsonValue("test");
    obj["value"] = JsonValue(42);
    JsonValue v(obj);
    REQUIRE(v.isObject());
    REQUIRE(v["name"].toString() == "test");
    REQUIRE(v["value"].toInt() == 42);
}

TEST_CASE("JsonValue Equality") {
    JsonValue a(42);
    JsonValue b(42);
    JsonValue c(99);
    REQUIRE(a == b);
    REQUIRE(a != c);
}

TEST_CASE("JsonValue FromJson") {
    std::string error;
    auto v = JsonValue::fromJson(R"({"key": "value", "num": 123})", false, &error);
    REQUIRE(error.empty());
    REQUIRE(v.isObject());
    REQUIRE(v["key"].toString() == "value");
    REQUIRE(v["num"].toInt() == 123);
}

TEST_CASE("JsonValue FromJsonInvalid") {
    std::string error;
    auto v = JsonValue::fromJson("{invalid json", false, &error);
    REQUIRE_FALSE(error.empty());
}

TEST_CASE("JsonValue ToJson") {
    JsonObject obj;
    obj["a"] = JsonValue(1);
    JsonValue v(obj);
    std::string json = v.toJson();
    REQUIRE_FALSE(json.empty());
    // Round-trip
    std::string error;
    auto v2 = JsonValue::fromJson(json, false, &error);
    REQUIRE(error.empty());
    REQUIRE(v2["a"].toInt() == 1);
}

TEST_CASE("JsonValue DefaultValues") {
    JsonValue v; // Null
    REQUIRE(v.toBool(true) == true);
    REQUIRE(v.toInt(99) == 99);
    REQUIRE((v.toDouble(1.5) > 1.4 && v.toDouble(1.5) < 1.6));
    REQUIRE(v.toString("def") == "def");
}

// --- New JsonValue tests ---

TEST_CASE("JsonValue CopyConstructor") {
    JsonValue original(std::string("copy me"));
    JsonValue copy(original);
    REQUIRE(copy.isString());
    REQUIRE(copy.toString() == "copy me");
    REQUIRE(original == copy);
}

TEST_CASE("JsonValue MoveConstructor") {
    JsonValue original(std::string("move me"));
    JsonValue moved(std::move(original));
    REQUIRE(moved.isString());
    REQUIRE(moved.toString() == "move me");
}

TEST_CASE("JsonValue Swap") {
    JsonValue a(std::string("alpha"));
    JsonValue b(42);
    a.swap(b);
    REQUIRE(a.isInt());
    REQUIRE(a.toInt() == 42);
    REQUIRE(b.isString());
    REQUIRE(b.toString() == "alpha");
}

TEST_CASE("JsonValue NestedObject") {
    JsonArray inner;
    inner.push_back(JsonValue(1));
    inner.push_back(JsonValue(2));

    JsonObject nested;
    nested["x"] = JsonValue(10);

    JsonObject root;
    root["arr"] = JsonValue(inner);
    root["obj"] = JsonValue(nested);
    root["str"] = JsonValue("hello");

    JsonValue v(root);
    REQUIRE(v["arr"].isArray());
    REQUIRE(v["arr"].toArray().size() == static_cast<size_t>(2));
    REQUIRE(v["arr"][0].toInt() == 1);
    REQUIRE(v["obj"].isObject());
    REQUIRE(v["obj"]["x"].toInt() == 10);
    REQUIRE(v["str"].toString() == "hello");
}

TEST_CASE("JsonValue ToJsonPrettyPrint") {
    JsonObject obj;
    obj["key"] = JsonValue("value");
    JsonValue v(obj);
    std::string pretty = v.toJson(2);
    // Pretty-printed JSON should contain newlines and spaces
    REQUIRE(pretty.find('\n') != std::string::npos);
    REQUIRE(pretty.find("  ") != std::string::npos);
    // Should still round-trip
    std::string error;
    auto v2 = JsonValue::fromJson(pretty, false, &error);
    REQUIRE(error.empty());
    REQUIRE(v2["key"].toString() == "value");
}

TEST_CASE("JsonValue UndefinedType") {
    JsonObject obj;
    obj["exists"] = JsonValue(1);
    JsonValue v(obj);
    const auto &missing = v["no_such_key"];
    // Non-existent key returns Undefined internally, which wraps as null in nlohmann::json
    REQUIRE((missing.isNull() || missing.isUndefined()));
}

TEST_CASE("JsonValue EmptyArrayAndObject") {
    JsonValue emptyArr(JsonArray{});
    REQUIRE(emptyArr.isArray());
    REQUIRE(emptyArr.toArray().size() == static_cast<size_t>(0));

    JsonValue emptyObj(JsonObject{});
    REQUIRE(emptyObj.isObject());
    REQUIRE(emptyObj.toObject().size() == static_cast<size_t>(0));
}

TEST_CASE("JsonValue CborRoundTrip") {
    JsonObject obj;
    obj["name"] = JsonValue("cbor_test");
    obj["num"] = JsonValue(42);
    JsonArray arr;
    arr.push_back(JsonValue(true));
    arr.push_back(JsonValue(3.14));
    obj["list"] = JsonValue(arr);

    JsonValue original(obj);
    auto cbor = original.toCbor();
    REQUIRE_FALSE(cbor.empty());

    std::string error;
    auto restored = JsonValue::fromCbor(stdc::array_view<uint8_t>(cbor.data(), cbor.size()), &error);
    REQUIRE(error.empty());
    REQUIRE(restored.isObject());
    REQUIRE(restored["name"].toString() == "cbor_test");
    REQUIRE(restored["num"].toInt() == 42);
    REQUIRE(restored["list"].isArray());
    REQUIRE(restored["list"].toArray().size() == static_cast<size_t>(2));
    REQUIRE(restored["list"][0].toBool() == true);
}

// ============================================================================
// ConfigAccessor Tests (18 original + 3 new = 21)
// ============================================================================

static JsonObject makeTestConfig() {
    JsonObject cfg;
    cfg["name"] = JsonValue("test-plugin");
    cfg["count"] = JsonValue(10);
    cfg["rate"] = JsonValue(0.75);
    cfg["enabled"] = JsonValue(true);
    cfg["path"] = JsonValue("models/test.onnx");

    JsonArray tags;
    tags.push_back(JsonValue("tag1"));
    tags.push_back(JsonValue("tag2"));
    tags.push_back(JsonValue("tag3"));
    cfg["tags"] = JsonValue(tags);

    return cfg;
}

TEST_CASE("ConfigAccessor GetString Required") {
    auto cfg = makeTestConfig();
    ConfigAccessor acc(cfg);
    auto result = acc.getString("name");
    REQUIRE(result.hasValue());
    REQUIRE(result.value() == "test-plugin");
}

TEST_CASE("ConfigAccessor GetString Missing") {
    auto cfg = makeTestConfig();
    ConfigAccessor acc(cfg);
    auto result = acc.getString("nonexistent");
    REQUIRE_FALSE(result.hasValue());
}

TEST_CASE("ConfigAccessor GetString Optional") {
    auto cfg = makeTestConfig();
    ConfigAccessor acc(cfg);
    auto val = acc.getString("nonexistent", "default_val");
    REQUIRE(val == "default_val");

    auto val2 = acc.getString("name", "default_val");
    REQUIRE(val2 == "test-plugin");
}

TEST_CASE("ConfigAccessor GetInt Required") {
    auto cfg = makeTestConfig();
    ConfigAccessor acc(cfg);
    auto result = acc.getInt("count");
    REQUIRE(result.hasValue());
    REQUIRE(result.value() == 10);
}

TEST_CASE("ConfigAccessor GetInt Missing") {
    auto cfg = makeTestConfig();
    ConfigAccessor acc(cfg);
    auto result = acc.getInt("missing_int");
    REQUIRE_FALSE(result.hasValue());
}

TEST_CASE("ConfigAccessor GetInt Optional") {
    auto cfg = makeTestConfig();
    ConfigAccessor acc(cfg);
    auto val = acc.getInt("missing_int", 42);
    REQUIRE(val == 42);

    auto val2 = acc.getInt("count", 42);
    REQUIRE(val2 == 10);
}

TEST_CASE("ConfigAccessor GetDouble Required") {
    auto cfg = makeTestConfig();
    ConfigAccessor acc(cfg);
    auto result = acc.getDouble("rate");
    REQUIRE(result.hasValue());
    REQUIRE((result.value() > 0.74 && result.value() < 0.76));
}

TEST_CASE("ConfigAccessor GetDouble Missing") {
    auto cfg = makeTestConfig();
    ConfigAccessor acc(cfg);
    auto result = acc.getDouble("missing_double");
    REQUIRE_FALSE(result.hasValue());
}

TEST_CASE("ConfigAccessor GetDouble Optional") {
    auto cfg = makeTestConfig();
    ConfigAccessor acc(cfg);
    auto val = acc.getDouble("missing_double", 1.5);
    REQUIRE((val > 1.4 && val < 1.6));

    auto val2 = acc.getDouble("rate", 1.5);
    REQUIRE((val2 > 0.74 && val2 < 0.76));
}

TEST_CASE("ConfigAccessor GetBool Required") {
    auto cfg = makeTestConfig();
    ConfigAccessor acc(cfg);
    auto result = acc.getBool("enabled");
    REQUIRE(result.hasValue());
    REQUIRE(result.value() == true);
}

TEST_CASE("ConfigAccessor GetBool Missing") {
    auto cfg = makeTestConfig();
    ConfigAccessor acc(cfg);
    auto result = acc.getBool("missing_bool");
    REQUIRE_FALSE(result.hasValue());
}

TEST_CASE("ConfigAccessor GetBool Optional") {
    auto cfg = makeTestConfig();
    ConfigAccessor acc(cfg);
    auto val = acc.getBool("missing_bool", false);
    REQUIRE(val == false);

    auto val2 = acc.getBool("enabled", false);
    REQUIRE(val2 == true);
}

TEST_CASE("ConfigAccessor GetPath Required") {
    auto cfg = makeTestConfig();
    ConfigAccessor acc(cfg, std::filesystem::path("D:/base"));
    auto result = acc.getPath("path");
    REQUIRE(result.hasValue());
    // Should resolve relative to basePath
    auto p = result.value().string();
    REQUIRE_FALSE(p.empty());
}

TEST_CASE("ConfigAccessor GetPath Missing") {
    auto cfg = makeTestConfig();
    ConfigAccessor acc(cfg);
    auto result = acc.getPath("missing_path");
    REQUIRE_FALSE(result.hasValue());
}

TEST_CASE("ConfigAccessor GetStringArray Required") {
    auto cfg = makeTestConfig();
    ConfigAccessor acc(cfg);
    auto result = acc.getStringArray("tags");
    REQUIRE(result.hasValue());
    REQUIRE(result.value().size() == static_cast<size_t>(3));
    REQUIRE(result.value()[0] == "tag1");
    REQUIRE(result.value()[1] == "tag2");
    REQUIRE(result.value()[2] == "tag3");
}

TEST_CASE("ConfigAccessor GetStringArray Missing") {
    auto cfg = makeTestConfig();
    ConfigAccessor acc(cfg);
    auto result = acc.getStringArray("missing_arr");
    REQUIRE_FALSE(result.hasValue());
}

TEST_CASE("ConfigAccessor GetStringArray Optional") {
    auto cfg = makeTestConfig();
    ConfigAccessor acc(cfg);
    std::vector<std::string> def = {"a", "b"};
    auto val = acc.getStringArray("missing_arr", def);
    REQUIRE(val.size() == static_cast<size_t>(2));
    REQUIRE(val[0] == "a");

    auto val2 = acc.getStringArray("tags", def);
    REQUIRE(val2.size() == static_cast<size_t>(3));
}

TEST_CASE("ConfigAccessor Has") {
    auto cfg = makeTestConfig();
    ConfigAccessor acc(cfg);
    REQUIRE(acc.has("name"));
    REQUIRE(acc.has("count"));
    REQUIRE_FALSE(acc.has("nonexistent"));
}

// --- New ConfigAccessor tests ---

TEST_CASE("ConfigAccessor GetStringArray NonStringElement") {
    JsonObject cfg;
    JsonArray arr;
    arr.push_back(JsonValue("ok"));
    arr.push_back(JsonValue(42)); // not a string
    cfg["mixed"] = JsonValue(arr);

    ConfigAccessor acc(cfg);
    auto result = acc.getStringArray("mixed");
    REQUIRE_FALSE(result.hasValue());
}

TEST_CASE("ConfigAccessor BasePath") {
    auto cfg = makeTestConfig();
    std::filesystem::path bp("D:/some/base/path");
    ConfigAccessor acc(cfg, bp);
    REQUIRE(acc.basePath() == bp);
}

TEST_CASE("ConfigAccessor Raw") {
    auto cfg = makeTestConfig();
    ConfigAccessor acc(cfg);
    const auto &raw = acc.raw();
    REQUIRE(raw.size() == cfg.size());
    REQUIRE(raw.count("name") > 0);
    REQUIRE(raw.count("count") > 0);
}

// ============================================================================
// ValidationChain Tests (12 original)
// ============================================================================

TEST_CASE("ValidationChain IntRange Valid") {
    ValidationChain chain;
    chain.validateIntRange(5, 0, 10, "val");
    auto result = chain.execute();
    REQUIRE(result.hasValue());
}

TEST_CASE("ValidationChain IntRange TooLow") {
    ValidationChain chain;
    chain.validateIntRange(-1, 0, 10, "val");
    auto result = chain.execute();
    REQUIRE_FALSE(result.hasValue());
}

TEST_CASE("ValidationChain IntRange TooHigh") {
    ValidationChain chain;
    chain.validateIntRange(11, 0, 10, "val");
    auto result = chain.execute();
    REQUIRE_FALSE(result.hasValue());
}

TEST_CASE("ValidationChain DoubleRange Valid") {
    ValidationChain chain;
    chain.validateDoubleRange(0.5, 0.0, 1.0, "rate");
    auto result = chain.execute();
    REQUIRE(result.hasValue());
}

TEST_CASE("ValidationChain DoubleRange TooLow") {
    ValidationChain chain;
    chain.validateDoubleRange(-0.1, 0.0, 1.0, "rate");
    auto result = chain.execute();
    REQUIRE_FALSE(result.hasValue());
}

TEST_CASE("ValidationChain DoubleRange TooHigh") {
    ValidationChain chain;
    chain.validateDoubleRange(1.1, 0.0, 1.0, "rate");
    auto result = chain.execute();
    REQUIRE_FALSE(result.hasValue());
}

TEST_CASE("ValidationChain StringAllowed Valid") {
    ValidationChain chain;
    chain.validateStringAllowed("cpu", {"cpu", "gpu", "auto"}, "device");
    auto result = chain.execute();
    REQUIRE(result.hasValue());
}

TEST_CASE("ValidationChain StringAllowed Invalid") {
    ValidationChain chain;
    chain.validateStringAllowed("tpu", {"cpu", "gpu", "auto"}, "device");
    auto result = chain.execute();
    REQUIRE_FALSE(result.hasValue());
}

TEST_CASE("ValidationChain ArrayNotEmpty Valid") {
    ValidationChain chain;
    chain.validateArrayNotEmpty({"a", "b"}, "items");
    auto result = chain.execute();
    REQUIRE(result.hasValue());
}

TEST_CASE("ValidationChain ArrayNotEmpty Empty") {
    ValidationChain chain;
    chain.validateArrayNotEmpty({}, "items");
    auto result = chain.execute();
    REQUIRE_FALSE(result.hasValue());
}

TEST_CASE("ValidationChain Chaining") {
    ValidationChain chain;
    chain.validateIntRange(5, 0, 10, "count")
        .validateDoubleRange(0.5, 0.0, 1.0, "rate")
        .validateStringAllowed("cpu", {"cpu", "gpu"}, "device");
    auto result = chain.execute();
    REQUIRE(result.hasValue());
}

TEST_CASE("ValidationChain Chaining FirstFails") {
    ValidationChain chain;
    chain.validateIntRange(100, 0, 10, "count")
        .validateDoubleRange(0.5, 0.0, 1.0, "rate")
        .validateStringAllowed("cpu", {"cpu", "gpu"}, "device");
    auto result = chain.execute();
    REQUIRE_FALSE(result.hasValue());
    REQUIRE(chain.hasError());
}

TEST_CASE("ValidationChain CustomValidator") {
    ValidationChain chain;
    chain.validate([]() -> Expected<bool> {
        return true;
    });
    auto result = chain.execute();
    REQUIRE(result.hasValue());
}