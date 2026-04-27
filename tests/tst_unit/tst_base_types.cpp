#include "tst_framework.h"

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

TEST_CASE(NamedObject_DefaultName_Empty) {
    NamedObject obj;
    ASSERT_TRUE(obj.objectName().empty());
}

TEST_CASE(NamedObject_SetGetName) {
    NamedObject obj;
    obj.setObjectName("test");
    ASSERT_EQ(obj.objectName(), "test");
}

TEST_CASE(NamedObject_Properties) {
    NamedObject obj;
    obj.setProperty("key", std::any(42));
    auto &val = obj.property("key");
    ASSERT_TRUE(val.has_value());
    ASSERT_EQ(std::any_cast<int>(val), 42);
}

TEST_CASE(NO_Create) {
    auto obj = NO<NamedObject>::create();
    ASSERT_NE(obj.get(), nullptr);
}

TEST_CASE(NO_As) {
    auto obj = NO<NamedObject>::create();
    auto casted = obj.as<NamedObject>();
    ASSERT_NE(casted.get(), nullptr);
    ASSERT_EQ(casted.get(), obj.get());
}

// =============================================================================
// ObjectPool
// =============================================================================

TEST_CASE(ObjectPool_AddAndGet) {
    ObjectPool pool;
    auto obj = NO<NamedObject>::create();
    pool.addObject("key1", obj);
    auto got = pool.getFirstObject("key1");
    ASSERT_EQ(got.get(), obj.get());
}

TEST_CASE(ObjectPool_AddMultiple) {
    ObjectPool pool;
    auto a = NO<NamedObject>::create();
    auto b = NO<NamedObject>::create();
    std::vector<NO<NamedObject>> objs = {a, b};
    pool.addObjects("grp", stdc::array_view<NO<NamedObject>>(objs.data(), objs.size()));
    ASSERT_EQ(pool.allObjects().size(), 2u);
}

TEST_CASE(ObjectPool_RemoveObject) {
    ObjectPool pool;
    auto obj = NO<NamedObject>::create();
    pool.addObject("key1", obj);
    pool.removeObject("key1", obj.get());
    auto got = pool.getFirstObject("key1");
    ASSERT_EQ(got.get(), nullptr);
}

TEST_CASE(ObjectPool_RemoveAllObjects) {
    ObjectPool pool;
    pool.addObject("a", NO<NamedObject>::create());
    pool.addObject("b", NO<NamedObject>::create());
    pool.removeAllObjects();
    ASSERT_TRUE(pool.allObjects().empty());
}

TEST_CASE(ObjectPool_GetObjectsByKey) {
    ObjectPool pool;
    auto a = NO<NamedObject>::create();
    auto b = NO<NamedObject>::create();
    auto c = NO<NamedObject>::create();
    pool.addObject("x", a);
    pool.addObject("x", b);
    pool.addObject("y", c);
    auto xs = pool.getObjects("x");
    ASSERT_EQ(xs.size(), 2u);
}

// =============================================================================
// LangCommon
// =============================================================================

TEST_CASE(TaggerRes_Constructor) {
    TaggerRes r("hello");
    ASSERT_EQ(r.lyric, "hello");
    ASSERT_EQ(r.language, "unknown");
    ASSERT_EQ(r.tag, "unknown");
    ASSERT_FALSE(r.discard);
}

TEST_CASE(TaggerRes_LanguageTags) {
    TaggerRes r("hi", "en", "ENG");
    ASSERT_EQ(r.lyric, "hi");
    ASSERT_EQ(r.language, "en");
    ASSERT_EQ(r.tag, "ENG");
}

TEST_CASE(G2pInput_Constructor) {
    G2pInput input("hello", "eng");
    ASSERT_EQ(input.lyric, "hello");
    ASSERT_EQ(input.g2pId, "eng");
}

TEST_CASE(G2pRes_DefaultConstructor) {
    G2pRes res;
    ASSERT_EQ(res.mode, "copy");
    ASSERT_EQ(res.errorType, NoError);
}

