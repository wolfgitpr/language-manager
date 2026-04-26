#include "tst_framework.h"

#include <LangCore/Support/JSON.h>
#include <LangCore/Support/ConfigAccessor.h>

using namespace LangCore;

// ============================================================================
// JsonValue Tests (14 original + 8 new = 22)
// ============================================================================

TEST_CASE(JsonValue_DefaultNull) {
    JsonValue v;
    ASSERT_TRUE(v.isNull());
    ASSERT_EQ(v.type(), JsonValue::Null);
}

TEST_CASE(JsonValue_Bool) {
    JsonValue v(true);
    ASSERT_TRUE(v.isBool());
    ASSERT_EQ(v.toBool(), true);

    JsonValue v2(false);
    ASSERT_EQ(v2.toBool(), false);
}

TEST_CASE(JsonValue_Double) {
    JsonValue v(3.14);
    ASSERT_TRUE(v.isDouble());
    ASSERT_TRUE(v.isNumber());
    ASSERT_TRUE(v.toDouble() > 3.13 && v.toDouble() < 3.15);
}

TEST_CASE(JsonValue_Int) {
    JsonValue v(static_cast<int64_t>(42));
    ASSERT_TRUE(v.isInt());
    ASSERT_TRUE(v.isNumber());
    ASSERT_EQ(v.toInt(), 42);
}

TEST_CASE(JsonValue_UInt) {
    JsonValue v(static_cast<uint64_t>(100));
    ASSERT_TRUE(v.isUInt());
    ASSERT_TRUE(v.isInt());
    ASSERT_EQ(v.toUInt(), static_cast<uint64_t>(100));
}

TEST_CASE(JsonValue_String) {
    JsonValue v(std::string("hello"));
    ASSERT_TRUE(v.isString());
    ASSERT_STREQ(v.toString().c_str(), "hello");
}

TEST_CASE(JsonValue_CString) {
    JsonValue v("world");
    ASSERT_TRUE(v.isString());
    ASSERT_STREQ(v.toString().c_str(), "world");
}

TEST_CASE(JsonValue_Array) {
    JsonArray arr;
    arr.push_back(JsonValue(1));
    arr.push_back(JsonValue(2));
    arr.push_back(JsonValue(3));
    JsonValue v(arr);
    ASSERT_TRUE(v.isArray());
    ASSERT_EQ(v.toArray().size(), static_cast<size_t>(3));
    ASSERT_EQ(v[0].toInt(), 1);
    ASSERT_EQ(v[1].toInt(), 2);
    ASSERT_EQ(v[2].toInt(), 3);
}

TEST_CASE(JsonValue_Object) {
    JsonObject obj;
    obj["name"] = JsonValue("test");
    obj["value"] = JsonValue(42);
    JsonValue v(obj);
    ASSERT_TRUE(v.isObject());
    ASSERT_STREQ(v["name"].toString().c_str(), "test");
    ASSERT_EQ(v["value"].toInt(), 42);
}

TEST_CASE(JsonValue_Equality) {
    JsonValue a(42);
    JsonValue b(42);
    JsonValue c(99);
    ASSERT_TRUE(a == b);
    ASSERT_TRUE(a != c);
}

TEST_CASE(JsonValue_FromJson) {
    std::string error;
    auto v = JsonValue::fromJson(R"({"key": "value", "num": 123})", false, &error);
    ASSERT_TRUE(error.empty());
    ASSERT_TRUE(v.isObject());
    ASSERT_STREQ(v["key"].toString().c_str(), "value");
    ASSERT_EQ(v["num"].toInt(), 123);
}

TEST_CASE(JsonValue_FromJsonInvalid) {
    std::string error;
    auto v = JsonValue::fromJson("{invalid json", false, &error);
    ASSERT_FALSE(error.empty());
}

