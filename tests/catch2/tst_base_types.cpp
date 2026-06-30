#include "catch.hpp"

#include <LangCore/Base/NamedObject.h>
#include <LangCore/Base/ObjectPool.h>
#include <LangCore/Base/LangCommon.h>
#include <LangCore/Base/AlignedAllocator.h>
#include <LangCore/Support/DisplayText.h>
#include <LangCore/Support/Tensor.h>
#include <LangCore/Support/JSON.h>

#include <cstdint>
#include <vector>

using namespace LangCore;

// =============================================================================
// NamedObject
// =============================================================================

TEST_CASE("NamedObject DefaultName Empty") {
    NamedObject obj;
    REQUIRE(obj.objectName().empty());
}

TEST_CASE("NamedObject SetGetName") {
    NamedObject obj;
    obj.setObjectName("test");
    REQUIRE(obj.objectName() == "test");
}

TEST_CASE("NamedObject Properties") {
    NamedObject obj;
    obj.setProperty("key", std::any(42));
    auto &val = obj.property("key");
    REQUIRE(val.has_value());
    REQUIRE(std::any_cast<int>(val) == 42);
}

TEST_CASE("NO Create") {
    auto obj = NO<NamedObject>::create();
    REQUIRE(obj.get() != nullptr);
}

TEST_CASE("NO As") {
    auto obj = NO<NamedObject>::create();
    auto casted = obj.as<NamedObject>();
    REQUIRE(casted.get() != nullptr);
    REQUIRE(casted.get() == obj.get());
}

// =============================================================================
// ObjectPool
// =============================================================================

TEST_CASE("ObjectPool AddAndGet") {
    ObjectPool pool;
    auto obj = NO<NamedObject>::create();
    pool.addObject("key1", obj);
    auto got = pool.getFirstObject("key1");
    REQUIRE(got.get() == obj.get());
}

TEST_CASE("ObjectPool AddMultiple") {
    ObjectPool pool;
    auto a = NO<NamedObject>::create();
    auto b = NO<NamedObject>::create();
    std::vector<NO<NamedObject>> objs = {a, b};
    pool.addObjects("grp", stdc::array_view<NO<NamedObject>>(objs.data(), objs.size()));
    REQUIRE(pool.allObjects().size() == 2u);
}

TEST_CASE("ObjectPool RemoveObject") {
    ObjectPool pool;
    auto obj = NO<NamedObject>::create();
    pool.addObject("key1", obj);
    pool.removeObject("key1", obj.get());
    auto got = pool.getFirstObject("key1");
    REQUIRE(got.get() == nullptr);
}

TEST_CASE("ObjectPool RemoveAllObjects") {
    ObjectPool pool;
    pool.addObject("a", NO<NamedObject>::create());
    pool.addObject("b", NO<NamedObject>::create());
    pool.removeAllObjects();
    REQUIRE(pool.allObjects().empty());
}

TEST_CASE("ObjectPool GetObjectsByKey") {
    ObjectPool pool;
    auto a = NO<NamedObject>::create();
    auto b = NO<NamedObject>::create();
    auto c = NO<NamedObject>::create();
    pool.addObject("x", a);
    pool.addObject("x", b);
    pool.addObject("y", c);
    auto xs = pool.getObjects("x");
    REQUIRE(xs.size() == 2u);
}

// =============================================================================
// LangCommon
// =============================================================================

TEST_CASE("TaggerRes Constructor") {
    TaggerRes r("hello");
    REQUIRE(r.lyric == "hello");
    REQUIRE(r.language == "unknown");
    REQUIRE(r.tag == "unknown");
    REQUIRE_FALSE(r.discard);
}

TEST_CASE("TaggerRes LanguageTags") {
    TaggerRes r("hi", "en", "ENG");
    REQUIRE(r.lyric == "hi");
    REQUIRE(r.language == "en");
    REQUIRE(r.tag == "ENG");
}

TEST_CASE("G2pInput Constructor") {
    G2pInput input("hello", "eng");
    REQUIRE(input.lyric == "hello");
    REQUIRE(input.g2pId == "eng");
}

TEST_CASE("G2pRes DefaultConstructor") {
    G2pRes res;
    REQUIRE(res.mode == "copy");
    REQUIRE(res.errorType == NoError);
}

