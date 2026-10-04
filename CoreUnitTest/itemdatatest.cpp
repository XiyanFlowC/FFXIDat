#include "pch.h"
#include "../FFXIDat/ItemData.h"
#include "../FFXIDat/CsvFile.h"
#include "../FFXIDat/MonBridge.h"
#include "../FFXIDat/RecordsOfEminence.h"
#include "../FFXIDat/ItemFormatV30.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <iostream>

namespace {
// Whether a variant can hold T at all. This is a question about the type, so a
// default constructed variant (which holds its first alternative) answers it
// just as well as any other value; std::holds_alternative asks about the
// alternative a particular value currently holds instead.
template <class T, class Variant>
struct VariantHolds : std::false_type {};

template <class T, class... Ts>
struct VariantHolds<T, std::variant<Ts...>> : std::bool_constant<(std::is_same_v<T, Ts> || ...)> {};

Record* GetRecord(ItemEntry& entry, ItemSpecType type)
{
	switch (type) {
	case ItemSpecType::NORMAL: return &entry.spec.normal.info_rec;
	case ItemSpecType::USABLE: return &entry.spec.usable.info_rec;
	case ItemSpecType::WEAPON: return &entry.spec.weapon.info_rec;
	case ItemSpecType::ARMOUR: return &entry.spec.armour.info_rec;
	case ItemSpecType::PUPPET: return &entry.spec.puppet.info_rec;
	case ItemSpecType::SLIP: return &entry.spec.slip.info_rec;
	case ItemSpecType::CURRENCY: return &entry.spec.currency.info_rec;
	case ItemSpecType::INSTINCT: return &entry.spec.instinct.info_rec;
	}
	throw std::invalid_argument("Unknown item type");
}

std::vector<char> MakeFile(ItemSpecType type, bool english = false)
{
	ItemEntry entry{};
	entry.header.id = 123;
	entry.header.extended_flags = 0xA55A;
	entry.header.stack_size = 12;
	entry.end_marker = 0xFF;
	entry.image_data[4000] = 0x35;
	Row row;
	row.GetCells().emplace_back(u8"Name");
	if (english) {
		row.GetCells().emplace_back(7);
		row.GetCells().emplace_back(u8"Singular");
		row.GetCells().emplace_back(u8"Plural");
	}
	row.GetCells().emplace_back(u8"Description");
	auto* rec = GetRecord(entry, type);
	const int capacity = static_cast<int>(reinterpret_cast<char*>(&entry.image_length) - reinterpret_cast<char*>(rec));
	row.WriteRow(rec, capacity);
	std::vector<char> bytes(sizeof(ItemEntry) * (type == ItemSpecType::CURRENCY ? 16 : 1));
	std::memcpy(bytes.data(), &entry, sizeof(entry));
	return bytes;
}

// Legacy (de/fr) record: 0xC00 bytes, 14 byte header, no extended_flags.
std::vector<char> MakeLegacyFile(ItemSpecType type)
{
	using Format = itmfmt::v10::Format;
	const size_t recordSize = Format::slotSize;
	std::vector<char> bytes(recordSize * (Format::IsCurrencySchema(type) ? Format::currencySlots : 1), 0);
	char* record = bytes.data();

	auto put16 = [&](size_t offset, uint16_t value) { std::memcpy(record + offset, &value, 2); };
	auto put32 = [&](size_t offset, uint32_t value) { std::memcpy(record + offset, &value, 4); };

	put32(0, 10240);      // id
	put16(4, 0x0820);     // flags
	put16(6, 12);         // stack_size (legacy header has no extended_flags)
	put16(8, 5);          // item_type
	put16(10, 0x1234);    // resource_id
	put16(12, 0x0001);    // valid_targets

	// German style row (9 cells), matching the de/fr accessors in the datum.
	Row row;
	row.GetCells().emplace_back(u8"Verhexter Hauberk");
	row.GetCells().emplace_back(1);
	row.GetCells().emplace_back(u8"Verhexter Hauberk");
	row.GetCells().emplace_back(u8"Verhexte Hauberks");
	row.GetCells().emplace_back(u8"Verhexter Hauberk");        // singular
	row.GetCells().emplace_back(u8"Ein Fluch.");
	row.GetCells().emplace_back(u8"Ein Fluch.");
	row.GetCells().emplace_back(u8"Verhexte Hauberks");        // plural
	row.GetCells().emplace_back(u8"Ein Fluch lastet darauf."); // description
	const size_t TextOffset = Format::TextOffset(type);
	const size_t TextCapacity = Format::textEnd - TextOffset;
	if (static_cast<size_t>(row.GetSize()) > TextCapacity)
		throw std::runtime_error("test row too large");
	row.WriteRow(reinterpret_cast<Record*>(record + TextOffset), static_cast<int>(TextCapacity));

	record[recordSize - 1] = static_cast<char>(0xFF);
	return bytes;
}

void Rotate(std::vector<char>& bytes, bool encrypt)
{
	for (char& value : bytes) {
		const auto byte = static_cast<uint8_t>(value);
		value = static_cast<char>(encrypt ? (byte << 5) | (byte >> 3) : (byte >> 5) | (byte << 3));
	}
}

std::vector<char> Load(const std::filesystem::path& path)
{
	std::ifstream file(path, std::ios::binary);
	if (!file) throw std::runtime_error("Cannot open test DAT");
	return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

void Save(const std::filesystem::path& path, const std::vector<char>& bytes)
{
	std::ofstream file(path, std::ios::binary);
	file.write(bytes.data(), bytes.size());
	if (!file) throw std::runtime_error("Cannot write test DAT");
}

const char* GameRoot()
{
	return std::getenv("FFXI_ITEM_TEST_GAME_ROOT");
}

class ItemDataTest : public ::testing::Test {
protected:
	std::filesystem::path folder;
	void SetUp() override
	{
		folder = std::filesystem::current_path() / ("item_dat_test_" +
			std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
		std::filesystem::create_directory(folder);
	}
	void TearDown() override { std::filesystem::remove_all(folder); }
};
}

TEST_F(ItemDataTest, AllCurrentTypesRoundTripByteIdentically)
{
	for (const auto type : {ItemSpecType::NORMAL, ItemSpecType::USABLE, ItemSpecType::WEAPON,
		ItemSpecType::ARMOUR, ItemSpecType::PUPPET, ItemSpecType::SLIP,
		ItemSpecType::CURRENCY, ItemSpecType::INSTINCT}) {
		for (bool english : {false, true}) {
			auto bytes = MakeFile(type, english);
			Rotate(bytes, true);
			const auto input = folder / "input.DAT";
			const auto output = folder / "output.DAT";
			Save(input, bytes);
			ItemData items;
			ASSERT_NO_THROW(items.Read(input.wstring(), type));
			ASSERT_EQ(items.data.size(), 1u);
			EXPECT_TRUE(items.data[0].name() == u8"Name");
			EXPECT_TRUE(items.data[0].description() == u8"Description");
			items.data[0].visit([](const auto &datum) {
				using Datum = std::remove_cvref_t<decltype(datum)>;
				// extended_flags exists in the v30 header only.
				if constexpr (std::is_same_v<typename Datum::Entry, itmfmt::v30::Entry>)
					EXPECT_EQ(datum.flags().extended_flags, 0xA55A);
			});
			ASSERT_NO_THROW(items.Write(output.wstring()));
			EXPECT_EQ(Load(output), bytes);
		}
	}
}

TEST_F(ItemDataTest, AllLegacyTypesRoundTripByteIdentically)
{
	for (const auto type : {ItemSpecType::NORMAL, ItemSpecType::USABLE, ItemSpecType::WEAPON,
		ItemSpecType::ARMOUR, ItemSpecType::PUPPET, ItemSpecType::SLIP,
		ItemSpecType::CURRENCY, ItemSpecType::INSTINCT}) {
		auto bytes = MakeLegacyFile(type);
		Rotate(bytes, true);
		const auto input = folder / "input.DAT";
		const auto output = folder / "output.DAT";
		Save(input, bytes);
		ItemData items;
		ASSERT_NO_THROW(items.Read(input.wstring(), type, itmfmt::Version::V10));
		ASSERT_EQ(items.data.size(), 1u);
		EXPECT_TRUE(items.data[0].name() == u8"Verhexter Hauberk");
		EXPECT_TRUE(items.data[0].name_sg() == u8"Verhexter Hauberk");
		EXPECT_TRUE(items.data[0].name_pl() == u8"Verhexte Hauberks");
		EXPECT_TRUE(items.data[0].description() == u8"Ein Fluch lastet darauf.");
		EXPECT_EQ(items.data[0].stack_size(), 12);
		EXPECT_EQ(items.data[0].item_type(), 5);
		EXPECT_EQ(items.data[0].resource_id(), 0x1234);
		EXPECT_TRUE(items.data[0].layoutId() == "v10");
		// The v10 datum owns the v10 typed view: originalEntry is the memory
		// image of the slot, typed as the v10 Entry.
		EXPECT_EQ(items.data[0].raw().size(), itmfmt::v10::Format::slotSize);
		items.data[0].visit([](const auto &datum) {
			using Datum = std::remove_cvref_t<decltype(datum)>;
			const bool typedAsThisVersion = std::is_same_v<typename Datum::Entry, itmfmt::v10::Entry>;
			EXPECT_TRUE(typedAsThisVersion);
			EXPECT_EQ(datum.originalEntry.end_marker, 0xFF);
		});
		ASSERT_NO_THROW(items.Write(output.wstring()));
		EXPECT_EQ(Load(output), bytes);
	}
}

TEST_F(ItemDataTest, LegacyAndTruncatedFilesAreRejectedByTheCurrentLayout)
{
	const auto input = folder / "input.DAT";
	ItemData items;
	items.encryptionSuppression = true;

	// A 0xC00 record file is not a multiple of the current 0x1400 record size.
	Save(input, MakeLegacyFile(ItemSpecType::ARMOUR));
	EXPECT_THROW(items.Read(input.wstring(), ItemSpecType::ARMOUR), std::runtime_error);

	// Truncated current layout file.
	auto bytes = MakeFile(ItemSpecType::NORMAL);
	bytes.resize(0x1400 - 1);
	Save(input, bytes);
	EXPECT_THROW(items.Read(input.wstring()), std::runtime_error);
}

TEST_F(ItemDataTest, LegacyLayoutRejectsCurrentFile)
{
	const auto input = folder / "input.DAT";
	Save(input, MakeFile(ItemSpecType::NORMAL));
	ItemData items;
	items.encryptionSuppression = true;
	EXPECT_THROW(items.Read(input.wstring(), ItemSpecType::NORMAL, itmfmt::Version::V10), std::runtime_error);
}

TEST_F(ItemDataTest, TranslatedTextPreservesAttributesImageAndTail)
{
	auto bytes = MakeFile(ItemSpecType::ARMOUR, true);
	const auto input = folder / "input.DAT";
	const auto output = folder / "output.DAT";
	Save(input, bytes);
	ItemData items;
	items.encryptionSuppression = true;
	items.Read(input.wstring(), ItemSpecType::ARMOUR);
	items.data[0].setName(u8"Translated");
	items.data[0].setDescription(u8"Translated description");
	items.Write(output.wstring());
	const auto actual = Load(output);
	EXPECT_TRUE(std::equal(bytes.begin(), bytes.begin() + 48, actual.begin()));
	EXPECT_TRUE(std::equal(bytes.begin() + 640, bytes.end(), actual.begin() + 640));
	ItemData reread;
	reread.encryptionSuppression = true;
	reread.Read(output.wstring(), ItemSpecType::ARMOUR);
	EXPECT_TRUE(reread.data[0].name() == u8"Translated");
	EXPECT_TRUE(reread.data[0].description() == u8"Translated description");
}

TEST_F(ItemDataTest, RejectsInvalidTextLayoutAndNonzeroCurrencyPadding)
{
	auto bytes = MakeFile(ItemSpecType::NORMAL);
	int32_t badOffset = 612;
	std::memcpy(bytes.data() + 32, &badOffset, sizeof(badOffset));
	const auto input = folder / "input.DAT";
	Save(input, bytes);
	ItemData items;
	items.encryptionSuppression = true;
	EXPECT_THROW(items.Read(input.wstring()), std::runtime_error);
	bytes = MakeFile(ItemSpecType::CURRENCY);
	bytes.back() = 1;
	Save(input, bytes);
	EXPECT_THROW(items.Read(input.wstring(), ItemSpecType::CURRENCY), std::runtime_error);
}

TEST_F(ItemDataTest, RejectsOversizedTranslationBeforeSerializing)
{
	const auto input = folder / "input.DAT";
	Save(input, MakeFile(ItemSpecType::NORMAL));
	const auto output = folder / "output.DAT";
	const std::vector<char> previousOutput = {'k', 'e', 'e', 'p'};
	Save(output, previousOutput);
	ItemData items;
	items.encryptionSuppression = true;
	items.Read(input.wstring());
	items.data[0].setDescription(std::u8string(1000, u8'x'));
	EXPECT_THROW(items.Write(output.wstring()), std::runtime_error);
	EXPECT_EQ(Load(output), previousOutput);
}

// The unknown array of a schema owns one column per element, whether its
// elements are bytes or words: the count is asked of the struct itself, so a
// struct that grows an element cannot leave the header behind.
TEST_F(ItemDataTest, CsvColumnCountsFollowCurrentUnknownArrays)
{
	// The columns of a v30 row that every schema shares, in order.
	const char8_t *commonColumns[] = { u8"ID", u8"Name", u8"Description", u8"Flags", u8"ExtendedFlags",
		u8"Stack", u8"Type", u8"ResID", u8"Targets", u8"ImageLength" };
	constexpr size_t common = std::size(commonColumns);

	for (const auto type : {ItemSpecType::SLIP, ItemSpecType::INSTINCT}) {
		const auto input = folder / "input.DAT";
		const auto output = folder / "output.csv";
		Save(input, MakeFile(type));
		ItemData items;
		items.encryptionSuppression = true;
		items.Read(input.wstring(), type);
		items.ToICsv(output.wstring());
		CsvFile csv(output, std::ios::in | std::ios::binary);
		std::vector<std::u8string> header, values;
		do { header.push_back(csv.NextCell()); } while (!csv.IsEol());
		csv.NextLine();
		do { values.push_back(csv.NextCell()); } while (!csv.IsEol());
		// The header and the row of one record are two views of the same column
		// list, so they line up cell for cell, and no column is a placeholder.
		ASSERT_EQ(header.size(), values.size());
		EXPECT_TRUE(std::none_of(header.begin(), header.end(), [](const auto &value) { return value.empty(); }));
		for (size_t column = 0; column < common; ++column)
			EXPECT_TRUE(header[column] == commonColumns[column]);
		// SlipSpec::ukn is an array of bytes, InstinctSpec::ukn an array of words.
		const size_t unknownColumns = type == ItemSpecType::SLIP
			? sizeof(itmfmt::v30::SlipSpec::ukn)
			: sizeof(itmfmt::v30::InstinctSpec::ukn) / sizeof(uint16_t);
		EXPECT_EQ(header.size(), common + unknownColumns);
	}
}

// The CSV of a legacy version lists the fields of that version's spec structs,
// including the unknown ones, and nothing else: the columns after the common ones
// are the spec columns of that version (WriteLegacyHeader in ItemFormatsCsv.cpp)
// and no column is a placeholder.
TEST_F(ItemDataTest, LegacyCsvReportsVersionSpecificFieldsAndUnknownBytes)
{
	const ItemSpecType types[] = { ItemSpecType::NORMAL, ItemSpecType::USABLE, ItemSpecType::WEAPON,
		ItemSpecType::ARMOUR, ItemSpecType::PUPPET, ItemSpecType::SLIP,
		ItemSpecType::CURRENCY, ItemSpecType::INSTINCT };

	// The columns every item version shares, in order. `ExtendedFlags` belongs to
	// the v30 header only, so a legacy header must not carry it; the English text
	// cells are absent as well because the row of this record has two cells.
	const char8_t *commonColumns[] = { u8"ID", u8"Name", u8"Description", u8"Flags",
		u8"Stack", u8"Type", u8"ResID", u8"Targets", u8"ImageLength" };
	constexpr size_t common = std::size(commonColumns);

	// The spec columns of a legacy row are the named fields of that version's
	// spec struct followed by one column per element of its unknown arrays. The
	// unknown part is asked of the structs themselves (the `sizeof` of the array
	// ItemFormatV10.h / V20.h declares), so a struct that grows an unknown byte
	// cannot leave the header behind; the named part is the field list of those
	// structs, which the CSV writer spells out column by column and which is what
	// `named` counts here.
	auto specColumns = [](itmfmt::Version version, ItemSpecType type) -> size_t {
		const bool v10 = version == itmfmt::Version::V10;
		switch (type)
		{
		case ItemSpecType::NORMAL:
			return 3 + (v10 ? sizeof(itmfmt::v10::NormalSpec::ukn) : sizeof(itmfmt::v20::NormalSpec::ukn));
		case ItemSpecType::USABLE:
			return 1 + (v10 ? sizeof(itmfmt::v10::UsableSpec::ukn) : sizeof(itmfmt::v20::UsableSpec::ukn));
		case ItemSpecType::WEAPON:
			// Level, Slots, Races, Jobs, DMG, Delay, DPS, Skill, Ukn12, Ukn7, Ukn9,
			// MaxCharges, CastFactor, UseTime, ReuseTime, Ukn20, RelatedItemId and
			// then the version tail (v10: UknAfterRelated; v20: SuperiorLevel,
			// Ukn2 ... iLvl, Ukn22, Ukn23).
			return v10 ? 18 : 22;
		case ItemSpecType::ARMOUR:
			// v10: Level .. UknAfterRelated; v20: the same block with SuperiorLevel
			// and the iLvl / Ukn3 / Ukn4 tail.
			return v10 ? 13 : 15;
		case ItemSpecType::PUPPET:
			return 11 + (v10 ? sizeof(itmfmt::v10::PuppetSpec::ukn) : sizeof(itmfmt::v20::PuppetSpec::ukn));
		case ItemSpecType::SLIP:
			return v10 ? sizeof(itmfmt::v10::SlipSpec::ukn) : sizeof(itmfmt::v20::SlipSpec::ukn);
		case ItemSpecType::CURRENCY:
			return v10 ? sizeof(itmfmt::v10::CurrencySpec::ukn) : sizeof(itmfmt::v20::CurrencySpec::ukn);
		case ItemSpecType::INSTINCT:
			return v10 ? sizeof(itmfmt::v10::InstinctSpec::ukn) : sizeof(itmfmt::v20::InstinctSpec::ukn);
		}
		return 0;
	};
	for (const auto version : { itmfmt::Version::V10, itmfmt::Version::V20 }) {
		for (size_t i = 0; i < std::size(types); ++i) {
			const auto type = types[i];
			const size_t TextOffset = version == itmfmt::Version::V10
				? itmfmt::v10::Format::TextOffset(type) : itmfmt::v20::Format::TextOffset(type);
			auto bytes = MakeLegacyFile(type);
			std::fill(bytes.begin() + 14, bytes.begin() + 640, 0);
			for (size_t offset = 14; offset < TextOffset; ++offset)
				bytes[offset] = static_cast<char>(offset);
			Row text;
			text.GetCells().emplace_back(u8"Name");
			text.GetCells().emplace_back(u8"Description");
			text.WriteRow(reinterpret_cast<Record*>(bytes.data() + TextOffset), static_cast<int>(640 - TextOffset));
			const auto input = folder / "input.DAT";
			const auto output = folder / "output.csv";
			Save(input, bytes);
			ItemData items;
			items.encryptionSuppression = true;
			ASSERT_NO_THROW(items.Read(input.wstring(), type, version));
			ASSERT_NO_THROW(items.ToICsv(output.wstring()));
			CsvFile csv(output, std::ios::in | std::ios::binary);
			std::vector<std::u8string> header, values;
			do { header.push_back(csv.NextCell()); } while (!csv.IsEol());
			csv.NextLine();
			do { values.push_back(csv.NextCell()); } while (!csv.IsEol());
			// The header and the row of one record are two views of the same
			// column list: they have to line up cell for cell.
			ASSERT_EQ(header.size(), values.size());
			// The common block is the shared column list of this layout, without
			// the extended_flags of the v30 header.
			for (size_t column = 0; column < common; ++column)
				EXPECT_TRUE(header[column] == commonColumns[column]);
			EXPECT_TRUE(std::find(header.begin(), header.end(), u8"ExtendedFlags") == header.end());
			// The spec block is what the structs of this version hold, and the
			// header ends there: no text cells are appended and, per the column
			// ownership rule, no column is left empty as a placeholder.
			EXPECT_EQ(header.size(), common + specColumns(version, type));
			EXPECT_TRUE(std::none_of(header.begin(), header.end(), [](const auto &value) { return value.empty(); }));
			EXPECT_TRUE(values[0] == u8"10240");
			EXPECT_TRUE(values[4] == u8"12");
			EXPECT_TRUE(values[5] == u8"5");
			EXPECT_TRUE(values[6] == u8"4660");
			EXPECT_TRUE(values[7] == u8"Self");
			// The record below is a tape of the slot offsets: the byte at slot
			// offset N holds N. A numeric column is printed in decimal, so the
			// number a field of `width` little endian bytes at offset N spells is
			// what that column has to show; a spec cell that shows it was read
			// through the struct of the version that parsed the record, not
			// through a zeroed view of another one.
			auto tape = [](size_t offset, size_t width) {
				uint32_t value = 0;
				for (size_t i = 0; i < width; ++i)
					value |= static_cast<uint32_t>(offset + i) << (8 * i);
				const std::string text = std::to_string(value);
				return std::u8string(text.begin(), text.end());
			};
			// gtest of this version cannot print a std::u8string, so a mismatch
			// reports both cells as plain text.
			auto ascii = [](const std::u8string &value) {
				std::string text;
				for (char8_t c : value) text.push_back(static_cast<char>(c));
				return text;
			};
			if (type == ItemSpecType::WEAPON || type == ItemSpecType::ARMOUR) {
				const bool currentFields = version == itmfmt::Version::V20;
				EXPECT_EQ(std::find(header.begin(), header.end(), u8"SuperiorLevel") != header.end(), currentFields);
				EXPECT_EQ(std::find(header.begin(), header.end(), u8"iLvl") != header.end(), currentFields);
				EXPECT_EQ(std::find(header.begin(), header.end(), u8"UknAfterRelated") != header.end(), !currentFields);

				const bool v10 = version == itmfmt::Version::V10;
				if (type == ItemSpecType::WEAPON) {
					using V10Entry = itmfmt::v10::Entry;
					using V20Entry = itmfmt::v20::Entry;
					// Level sits at the beginning of every legacy weapon spec and dmg
					// right after the flags block, so both columns carry the offset of
					// the record of that version (header included), and the two
					// versions differ.
					EXPECT_TRUE(header[common] == u8"Level");
					const auto level = tape(v10 ? offsetof(V10Entry, spec.weapon.level) : offsetof(V20Entry, spec.weapon.level),
						v10 ? sizeof(V10Entry::spec.weapon.level) : sizeof(V20Entry::spec.weapon.level));
					EXPECT_TRUE(values[common] == level)
						<< "Level: " << ascii(values[common]) << ", the tape says " << ascii(level);
					const auto dmg = std::find(header.begin() + common, header.end(), u8"DMG");
					ASSERT_NE(dmg, header.end()) << "the weapon header has no DMG column";
					const auto dmgCell = values[static_cast<size_t>(dmg - header.begin())];
					const auto dmgTape = tape(v10 ? offsetof(V10Entry, spec.weapon.dmg) : offsetof(V20Entry, spec.weapon.dmg),
						v10 ? sizeof(V10Entry::spec.weapon.dmg) : sizeof(V20Entry::spec.weapon.dmg));
					EXPECT_TRUE(dmgCell == dmgTape)
						<< "DMG: " << ascii(dmgCell) << ", the tape says " << ascii(dmgTape);
				}
				else {
					using V10Entry = itmfmt::v10::Entry;
					using V20Entry = itmfmt::v20::Entry;
					// ShieldSize sits at a different slot offset in v10 than in v20:
					// the column follows the struct of the version that read the
					// record, not that of the other one.
					const auto shieldSize = std::find(header.begin() + common, header.end(), u8"ShieldSize");
					ASSERT_NE(shieldSize, header.end()) << "the armour header has no ShieldSize column";
					const auto shieldCell = values[static_cast<size_t>(shieldSize - header.begin())];
					const auto shieldTape = tape(
						v10 ? offsetof(V10Entry, spec.armour.shield_size) : offsetof(V20Entry, spec.armour.shield_size),
						v10 ? sizeof(V10Entry::spec.armour.shield_size) : sizeof(V20Entry::spec.armour.shield_size));
					EXPECT_TRUE(shieldCell == shieldTape)
						<< "ShieldSize: " << ascii(shieldCell) << ", the tape says " << ascii(shieldTape);
				}
			}
			else {
				// The last spec field is a byte of the raw prefix, which the test
				// filled with its own offset (the last one before the text record).
				const auto expected = std::to_string(TextOffset - 1);
				EXPECT_TRUE(values[common + specColumns(version, type) - 1] == std::u8string(expected.begin(), expected.end()));
			}
		}
	}
}

TEST_F(ItemDataTest, LegacyTypeSuffixHelpers)
{
	EXPECT_TRUE(HasLegacyTypeSuffix(u8"inb_o"));
	EXPECT_FALSE(HasLegacyTypeSuffix(u8"inb"));
	EXPECT_TRUE(HasLegacyTypeSuffix("mbd_o"));
	EXPECT_FALSE(HasLegacyTypeSuffix("_o"));
	EXPECT_TRUE(StripLegacyTypeSuffix(u8"erq_o") == u8"erq");
	EXPECT_TRUE(StripLegacyTypeSuffix(u8"erq") == u8"erq");
	EXPECT_EQ(BaseTypeOf("iab_o"), "iab");
	EXPECT_TRUE(IsItemTypeCode("inb_o"));
	EXPECT_FALSE(IsItemTypeCode("mbd_o"));
}

TEST_F(ItemDataTest, InstalledCurrentDatsRoundTripByteIdentically)
{
	const char* gameRoot = GameRoot();
	if (!gameRoot) {
		// gtest 1.8 has no GTEST_SKIP; make the skip visible in console and XML.
		RecordProperty("skipped", "FFXI_ITEM_TEST_GAME_ROOT is not set");
		std::cout << "[ SKIPPED  ] set FFXI_ITEM_TEST_GAME_ROOT to verify against real DATs" << std::endl;
		return;
	}
	const std::tuple<const char*, ItemSpecType, itmfmt::Version> files[] = {
		{"ROM/0/4", ItemSpecType::NORMAL, itmfmt::Version::V30},
		{"ROM/118/106", ItemSpecType::NORMAL, itmfmt::Version::V30},
		{"ROM/0/5", ItemSpecType::USABLE, itmfmt::Version::V30},
		{"ROM/118/107", ItemSpecType::USABLE, itmfmt::Version::V30},
		{"ROM/0/6", ItemSpecType::WEAPON, itmfmt::Version::V30},
		{"ROM/118/108", ItemSpecType::WEAPON, itmfmt::Version::V30},
		{"ROM/0/7", ItemSpecType::ARMOUR, itmfmt::Version::V30},
		{"ROM/118/109", ItemSpecType::ARMOUR, itmfmt::Version::V30},
		{"ROM/0/8", ItemSpecType::PUPPET, itmfmt::Version::V30},
		{"ROM/118/110", ItemSpecType::PUPPET, itmfmt::Version::V30},
		{"ROM/0/9", ItemSpecType::CURRENCY, itmfmt::Version::V30},
		{"ROM/174/48", ItemSpecType::CURRENCY, itmfmt::Version::V30},
		{"ROM/301/114", ItemSpecType::NORMAL, itmfmt::Version::V30},
		{"ROM/301/115", ItemSpecType::NORMAL, itmfmt::Version::V30},
		{"ROM/217/20", ItemSpecType::SLIP, itmfmt::Version::V30},
		{"ROM/217/21", ItemSpecType::SLIP, itmfmt::Version::V30},
		{"ROM/286/72", ItemSpecType::ARMOUR, itmfmt::Version::V30},
		{"ROM/286/73", ItemSpecType::ARMOUR, itmfmt::Version::V30},
		{"ROM/288/79", ItemSpecType::INSTINCT, itmfmt::Version::V30},
		{"ROM/288/80", ItemSpecType::INSTINCT, itmfmt::Version::V30},
		{"ROM/387/13", ItemSpecType::NORMAL, itmfmt::Version::V30},
		{"ROM/387/14", ItemSpecType::NORMAL, itmfmt::Version::V30},
		// de/fr tables, still on the oldest known layout
		{"ROM/176/101", ItemSpecType::NORMAL, itmfmt::Version::V10},
		{"ROM/178/40", ItemSpecType::NORMAL, itmfmt::Version::V10},
		{"ROM/176/102", ItemSpecType::USABLE, itmfmt::Version::V10},
		{"ROM/178/41", ItemSpecType::USABLE, itmfmt::Version::V10},
		{"ROM/176/103", ItemSpecType::WEAPON, itmfmt::Version::V10},
		{"ROM/178/42", ItemSpecType::WEAPON, itmfmt::Version::V10},
		{"ROM/176/104", ItemSpecType::ARMOUR, itmfmt::Version::V10},
		{"ROM/178/43", ItemSpecType::ARMOUR, itmfmt::Version::V10},
		{"ROM/176/105", ItemSpecType::PUPPET, itmfmt::Version::V10},
		{"ROM/178/44", ItemSpecType::PUPPET, itmfmt::Version::V10},
		{"ROM/176/106", ItemSpecType::CURRENCY, itmfmt::Version::V10},
		{"ROM/178/45", ItemSpecType::CURRENCY, itmfmt::Version::V10},
		{"ROM/217/25", ItemSpecType::SLIP, itmfmt::Version::V10},
		{"ROM/217/26", ItemSpecType::SLIP, itmfmt::Version::V10},
		{"ROM/286/74", ItemSpecType::ARMOUR, itmfmt::Version::V10},
		{"ROM/286/75", ItemSpecType::ARMOUR, itmfmt::Version::V10},
		{"ROM/288/81", ItemSpecType::INSTINCT, itmfmt::Version::V10},
		{"ROM/288/82", ItemSpecType::INSTINCT, itmfmt::Version::V10},
		{"ROM/301/116", ItemSpecType::NORMAL, itmfmt::Version::V10},
		{"ROM/301/117", ItemSpecType::NORMAL, itmfmt::Version::V10},
	};
	for (const auto& [relative, type, version] : files) {
		SCOPED_TRACE(relative);
		const auto input = std::filesystem::path(gameRoot) / (std::string(relative) + ".DAT");
		const auto output = folder / "output.DAT";
		ItemData items;
		ASSERT_NO_THROW(items.Read(input.wstring(), type, version));
		ASSERT_FALSE(items.data.empty());
		ASSERT_NO_THROW(items.Write(output.wstring()));
		const auto expected = Load(input);
		const auto actual = Load(output);
		ASSERT_EQ(expected.size(), actual.size());
		EXPECT_TRUE(expected == actual);
	}
}

TEST_F(ItemDataTest, InstalledMonBridgeRoundTripByteIdentically)
{
	const char* gameRoot = GameRoot();
	if (!gameRoot) {
		// gtest 1.8 has no GTEST_SKIP; make the skip visible in console and XML.
		RecordProperty("skipped", "FFXI_ITEM_TEST_GAME_ROOT is not set");
		std::cout << "[ SKIPPED  ] set FFXI_ITEM_TEST_GAME_ROOT to verify against real DATs" << std::endl;
		return;
	}
	const std::pair<const char*, bool> files[] = {
		{"ROM/288/66", false}, {"ROM/288/67", false},
		{"ROM/288/68", true}, {"ROM/288/69", true},
	};
	for (const auto& [relative, legacy] : files) {
		SCOPED_TRACE(relative);
		const auto input = std::filesystem::path(gameRoot) / (std::string(relative) + ".DAT");
		const auto output = folder / "output.dat";
		MonBridge mb;
		ASSERT_NO_THROW(mb.Read(input.wstring(), legacy ? slotfile::Version::V10 : slotfile::Version::V30));
		ASSERT_FALSE(mb.data.empty());
		ASSERT_NO_THROW(mb.Write(output.wstring()));
		EXPECT_TRUE(Load(input) == Load(output));
	}
}

TEST_F(ItemDataTest, InstalledRoeRoundTripByteIdentically)
{
	const char* gameRoot = GameRoot();
	if (!gameRoot) {
		// gtest 1.8 has no GTEST_SKIP; make the skip visible in console and XML.
		RecordProperty("skipped", "FFXI_ITEM_TEST_GAME_ROOT is not set");
		std::cout << "[ SKIPPED  ] set FFXI_ITEM_TEST_GAME_ROOT to verify against real DATs" << std::endl;
		return;
	}
	const std::tuple<const char*, bool, bool> files[] = {
		{"ROM/307/15", false, true}, {"ROM/307/16", false, true},
		{"ROM/307/17", true, true}, {"ROM/307/18", true, true},
		{"ROM/307/23", false, false}, {"ROM/307/24", false, false},
		{"ROM/307/25", true, false}, {"ROM/307/26", true, false},
	};
	for (const auto& [relative, legacy, isQuest] : files) {
		SCOPED_TRACE(relative);
		const auto input = std::filesystem::path(gameRoot) / (std::string(relative) + ".DAT");
		const auto output = folder / "output.dat";
		RecordsOfEminence roe;
		if (isQuest) {
			ASSERT_NO_THROW(roe.ReadQuest(input.wstring(), legacy ? slotfile::Version::V10 : slotfile::Version::V30));
			ASSERT_FALSE(roe.questData.empty());
			ASSERT_NO_THROW(roe.WriteQuest(output.wstring()));
		}
		else {
			ASSERT_NO_THROW(roe.ReadCategory(input.wstring(), legacy ? slotfile::Version::V10 : slotfile::Version::V30));
			ASSERT_FALSE(roe.categoryData.empty());
			ASSERT_NO_THROW(roe.WriteCategory(output.wstring()));
		}
		EXPECT_TRUE(Load(input) == Load(output));
	}
}

// The layout header has to agree with the constants measured from the live files.
TEST_F(ItemDataTest, ItemFormatV30ConstantsMatchMeasurements)
{
	using Format = itmfmt::v30::Format;
	EXPECT_EQ(Format::id, "v30");
	EXPECT_EQ(Format::slotSize, 0x1400u);
	EXPECT_EQ(Format::currencySlots, 16u);
	EXPECT_EQ(Format::cipherSpan, Format::slotSize);
	EXPECT_EQ(Format::blobLengthOffset, 640u);
	EXPECT_EQ(Format::blobDataOffset, 644u);
	EXPECT_EQ(Format::blobCapacity, 4475u);
	EXPECT_EQ(Format::TextOffset(itmfmt::SpecType::NORMAL), 28u);
	EXPECT_EQ(Format::TextOffset(itmfmt::SpecType::USABLE), 28u);
	EXPECT_EQ(Format::TextOffset(itmfmt::SpecType::WEAPON), 60u);
	EXPECT_EQ(Format::TextOffset(itmfmt::SpecType::ARMOUR), 48u);
	EXPECT_EQ(Format::TextOffset(itmfmt::SpecType::PUPPET), 28u);
	EXPECT_EQ(Format::TextOffset(itmfmt::SpecType::SLIP), 84u);
	EXPECT_EQ(Format::TextOffset(itmfmt::SpecType::CURRENCY), 20u);
	EXPECT_EQ(Format::TextOffset(itmfmt::SpecType::INSTINCT), 44u);
	EXPECT_EQ(sizeof(itmfmt::v30::Entry), 0x1400u);
	EXPECT_EQ(sizeof(itmfmt::v30::Header), 16u);
}

// Every layout constant of every family has to agree with what was measured from
// the live files: the structs own the
// offsets, and these checks pin them to the measurements.
TEST_F(ItemDataTest, AllFamilyLayoutsMatchMeasurements)
{
	using V10 = itmfmt::v10::Format;
	EXPECT_EQ(V10::id, "v10");
	EXPECT_EQ(V10::slotSize, 0xC00u);
	EXPECT_EQ(V10::currencySlots, 16u);
	EXPECT_EQ(V10::blobCapacity, 2427u);
	EXPECT_EQ(V10::textEnd, 640u);
	EXPECT_EQ(sizeof(itmfmt::v10::Header), 14u);
	EXPECT_EQ(V10::headerStackSizeOffset, 6u);
	EXPECT_EQ(V10::TextOffset(ItemSpecType::NORMAL), 24u);
	EXPECT_EQ(V10::TextOffset(ItemSpecType::USABLE), 24u);
	EXPECT_EQ(V10::TextOffset(ItemSpecType::WEAPON), 48u);
	EXPECT_EQ(V10::TextOffset(ItemSpecType::ARMOUR), 40u);
	EXPECT_EQ(V10::TextOffset(ItemSpecType::PUPPET), 24u);
	EXPECT_EQ(V10::TextOffset(ItemSpecType::SLIP), 84u);
	EXPECT_EQ(V10::TextOffset(ItemSpecType::CURRENCY), 16u);
	EXPECT_EQ(V10::TextOffset(ItemSpecType::INSTINCT), 40u);

	using MB30 = mbfmt::v30::Format;
	EXPECT_EQ(MB30::slotSize, 0x1400u);
	EXPECT_EQ(MB30::blobCapacity, 4475u);
	EXPECT_EQ(MB30::TextOffset(mbfmt::Schema::BRIDGE), 116u);
	EXPECT_EQ(MB30::recSize, 524u);
	using MB10 = mbfmt::v10::Format;
	EXPECT_EQ(MB10::slotSize, 0xC00u);
	EXPECT_EQ(MB10::blobCapacity, 2427u);
	EXPECT_EQ(MB10::TextOffset(mbfmt::Schema::BRIDGE), 112u);
	EXPECT_EQ(MB10::recSize, 528u);

	EXPECT_EQ(roefmt::v30::Quest::slotSize, 0x1400u);
	EXPECT_EQ(roefmt::v30::Quest::TextOffset(roefmt::Schema::QUEST), 32u);
	EXPECT_EQ(roefmt::v30::Quest::TextCapacity(roefmt::Schema::QUEST), 5087u);
	EXPECT_EQ(roefmt::v30::Category::TextOffset(roefmt::Schema::CATEGORY), 568u);
	EXPECT_EQ(roefmt::v30::Category::TextCapacity(roefmt::Schema::CATEGORY), 4551u);
	EXPECT_EQ(roefmt::v10::Quest::slotSize, 0xC00u);
	EXPECT_EQ(roefmt::v10::Quest::TextOffset(roefmt::Schema::QUEST), 28u);
	EXPECT_EQ(roefmt::v10::Quest::TextCapacity(roefmt::Schema::QUEST), 3043u);
	EXPECT_EQ(roefmt::v10::Category::TextOffset(roefmt::Schema::CATEGORY), 568u);
	EXPECT_EQ(roefmt::v10::Category::TextCapacity(roefmt::Schema::CATEGORY), 2503u);

	// A family can have two labels for one layout; the aliases record that.
	EXPECT_TRUE((std::is_same_v<mbfmt::v20::Format, mbfmt::v10::Format>));
	// v20 category uses the v10 record, but it is its own layout of the v20 version.
	EXPECT_EQ(sizeof(roefmt::v20::Category::Entry), sizeof(roefmt::v10::Category::Entry));
	EXPECT_EQ(roefmt::v20::Category::TextOffset(roefmt::Schema::CATEGORY), roefmt::v10::Category::TextOffset(roefmt::Schema::CATEGORY));
	// ... but the Records of Eminence quest record changed in v20: it already
	// carries uni_reward, so it is a type of its own.
	EXPECT_FALSE((std::is_same_v<roefmt::v20::Quest, roefmt::v10::Quest>));

	// v20: the ja/en tables before the 2026-09 update (LocCNTxtOld corpus).
	using V20 = itmfmt::v20::Format;
	EXPECT_EQ(V20::id, "v20");
	EXPECT_EQ(V20::slotSize, 0xC00u);
	EXPECT_EQ(V20::blobCapacity, 2427u);
	EXPECT_EQ(V20::TextOffset(ItemSpecType::NORMAL), 24u);
	EXPECT_EQ(V20::TextOffset(ItemSpecType::USABLE), 28u);
	EXPECT_EQ(V20::TextOffset(ItemSpecType::WEAPON), 56u);
	EXPECT_EQ(V20::TextOffset(ItemSpecType::ARMOUR), 44u);
	EXPECT_EQ(V20::TextOffset(ItemSpecType::PUPPET), 24u);
	EXPECT_EQ(V20::TextOffset(ItemSpecType::SLIP), 84u);
	EXPECT_EQ(V20::TextOffset(ItemSpecType::CURRENCY), 16u);
	EXPECT_EQ(V20::TextOffset(ItemSpecType::INSTINCT), 40u);
	EXPECT_EQ(roefmt::v20::Quest::slotSize, 0xC00u);
	EXPECT_EQ(roefmt::v20::Quest::TextOffset(roefmt::Schema::QUEST), 32u);
	EXPECT_EQ(roefmt::v20::Quest::TextCapacity(roefmt::Schema::QUEST), 3039u);

	// Routing: the data side annotation is the only version signal.
	//
	// The rows of the table are a moving target (a version can be added, and a
	// version can get another alias), so what is checked here are properties of
	// the table rather than its current rows: a row that copies the constants of
	// a neighbouring layout, a duplicated row and a missing documented
	// annotation still turn these red.
	//
	// One entry per version of the family: its label, the layout the explicit
	// path loads for it (`ItemData::Read(path, type, version)`, WithLayout in
	// ItemData.cpp) and whether the store keeps an alternative for it
	// (DatumStore in ItemFormats.h).
	struct KnownLayout
	{
		itmfmt::Version version;
		std::string_view id;
		size_t slotSize;
		size_t currencySlots;
		bool heldByStore;
	};
	const KnownLayout known[] = {
		{ V10::version, V10::id, V10::slotSize, V10::currencySlots,
			VariantHolds<std::vector<itmfmt::v10::Datum>, itmfmt::DatumStore>::value },
		{ V20::version, V20::id, V20::slotSize, V20::currencySlots,
			VariantHolds<std::vector<itmfmt::v20::Datum>, itmfmt::DatumStore>::value },
		{ itmfmt::v30::Format::version, itmfmt::v30::Format::id,
			itmfmt::v30::Format::slotSize, itmfmt::v30::Format::currencySlots,
			VariantHolds<std::vector<itmfmt::v30::Datum>, itmfmt::DatumStore>::value },
	};
	auto layoutOf = [&](itmfmt::Version version) -> const KnownLayout * {
		for (const KnownLayout &layout : known)
			if (layout.version == version) return &layout;
		return nullptr;
	};
	// The trait behind that column asks about the type list of the store, so a
	// type the store cannot hold has to answer false: that keeps the column from
	// being true whatever the store looks like.
	EXPECT_FALSE((VariantHolds<std::vector<int>, itmfmt::DatumStore>::value));
	EXPECT_TRUE((VariantHolds<std::vector<itmfmt::v20::Datum>, itmfmt::DatumStore>::value));
	auto routed = [](itmfmt::Version version) {
		return std::any_of(std::begin(itmfmt::VERSION_ROUTES), std::end(itmfmt::VERSION_ROUTES),
			[&](const itmfmt::VersionRoute &route) { return route.version == version; });
	};
	auto annotated = [](std::string_view suffix) -> const itmfmt::Version * {
		for (const auto &route : itmfmt::VERSION_ROUTES)
			if (route.suffix == suffix) return &route.version;
		return nullptr;
	};

	std::vector<std::string_view> suffixes;
	for (const auto &route : itmfmt::VERSION_ROUTES)
	{
		SCOPED_TRACE("route " + std::string(route.suffix));
		// A row describes the layout of the version it names; one that carries
		// the constants of a neighbouring layout is the defect this catches.
		const KnownLayout *layout = layoutOf(route.version);
		ASSERT_NE(layout, nullptr) << "a route names a version this family does not have";
		EXPECT_EQ(route.id, layout->id);
		EXPECT_EQ(route.slotSize, layout->slotSize);
		EXPECT_EQ(route.currencySlots, layout->currencySlots);
		// ... and it is only a usable row if that version can be loaded at all,
		// which needs a layout of that version and a slot of the store to keep
		// its records in.
		EXPECT_TRUE(layout->heldByStore) << "no store alternative keeps the records of this version";
		// One annotation per row: a duplicate would make the sniffer ambiguous.
		EXPECT_EQ(std::count(suffixes.begin(), suffixes.end(), route.suffix), 0);
		suffixes.push_back(route.suffix);
	}

	// The annotations the family documents: the newest known layout is spelled
	// with no suffix at all, the oldest one as "_o".
	ASSERT_NE(annotated(""), nullptr) << "the newest known layout has to be annotated";
	EXPECT_EQ(*annotated(""), itmfmt::CURRENT_VERSION);
	ASSERT_NE(annotated("_o"), nullptr) << "the oldest known layout has to be annotated";
	EXPECT_EQ(*annotated("_o"), itmfmt::OLDEST_VERSION);

	// Every version of the family is selectable by one of the two paths: the
	// annotation of a row, or an explicit `Version` request, which loads any
	// version the store keeps an alternative for (the pre-update v20 corpus is
	// read that way). A version neither annotated nor kept could not be loaded
	// by any path.
	for (const KnownLayout &layout : known)
	{
		SCOPED_TRACE(std::string(layout.id));
		EXPECT_TRUE(routed(layout.version) || layout.heldByStore)
			<< "no annotation and no explicit load selects this version";
	}
}

// The container itself, without any family facade: read a live DAT, assemble it
// again and compare the bytes.
TEST_F(ItemDataTest, ContainerReproducesInstalledDats)
{
	const char* gameRoot = GameRoot();
	if (!gameRoot) {
		RecordProperty("skipped", "FFXI_ITEM_TEST_GAME_ROOT is not set");
		std::cout << "[ SKIPPED  ] set FFXI_ITEM_TEST_GAME_ROOT to verify against real DATs" << std::endl;
		return;
	}

	auto check = [&](const char* relative, auto&& read) {
		SCOPED_TRACE(relative);
		const auto input = std::filesystem::path(gameRoot) / (std::string(relative) + ".DAT");
		const auto expected = Load(input);
		const auto actual = read(input.wstring());
		EXPECT_EQ(expected.size(), actual.size());
		EXPECT_TRUE(expected == actual);
	};

	check("ROM/0/4", [](const std::wstring& path) {
		slotfile::SlotFile<itmfmt::v30::Format> file;
		file.Read(path, ItemSpecType::NORMAL);
		return file.Serialize(file.data);
	});
	check("ROM/176/101", [](const std::wstring& path) {
		slotfile::SlotFile<itmfmt::v10::Format> file;
		file.Read(path, ItemSpecType::NORMAL);
		return file.Serialize(file.data);
	});
	check("ROM/0/9", [](const std::wstring& path) {
		slotfile::SlotFile<itmfmt::v30::Format> file;
		file.Read(path, ItemSpecType::CURRENCY);
		return file.Serialize(file.data);
	});
	check("ROM/176/106", [](const std::wstring& path) {
		slotfile::SlotFile<itmfmt::v10::Format> file;
		file.Read(path, ItemSpecType::CURRENCY);
		return file.Serialize(file.data);
	});
	check("ROM/288/66", [](const std::wstring& path) {
		slotfile::SlotFile<mbfmt::v30::Format> file;
		file.Read(path);
		return file.Serialize(file.data);
	});
	check("ROM/288/68", [](const std::wstring& path) {
		slotfile::SlotFile<mbfmt::v10::Format> file;
		file.Read(path);
		return file.Serialize(file.data);
	});
	check("ROM/307/15", [](const std::wstring& path) {
		slotfile::SlotFile<roefmt::v30::Quest> file;
		file.Read(path, roefmt::Schema::QUEST);
		return file.Serialize(file.data);
	});
	check("ROM/307/17", [](const std::wstring& path) {
		slotfile::SlotFile<roefmt::v10::Quest> file;
		file.Read(path, roefmt::Schema::QUEST);
		return file.Serialize(file.data);
	});
	check("ROM/307/23", [](const std::wstring& path) {
		slotfile::SlotFile<roefmt::v30::Category> file;
		file.Read(path, roefmt::Schema::CATEGORY);
		return file.Serialize(file.data);
	});
	check("ROM/307/25", [](const std::wstring& path) {
		slotfile::SlotFile<roefmt::v10::Category> file;
		file.Read(path, roefmt::Schema::CATEGORY);
		return file.Serialize(file.data);
	});
}

// What the container itself refuses: a wrong stride, a missing terminator and a
// record whose text table does not fit the slot. The text record of the current
// normal item sits at offset 28, its table at 28 + 4, so a test can patch the
// count, an offset and a type in place.
TEST_F(ItemDataTest, ContainerRejectsMalformedSlots)
{
	using File = slotfile::SlotFile<itmfmt::v30::Format>;
	const auto input = folder / "input.DAT";

	auto bytes = MakeFile(ItemSpecType::NORMAL);
	bytes.resize(bytes.size() - 1);
	Save(input, bytes);
	File truncated;
	EXPECT_THROW(truncated.Read(input.wstring()), std::runtime_error);

	bytes = MakeFile(ItemSpecType::NORMAL);
	bytes[0x1400 - 1] = 0x00;
	Save(input, bytes);
	File badMarker;
	EXPECT_THROW(badMarker.Read(input.wstring()), std::runtime_error);

	// The text record of the current normal item starts at 28.
	constexpr size_t text = 28;
	constexpr size_t table = text + 4;
	constexpr size_t firstCell = table + 8;

	auto rejects = [&](const std::vector<char>& data, const char* what) {
		SCOPED_TRACE(what);
		Save(input, data);
		File file;
		EXPECT_THROW(file.Read(input.wstring()), std::runtime_error);
	};
	auto put32 = [](std::vector<char>& data, size_t offset, int32_t value) {
		std::memcpy(data.data() + offset, &value, sizeof(value));
	};

	// A cell count that is zero, negative or too large for the text area: the
	// area is 612 bytes, so at most (612 - 4) / 8 = 76 cells fit.
	bytes = MakeFile(ItemSpecType::NORMAL);
	put32(bytes, text, 0);
	rejects(bytes, "cellCount = 0");

	bytes = MakeFile(ItemSpecType::NORMAL);
	put32(bytes, text, 1000);
	rejects(bytes, "cellCount = 1000");

	bytes = MakeFile(ItemSpecType::NORMAL);
	put32(bytes, text, -1);
	rejects(bytes, "cellCount = -1");

	// A cell that points into the table itself.
	bytes = MakeFile(ItemSpecType::NORMAL);
	put32(bytes, table, 0);
	rejects(bytes, "cell offset 0 (< table end)");

	bytes = MakeFile(ItemSpecType::NORMAL);
	put32(bytes, table, 4);
	rejects(bytes, "cell offset 4 (< table end)");

	// A cell that ends past the text area: the area ends at 640, i.e. 612 bytes
	// after the text record.
	bytes = MakeFile(ItemSpecType::NORMAL);
	put32(bytes, table, 612);
	rejects(bytes, "cell offset = 612");

	bytes = MakeFile(ItemSpecType::NORMAL);
	put32(bytes, table, 613);
	rejects(bytes, "cell offset > limit");

	// A string cell without its NUL terminator inside the text area.
	bytes = MakeFile(ItemSpecType::NORMAL);
	std::memset(bytes.data() + text + 12, 'x', 612 - 12);
	rejects(bytes, "unterminated string cell");

	// A cell type other than 0 (string) or 1 (int).
	bytes = MakeFile(ItemSpecType::NORMAL);
	put32(bytes, table + 4, 2);
	rejects(bytes, "cell type 2");

	// A currency file must be exactly currencySlots long and zero padded.
	bytes = MakeFile(ItemSpecType::CURRENCY);
	bytes.resize(0x1400);
	Save(input, bytes);
	File shortCurrency;
	EXPECT_THROW(shortCurrency.Read(input.wstring(), ItemSpecType::CURRENCY), std::runtime_error);
}

// Text records that are not canonical but still well formed: the container
// parses them and the rewrite normalizes them, so a rewrite is byte identical
// only when the record was already canonical (the writer lays the cells out back
// to back, in table order, each cell starting on its four byte boundary).
//
// Byte level round trip is only promised for the canonical records the installed
// client writes: a non canonical layout that the parser accepts (discontinuous
// cells, a shared offset, a nonzero hole) is rearranged by an unmodified
// rewrite, so its byte for byte losslessness is out of scope. See
// docs/FILE_FORMATS.md ("无损回写范围") and itmfmt::DatumBase::store.
TEST_F(ItemDataTest, NonCanonicalTextRecordsParseAndRewrite)
{
	using File = slotfile::SlotFile<itmfmt::v30::Format>;
	const auto input = folder / "input.DAT";
	File file;
	// The cases below are built in memory and are not encrypted, so the cipher
	// of the container has to stay out of the way: a rotation would turn the
	// cell count and every offset into something else.
	file.encryptionSuppression = true;

	// The text record of the current normal item: its cell count at 28, its cell
	// table (one 8 byte RecordSpec per cell) at 32, the cell payloads behind it.
	constexpr size_t text = 28;
	constexpr size_t table = text + 4;
	constexpr size_t cell0 = table;
	constexpr size_t cell1 = table + 8;

	// A string cell is a 28 byte header (the marker `one == 1` and six zero
	// words) followed by the NUL terminated text, padded to a four byte boundary
	// (Cell::GetSize in Record.cpp), so the canonical offset of the cell behind
	// "Alpha" is 20 + 36.
	auto cellSize = [](const char* value) {
		return 28 + ((std::strlen(value) + 1 + 3) & ~static_cast<size_t>(3));
	};
	const size_t canonical = 20 + cellSize("Alpha");

	// The text area of every case holds two string cells whose payloads are laid
	// out by hand at the given offsets; every offset but the canonical one leaves
	// a hole, i.e. bytes that belong to no payload (54..59 for the 60 below).
	// `nonzeroAt` marks one byte of that hole.
	constexpr size_t noMark = static_cast<size_t>(-1);
	auto build = [&](size_t firstOffset, size_t secondOffset, size_t nonzeroAt) {
		auto bytes = MakeFile(ItemSpecType::NORMAL);
		int32_t count = 2;
		std::memcpy(bytes.data() + text, &count, sizeof(count));
		int32_t offset = static_cast<int32_t>(firstOffset);
		std::memcpy(bytes.data() + cell0, &offset, sizeof(offset));
		offset = static_cast<int32_t>(secondOffset);
		std::memcpy(bytes.data() + cell1, &offset, sizeof(offset));
		int32_t type = 0;
		std::memcpy(bytes.data() + cell0 + 4, &type, sizeof(type));
		std::memcpy(bytes.data() + cell1 + 4, &type, sizeof(type));
		std::vector<size_t> written;
		auto writeCell = [&](size_t at, const char* value) {
			// Cells that share one offset share one payload, so the first text
			// written there is the one both of them read.
			if (std::find(written.begin(), written.end(), at) != written.end()) return;
			written.push_back(at);
			const std::vector<char> header(28, 0);
			int32_t one = 1;
			std::memcpy(bytes.data() + text + at, &one, sizeof(one));
			std::memcpy(bytes.data() + text + at + 4, header.data() + 4, 24);
			std::memcpy(bytes.data() + text + at + 28, value, std::strlen(value) + 1);
		};
		writeCell(firstOffset, "Alpha");
		writeCell(secondOffset, "Beta");
		if (nonzeroAt != noMark) bytes[text + nonzeroAt] = 'z';
		return bytes;
	};

	struct Case
	{
		std::vector<char> bytes;
		bool byteIdentical;
		const char* what;
		const char8_t* name;
		const char8_t* description;
	};
	const std::vector<Case> cases = {
		// A hole between the two payloads (the second cell sits at 60 instead of
		// its canonical 56) is left alone by the parse, but the rewrite puts the
		// second cell on its canonical offset, so the result is canonical and no
		// longer byte identical.
		{ build(20, 60, noMark), false, "gap between cells", u8"Alpha", u8"Beta" },
		// A nonzero byte in that hole: same as above. The hole is not required to
		// be zero, neither by the parser nor by the writer.
		{ build(20, 60, 56), false, "nonzero byte in the gap", u8"Alpha", u8"Beta" },
		// Two cells that share one payload: both read "Alpha", and the rewrite
		// gives the second cell a payload of its own, so the result differs.
		{ build(20, 20, noMark), false, "shared cell offset", u8"Alpha", u8"Alpha" },
		// The canonical layout is byte identical: the rewrite only normalizes
		// what was not canonical in the first place.
		{ build(20, canonical, noMark), true, "canonical layout", u8"Alpha", u8"Beta" },
	};

	for (const auto& test : cases) {
		SCOPED_TRACE(test.what);
		Save(input, test.bytes);
		// Well formed but non canonical: a hole between the cells, a nonzero byte
		// in that hole and a shared cell offset are all accepted, see the
		// contract of slotfile::ValidateTextRecord.
		ASSERT_NO_THROW(file.Read(input.wstring()));
		ASSERT_EQ(file.data.size(), 1u);
		EXPECT_TRUE(file.data[0].name() == test.name);
		EXPECT_TRUE(file.data[0].description() == test.description);
		const auto rewritten = file.Serialize(file.data);
		ASSERT_EQ(rewritten.size(), test.bytes.size());
		EXPECT_EQ(rewritten == test.bytes, test.byteIdentical);
	}
}

// The ja/en tables before the 2026-09 update still live in the pre-update corpus
// (LocCNTxtOld). They use the v20 layout, which has no data side suffix, so it is
// requested explicitly.
TEST_F(ItemDataTest, PreUpdateCorpusRoundTripsByteIdentically)
{
	const char* root = std::getenv("FFXI_ITEM_TEST_OLD_ROOT");
	if (!root) {
		RecordProperty("skipped", "FFXI_ITEM_TEST_OLD_ROOT is not set");
		std::cout << "[ SKIPPED  ] set FFXI_ITEM_TEST_OLD_ROOT to verify the v20 corpus" << std::endl;
		return;
	}

	const std::pair<const char*, ItemSpecType> files[] = {
		{"ROM/0/4", ItemSpecType::NORMAL}, {"ROM/0/5", ItemSpecType::USABLE},
		{"ROM/0/6", ItemSpecType::WEAPON}, {"ROM/0/7", ItemSpecType::ARMOUR},
		{"ROM/0/8", ItemSpecType::PUPPET}, {"ROM/0/9", ItemSpecType::CURRENCY},
		{"ROM/217/20", ItemSpecType::SLIP}, {"ROM/288/79", ItemSpecType::INSTINCT},
	};
	for (const auto& [relative, type] : files) {
		const auto input = std::filesystem::path(root) / (std::string(relative) + ".DAT");
		if (!std::filesystem::exists(input)) continue;
		SCOPED_TRACE(relative);
		const auto output = folder / "output.DAT";
		ItemData data;
		ASSERT_NO_THROW(data.Read(input.wstring(), type, itmfmt::Version::V20));
		ASSERT_FALSE(data.data.empty());
		EXPECT_TRUE(data.version() == itmfmt::Version::V20);
		ASSERT_NO_THROW(data.Write(output.wstring()));
		EXPECT_TRUE(Load(input) == Load(output));
	}

	{
		const auto input = std::filesystem::path(root) / "ROM/307/15.DAT";
		if (std::filesystem::exists(input)) {
			SCOPED_TRACE("ROM/307/15");
			const auto output = folder / "roe_quest.dat";
			RecordsOfEminence roe;
			ASSERT_NO_THROW(roe.ReadQuest(input.wstring(), roefmt::Version::V20));
			ASSERT_FALSE(roe.questData.empty());
			EXPECT_TRUE(roe.version() == roefmt::Version::V20);
			ASSERT_NO_THROW(roe.WriteQuest(output.wstring()));
			EXPECT_TRUE(Load(input) == Load(output));
		}
	}
	{
		const auto input = std::filesystem::path(root) / "ROM/307/23.DAT";
		if (std::filesystem::exists(input)) {
			SCOPED_TRACE("ROM/307/23");
			const auto output = folder / "roe_category.dat";
			RecordsOfEminence roe;
			ASSERT_NO_THROW(roe.ReadCategory(input.wstring(), roefmt::Version::V20));
			ASSERT_FALSE(roe.categoryData.empty());
			ASSERT_NO_THROW(roe.WriteCategory(output.wstring()));
			EXPECT_TRUE(Load(input) == Load(output));
		}
	}
	{
		// MonBridge v20 uses the same record as v10, so the legacy flag selects it.
		const auto input = std::filesystem::path(root) / "ROM/288/66.DAT";
		if (std::filesystem::exists(input)) {
			SCOPED_TRACE("ROM/288/66");
			const auto output = folder / "monbridge.dat";
			MonBridge mb;
			ASSERT_NO_THROW(mb.Read(input.wstring(), slotfile::Version::V20));
			ASSERT_FALSE(mb.data.empty());
			ASSERT_NO_THROW(mb.Write(output.wstring()));
			EXPECT_TRUE(Load(input) == Load(output));
		}
	}
}

// The typed view belongs to the layout that read the record: a v10 record is
// typed as the v10 Entry and a v20 record as the v20 one, so the spec area of an
// older record is readable and its values come from that record's own bytes.
//
// The same slots are patched for both versions (offset 36 holds 11, 42 holds 22,
// 46 holds 33). The two structs place `jobs` differently - v10 has a 2 byte
// JobFlags there, v20 a 4 byte word - so the very same bytes read back as a
// different (and known) value per version. A shared, newer-version-only typed
// view cannot produce this.

// The typed view belongs to the layout that read the record: a v10 record is
// typed as the v10 Entry and a v20 record as the v20 one, so the spec area of an
// older record is readable and its values come from that record's own bytes.
// The two versions place the spec fields differently, so a shared typed view of
// the newest layout cannot produce what this test checks.
TEST_F(ItemDataTest, LegacySpecIsReadThroughThatVersionOwnTypedView)
{
	const itmfmt::Version versions[] = { itmfmt::Version::V10, itmfmt::Version::V20 };
	const uint16_t racesValue = 11, dmgValue = 22, delayValue = 33;
	for (const auto version : versions) {
		SCOPED_TRACE(static_cast<int>(version));
		const size_t textOffset = version == itmfmt::Version::V10
			? itmfmt::v10::Format::TextOffset(ItemSpecType::WEAPON)
			: itmfmt::v20::Format::TextOffset(ItemSpecType::WEAPON);
		auto bytes = MakeLegacyFile(ItemSpecType::WEAPON);
		// Clear the spec area and the text area, patch the spec words, and lay the
		// text record where this version puts it (v10 after 48 bytes, v20 after 56).
		std::fill(bytes.begin() + 14, bytes.begin() + 640, 0);
		auto patch16 = [&](size_t specOffset, uint16_t value) {
			std::memcpy(bytes.data() + sizeof(itmfmt::v10::Header) + specOffset, &value, 2);
		};
		// Each layout is patched at its own offsets, so every version reads the
		// words its own struct places there (v20 puts slvl where v10 keeps the
		// second half of its 2 byte JobFlags).
		patch16(version == itmfmt::Version::V10
			? offsetof(itmfmt::v10::WeaponSpec, races) : offsetof(itmfmt::v20::WeaponSpec, races), racesValue);
		patch16(version == itmfmt::Version::V10
			? offsetof(itmfmt::v10::WeaponSpec, dmg) : offsetof(itmfmt::v20::WeaponSpec, dmg), dmgValue);
		patch16(version == itmfmt::Version::V10
			? offsetof(itmfmt::v10::WeaponSpec, delay) : offsetof(itmfmt::v20::WeaponSpec, delay), delayValue);
		Row text;
		text.GetCells().emplace_back(u8"Name");
		text.GetCells().emplace_back(u8"Description");
		text.WriteRow(reinterpret_cast<Record*>(bytes.data() + textOffset), static_cast<int>(640 - textOffset));
		const auto input = folder / "input.DAT";
		Save(input, bytes);
		ItemData items;
		items.encryptionSuppression = true;
		ASSERT_NO_THROW(items.Read(input.wstring(), ItemSpecType::WEAPON, version));
		ASSERT_EQ(items.data.size(), 1u);
		items.data[0].visit([&](const auto &datum) {
			using Datum = std::remove_cvref_t<decltype(datum)>;
			// The typed view of this record is the Entry of its own layout.
			const bool typedAsThisVersion = version == itmfmt::Version::V10
				? std::is_same_v<typename Datum::Entry, itmfmt::v10::Entry>
				: std::is_same_v<typename Datum::Entry, itmfmt::v20::Entry>;
			EXPECT_TRUE(typedAsThisVersion);

			const auto &spec = datum.originalEntry.spec.weapon;
			// The race mask is 16 bits in both versions, so the same two bytes read
			// back the same; the words after it describe this version only (v10 has
			// no slvl / ukn2, v20 no ukn_after_related), which is what makes this a
			// per version view and not a shared one.
			uint16_t races = 0;
			if constexpr (std::is_same_v<std::remove_cvref_t<decltype(spec.races)>, uint16_t>)
				races = spec.races;
			else
				std::memcpy(&races, &spec.races, sizeof(spec.races));
			EXPECT_EQ(races, racesValue);
			EXPECT_EQ(spec.dmg, dmgValue);
			EXPECT_EQ(spec.delay, delayValue);
			const bool hasSlvl = requires { spec.slvl; };
			EXPECT_EQ(hasSlvl, version == itmfmt::Version::V20);
		});
	}
}