#pragma once

// Records of Eminence layout v30: the 0x1400 byte records of ROM/307/15 (quest)
// and ROM/307/23 (category) used by the ja/en tables after the 2026-09 update.
//
// This family stores two schemas that share the container: a quest entry with the
// reward block in front of the text, and a category entry with the child list in
// front of it. Both are modelled here; the layout types at the end of this file
// pick one schema and carry its offsets.

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "Record.h"
#include "SlotFile.h"
#include "CsvFile.h"

namespace roefmt
{

enum class Schema
{
	QUEST,
	CATEGORY,
};

using Version = slotfile::Version;

// The file level context of a CSV export: this family has one column set per
// schema, and a file holds one schema only.
struct CsvContext
{
	Schema schema = Schema::QUEST;
	// The text cell count of the records being exported. Every record of a
	// language shares one count, so this identifies the cell layout, which is
	// what decides whether a layout has a long name cell (ja has none).
	size_t textCells = 0;
};

namespace v30
{

#pragma pack(push, 1)

struct QuestEntry
{
	uint32_t id;
	uint32_t release_date; // Date in numeric format (e.g., 20141005 = 5 Oct 2014)
	uint32_t repeatable; // 0 = no, 1 = yes
	uint32_t target_count;
	uint32_t emi_reward;
	uint32_t exp_reward;
	uint32_t cap_reward;
	uint32_t uni_reward;
	union {
		char raw[5087];
		Record info_rec;
		// For Japanese, 3 cells in total, cell 0 is quest name, cell 1 is description, cell 2 is empty
		// For English, 5 cells in total, cell 0&1 are quest name (seems identical, or singular/plural form?),
		// cell 2 is empty, cell 3 is description, cell 4 is empty
	} info;
	char terminator; // must be 0xFF
};

struct CategoryEntry
{
	uint32_t id;
	uint32_t count_of_children;
	struct {
		uint32_t child_id; // refers to QuestEntry::id or CategoryEntry::id
		uint32_t quest_flag; // 0 = category, non-zero = actual quest
		uint32_t ukn[3]; // unknown, seems to be always 0
	} children[28];
	union {
		char raw[4551];
		Record info_rec; // Cell 0 is category name
	} info;
	char terminator; // must be 0xFF
};

#pragma pack(pop)

static_assert(sizeof(QuestEntry) == 0x1400, "v30 quest record size mismatch");
static_assert(sizeof(CategoryEntry) == 0x1400, "v30 category record size mismatch");
static_assert(offsetof(QuestEntry, info) == 32, "v30 quest text offset mismatch");
static_assert(offsetof(CategoryEntry, info) == 568, "v30 category text offset mismatch");

} // namespace v30

// ---------------------------------------------------------------- datums

// One Record of Eminence entry. The two schemas are different C++ types; both
// keep the exact slot bytes, the parsed text row and a typed view of the newest
// known layout of their schema.

class QuestDatum
{
public:
	uint32_t id = 0;
	uint32_t release_date = 0; // Date in numeric format (e.g., 20141005 = Oct 5, 2014)

	std::vector<char> rawBytes;
	std::string_view layoutId;

	// Typed view of the newest known layout of this schema.
	v30::QuestEntry originalEntry;

	Row originalRow;
	bool hasOriginalRow = false;

	Schema schema = Schema::QUEST;

	const std::vector<char> &raw() const { return rawBytes; }
	Row &row() { return originalRow; }
	const Row &row() const { return originalRow; }

	size_t cellCount() const { return hasOriginalRow ? originalRow.GetCellsConst().size() : 0; }

	template <class L>
	void load(const char *slot, typename L::Schema schema)
	{
		using Entry = typename L::Entry;
		if (schema != Schema::QUEST)
			throw std::logic_error("Quest datum cannot hold a category record");

		layoutId = L::id;
		rawBytes.assign(slot, slot + L::slotSize);

		if constexpr (std::is_same_v<Entry, v30::QuestEntry>)
			std::memcpy(&originalEntry, slot, sizeof(v30::QuestEntry));
		else
			std::memset(&originalEntry, 0, sizeof(v30::QuestEntry));

		originalEntry.id = slotfile::ReadU32(slot + offsetof(Entry, id));
		originalEntry.release_date = slotfile::ReadU32(slot + offsetof(Entry, release_date));
		originalEntry.repeatable = slotfile::ReadU32(slot + offsetof(Entry, repeatable));
		originalEntry.target_count = slotfile::ReadU32(slot + offsetof(Entry, target_count));
		originalEntry.emi_reward = slotfile::ReadU32(slot + offsetof(Entry, emi_reward));
		originalEntry.exp_reward = slotfile::ReadU32(slot + offsetof(Entry, exp_reward));
		originalEntry.cap_reward = slotfile::ReadU32(slot + offsetof(Entry, cap_reward));
		if constexpr (L::hasUniReward)
			originalEntry.uni_reward = slotfile::ReadU32(slot + offsetof(Entry, uni_reward));
		else
			originalEntry.uni_reward = 0;
		originalEntry.terminator = static_cast<char>(0xFF);

		id = originalEntry.id;
		release_date = originalEntry.release_date;

		originalRow.ReadRow(reinterpret_cast<Record *>(const_cast<char *>(slot) + L::TextOffset(schema)),
			static_cast<int>(L::TextCapacity(schema)));
		hasOriginalRow = true;
	}