TEST_CASE(JsonValue_ToJson) {
    JsonObject obj;
    obj["a"] = JsonValue(1);
    JsonValue v(obj);
    std::string json = v.toJson();
    ASSERT_FALSE(json.empty());
    // Round-trip
    std::string error;
    auto v2 = JsonValue::fromJson(json, false, &error);
    ASSERT_TRUE(error.empty());
    ASSERT_EQ(v2["a"].toInt(), 1);
}

TEST_CASE(JsonValue_DefaultValues) {
    JsonValue v; // Null
    ASSERT_EQ(v.toBool(true), true);
    ASSERT_EQ(v.toInt(99), 99);
    ASSERT_TRUE(v.toDouble(1.5) > 1.4 && v.toDouble(1.5) < 1.6);
    ASSERT_STREQ(v.toString("def").c_str(), "def");
}

// --- New JsonValue tests ---

TEST_CASE(JsonValue_CopyConstructor) {
    JsonValue original(std::string("copy me"));
    JsonValue copy(original);
    ASSERT_TRUE(copy.isString());
    ASSERT_STREQ(copy.toString().c_str(), "copy me");
    ASSERT_TRUE(original == copy);
}

TEST_CASE(JsonValue_MoveConstructor) {
    JsonValue original(std::string("move me"));
    JsonValue moved(std::move(original));
    ASSERT_TRUE(moved.isString());
    ASSERT_STREQ(moved.toString().c_str(), "move me");
}

TEST_CASE(JsonValue_Swap) {
    JsonValue a(std::string("alpha"));
    JsonValue b(42);
    a.swap(b);
    ASSERT_TRUE(a.isInt());
    ASSERT_EQ(a.toInt(), 42);
    ASSERT_TRUE(b.isString());
    ASSERT_STREQ(b.toString().c_str(), "alpha");
}

TEST_CASE(JsonValue_NestedObject) {
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
    ASSERT_TRUE(v["arr"].isArray());
    ASSERT_EQ(v["arr"].toArray().size(), static_cast<size_t>(2));
    ASSERT_EQ(v["arr"][0].toInt(), 1);
    ASSERT_TRUE(v["obj"].isObject());
    ASSERT_EQ(v["obj"]["x"].toInt(), 10);
    ASSERT_STREQ(v["str"].toString().c_str(), "hello");
}

TEST_CASE(JsonValue_ToJsonPrettyPrint) {
    JsonObject obj;
    obj["key"] = JsonValue("value");
    JsonValue v(obj);
    std::string pretty = v.toJson(2);
    // Pretty-printed JSON should contain newlines and spaces
    ASSERT_TRUE(pretty.find('\n') != std::string::npos);
    ASSERT_TRUE(pretty.find("  ") != std::string::npos);
    // Should still round-trip
    std::string error;
    auto v2 = JsonValue::fromJson(pretty, false, &error);
    ASSERT_TRUE(error.empty());
    ASSERT_STREQ(v2["key"].toString().c_str(), "value");
}

TEST_CASE(JsonValue_UndefinedType) {
    JsonObject obj;
    obj["exists"] = JsonValue(1);
    JsonValue v(obj);
    const auto &missing = v["no_such_key"];
    // Non-existent key returns Undefined internally, which wraps as null in nlohmann::json
    ASSERT_TRUE(missing.isNull() || missing.isUndefined());
}

TEST_CASE(JsonValue_EmptyArrayAndObject) {
    JsonValue emptyArr(JsonArray{});
    ASSERT_TRUE(emptyArr.isArray());
    ASSERT_EQ(emptyArr.toArray().size(), static_cast<size_t>(0));

    JsonValue emptyObj(JsonObject{});
    ASSERT_TRUE(emptyObj.isObject());
    ASSERT_EQ(emptyObj.toObject().size(), static_cast<size_t>(0));
}

