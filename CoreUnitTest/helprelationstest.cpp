#include "pch.h"
#include "../FFXIDat/HelpData.h"
#include <array>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>

namespace
{
std::vector<char> LoadHelpRelationBytes(const std::filesystem::path& path)
{
	std::ifstream file(path, std::ios::binary);
	if (!file) throw std::runtime_error("Cannot read help relation test file");
	return { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
}

class HelpRelationsTest : public ::testing::Test
{
protected:
	std::filesystem::path folder;
	void SetUp() override
	{
		folder = std::filesystem::temp_directory_path() / ("help_relation_test_" +
			std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
		std::filesystem::create_directory(folder);
	}
	void TearDown() override { std::filesystem::remove_all(folder); }
};
}

TEST_F(HelpRelationsTest, EnglishStringsRemainIndependentAndIntegerIsProtected)
{
	std::vector<char> bytes(helpfmt::Format::slotSize, 0);
	slotfile::WriteU32(bytes.data(), 63034);
	bytes.back() = static_cast<char>(0xFF);
	Row row;
	row.GetCells().emplace_back(u8"Communication");
	row.GetCells().emplace_back(7);
	row.GetCells().emplace_back(u8"Emote");
	row.GetCells().emplace_back(u8"Emotes");
	row.GetCells().emplace_back(u8"Friends / Emotes");
	row.WriteRow(reinterpret_cast<Record*>(bytes.data() + 8), static_cast<int>(helpfmt::Format::TextCapacity(helpfmt::Schema::HELP)));
	HelpData file;
	ASSERT_NO_THROW(file.Parse(bytes));
	file.encryptionSuppression = true;
	EXPECT_EQ(file.Serialize(file.data), bytes);
	const auto csv = folder / "english.csv";
	ASSERT_NO_THROW(file.ToICsv(csv.wstring()));
	HelpData imported;
	imported.encryptionSuppression = true;
	imported.Parse(bytes);
	ASSERT_NO_THROW(imported.FromCsv(csv.wstring()));
	EXPECT_EQ(imported.Serialize(imported.data), bytes);
	EXPECT_TRUE(imported.data[0].row()[0].Get<std::u8string>() == u8"Communication");
	EXPECT_TRUE(imported.data[0].row()[2].Get<std::u8string>() == u8"Emote");
	EXPECT_TRUE(imported.data[0].row()[3].Get<std::u8string>() == u8"Emotes");
	file.data[0].row()[1].Set(8);
	file.ToICsv(csv.wstring());
	EXPECT_THROW(imported.FromCsv(csv.wstring()), std::runtime_error);
	EXPECT_EQ(imported.data[0].row()[1].Get<int>(), 7);
}

TEST_F(HelpRelationsTest, InstalledEnglishHelpRoundTripsIncludingCsv)
{
	const char* root = std::getenv("FFXI_ITEM_TEST_GAME_ROOT");
	if (!root)
	{
		RecordProperty("skipped", "FFXI_ITEM_TEST_GAME_ROOT is not set");
		return;
	}
	const auto path = std::filesystem::path(root) / "ROM/332/48.DAT";
	HelpData file;
	ASSERT_NO_THROW(file.Read(path.wstring()));
	ASSERT_EQ(file.data.size(), 240u);
	ASSERT_EQ(file.data[1].row().GetCellsConst().size(), 5u);
	EXPECT_TRUE(file.data[1].row()[0].Get<std::u8string>() == u8"Movement");
	EXPECT_TRUE(file.data[1].row()[4].Get<std::u8string>() == u8"Switching between autorun/walk character movement modes.");
	const auto bytes = LoadHelpRelationBytes(path);
	EXPECT_EQ(file.Serialize(file.data), bytes);
	const auto csv = folder / "live_english.csv";
	ASSERT_NO_THROW(file.ToICsv(csv.wstring()));
	ASSERT_NO_THROW(file.FromCsv(csv.wstring()));
	EXPECT_EQ(file.Serialize(file.data), bytes);
}

TEST_F(HelpRelationsTest, InstalledCategoryListsResolveToTheirHelpTable)
{
	const char* root = std::getenv("FFXI_ITEM_TEST_GAME_ROOT");
	if (!root)
	{
		RecordProperty("skipped", "FFXI_ITEM_TEST_GAME_ROOT is not set");
		return;
	}
	const auto base = std::filesystem::path(root) / "ROM/332";
	std::vector<char> japaneseMetadata;
	for (int categoryNumber : { 47, 49 })
	{
		HelpData help;
		help.Read((base / (std::to_string(categoryNumber - 1) + ".DAT")).wstring());
		std::set<uint32_t> ids;
		for (const auto& datum : help.data) ids.insert(datum.id);
		auto bytes = LoadHelpRelationBytes(base / (std::to_string(categoryNumber) + ".DAT"));
		ASSERT_EQ(bytes.size(), 16u * 0x1400);
		slotfile::Ror5Cipher::decrypt(bytes.data(), bytes.size());
		std::vector<char> metadata;
		std::set<uint32_t> referenced;
		const std::array<uint16_t, 6> activeCounts = { 56, 7, 14, 4, 11, 11 };
		for (size_t i = 0; i < 16; ++i)
		{
			const char* slot = bytes.data() + i * 0x1400;
			EXPECT_EQ(slotfile::ReadU32(slot), 0xF620u + i);
			EXPECT_EQ(slotfile::ReadU8(slot + 0x13FF), 255);
			const auto count = slotfile::ReadU16(slot + 6);
			ASSERT_LE(count, 64);
			EXPECT_EQ(count, i < 6 ? activeCounts[i] : 0);
			EXPECT_EQ(slotfile::ReadU16(slot + 4), i < 6 ? i + 1 : 0);
			for (size_t j = 0; j < count; ++j)
			{
				const auto id = slotfile::ReadU32(slot + 8 + 4 * j);
				EXPECT_EQ(ids.count(id), 1u);
				EXPECT_TRUE(referenced.insert(id).second);
			}
			for (size_t j = count; j < 64; ++j)
				EXPECT_EQ(slotfile::ReadU32(slot + 8 + 4 * j), 0u);
			metadata.insert(metadata.end(), slot, slot + 0x108);
			ASSERT_NO_THROW(slotfile::ValidateTextRecord(slot, 0x108, 0x13FF));
			Row row;
			ASSERT_NO_THROW(row.ReadRow(reinterpret_cast<Record*>(const_cast<char*>(slot) + 0x108), 0x13FF - 0x108));
			ASSERT_EQ(row.GetCellsConst().size(), categoryNumber == 47 ? 2u : 5u);
			if (categoryNumber == 49 && i == 0)
				EXPECT_TRUE(row[0].Get<std::u8string>() == u8"Basics");
		}
		EXPECT_EQ(referenced.size(), 103u);
		if (categoryNumber == 47) japaneseMetadata = metadata;
		else EXPECT_EQ(metadata, japaneseMetadata);
	}
}