	template <class L>
	void store(char *slot)
	{
		using Entry = typename L::Entry;
		slotfile::WriteU32(slot + offsetof(Entry, id), id);
		slotfile::WriteU32(slot + offsetof(Entry, release_date), release_date);
		slotfile::WriteU32(slot + offsetof(Entry, repeatable), originalEntry.repeatable);
		slotfile::WriteU32(slot + offsetof(Entry, target_count), originalEntry.target_count);
		slotfile::WriteU32(slot + offsetof(Entry, emi_reward), originalEntry.emi_reward);
		slotfile::WriteU32(slot + offsetof(Entry, exp_reward), originalEntry.exp_reward);
		slotfile::WriteU32(slot + offsetof(Entry, cap_reward), originalEntry.cap_reward);
		if constexpr (L::hasUniReward)
			slotfile::WriteU32(slot + offsetof(Entry, uni_reward), originalEntry.uni_reward);

		if (hasOriginalRow)
		{
			const size_t capacity = L::TextCapacity(schema);
			if (static_cast<size_t>(originalRow.GetSize()) > capacity)
				throw std::runtime_error("ROE quest text exceeds record capacity for id=" + std::to_string(id));
			originalRow.WriteRow(reinterpret_cast<Record *>(slot + L::TextOffset(schema)),
				static_cast<int>(capacity));
		}
	}

	// ============ Text Field Accessors ============

	// Get quest name (Cell 0)
	std::u8string questName() const
	{
		if (!hasOriginalRow)
			throw std::runtime_error("No original row data");

		const auto &cells = originalRow.GetCellsConst();
		if (cells.empty())
			throw std::out_of_range("Cell 0 does not exist");
		if (cells[0].GetType() != 0)
			throw std::runtime_error("Cell 0 is not a string");

		return cells[0].Get<std::u8string>();
	}

	// Set quest name (Cell 0). The long name cells of the layout (en cell 2, fr
	// cells 3 and 4, de cells 4 and 7) are updated as well, but only where they
	// still agree with cell 0.
	bool setQuestName(const std::u8string &newName)
	{
		if (!hasOriginalRow) return false;

		auto &cells = originalRow.GetCells();
		if (cells.empty()) return false;

		// The long name cells are not plain copies: on the live tables they hold the
		// full form of the same name (16 of 8176 fr rows, 14 of 8176 de rows and
		// 57 of 4088 en rows differ from cell 0). Only the cells that agreed with
		// cell 0 are updated, so a full form is never overwritten by the short one.
		// The list of those cells comes from the same table as questNameLong, so
		// the layout is written down once.
		std::u8string previous;
		if (cells[0].GetType() == 0) previous = cells[0].Get<std::u8string>();
		cells[0].Set(newName);
		const size_t longNames = LongNameCount(cells.size());
		for (size_t i = 0; i < longNames; ++i)
		{
			const size_t index = LongNameCell(cells.size(), i);
			if (index < cells.size() && cells[index].GetType() == 0 &&
				cells[index].Get<std::u8string>() == previous)
				cells[index].Set(newName);
		}
		return true;
	}

	// Cell layout, confirmed on the live tables: every record of a language shares
	// one cell count, so the count identifies the layout.
	//
	//   ja  3 cells: 0 name, 1 description, 2 empty
	//   en  6 cells: 0 name, 1 flag, 2 name, 3 empty, 4 description, 5 empty
	//   fr  7 cells: 0 name, 1 flag, 2 int, 3 name, 4 name, 5 description, 6 empty
	//   de 10 cells: 0 name, 1 flag, 2 int, 3 int, 4 name, 5 int, 6 int, 7 name, 8 description, 9 empty
	//
	// An unknown cell count returns an out of range index, so the caller reports
	// "cell not found" instead of silently reading another field.
	static size_t DescriptionCell(size_t count)
	{
		switch (count)
		{
		case 3: return 1;  // ja
		case 6: return 4;  // en
		case 7: return 5;  // fr
		case 10: return 8; // de
		}
		return count;
	}