TEST_CASE("G2pRes FullConstructor") {
    G2pRes res("hello", "eng", "", "hh ah l ow", std::vector<std::string>{}, "g2p", NoError);
    REQUIRE(res.lyric == "hello");
    REQUIRE(res.g2pId == "eng");
    REQUIRE(res.pronunciation == "hh ah l ow");
    REQUIRE(res.mode == "g2p");
    REQUIRE(res.errorType == NoError);
    // candidates auto-populated from pronunciation
    REQUIRE(res.candidates.size() == 1u);
    REQUIRE(res.candidates[0] == "hh ah l ow");
}

TEST_CASE("G2pRes EmptyPronunciation FallbackToLyric") {
    G2pRes res("hello", "eng", "", "", std::vector<std::string>{}, "copy", NoError);
    REQUIRE(res.pronunciation == "hello");
}

TEST_CASE("G2pErrorType Values") {
    REQUIRE(static_cast<int>(NoError) == 0);
    REQUIRE(static_cast<int>(InvalidLyric) == 1);
    REQUIRE(static_cast<int>(ModelInferenceFailed) == 2);
    REQUIRE(static_cast<int>(PhonemeGenerationFailed) == 3);
    REQUIRE(static_cast<int>(DriverUnavailable) == 4);
    REQUIRE(static_cast<int>(UnknownError) == 5);
}

// G2pRes::isOk / isFailed — 4 种 mode + errorType 组合
TEST_CASE("g2pRes_isOk_convert_success") {
    G2pRes res("hello", "eng", "", {}, "hh ah l ow", {}, "convert", NoError);
    REQUIRE(res.isOk());
    REQUIRE_FALSE(res.isFailed());
}

TEST_CASE("g2pRes_isOk_copy_noError") {
    // 合法的原词保留（如标点/数字），mode=="copy" + NoError → 不是失败
    G2pRes res("hello", "eng", "", {}, "hello", {}, "copy", NoError);
    REQUIRE(res.isOk());
    REQUIRE_FALSE(res.isFailed());
}

TEST_CASE("g2pRes_isFailed_copy_withError") {
    // 推理失败兜底：mode=="copy" + 非 NoError → 失败
    G2pRes res("hello", "eng", "", {}, "hello", {}, "copy", ModelInferenceFailed);
    REQUIRE_FALSE(res.isOk());
    REQUIRE(res.isFailed());
}

TEST_CASE("g2pRes_isOk_skip_noError") {
    // 空 lyric 跳过：mode=="skip" + NoError → 不是失败
    G2pRes res("", "eng", "", {}, "", {}, "skip", NoError);
    REQUIRE(res.isOk());
    REQUIRE_FALSE(res.isFailed());
}

// =============================================================================
// DisplayText
// =============================================================================

TEST_CASE("DisplayText DefaultIsEmpty") {
    DisplayText dt;
    REQUIRE(dt.isEmpty());
}

TEST_CASE("DisplayText FromString") {
    DisplayText dt(std::string("hello"));
    REQUIRE(dt.text() == "hello");
}

TEST_CASE("DisplayText FromJsonString") {
    JsonValue jv(std::string("test"));
    DisplayText dt(jv);
    REQUIRE(dt.text() == "test");
}

TEST_CASE("DisplayText FromJsonObject") {
    JsonObject obj;
    obj["_"] = JsonValue(std::string("default"));
    obj["en"] = JsonValue(std::string("English"));
    obj["zh"] = JsonValue(std::string("\xe4\xb8\xad\xe6\x96\x87"));
    JsonValue jv2(obj);
    DisplayText dt(jv2);
    REQUIRE(dt.text("en") == "English");
    REQUIRE(dt.text("zh") == "\xe4\xb8\xad\xe6\x96\x87");
}

TEST_CASE("DisplayText FallbackOrder") {
    JsonObject obj;
    obj["ja"] = JsonValue(std::string("Japanese"));
    JsonValue jv3(obj);
    DisplayText dt(jv3);
    REQUIRE(dt.defaultText() == "Japanese");
}

TEST_CASE("DisplayText AssignString") {
    DisplayText dt;
    dt = std::string("assigned");
    REQUIRE(dt.text() == "assigned");
}

// =============================================================================
// Tensor
// =============================================================================

