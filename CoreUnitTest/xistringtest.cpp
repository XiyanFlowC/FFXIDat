#include "pch.h"
#include "pch.h"
#include "../FFXIDat/XiString.h"
#include "../FFXIDat/EventString.h"
#include <string>


// ==================== XiString Encode/Decode Tests ====================

class XiStringCodecTest : public ::testing::Test {
protected:
    // Helper function to compare encoded/decoded strings
    void TestRoundTrip(const std::string& original) {
        std::string encoded = XiString::Encode(original);
        std::string decoded = XiString::Decode(encoded);
        EXPECT_EQ(decoded, original);
    }
};

// Basic text tests
TEST_F(XiStringCodecTest, EncodeDecodeSimpleText) {
    TestRoundTrip("Hello World");
}

TEST_F(XiStringCodecTest, EncodeDecodeEmptyString) {
    TestRoundTrip("");
}

// Escape sequences
TEST_F(XiStringCodecTest, EncodeDecodeBackslash) {
    TestRoundTrip("Path\\\\File");
}

TEST_F(XiStringCodecTest, EncodeDecodeDollar) {
    TestRoundTrip("Price: $$180");
}

TEST_F(XiStringCodecTest, EncodeDecodeHexEscape) {
    std::string original = "\\x41\\x42\\x43";
    std::string encoded = XiString::Encode(original);
    EXPECT_EQ(encoded, "ABC");
}

// $if control sequence
TEST_F(XiStringCodecTest, EncodeDecodeIf) {
    TestRoundTrip("$if:01:02:03:04;");
}

TEST_F(XiStringCodecTest, EncodeDecodeIfWithContent) {
    TestRoundTrip("$if:01:02:03:04;$eq;Content$endif;");
}

TEST_F(XiStringCodecTest, EncodeDecodeIfWithElse) {
    TestRoundTrip("$if:01:02:03:04;$eq;True$else;False$endif;");
}

// $switch control sequence
TEST_F(XiStringCodecTest, EncodeDecodeSwitch) {
    TestRoundTrip("$switch:01:8010:8020;case1$sep;out of switch");
}

TEST_F(XiStringCodecTest, EncodeSwitchMultipleCases) {
    TestRoundTrip("$switch:01:8010:8020:8030:8040;123$sep;456$sep;lost of switch");
}

TEST_F(XiStringCodecTest, EncodeSwitchCalculatesCaseCount) {
    std::string original = "$switch:05:1880:2880:3880;";
    std::string encoded = XiString::Encode(original);
    // Should have case count of 2 (3 params - 1 base param)
    // Verify by decoding
    std::string decoded = XiString::Decode(encoded);
    EXPECT_EQ(decoded, original);
}

// $time control sequence
TEST_F(XiStringCodecTest, EncodeDecodeTime) {
    TestRoundTrip("$time:01:02:03:04;");
}

// $num control sequence
TEST_F(XiStringCodecTest, EncodeDecodeNum) {
    TestRoundTrip("$num:FF;");
}

// $str control sequence
TEST_F(XiStringCodecTest, EncodeDecodeStr) {
    TestRoundTrip("$str:AB:CD;");
}

// $item control sequence
TEST_F(XiStringCodecTest, EncodeDecodeItem) {
    TestRoundTrip("$item:01:02:03:04:05:06;");
}

// $sep control sequence
TEST_F(XiStringCodecTest, EncodeSep) {
    TestRoundTrip("Option1$sep;Option2");
}

// Comparison operators
TEST_F(XiStringCodecTest, EncodeDecodeNe) {
    TestRoundTrip("$if:01:02:03:04;$ne;Not equal$endif;");
}

TEST_F(XiStringCodecTest, EncodeDecodeEq) {
    TestRoundTrip("$if:01:02:03:04;$eq;Equal$endif;");
}

TEST_F(XiStringCodecTest, EncodeDecodeLe) {
    TestRoundTrip("$if:01:02:03:04;$le;Less or equal$endif;");
}

TEST_F(XiStringCodecTest, EncodeDecodeGe) {
    TestRoundTrip("$if:01:02:03:04;$ge;Greater or equal$endif;");
}

TEST_F(XiStringCodecTest, EncodDecodeLt) {
    TestRoundTrip("$if:01:02:03:04;$lt;Less than$endif;");
}

TEST_F(XiStringCodecTest, EncodeDecodeGt) {
    TestRoundTrip("$if:01:02:03:04;$gt;Greater than$endif;");
}

// Complex nested structures
TEST_F(XiStringCodecTest, EncodeDecodeNestedIf) {
    TestRoundTrip("$if:01:02:03:04;$eq;$if:05:06:07:08;$ne;Inner$endif;$endif;");
}

TEST_F(XiStringCodecTest, EncodeDecodeComplexExpression) {
    TestRoundTrip("Name: $str:01:02; Value: $num:FF; $if:10:20:30:40;$eq;Active$else;Inactive$endif;");
}