	// The long name of a quest: en 1 cell, fr 2 cells, de 2 cells, ja none. On the
	// live tables that cell is not a copy of cell 0 but the full form of the same
	// name - the short and the long cell differ on 16 of 8176 fr rows, 14 of 8176
	// de rows and 57 of 4088 en rows (for example "U. Communique A (UC)" against
	// "Unity Communique A (UC)"), which is an abbreviation against its full form,
	// so the long cell is its own field and not a derived value. The two cells of
	// fr and of de hold the same text in every observed row.
	static size_t LongNameCount(size_t count)
	{
		switch (count)
		{
		case 6: return 1;  // en
		case 7: return 2;  // fr
		case 10: return 2; // de
		}
		return 0; // ja and unknown layouts carry no long name
	}

	// The cell of the index-th long name; an unknown count or an index past the
	// end of the list returns count, an out of range index, so the caller reports
	// "cell not found" instead of silently reading another field.
	static size_t LongNameCell(size_t count, size_t index)
	{
		switch (count)
		{
		case 6: return index == 0 ? 2 : count;                 // en
		case 7: return index == 0 ? 3 : (index == 1 ? 4 : count);  // fr
		case 10: return index == 0 ? 4 : (index == 1 ? 7 : count); // de
		}
		return count;
	}

	static size_t NoteCell(size_t count)
	{
		switch (count)
		{
		case 3: return 2;  // ja
		case 6: return 5;  // en
		case 7: return 6;  // fr
		case 10: return 9; // de
		}
		return count;
	}

	// Get description (ja 1, en 4, fr 5, de 8)
	std::u8string description() const
	{
		if (!hasOriginalRow)
			throw std::runtime_error("No original row data");

		const auto &cells = originalRow.GetCellsConst();
		const size_t index = DescriptionCell(cells.size());
		if (index < cells.size() && cells[index].GetType() == 0)
			return cells[index].Get<std::u8string>();

		throw std::out_of_range("Description cell not found");
	}

	bool setDescription(const std::u8string &newDesc)
	{
		if (!hasOriginalRow) return false;

		auto &cells = originalRow.GetCells();
		const size_t index = DescriptionCell(cells.size());
		if (index >= cells.size()) return false;
		cells[index].Set(newDesc);
		return true;
	}

	// Get note (ja 2, en 5, fr 6, de 9 - every observed language leaves it empty)
	std::u8string note() const
	{
		if (!hasOriginalRow)
			throw std::runtime_error("No original row data");

		const auto &cells = originalRow.GetCellsConst();
		const size_t index = NoteCell(cells.size());
		if (index < cells.size() && cells[index].GetType() == 0)
			return cells[index].Get<std::u8string>();

		return u8"";
	}

	// Get the long name of the quest (en 2, fr 3&4, de 4&7): the full form of the
	// name, of which cell 0 usually holds an abbreviation. The first cell that
	// differs from cell 0 is preferred; when every long cell agrees with cell 0
	// the first one is returned. A layout without a long name cell (ja) and a
	// datum without a row return an empty string, like note() does.
	std::u8string questNameLong() const
	{
		if (!hasOriginalRow) return u8"";

		const auto &cells = originalRow.GetCellsConst();
		const size_t longNames = LongNameCount(cells.size());
		if (longNames == 0) return u8"";

		std::u8string shortName;
		if (!cells.empty() && cells[0].GetType() == 0) shortName = cells[0].Get<std::u8string>();

		std::u8string first;
		bool hasFirst = false;
		for (size_t i = 0; i < longNames; ++i)
		{
			const size_t index = LongNameCell(cells.size(), i);
			if (index >= cells.size() || cells[index].GetType() != 0) continue;
			std::u8string value = cells[index].Get<std::u8string>();
			if (!hasFirst)
			{
				first = value;
				hasFirst = true;
			}
			if (value != shortName) return value;
		}
		return hasFirst ? first : u8"";
	}

	// Set every long name cell of the layout (the two of fr and de hold the same
	// text on the live tables). False when the layout has no long name cell, when
	// the datum has no row or when a cell cannot be written.
	bool setQuestNameLong(const std::u8string &newName)
	{
		if (!hasOriginalRow) return false;

		auto &cells = originalRow.GetCells();
		const size_t longNames = LongNameCount(cells.size());
		if (longNames == 0) return false;

		for (size_t i = 0; i < longNames; ++i)
		{
			const size_t index = LongNameCell(cells.size(), i);
			if (index >= cells.size()) return false;
			cells[index].Set(newName);
		}
		return true;
	}