TEST_CASE(G2pRes_FullConstructor) {
    G2pRes res("hello", "eng", "", "hh ah l ow", std::vector<std::string>{}, "g2p", NoError);
    ASSERT_EQ(res.lyric, "hello");
    ASSERT_EQ(res.g2pId, "eng");
    ASSERT_EQ(res.pronunciation, "hh ah l ow");
    ASSERT_EQ(res.mode, "g2p");
    ASSERT_EQ(res.errorType, NoError);
    // candidates auto-populated from pronunciation
    ASSERT_EQ(res.candidates.size(), 1u);
    ASSERT_EQ(res.candidates[0], "hh ah l ow");
}

TEST_CASE(G2pRes_EmptyPronunciation_FallbackToLyric) {
    G2pRes res("hello", "eng", "", "", std::vector<std::string>{}, "copy", NoError);
    ASSERT_EQ(res.pronunciation, "hello");
}

TEST_CASE(G2pErrorType_Values) {
    ASSERT_EQ(static_cast<int>(NoError), 0);
    ASSERT_EQ(static_cast<int>(InvalidLyric), 1);
    ASSERT_EQ(static_cast<int>(ModelInferenceFailed), 2);
    ASSERT_EQ(static_cast<int>(PhonemeGenerationFailed), 3);
    ASSERT_EQ(static_cast<int>(DriverUnavailable), 4);
    ASSERT_EQ(static_cast<int>(UnknownError), 5);
}

// =============================================================================
// DisplayText
// =============================================================================

TEST_CASE(DisplayText_DefaultIsEmpty) {
    DisplayText dt;
    ASSERT_TRUE(dt.isEmpty());
}

TEST_CASE(DisplayText_FromString) {
    DisplayText dt(std::string("hello"));
    ASSERT_EQ(dt.text(), "hello");
}

TEST_CASE(DisplayText_FromJsonString) {
    JsonValue jv(std::string("test"));
    DisplayText dt(jv);
    ASSERT_EQ(dt.text(), "test");
}

TEST_CASE(DisplayText_FromJsonObject) {
    JsonObject obj;
    obj["_"] = JsonValue(std::string("default"));
    obj["en"] = JsonValue(std::string("English"));
    obj["zh"] = JsonValue(std::string("\xe4\xb8\xad\xe6\x96\x87"));
    JsonValue jv2(obj);
    DisplayText dt(jv2);
    ASSERT_EQ(dt.text("en"), "English");
    ASSERT_EQ(dt.text("zh"), "\xe4\xb8\xad\xe6\x96\x87");
}

TEST_CASE(DisplayText_FallbackOrder) {
    JsonObject obj;
    obj["ja"] = JsonValue(std::string("Japanese"));
    JsonValue jv3(obj);
    DisplayText dt(jv3);
    ASSERT_EQ(dt.defaultText(), "Japanese");
}

TEST_CASE(DisplayText_AssignString) {
    DisplayText dt;
    dt = std::string("assigned");
    ASSERT_EQ(dt.text(), "assigned");
}

// =============================================================================
// Tensor
// =============================================================================

TEST_CASE(Tensor_CreateFloat) {
    auto exp = Tensor::create(ITensor::Float, {2, 3});
    ASSERT_TRUE(exp.hasValue());
    auto t = exp.take();
    ASSERT_NE(t.get(), nullptr);
    auto s = t->shape();
    ASSERT_EQ(s.size(), 2u);
    ASSERT_EQ(s[0], 2);
    ASSERT_EQ(s[1], 3);
}

TEST_CASE(Tensor_CreateInt64) {
    auto exp = Tensor::create(ITensor::Int64, {4});
    ASSERT_TRUE(exp.hasValue());
    auto t = exp.take();
    ASSERT_NE(t.get(), nullptr);
    ASSERT_EQ(t->shape().size(), 1u);
    ASSERT_EQ(t->shape()[0], 4);
}