TEST_CASE("Tensor CreateFloat") {
    auto exp = Tensor::create(ITensor::Float, {2, 3});
    REQUIRE(exp.hasValue());
    auto t = exp.take();
    REQUIRE(t.get() != nullptr);
    auto s = t->shape();
    REQUIRE(s.size() == 2u);
    REQUIRE(s[0] == 2);
    REQUIRE(s[1] == 3);
}

TEST_CASE("Tensor CreateInt64") {
    auto exp = Tensor::create(ITensor::Int64, {4});
    REQUIRE(exp.hasValue());
    auto t = exp.take();
    REQUIRE(t.get() != nullptr);
    REQUIRE(t->shape().size() == 1u);
    REQUIRE(t->shape()[0] == 4);
}

TEST_CASE("Tensor ElementCount") {
    auto exp = Tensor::create(ITensor::Float, {2, 3});
    REQUIRE(exp.hasValue());
    auto t = exp.take();
    REQUIRE(t->elementCount() == 6u);
}

TEST_CASE("Tensor CreateScalar") {
    auto exp = Tensor::createScalar(3.14f);
    REQUIRE(exp.hasValue());
    auto t = exp.take();
    REQUIRE(t.get() != nullptr);
    const float *p = t->data<float>();
    REQUIRE(p != nullptr);
    REQUIRE((*p > 3.13f && *p < 3.15f));
}

TEST_CASE("Tensor CreateFilled") {
    auto exp = Tensor::createFilled<float>({3}, 1.0f);
    REQUIRE(exp.hasValue());
    auto t = exp.take();
    REQUIRE(t->elementCount() == 3u);
    const float *p = t->data<float>();
    REQUIRE(p != nullptr);
    for (size_t i = 0; i < 3; i++) {
        REQUIRE((p[i] > 0.99f && p[i] < 1.01f));
    }
}

TEST_CASE("Tensor Clone") {
    auto exp = Tensor::createFilled<float>({4}, 2.0f);
    REQUIRE(exp.hasValue());
    auto orig = exp.take();
    auto cloned = orig->clone();
    REQUIRE(cloned.get() != nullptr);
    REQUIRE(cloned->elementCount() == orig->elementCount());
    const float *a = orig->data<float>();
    const float *b = cloned->data<float>();
    REQUIRE(a != b); // different memory
    for (size_t i = 0; i < orig->elementCount(); i++) {
        REQUIRE((a[i] > 1.99f && a[i] < 2.01f));
        REQUIRE((b[i] > 1.99f && b[i] < 2.01f));
    }
}

TEST_CASE("Tensor MutableData") {
    auto exp = Tensor::create(ITensor::Float, {3});
    REQUIRE(exp.hasValue());
    auto t = exp.take();
    float *p = t->mutableData<float>();
    REQUIRE(p != nullptr);
    p[0] = 10.0f;
    p[1] = 20.0f;
    p[2] = 30.0f;
    const float *cp = t->data<float>();
    REQUIRE((cp[0] > 9.99f && cp[0] < 10.01f));
    REQUIRE((cp[1] > 19.99f && cp[1] < 20.01f));
    REQUIRE((cp[2] > 29.99f && cp[2] < 30.01f));
}

// =============================================================================
// AlignedAllocator
// =============================================================================

TEST_CASE("AlignedAllocator AllocateDeallocate") {
    AlignedAllocator<float, 64> alloc;
    float *p = alloc.allocate(10);
    REQUIRE(p != nullptr);
    alloc.deallocate(p, 10);
}

TEST_CASE("AlignedAllocator Alignment") {
    AlignedAllocator<float, 64> alloc;
    float *p = alloc.allocate(16);
    auto addr = reinterpret_cast<std::uintptr_t>(p);
    REQUIRE(addr % 64 == 0u);
    alloc.deallocate(p, 16);
}

TEST_CASE("AlignedAllocator VectorUsage") {
    std::vector<float, AlignedAllocator<float, 64>> vec;
    vec.push_back(1.0f);
    vec.push_back(2.0f);
    vec.push_back(3.0f);
    REQUIRE(vec.size() == 3u);
    REQUIRE((vec[0] > 0.99f && vec[0] < 1.01f));
    REQUIRE((vec[1] > 1.99f && vec[1] < 2.01f));
    REQUIRE((vec[2] > 2.99f && vec[2] < 3.01f));
    auto addr = reinterpret_cast<std::uintptr_t>(vec.data());
    REQUIRE(addr % 64 == 0u);
}