// SJIS characters
TEST_F(XiStringCodecTest, EncodeSJISCharacters) {
    std::string original = "\x82\xA0\x82\xA2\x82\xA4"; // ¤¢¤¤¤¦
    std::string encoded = XiString::Encode(original);
    std::string decoded = XiString::Decode(encoded);
    EXPECT_EQ(decoded, original);
}

// Mixed content
TEST_F(XiStringCodecTest, EncodeMixedContent) {
    std::string original("Text \\x41 $num:10; more $$money $if:01:02:03:04;$eq;yes$endif;");
	std::string encoded = XiString::Encode(original);
	std::string decoded = XiString::Decode(encoded);
	EXPECT_EQ(decoded, "Text A $num:10; more $$money $if:01:02:03:04;$eq;yes$endif;");
}

// Edge cases
TEST_F(XiStringCodecTest, EncodeDecodeOnlyControlSequences) {
    TestRoundTrip("$num:01;$str:02:03;$sep;");
}

TEST_F(XiStringCodecTest, EncodeMultipleEscapes) {
    TestRoundTrip("\\\\x41\\\\x42$$$$");
}

// Error cases
TEST_F(XiStringCodecTest, EncodeMissingIfTerminator) {
    EXPECT_THROW(XiString::Encode("$if:01:02:03"), std::runtime_error);
}

TEST_F(XiStringCodecTest, EncodeMissingSwitchTerminator) {
    EXPECT_THROW(XiString::Encode("$switch:01:8010"), std::runtime_error);
}

TEST_F(XiStringCodecTest, EncodeMissingTimeTerminator) {
    EXPECT_THROW(XiString::Encode("$time:01:02:03:04"), std::runtime_error);
}

TEST_F(XiStringCodecTest, EncodeInvalidControlSequence) {
    EXPECT_THROW(XiString::Encode("$invalid:01;"), std::runtime_error);
}

// GetStep tests
TEST_F(XiStringCodecTest, GetStepReturnsZeroForNonControl) {
    EXPECT_EQ(XiString::GetStep("ABC"), 0);
}

TEST_F(XiStringCodecTest, GetStepForSwitch) {
    // FA 40 82 01 param caseCount(2bytes) base(2bytes) + N*2bytes
    // For 2 cases: 2+1+1+1+2+2+2*2 = 13
    std::string seq = "\xFA\x40\x82\x01\x05\x82\x80\x80\x10\x80\x20";
    EXPECT_EQ(XiString::GetStep(seq.c_str()), 13);
}

TEST_F(XiStringCodecTest, GetStepForTime) {
    // FA 40 81 85 + 4 bytes = 2+1+4 = 7
    EXPECT_EQ(XiString::GetStep("\xFA\x40\x81\x85"), 7);
}

TEST_F(XiStringCodecTest, GetStepForIf) {
    // FA 40 83 01 + 4 bytes = 2+1+4 = 7
    EXPECT_EQ(XiString::GetStep("\xFA\x40\x83\x01"), 7);
}

TEST_F(XiStringCodecTest, GetStepForSep) {
    // FA 40 8D = 3
    EXPECT_EQ(XiString::GetStep("\xFA\x40\x8D"), 3);
}

// Comprehensive round-trip test
TEST_F(XiStringCodecTest, ComprehensiveRoundTrip) {
    std::string original = 
        "Quest: $str:01:02;\r"
        "Status: $if:10:20:30:40;$eq;Complete$else;Incomplete$endif;\r"
        "Reward: $item:01:02:03:04:05:06;\r"
        "Count: $num:FF;\r"
        "Price: $$180\r"
        "Path: \\\\x42\\\\x43";

    std::string encoded = XiString::Encode(original);
    std::string decoded = XiString::Decode(encoded);
    EXPECT_EQ(decoded, original);
}
TEST_F(XiStringCodecTest, IfDecodeTest) {
    std::string original = "If \xFA\x40\x83\x01\x81\x03\x81\x80\xFA\x40\x89\x0AQWERTY\xFA\x40\x84\x04XLTE<out of if>";
	std::string decoded = XiString::Decode(original);
	EXPECT_EQ(decoded, "If $if:81:03:81:80;$lt;QWERTY$else;XLTE$endif;<out of if>");
}

// Test with real game-like data
TEST_F(XiStringCodecTest, RealWorldExample1) {
    TestRoundTrip("Search result: $if:81:03:81:80;$ne;$num:81; people $else;Only one person $endif;found in this area.");
}

TEST_F(XiStringCodecTest, RealWorldExample2) {
    TestRoundTrip("$str:80:80; invites you to form an alliance with $switch:81:A080:8C80:9280:9880;his$sep;her$sep;their$sep; party.");
}

TEST_F(XiStringCodecTest, RealWorldExample3) {
    TestRoundTrip("$switch:81:FB80:A080:A880:AF80:B680:BD80:C480:CA80:D180:D880:DF80:E680:ED80:F480;dummy$sep;Jan.$sep;Feb.$sep;Mar.$sep;Apr.$sep;May$sep;Jun.$sep;Jul.$sep;Aug.$sep;Sep.$sep;Oct.$sep;Nov.$sep;Dec.$sep;");
}