TEST_CASE(Tensor_ElementCount) {
    auto exp = Tensor::create(ITensor::Float, {2, 3});
    ASSERT_TRUE(exp.hasValue());
    auto t = exp.take();
    ASSERT_EQ(t->elementCount(), 6u);
}

TEST_CASE(Tensor_CreateScalar) {
    auto exp = Tensor::createScalar(3.14f);
    ASSERT_TRUE(exp.hasValue());
    auto t = exp.take();
    ASSERT_NE(t.get(), nullptr);
    const float *p = t->data<float>();
    ASSERT_NE(p, nullptr);
    ASSERT_TRUE(*p > 3.13f && *p < 3.15f);
}

TEST_CASE(Tensor_CreateFilled) {
    auto exp = Tensor::createFilled<float>({3}, 1.0f);
    ASSERT_TRUE(exp.hasValue());
    auto t = exp.take();
    ASSERT_EQ(t->elementCount(), 3u);
    const float *p = t->data<float>();
    ASSERT_NE(p, nullptr);
    for (size_t i = 0; i < 3; i++) {
        ASSERT_TRUE(p[i] > 0.99f && p[i] < 1.01f);
    }
}

TEST_CASE(Tensor_Clone) {
    auto exp = Tensor::createFilled<float>({4}, 2.0f);
    ASSERT_TRUE(exp.hasValue());
    auto orig = exp.take();
    auto cloned = orig->clone();
    ASSERT_NE(cloned.get(), nullptr);
    ASSERT_EQ(cloned->elementCount(), orig->elementCount());
    const float *a = orig->data<float>();
    const float *b = cloned->data<float>();
    ASSERT_NE(a, b); // different memory
    for (size_t i = 0; i < orig->elementCount(); i++) {
        ASSERT_TRUE(a[i] > 1.99f && a[i] < 2.01f);
        ASSERT_TRUE(b[i] > 1.99f && b[i] < 2.01f);
    }
}

TEST_CASE(Tensor_MutableData) {
    auto exp = Tensor::create(ITensor::Float, {3});
    ASSERT_TRUE(exp.hasValue());
    auto t = exp.take();
    float *p = t->mutableData<float>();
    ASSERT_NE(p, nullptr);
    p[0] = 10.0f;
    p[1] = 20.0f;
    p[2] = 30.0f;
    const float *cp = t->data<float>();
    ASSERT_TRUE(cp[0] > 9.99f && cp[0] < 10.01f);
    ASSERT_TRUE(cp[1] > 19.99f && cp[1] < 20.01f);
    ASSERT_TRUE(cp[2] > 29.99f && cp[2] < 30.01f);
}

// =============================================================================
// AlignedAllocator
// =============================================================================

TEST_CASE(AlignedAllocator_AllocateDeallocate) {
    AlignedAllocator<float, 64> alloc;
    float *p = alloc.allocate(10);
    ASSERT_NE(p, nullptr);
    alloc.deallocate(p, 10);
}

TEST_CASE(AlignedAllocator_Alignment) {
    AlignedAllocator<float, 64> alloc;
    float *p = alloc.allocate(16);
    auto addr = reinterpret_cast<std::uintptr_t>(p);
    ASSERT_EQ(addr % 64, 0u);
    alloc.deallocate(p, 16);
}

TEST_CASE(AlignedAllocator_VectorUsage) {
    std::vector<float, AlignedAllocator<float, 64>> vec;
    vec.push_back(1.0f);
    vec.push_back(2.0f);
    vec.push_back(3.0f);
    ASSERT_EQ(vec.size(), 3u);
    ASSERT_TRUE(vec[0] > 0.99f && vec[0] < 1.01f);
    ASSERT_TRUE(vec[1] > 1.99f && vec[1] < 2.01f);
    ASSERT_TRUE(vec[2] > 2.99f && vec[2] < 3.01f);
    auto addr = reinterpret_cast<std::uintptr_t>(vec.data());
    ASSERT_EQ(addr % 64, 0u);
}