TEST_CASE(JsonValue_CborRoundTrip) {
    JsonObject obj;
    obj["name"] = JsonValue("cbor_test");
    obj["num"] = JsonValue(42);
    JsonArray arr;
    arr.push_back(JsonValue(true));
    arr.push_back(JsonValue(3.14));
    obj["list"] = JsonValue(arr);

    JsonValue original(obj);
    auto cbor = original.toCbor();
    ASSERT_FALSE(cbor.empty());

    std::string error;
    auto restored = JsonValue::fromCbor(stdc::array_view<uint8_t>(cbor.data(), cbor.size()), &error);
    ASSERT_TRUE(error.empty());
    ASSERT_TRUE(restored.isObject());
    ASSERT_STREQ(restored["name"].toString().c_str(), "cbor_test");
    ASSERT_EQ(restored["num"].toInt(), 42);
    ASSERT_TRUE(restored["list"].isArray());
    ASSERT_EQ(restored["list"].toArray().size(), static_cast<size_t>(2));
    ASSERT_EQ(restored["list"][0].toBool(), true);
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

TEST_CASE(ConfigAccessor_GetString_Required) {
    auto cfg = makeTestConfig();
    ConfigAccessor acc(cfg);
    auto result = acc.getString("name");
    ASSERT_TRUE(result.hasValue());
    ASSERT_STREQ(result.value().c_str(), "test-plugin");
}

TEST_CASE(ConfigAccessor_GetString_Missing) {
    auto cfg = makeTestConfig();
    ConfigAccessor acc(cfg);
    auto result = acc.getString("nonexistent");
    ASSERT_FALSE(result.hasValue());
}

TEST_CASE(ConfigAccessor_GetString_Optional) {
    auto cfg = makeTestConfig();
    ConfigAccessor acc(cfg);
    auto val = acc.getString("nonexistent", "default_val");
    ASSERT_STREQ(val.c_str(), "default_val");

    auto val2 = acc.getString("name", "default_val");
    ASSERT_STREQ(val2.c_str(), "test-plugin");
}

TEST_CASE(ConfigAccessor_GetInt_Required) {
    auto cfg = makeTestConfig();
    ConfigAccessor acc(cfg);
    auto result = acc.getInt("count");
    ASSERT_TRUE(result.hasValue());
    ASSERT_EQ(result.value(), 10);
}

TEST_CASE(ConfigAccessor_GetInt_Missing) {
    auto cfg = makeTestConfig();
    ConfigAccessor acc(cfg);
    auto result = acc.getInt("missing_int");
    ASSERT_FALSE(result.hasValue());
}

TEST_CASE(ConfigAccessor_GetInt_Optional) {
    auto cfg = makeTestConfig();
    ConfigAccessor acc(cfg);
    auto val = acc.getInt("missing_int", 42);
    ASSERT_EQ(val, 42);

    auto val2 = acc.getInt("count", 42);
    ASSERT_EQ(val2, 10);
}

TEST_CASE(ConfigAccessor_GetDouble_Required) {
    auto cfg = makeTestConfig();
    ConfigAccessor acc(cfg);
    auto result = acc.getDouble("rate");
    ASSERT_TRUE(result.hasValue());
    ASSERT_TRUE(result.value() > 0.74 && result.value() < 0.76);
}

TEST_CASE(ConfigAccessor_GetDouble_Missing) {
    auto cfg = makeTestConfig();
    ConfigAccessor acc(cfg);
    auto result = acc.getDouble("missing_double");
    ASSERT_FALSE(result.hasValue());
}

TEST_CASE(ConfigAccessor_GetDouble_Optional) {
    auto cfg = makeTestConfig();
    ConfigAccessor acc(cfg);
    auto val = acc.getDouble("missing_double", 1.5);
    ASSERT_TRUE(val > 1.4 && val < 1.6);

    auto val2 = acc.getDouble("rate", 1.5);
    ASSERT_TRUE(val2 > 0.74 && val2 < 0.76);
}

TEST_CASE(ConfigAccessor_GetBool_Required) {
    auto cfg = makeTestConfig();
    ConfigAccessor acc(cfg);
    auto result = acc.getBool("enabled");
    ASSERT_TRUE(result.hasValue());
    ASSERT_EQ(result.value(), true);
}

TEST_CASE(ConfigAccessor_GetBool_Missing) {
    auto cfg = makeTestConfig();
    ConfigAccessor acc(cfg);
    auto result = acc.getBool("missing_bool");
    ASSERT_FALSE(result.hasValue());
}

TEST_CASE(ConfigAccessor_GetBool_Optional) {
    auto cfg = makeTestConfig();
    ConfigAccessor acc(cfg);
    auto val = acc.getBool("missing_bool", false);
    ASSERT_EQ(val, false);

    auto val2 = acc.getBool("enabled", false);
    ASSERT_EQ(val2, true);
}

TEST_CASE(ConfigAccessor_GetPath_Required) {
    auto cfg = makeTestConfig();
    ConfigAccessor acc(cfg, std::filesystem::path("D:/base"));
    auto result = acc.getPath("path");
    ASSERT_TRUE(result.hasValue());
    // Should resolve relative to basePath
    auto p = result.value().string();
    ASSERT_FALSE(p.empty());
}

TEST_CASE(ConfigAccessor_GetPath_Missing) {
    auto cfg = makeTestConfig();
    ConfigAccessor acc(cfg);
    auto result = acc.getPath("missing_path");
    ASSERT_FALSE(result.hasValue());
}

TEST_CASE(ConfigAccessor_GetStringArray_Required) {
    auto cfg = makeTestConfig();
    ConfigAccessor acc(cfg);
    auto result = acc.getStringArray("tags");
    ASSERT_TRUE(result.hasValue());
    ASSERT_EQ(result.value().size(), static_cast<size_t>(3));
    ASSERT_STREQ(result.value()[0].c_str(), "tag1");
    ASSERT_STREQ(result.value()[1].c_str(), "tag2");
    ASSERT_STREQ(result.value()[2].c_str(), "tag3");
}

TEST_CASE(ConfigAccessor_GetStringArray_Missing) {
    auto cfg = makeTestConfig();
    ConfigAccessor acc(cfg);
    auto result = acc.getStringArray("missing_arr");
    ASSERT_FALSE(result.hasValue());
}

TEST_CASE(ConfigAccessor_GetStringArray_Optional) {
    auto cfg = makeTestConfig();
    ConfigAccessor acc(cfg);
    std::vector<std::string> def = {"a", "b"};
    auto val = acc.getStringArray("missing_arr", def);
    ASSERT_EQ(val.size(), static_cast<size_t>(2));
    ASSERT_STREQ(val[0].c_str(), "a");

    auto val2 = acc.getStringArray("tags", def);
    ASSERT_EQ(val2.size(), static_cast<size_t>(3));
}

TEST_CASE(ConfigAccessor_Has) {
    auto cfg = makeTestConfig();
    ConfigAccessor acc(cfg);
    ASSERT_TRUE(acc.has("name"));
    ASSERT_TRUE(acc.has("count"));
    ASSERT_FALSE(acc.has("nonexistent"));
}

// --- New ConfigAccessor tests ---

TEST_CASE(ConfigAccessor_GetStringArray_NonStringElement) {
    JsonObject cfg;
    JsonArray arr;
    arr.push_back(JsonValue("ok"));
    arr.push_back(JsonValue(42)); // not a string
    cfg["mixed"] = JsonValue(arr);

    ConfigAccessor acc(cfg);
    auto result = acc.getStringArray("mixed");
    ASSERT_FALSE(result.hasValue());
}

TEST_CASE(ConfigAccessor_BasePath) {
    auto cfg = makeTestConfig();
    std::filesystem::path bp("D:/some/base/path");
    ConfigAccessor acc(cfg, bp);
    ASSERT_EQ(acc.basePath(), bp);
}

TEST_CASE(ConfigAccessor_Raw) {
    auto cfg = makeTestConfig();
    ConfigAccessor acc(cfg);
    const auto &raw = acc.raw();
    ASSERT_EQ(raw.size(), cfg.size());
    ASSERT_TRUE(raw.count("name") > 0);
    ASSERT_TRUE(raw.count("count") > 0);
}

// ============================================================================
// ValidationChain Tests (12 original)
// ============================================================================

TEST_CASE(ValidationChain_IntRange_Valid) {
    ValidationChain chain;
    chain.validateIntRange(5, 0, 10, "val");
    auto result = chain.execute();
    ASSERT_TRUE(result.hasValue());
}

TEST_CASE(ValidationChain_IntRange_TooLow) {
    ValidationChain chain;
    chain.validateIntRange(-1, 0, 10, "val");
    auto result = chain.execute();
    ASSERT_FALSE(result.hasValue());
}

TEST_CASE(ValidationChain_IntRange_TooHigh) {
    ValidationChain chain;
    chain.validateIntRange(11, 0, 10, "val");
    auto result = chain.execute();
    ASSERT_FALSE(result.hasValue());
}

TEST_CASE(ValidationChain_DoubleRange_Valid) {
    ValidationChain chain;
    chain.validateDoubleRange(0.5, 0.0, 1.0, "rate");
    auto result = chain.execute();
    ASSERT_TRUE(result.hasValue());
}

TEST_CASE(ValidationChain_DoubleRange_TooLow) {
    ValidationChain chain;
    chain.validateDoubleRange(-0.1, 0.0, 1.0, "rate");
    auto result = chain.execute();
    ASSERT_FALSE(result.hasValue());
}

TEST_CASE(ValidationChain_DoubleRange_TooHigh) {
    ValidationChain chain;
    chain.validateDoubleRange(1.1, 0.0, 1.0, "rate");
    auto result = chain.execute();
    ASSERT_FALSE(result.hasValue());
}

TEST_CASE(ValidationChain_StringAllowed_Valid) {
    ValidationChain chain;
    chain.validateStringAllowed("cpu", {"cpu", "gpu", "auto"}, "device");
    auto result = chain.execute();
    ASSERT_TRUE(result.hasValue());
}

TEST_CASE(ValidationChain_StringAllowed_Invalid) {
    ValidationChain chain;
    chain.validateStringAllowed("tpu", {"cpu", "gpu", "auto"}, "device");
    auto result = chain.execute();
    ASSERT_FALSE(result.hasValue());
}

TEST_CASE(ValidationChain_ArrayNotEmpty_Valid) {
    ValidationChain chain;
    chain.validateArrayNotEmpty({"a", "b"}, "items");
    auto result = chain.execute();
    ASSERT_TRUE(result.hasValue());
}

TEST_CASE(ValidationChain_ArrayNotEmpty_Empty) {
    ValidationChain chain;
    chain.validateArrayNotEmpty({}, "items");
    auto result = chain.execute();
    ASSERT_FALSE(result.hasValue());
}

TEST_CASE(ValidationChain_Chaining) {
    ValidationChain chain;
    chain.validateIntRange(5, 0, 10, "count")
        .validateDoubleRange(0.5, 0.0, 1.0, "rate")
        .validateStringAllowed("cpu", {"cpu", "gpu"}, "device");
    auto result = chain.execute();
    ASSERT_TRUE(result.hasValue());
}

TEST_CASE(ValidationChain_Chaining_FirstFails) {
    ValidationChain chain;
    chain.validateIntRange(100, 0, 10, "count")
        .validateDoubleRange(0.5, 0.0, 1.0, "rate")
        .validateStringAllowed("cpu", {"cpu", "gpu"}, "device");
    auto result = chain.execute();
    ASSERT_FALSE(result.hasValue());
    ASSERT_TRUE(chain.hasError());
}

TEST_CASE(ValidationChain_CustomValidator) {
    ValidationChain chain;
    chain.validate([]() -> Expected<bool> {
        return true;
    });
    auto result = chain.execute();
    ASSERT_TRUE(result.hasValue());
}
