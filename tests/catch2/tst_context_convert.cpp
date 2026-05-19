#include "catch.hpp"

#include <LangCore/Base/LangCommon.h>
#include <LangCore/Support/ContextUtils.h>

// --- G2pInput construction ---

TEST_CASE("convertInput_defaultConstruction") {
    LangCore::G2pInput input;
    REQUIRE(input.lyric.empty());
    REQUIRE(input.g2pId.empty());
    REQUIRE(input.context.empty());
}

TEST_CASE("convertInput_withContext") {
    LangCore::G2pInput input("你好", "g2p-cmn-custom", "SingerA");
    REQUIRE(input.lyric == "你好");
    REQUIRE(input.g2pId == "g2p-cmn-custom");
    REQUIRE(input.context == "SingerA");
}

TEST_CASE("convertInput_defaultContext") {
    LangCore::G2pInput input("hello", "g2p-eng", "");
    REQUIRE(input.context == "");
}

TEST_CASE("convertInput_contextOmitted") {
    LangCore::G2pInput input("hello", "g2p-eng");
    REQUIRE(input.context == "");
}

// --- G2pRes context field ---

TEST_CASE("g2pRes_contextField") {
    LangCore::G2pRes res("hello", "eng", "SingerA", "hh ah l ow");
    REQUIRE(res.context == "SingerA");
}

TEST_CASE("g2pRes_defaultContext") {
    LangCore::G2pRes res("hello", "eng", "", "hh ah l ow");
    REQUIRE(res.context == "");
}

TEST_CASE("g2pRes_contextPreserved") {
    LangCore::G2pRes res("hello", "eng", "SingerA", "hh ah l ow");
    REQUIRE(res.context == "SingerA");
    REQUIRE(res.lyric == "hello");
    REQUIRE(res.g2pId == "eng");
    REQUIRE(res.pronunciation == "hh ah l ow");
}

// --- Context name validation ---

TEST_CASE("validate_contextForConvert_valid") {
    REQUIRE(LangCore::ContextUtils::validateContextName("").hasValue());
    REQUIRE(LangCore::ContextUtils::validateContextName("SingerA").hasValue());
    REQUIRE(LangCore::ContextUtils::validateContextName("singer-v1.0_test").hasValue());
    REQUIRE(LangCore::ContextUtils::validateContextName("A").hasValue());
    REQUIRE(LangCore::ContextUtils::validateContextName("0123456789").hasValue());
}

TEST_CASE("validate_contextForConvert_invalid") {
    REQUIRE_FALSE(LangCore::ContextUtils::validateContextName("Singer A").hasValue());
    REQUIRE_FALSE(LangCore::ContextUtils::validateContextName("A:B").hasValue());
    std::string tooLong(129, 'X');
    REQUIRE_FALSE(LangCore::ContextUtils::validateContextName(tooLong).hasValue());
    REQUIRE_FALSE(LangCore::ContextUtils::validateContextName("A/B").hasValue());
    REQUIRE_FALSE(LangCore::ContextUtils::validateContextName("A\\B").hasValue());
}

// --- Empty input vector ---

TEST_CASE("convert_emptyInput_conceptual") {
    std::vector<LangCore::G2pInput> inputs;
    REQUIRE(inputs.size() == 0u);
}
