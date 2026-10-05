#include "pch.h"
#include "../FFXIDat/HelpCategory.h"
#include "../FFXIDat/HelpData.h"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>

namespace
{
std::vector<char> CategoryBytes(bool english = false, size_t count = 3)
{
	std::vector<char> bytes(hlcfmt::Format::slotSize, 0);
	slotfile::WriteU32(bytes.data(), 0xF620);
	slotfile::WriteU16(bytes.data() + 4, 1);
	slotfile::WriteU16(bytes.data() + 6, static_cast<uint16_t>(count));
	for (size_t i = 0; i < count; ++i)
		slotfile::WriteU32(bytes.data() + 8 + i * 4, static_cast<uint32_t>(63090 - i));
	if (count < 64)
		slotfile::WriteU32(bytes.data() + 8 + count * 4, 0xA55A1234);
	bytes[0x1200] = 0x35;
	bytes.back() = static_cast<char>(0xFF);
	Row row;
	row.GetCells().emplace_back(u8"Basics");
	if (english)
	{
		row.GetCells().emplace_back(7);
		row.GetCells().emplace_back(u8"Name Two");
		row.GetCells().emplace_back(u8"Name Three");
	}
	row.GetCells().emplace_back(u8"Basic information");
	row.WriteRow(reinterpret_cast<Record*>(bytes.data() + 0x108),
		static_cast<int>(hlcfmt::Format::TextCapacity(hlcfmt::Schema::CATEGORY)));
	return bytes;
}

std::vector<char> ReadCategoryBytes(const std::filesystem::path& path)
{
	std::ifstream file(path, std::ios::binary);
	if (!file) throw std::runtime_error("Cannot read help category test file");
	return { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
}

class HelpCategoryTest : public ::testing::Test
{
protected:
	std::filesystem::path folder;
	void SetUp() override
	{
		folder = std::filesystem::temp_directory_path() / ("help_category_test_" +
			std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
		std::filesystem::create_directory(folder);
	}
	void TearDown() override { std::filesystem::remove_all(folder); }
};
}

TEST_F(HelpCategoryTest, BothLanguagesRoundTripAndPreserveMemberOrder)
{
	for (bool english : { false, true })
	{
		const auto bytes = CategoryBytes(english);
		HelpCategory file;
		file.encryptionSuppression = true;
		ASSERT_NO_THROW(file.Parse(bytes));
		ASSERT_EQ(file.data.size(), 1u);
		EXPECT_EQ(file.data[0].members, (std::vector<uint32_t>{ 63090, 63089, 63088 }));
		EXPECT_TRUE(HelpCategory::MemberIds(file.data[0]) == u8"63090|63089|63088");
		EXPECT_EQ(file.Serialize(file.data), bytes);
		const int description = english ? 4 : 1;
		file.data[0].row()[description].Set(u8"Updated category description");
		const auto edited = file.Serialize(file.data);
		EXPECT_TRUE(std::equal(bytes.begin(), bytes.begin() + 0x108, edited.begin()));
		EXPECT_EQ(edited[0x1200], 0x35);
		HelpCategory reread;
		reread.Parse(edited);
		EXPECT_TRUE(reread.data[0].row()[description].Get<std::u8string>() == u8"Updated category description");
		file.encryptionSuppression = false;
		const auto path = folder / "encrypted.DAT";
		file.Write(path.wstring());
		ASSERT_NO_THROW(reread.Read(path.wstring()));
		EXPECT_EQ(reread.data[0].members, file.data[0].members);
		EXPECT_TRUE(reread.data[0].row()[description].Get<std::u8string>() == u8"Updated category description");
	}
}

TEST_F(HelpCategoryTest, EmptyAndFullMemberListsAreAccepted)
{
	for (size_t count : { 0u, 64u })
	{
		HelpCategory file;
		file.encryptionSuppression = true;
		const auto bytes = CategoryBytes(false, count);
		ASSERT_NO_THROW(file.Parse(bytes));
		EXPECT_EQ(file.data[0].members.size(), count);
		EXPECT_EQ(file.Serialize(file.data), bytes);
	}
}

TEST_F(HelpCategoryTest, MalformedSlotsAreRejectedWithoutReplacingExistingData)
{
	HelpCategory file;
	file.Parse(CategoryBytes());
	auto bytes = CategoryBytes();
	slotfile::WriteU16(bytes.data() + 6, 65);
	EXPECT_THROW(file.Parse(bytes), std::runtime_error);
	bytes = CategoryBytes();
	bytes.back() = 0;
	EXPECT_THROW(file.Parse(bytes), std::runtime_error);
	bytes = CategoryBytes();
	slotfile::WriteI32(bytes.data() + 0x108, 1);
	EXPECT_THROW(file.Parse(bytes), std::runtime_error);
	bytes = CategoryBytes();
	slotfile::WriteI32(bytes.data() + 0x10C, 0x1400);
	EXPECT_THROW(file.Parse(bytes), std::runtime_error);
	bytes = CategoryBytes();
	bytes.pop_back();
	EXPECT_THROW(file.Parse(bytes), std::runtime_error);
	ASSERT_EQ(file.data.size(), 1u);
	EXPECT_EQ(file.data[0].members.size(), 3u);
	HelpData entries;
	EXPECT_THROW(entries.Parse(CategoryBytes()), std::runtime_error);
}

TEST_F(HelpCategoryTest, OversizedTextAndMemberListDoNotOpenOutput)
{
	HelpCategory file;
	file.Parse(CategoryBytes());
	const auto path = folder / "oversized.DAT";
	file.data[0].row()[1].Set(std::u8string(0x1400, u8'x'));
	EXPECT_THROW(file.Write(path.wstring()), std::runtime_error);
	EXPECT_FALSE(std::filesystem::exists(path));
	file.Parse(CategoryBytes());
	file.data[0].members.resize(65);
	EXPECT_THROW(file.Write(path.wstring()), std::runtime_error);
	EXPECT_FALSE(std::filesystem::exists(path));
}

TEST_F(HelpCategoryTest, CsvRoundTripAndTextEditsPreserveMetadata)
{
	for (bool english : { false, true })
	{
		HelpCategory original;
		original.encryptionSuppression = true;
		original.Parse(CategoryBytes(english));
		const auto csv = folder / "category.csv";
		original.ToICsv(csv.wstring());
		HelpCategory imported;
		imported.encryptionSuppression = true;
		imported.Parse(CategoryBytes(english));
		ASSERT_NO_THROW(imported.FromCsv(csv.wstring()));
		EXPECT_EQ(imported.Serialize(imported.data), CategoryBytes(english));
		const int description = english ? 4 : 1;
		original.data[0].row()[description].Set(u8"Quoted \"text\",\nnew line");
		if (english) original.data[0].row()[2].Set(u8"Independent second name");
		original.ToICsv(csv.wstring());
		ASSERT_NO_THROW(imported.FromCsv(csv.wstring()));
		EXPECT_TRUE(imported.data[0].row()[description].Get<std::u8string>() == u8"Quoted \"text\",\nnew line");
		EXPECT_EQ(imported.data[0].members, original.data[0].members);
		if (english)
		{
			EXPECT_TRUE(imported.data[0].row()[2].Get<std::u8string>() == u8"Independent second name");
			EXPECT_TRUE(imported.data[0].row()[3].Get<std::u8string>() == u8"Name Three");
		}
	}
}

TEST_F(HelpCategoryTest, CsvRejectsMetadataChangesDuplicatesAndMissingRecordsAtomically)
{
	const auto bytes = CategoryBytes(true);
	const auto csv = folder / "invalid.csv";
	HelpCategory original;
	original.Parse(bytes);
	HelpCategory imported;
	imported.encryptionSuppression = true;
	imported.Parse(bytes);
	for (int mutation = 0; mutation < 7; ++mutation)
	{
		original.Parse(bytes);
		auto& datum = original.data[0];
		switch (mutation)
		{
		case 0: ++datum.id; break;
		case 1: ++datum.index; break;
		case 2: datum.members.pop_back(); break;
		case 3: std::reverse(datum.members.begin(), datum.members.end()); break;
		case 4: datum.row()[1].Set(8); break;
		case 5: original.data.push_back(datum); break;
		case 6: original.data.clear(); break;
		}
		original.ToICsv(csv.wstring());
		EXPECT_THROW(imported.FromCsv(csv.wstring()), std::runtime_error);
		EXPECT_EQ(imported.Serialize(imported.data), bytes);
	}
}

TEST_F(HelpCategoryTest, InstalledJapaneseAndEnglishCategoriesRoundTripIncludingCsv)
{
	const char* root = std::getenv("FFXI_ITEM_TEST_GAME_ROOT");
	if (!root)
	{
		RecordProperty("skipped", "FFXI_ITEM_TEST_GAME_ROOT is not set");
		return;
	}
	for (int number : { 47, 49 })
	{
		const auto path = std::filesystem::path(root) / "ROM/332" / (std::to_string(number) + ".DAT");
		const auto bytes = ReadCategoryBytes(path);
		HelpCategory file;
		ASSERT_NO_THROW(file.Read(path.wstring()));
		ASSERT_EQ(file.data.size(), 16u);
		EXPECT_EQ(file.data[0].id, 0xF620u);
		EXPECT_EQ(file.data[0].members.size(), 56u);
		EXPECT_EQ(file.Serialize(file.data), bytes);
		size_t total = 0;
		for (const auto& datum : file.data) total += datum.members.size();
		EXPECT_EQ(total, 103u);
		const auto csv = folder / "live.csv";
		ASSERT_NO_THROW(file.ToICsv(csv.wstring()));
		ASSERT_NO_THROW(file.FromCsv(csv.wstring()));
		EXPECT_EQ(file.Serialize(file.data), bytes);
	}
}