	bool setNote(const std::u8string &newNote)
	{
		if (!hasOriginalRow) return false;

		auto &cells = originalRow.GetCells();
		const size_t index = NoteCell(cells.size());
		if (index >= cells.size()) return false;
		cells[index].Set(newNote);
		return true;
	}
};

class CategoryDatum
{
public:
	uint32_t id = 0;

	std::vector<char> rawBytes;
	std::string_view layoutId;

	// Typed view of the newest known layout of this schema.
	v30::CategoryEntry originalEntry;

	Row originalRow;
	bool hasOriginalRow = false;

	Schema schema = Schema::CATEGORY;

	const std::vector<char> &raw() const { return rawBytes; }
	Row &row() { return originalRow; }
	const Row &row() const { return originalRow; }

	size_t cellCount() const { return hasOriginalRow ? originalRow.GetCellsConst().size() : 0; }

	template <class L>
	void load(const char *slot, typename L::Schema schema)
	{
		using Entry = typename L::Entry;
		if (schema != Schema::CATEGORY)
			throw std::logic_error("Category datum cannot hold a quest record");

		layoutId = L::id;
		rawBytes.assign(slot, slot + L::slotSize);

		if constexpr (std::is_same_v<Entry, v30::CategoryEntry>)
			std::memcpy(&originalEntry, slot, sizeof(v30::CategoryEntry));
		else
			std::memset(&originalEntry, 0, sizeof(v30::CategoryEntry));

		originalEntry.id = slotfile::ReadU32(slot + offsetof(Entry, id));
		originalEntry.count_of_children = slotfile::ReadU32(slot + offsetof(Entry, count_of_children));
		std::memcpy(originalEntry.children, slot + offsetof(Entry, children), sizeof(originalEntry.children));
		originalEntry.terminator = static_cast<char>(0xFF);

		id = originalEntry.id;

		originalRow.ReadRow(reinterpret_cast<Record *>(const_cast<char *>(slot) + L::TextOffset(schema)),
			static_cast<int>(L::TextCapacity(schema)));
		hasOriginalRow = true;
	}

	template <class L>
	void store(char *slot)
	{
		using Entry = typename L::Entry;
		slotfile::WriteU32(slot + offsetof(Entry, id), id);
		slotfile::WriteU32(slot + offsetof(Entry, count_of_children), originalEntry.count_of_children);

		if (hasOriginalRow)
		{
			const size_t capacity = L::TextCapacity(schema);
			if (static_cast<size_t>(originalRow.GetSize()) > capacity)
				throw std::runtime_error("ROE category text exceeds record capacity for id=" + std::to_string(id));
			originalRow.WriteRow(reinterpret_cast<Record *>(slot + L::TextOffset(schema)),
				static_cast<int>(capacity));
		}
	}

	// ============ Text Field Accessors ============

	// Get category name (Cell 0)
	std::u8string categoryName() const
	{
		if (!hasOriginalRow)
			throw std::runtime_error("No original row data");

		const auto &cells = originalRow.GetCellsConst();
		if (cells.empty())
			throw std::out_of_range("Cell 0 does not exist");
		if (cells[0].GetType() != 0)
			throw std::runtime_error("Cell 0 is not a string");

		return cells[0].Get<std::u8string>();
	}

	bool setCategoryName(const std::u8string &newName)
	{
		if (!hasOriginalRow) return false;

		auto &cells = originalRow.GetCells();
		if (cells.empty()) return false;

		cells[0].Set(newName);
		return true;
	}
};

// ---------------------------------------------------------------- layouts

// CSV view of both schemas; defined in RoeFormatsCsv.cpp. The hooks are declared
// here so that every version of the family can reach them across translation
// units (VersionFormat::id / VersionFormat::scope above are shared the same way).
void WriteQuestCsvHeader(CsvFile &csv, const CsvContext &context);
void WriteQuestCsvRow(CsvFile &csv, const QuestDatum &datum);
void WriteCategoryCsvHeader(CsvFile &csv, const CsvContext &context);
void WriteCategoryCsvRow(CsvFile &csv, const CategoryDatum &datum);
namespace detail
{
// Binds one schema of one version: the version (Format, below) owns the offsets
// and the record structures, this only picks the entry type, the datum type and
// the defaults the container works with.
template <class VersionFormat, Schema K>
struct SchemaOf
{
	using Entry = typename VersionFormat::template EntryOf<K>;
	using Datum = std::conditional_t<K == Schema::QUEST, QuestDatum, CategoryDatum>;
	using Schema = roefmt::Schema;
	using CsvContext = roefmt::CsvContext;

