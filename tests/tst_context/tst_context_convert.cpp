#include "tst_framework.h"

#include <LangCore/Base/LangCommon.h>
#include <LangCore/Support/ContextUtils.h>

// --- G2pInput construction ---

TEST_CASE(convertInput_defaultConstruction) {
    LangCore::G2pInput input;
    ASSERT_TRUE(input.lyric.empty());
    ASSERT_TRUE(input.g2pId.empty());
    ASSERT_TRUE(input.context.empty());
}

TEST_CASE(convertInput_withContext) {
    LangCore::G2pInput input("你好", "g2p-cmn-custom", "SingerA");
    ASSERT_STREQ(input.lyric.c_str(), "你好");
    ASSERT_STREQ(input.g2pId.c_str(), "g2p-cmn-custom");
    ASSERT_STREQ(input.context.c_str(), "SingerA");
}

TEST_CASE(convertInput_defaultContext) {
    LangCore::G2pInput input("hello", "g2p-eng", "");
    ASSERT_STREQ(input.context.c_str(), "");
}

TEST_CASE(convertInput_contextOmitted) {
    LangCore::G2pInput input("hello", "g2p-eng");
    ASSERT_STREQ(input.context.c_str(), "");
}

// --- G2pRes context field ---

TEST_CASE(g2pRes_contextField) {
    LangCore::G2pRes res("hello", "eng", "SingerA", "hh ah l ow");
    ASSERT_STREQ(res.context.c_str(), "SingerA");
}

TEST_CASE(g2pRes_defaultContext) {
    LangCore::G2pRes res("hello", "eng", "", "hh ah l ow");
    ASSERT_STREQ(res.context.c_str(), "");
}

TEST_CASE(g2pRes_contextPreserved) {
    LangCore::G2pRes res("hello", "eng", "SingerA", "hh ah l ow");
    ASSERT_STREQ(res.context.c_str(), "SingerA");
    ASSERT_STREQ(res.lyric.c_str(), "hello");
    ASSERT_STREQ(res.g2pId.c_str(), "eng");
    ASSERT_STREQ(res.pronunciation.c_str(), "hh ah l ow");
}

// --- Context name validation ---

TEST_CASE(validate_contextForConvert_valid) {
    ASSERT_TRUE(LangCore::ContextUtils::validateContextName("").hasValue());
    ASSERT_TRUE(LangCore::ContextUtils::validateContextName("SingerA").hasValue());
    ASSERT_TRUE(LangCore::ContextUtils::validateContextName("singer-v1.0_test").hasValue());
    ASSERT_TRUE(LangCore::ContextUtils::validateContextName("A").hasValue());
    ASSERT_TRUE(LangCore::ContextUtils::validateContextName("0123456789").hasValue());
}

TEST_CASE(validate_contextForConvert_invalid) {
    ASSERT_FALSE(LangCore::ContextUtils::validateContextName("Singer A").hasValue());
    ASSERT_FALSE(LangCore::ContextUtils::validateContextName("A:B").hasValue());
    std::string tooLong(129, 'X');
    ASSERT_FALSE(LangCore::ContextUtils::validateContextName(tooLong).hasValue());
    ASSERT_FALSE(LangCore::ContextUtils::validateContextName("A/B").hasValue());
    ASSERT_FALSE(LangCore::ContextUtils::validateContextName("A\\B").hasValue());
}

// --- Empty input vector ---

TEST_CASE(convert_emptyInput_conceptual) {
    std::vector<LangCore::G2pInput> inputs;
    ASSERT_EQ(inputs.size(), 0u);
}
