#include "catch.hpp"

#include <LangCore/Support/ContextUtils.h>

using namespace LangCore;

// --- FQID Parsing ---

TEST_CASE("parse_plain") {
    auto result = ContextUtils::parseFqid("g2p-cmn-official");
    REQUIRE(result.context == "");
    REQUIRE(result.moduleId == "g2p-cmn-official");
}

TEST_CASE("parse_withContext") {
    auto result = ContextUtils::parseFqid("SingerA:g2p-cmn-custom");
    REQUIRE(result.context == "SingerA");
    REQUIRE(result.moduleId == "g2p-cmn-custom");
}

TEST_CASE("parse_emptyContext") {
    auto result = ContextUtils::parseFqid(":g2p-cmn");
    REQUIRE(result.context == "");
    REQUIRE(result.moduleId == "g2p-cmn");
}

TEST_CASE("parse_multipleColons") {
    auto result = ContextUtils::parseFqid("A:B:C");
    REQUIRE(result.context == "A");
    REQUIRE(result.moduleId == "B:C");
}

// --- FQID Formatting ---

TEST_CASE("format_plain") {
    auto fqid = ContextUtils::formatFqid("", "g2p-cmn");
    REQUIRE(fqid == "g2p-cmn");
}

TEST_CASE("format_withContext") {
    auto fqid = ContextUtils::formatFqid("SingerA", "g2p-cmn");
    REQUIRE(fqid == "SingerA:g2p-cmn");
}

// --- Context Name Validation ---

TEST_CASE("validate_emptyContext") {
    auto result = ContextUtils::validateContextName("");
    REQUIRE(result.hasValue());
}

TEST_CASE("validate_validContext") {
    auto result = ContextUtils::validateContextName("SingerA");
    REQUIRE(result.hasValue());
}

TEST_CASE("validate_validWithDotDash") {
    auto result = ContextUtils::validateContextName("Singer.A-v1_0");
    REQUIRE(result.hasValue());
}

TEST_CASE("validate_invalidSpace") {
    auto result = ContextUtils::validateContextName("Singer A");
    REQUIRE_FALSE(result.hasValue());
}

TEST_CASE("validate_invalidColon") {
    auto result = ContextUtils::validateContextName("A:B");
    REQUIRE_FALSE(result.hasValue());
}

TEST_CASE("validate_invalidSlash") {
    auto result = ContextUtils::validateContextName("A/B");
    REQUIRE_FALSE(result.hasValue());
}

TEST_CASE("validate_invalidBackslash") {
    auto result = ContextUtils::validateContextName("A\\B");
    REQUIRE_FALSE(result.hasValue());
}

TEST_CASE("validate_tooLong") {
    std::string longName(129, 'A');
    auto result = ContextUtils::validateContextName(longName);
    REQUIRE_FALSE(result.hasValue());
}

TEST_CASE("validate_maxLength") {
    std::string maxName(128, 'A');
    auto result = ContextUtils::validateContextName(maxName);
    REQUIRE(result.hasValue());
}

// --- Module ID Validation ---

TEST_CASE("validateModuleId_valid") {
    auto result = ContextUtils::validateModuleId("g2p-cmn-custom");
    REQUIRE(result.hasValue());
}

TEST_CASE("validateModuleId_withColon") {
    auto result = ContextUtils::validateModuleId("g2p:cmn");
    REQUIRE_FALSE(result.hasValue());
}

// --- FQID Roundtrip ---

TEST_CASE("fqid_roundtrip_withContext") {
    auto fqid = ContextUtils::formatFqid("SingerA", "g2p-cmn-custom");
    auto parsed = ContextUtils::parseFqid(fqid);
    REQUIRE(parsed.context == "SingerA");
    REQUIRE(parsed.moduleId == "g2p-cmn-custom");
}

TEST_CASE("fqid_roundtrip_defaultContext") {
    auto fqid = ContextUtils::formatFqid("", "g2p-cmn-official");
    auto parsed = ContextUtils::parseFqid(fqid);
    REQUIRE(parsed.context == "");
    REQUIRE(parsed.moduleId == "g2p-cmn-official");
}
