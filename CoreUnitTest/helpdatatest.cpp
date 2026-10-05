#include "pch.h"
#include "../FFXIDat/HelpData.h"
#include "../FFXIDat/ItemFormatV30.h"
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>

namespace
{
std::vector<char> HelpBytes()
{
	std::vector<char> bytes(helpfmt::Format::slotSize, 0);
	slotfile::WriteU32(bytes.data(), 63025);
	slotfile::WriteU8(bytes.data() + 4, 1);
	slotfile::WriteU8(bytes.data() + 5, 3);
	slotfile::WriteU16(bytes.data() + 6, 0xA55A);
	bytes[0x1200] = 0x35;
	bytes.back() = static_cast<char>(0xFF);
	Row row;
	row.GetCells().emplace_back(u8"Movement");
	row.GetCells().emplace_back(u8"Move / auto-run.");
	row.WriteRow(reinterpret_cast<Record*>(bytes.data() + 8), helpfmt::Format::TextCapacity(helpfmt::Schema::HELP));
	return bytes;
}

std::vector<char> ReadBytes(const std::filesystem::path& path)
{
	std::ifstream file(path, std::ios::binary);
	if (!file) throw std::runtime_error("Cannot open help test file");
	return { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
}

class HelpDataTest : public ::testing::Test
{
protected:
	std::filesystem::path folder;
	void SetUp() override
	{
		folder = std::filesystem::temp_directory_path() / ("help_dat_test_" +
			std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
		std::filesystem::create_directory(folder);
	}
	void TearDown() override { std::filesystem::remove_all(folder); }
};
}

TEST_F(HelpDataTest, RoundTripAndTextEditPreserveOpaqueBytes)
{
	const auto bytes = HelpBytes();
	HelpData file;
	file.Parse(bytes);
	file.encryptionSuppression = true;
	EXPECT_EQ(file.Serialize(file.data), bytes);
	file.data[0].row()[1].Set(u8"Changed description");
	const auto edited = file.Serialize(file.data);
	HelpData reread;
	reread.Parse(edited);
	EXPECT_TRUE(reread.data[0].row()[1].Get<std::u8string>() == u8"Changed description");
	EXPECT_EQ(reread.data[0].reserved, 0xA55A);
	EXPECT_EQ(edited[0x1200], 0x35);
	file.encryptionSuppression = false;
	file.Write((folder / "edited.DAT").wstring());
	ASSERT_NO_THROW(reread.Read((folder / "edited.DAT").wstring()));
	EXPECT_TRUE(reread.data[0].row()[1].Get<std::u8string>() == u8"Changed description");
}

TEST_F(HelpDataTest, InvalidSlotsAndOversizedTextAreRejected)
{
	HelpData file;
	auto bytes = HelpBytes();
	file.Parse(bytes);
	slotfile::SlotFile<itmfmt::v30::Format> items;
	EXPECT_THROW(items.Parse(bytes, itmfmt::SpecType::USABLE), std::exception);
	bytes.back() = 0;
	EXPECT_THROW(file.Parse(bytes), std::runtime_error);
	EXPECT_EQ(file.data.size(), 1u);
	bytes = HelpBytes();
	slotfile::WriteI32(bytes.data() + 8, 1);
	EXPECT_THROW(file.Parse(bytes), std::runtime_error);
	bytes = HelpBytes();
	slotfile::WriteI32(bytes.data() + 12, 0x1400);
	EXPECT_THROW(file.Parse(bytes), std::runtime_error);
	bytes = HelpBytes();
	bytes.pop_back();
	EXPECT_THROW(file.Parse(bytes), std::runtime_error);
	file.data[0].row()[1].Set(std::u8string(0x1400, u8'x'));
	const auto output = folder / "oversized.DAT";
	EXPECT_THROW(file.Write(output.wstring()), std::runtime_error);
	EXPECT_FALSE(std::filesystem::exists(output));
}

TEST_F(HelpDataTest, CsvRoundTripPreservesMetadataAndEscapedText)
{
	HelpData file;
	file.Parse(HelpBytes());
	file.data[0].row()[1].Set(u8"Quote: \"text\",\nnext line");
	const auto csv = folder / "help.csv";
	file.ToICsv(csv.wstring());
	HelpData imported;
	imported.Parse(HelpBytes());
	ASSERT_NO_THROW(imported.FromCsv(csv.wstring()));
	EXPECT_TRUE(imported.data[0].row()[1].Get<std::u8string>() == u8"Quote: \"text\",\nnext line");
	EXPECT_EQ(imported.data[0].reserved, 0xA55A);
	EXPECT_EQ(imported.data[0].unknown, 3);
	file.data[0].unknown = 2;
	file.ToICsv(csv.wstring());
	EXPECT_THROW(imported.FromCsv(csv.wstring()), std::runtime_error);
	EXPECT_EQ(imported.data[0].unknown, 3);
}

TEST_F(HelpDataTest, InstalledHelpTableRoundTripsByteIdentically)
{
	const char* root = std::getenv("FFXI_ITEM_TEST_GAME_ROOT");
	if (!root)
	{
		RecordProperty("skipped", "FFXI_ITEM_TEST_GAME_ROOT is not set");
		std::cout << "[ SKIPPED ] set FFXI_ITEM_TEST_GAME_ROOT to check installed help DAT" << std::endl;
		return;
	}
	const auto input = std::filesystem::path(root) / "ROM/332/46.DAT";
	HelpData file;
	ASSERT_NO_THROW(file.Read(input.wstring()));
	ASSERT_EQ(file.data.size(), 240u);
	EXPECT_EQ(file.data.front().id, 63024u);
	EXPECT_EQ(file.data.back().id, 63263u);
	EXPECT_EQ(file.Serialize(file.data), ReadBytes(input));
	const auto csv = folder / "live.csv";
	ASSERT_NO_THROW(file.ToICsv(csv.wstring()));
	ASSERT_NO_THROW(file.FromCsv(csv.wstring()));
	EXPECT_EQ(file.Serialize(file.data), ReadBytes(input));
}