	static constexpr std::string_view id = VersionFormat::id;
	static constexpr std::string_view scope = VersionFormat::scope;
	static constexpr Version version = VersionFormat::version;
	static constexpr Schema schema = K;
	static constexpr size_t slotSize = VersionFormat::slotSize;
	static constexpr size_t cipherSpan = VersionFormat::slotSize;
	static constexpr size_t currencySlots = VersionFormat::currencySlots;
	static constexpr size_t blobLengthOffset = VersionFormat::blobLengthOffset;
	static constexpr size_t blobDataOffset = VersionFormat::blobDataOffset;
	static constexpr size_t blobCapacity = VersionFormat::blobCapacity;
	static constexpr size_t textEnd = VersionFormat::textEnd;
	static constexpr bool hasUniReward = VersionFormat::hasUniReward;
	static constexpr Schema DEFAULT_SCHEMA = K;

	static constexpr bool IsCurrencySchema(Schema s) { return VersionFormat::IsCurrencySchema(s); }
	static constexpr size_t TextOffset(Schema s) { return VersionFormat::TextOffset(s); }
	static constexpr size_t TextCapacity(Schema s) { return VersionFormat::TextCapacity(s); }

	static CsvContext GetCsvContext(std::span<const Datum> records)
	{
		CsvContext context;
		context.schema = K;
		if (!records.empty() && records.front().hasOriginalRow)
			context.textCells = records.front().originalRow.GetCellsConst().size();
		return context;
	}
	static void WriteCsvHeader(CsvFile &csv, const CsvContext &context)
	{
		if constexpr (K == Schema::QUEST) WriteQuestCsvHeader(csv, context);
		else WriteCategoryCsvHeader(csv, context);
	}
	static void WriteCsvRow(CsvFile &csv, const CsvContext &context, const Datum &datum)
	{
		if constexpr (K == Schema::QUEST) WriteQuestCsvRow(csv, datum);
		else WriteCategoryCsvRow(csv, datum);
	}
};
} // namespace detail

namespace v30
{
// One format per version, this is where the schema axis lives: TextOffset picks
// the record's text position per schema.
struct Format
{
	using Schema = roefmt::Schema;
	using QuestEntry = v30::QuestEntry;
	using CategoryEntry = v30::CategoryEntry;
	template <Schema K> using EntryOf = std::conditional_t<K == Schema::QUEST, QuestEntry, CategoryEntry>;

	static constexpr std::string_view id = "v30";
	static constexpr std::string_view scope = "ja/en Records of Eminence tables, 2026-10 update onwards";
	static constexpr Version version = Version::V30;

	static constexpr size_t slotSize = sizeof(QuestEntry); // == sizeof(CategoryEntry)
	static constexpr size_t cipherSpan = slotSize;
	static constexpr size_t currencySlots = 1; // no currency form in this family
	static constexpr size_t blobLengthOffset = 0; // no tail blob in this family
	static constexpr size_t blobDataOffset = 0;
	static constexpr size_t blobCapacity = 0;
	static constexpr size_t textEnd = slotSize - 1;
	static constexpr bool hasUniReward = true;

	static constexpr bool IsCurrencySchema(Schema) { return false; }
	static constexpr size_t TextOffset(Schema s)
	{
		return s == Schema::QUEST ? offsetof(QuestEntry, info) : offsetof(CategoryEntry, info);
	}
	static constexpr size_t TextCapacity(Schema s) { return textEnd - TextOffset(s); }
};

using Quest = detail::SchemaOf<Format, Schema::QUEST>;
using Category = detail::SchemaOf<Format, Schema::CATEGORY>;

static_assert(Format::slotSize == 0x1400, "v30 slot size mismatch");
static_assert(Format::TextOffset(Schema::QUEST) == 32, "v30 quest text offset mismatch");
static_assert(Format::TextOffset(Schema::CATEGORY) == 568, "v30 category text offset mismatch");
static_assert(Quest::TextCapacity(Schema::QUEST) == 5087, "v30 quest text capacity mismatch");
static_assert(Category::TextCapacity(Schema::CATEGORY) == 4551, "v30 category text capacity mismatch");
static_assert(slotfile::SlotDatum<QuestDatum, Quest>, "v30 quest datum does not match the container protocol");
static_assert(slotfile::SlotDatum<CategoryDatum, Category>, "v30 category datum does not match the container protocol");
} // namespace v30
} // namespace roefmt
