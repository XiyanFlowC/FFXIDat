#include "pch.h"
#include "../FFXIDat/XiString.h"
#include "../FFXIDat/EventString.h"
#include <string>

// ==================== EventStringCodecUtil Tests ====================

class EventStringCodecTest : public ::testing::Test {
protected:
    EventStringCodecUtil& codec = EventStringCodecUtil::Instance();
};

// Basic encode/decode tests
TEST_F(EventStringCodecTest, EncodeDecodeSimpleText) {
    std::string original = "Hello World";
    std::string encoded = codec.Encode(original);
    std::string decoded = codec.Decode(encoded);
    EXPECT_EQ(decoded, original);
}

TEST_F(EventStringCodecTest, EncodeDecodeEmptyString) {
    std::string original = "";
    std::string encoded = codec.Encode(original);
    std::string decoded = codec.Decode(encoded);
    EXPECT_EQ(decoded, original);
}

TEST_F(EventStringCodecTest, EncodeAddsNullTerminator) {
    std::string original = "Test";
    std::string encoded = codec.Encode(original);
    EXPECT_EQ(encoded[encoded.size() - 1], '\0');
}

// Control sequence tests
TEST_F(EventStringCodecTest, EncodeDecodeLineFeed) {
    std::string original = "Line1<lf>Line2";
    std::string encoded = codec.Encode(original);
    std::string decoded = codec.Decode(encoded);
    EXPECT_EQ(decoded, original);
}

TEST_F(EventStringCodecTest, EncodeDecodeName) {
    std::string original = "Hello <name>";
    std::string encoded = codec.Encode(original);
    std::string decoded = codec.Decode(encoded);
    EXPECT_EQ(decoded, original);
}

TEST_F(EventStringCodecTest, EncodeDecodeNum) {
    std::string original = "Value: <num:1A>";
    std::string encoded = codec.Encode(original);
    std::string decoded = codec.Decode(encoded);
    EXPECT_EQ(decoded, original);
}

TEST_F(EventStringCodecTest, EncodeDecodeItem) {
    std::string original = "Get <item:5>";
    std::string encoded = codec.Encode(original);
    std::string decoded = codec.Decode(encoded);
    EXPECT_EQ(decoded, original);
}

TEST_F(EventStringCodecTest, EncodeDecodeMagic) {
    std::string original = "Cast <magic:A>";
    std::string encoded = codec.Encode(original);
    std::string decoded = codec.Decode(encoded);
    EXPECT_EQ(decoded, original);
}

TEST_F(EventStringCodecTest, EncodeDecodeSwitch) {
    std::string original = "Option <switch:2>[123/456/789/987]";
    std::string encoded = codec.Encode(original);
    std::string decoded = codec.Decode(encoded);
    EXPECT_EQ(decoded, original);
}

// Gender control sequences
TEST_F(EventStringCodecTest, EncodeDecodeGender) {
    std::string original = "Player <gender>";
    std::string encoded = codec.Encode(original);
    std::string decoded = codec.Decode(encoded);
    EXPECT_EQ(decoded, original);
}

TEST_F(EventStringCodecTest, EncodeDecodeGenderSrc) {
    std::string original = "Source <gender-src>";
    std::string encoded = codec.Encode(original);
    std::string decoded = codec.Decode(encoded);
    EXPECT_EQ(decoded, original);
}

TEST_F(EventStringCodecTest, EncodeDecodeGenderDst) {
    std::string original = "Target <gender-dst>";
    std::string encoded = codec.Encode(original);
    std::string decoded = codec.Decode(encoded);
    EXPECT_EQ(decoded, original);
}

// Multiple control sequences
TEST_F(EventStringCodecTest, EncodeDecodeMultipleControls) {
    std::string original = "<name> got <item:1><lf>HP: <num:FF>";
    std::string encoded = codec.Encode(original);
    std::string decoded = codec.Decode(encoded);
    EXPECT_EQ(decoded, original);
}

// Hex value control sequences
TEST_F(EventStringCodecTest, EncodeDecodeHexValue) {
    std::string original = "Code <A5>";
    std::string encoded = codec.Encode(original);
    std::string decoded = codec.Decode(encoded);
    EXPECT_EQ(decoded, original);
}

// ins control sequence with variable length
TEST_F(EventStringCodecTest, EncodeDecodeInsSequence) {
    std::string original = "Test <ins:8:1:2:3:4:5:6:7:8>";
    std::string encoded = codec.Encode(original);
    std::string decoded = codec.Decode(encoded);
    EXPECT_EQ(decoded, original);
}

// Edge case: string ending control
TEST_F(EventStringCodecTest, EncodeDecodeEndingControl) {
    std::string original = "Text<->More";
    std::string encoded = codec.Encode(original);
    // Ending control should terminate the string
    std::string decoded = codec.Decode(encoded);
    EXPECT_EQ(decoded, "Text<->");
}

// SJIS double-byte characters
TEST_F(EventStringCodecTest, DecodeSJISCharacters) {
    // SJIS test: 0x82A0 is дв in Shift-JIS
    std::string encoded = "\x82\xA0Test\x80";
    std::string decoded = codec.Decode(encoded);
    EXPECT_TRUE(decoded.find('\x82') != std::string::npos);
    EXPECT_TRUE(decoded.find('\xA0') != std::string::npos);
}

// Special case: 0x7F sequences
TEST_F(EventStringCodecTest, Decode7FSequence) {
    std::string encoded = "\x7F\x85\x00"; // gender sequence
    std::string decoded = codec.Decode(encoded);
    EXPECT_EQ(decoded, "<gender>");
}

// Null byte handling
TEST_F(EventStringCodecTest, DecodeStopsAtNull) {
    std::string encoded = "Hello\x00World\x00";
    std::string decoded = codec.Decode(encoded.c_str(), encoded.size());
    EXPECT_EQ(decoded, "Hello");
}