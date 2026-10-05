#pragma once

#include "SlotFile.h"

namespace hlcfmt
{
enum class Schema { CATEGORY };

#pragma pack(push, 1)
struct Entry
{
	uint32_t id;
	uint16_t index;
	uint16_t count;
	uint32_t members[64];
	char text[0x1400 - 0x108 - 1];
	char terminator;
};
#pragma pack(pop)

static_assert(sizeof(Entry) == 0x1400);
static_assert(offsetof(Entry, members) == 8);
static_assert(offsetof(Entry, text) == 0x108);
static_assert(offsetof(Entry, terminator) == 0x13FF);

class Datum
{
public:
	uint32_t id = 0;
	uint16_t index = 0;
	std::vector<uint32_t> members;
	Schema schema = Schema::CATEGORY;

	Row &row() { return textRow; }
	const Row &row() const { return textRow; }
	const std::vector<char> &raw() const { return rawBytes; }

	template <class L>
	void load(const char *slot, typename L::Schema value)
	{
		schema = value;
		id = slotfile::ReadU32(slot);
		index = slotfile::ReadU16(slot + 4);
		const auto count = slotfile::ReadU16(slot + 6);
		if (count > L::memberCapacity)
			throw std::runtime_error("Help category member count exceeds capacity");
		members.clear();
		for (size_t i = 0; i < count; ++i)
			members.push_back(slotfile::ReadU32(slot + L::membersOffset + i * 4));
		rawBytes.assign(slot, slot + L::slotSize);
		textRow.ReadRow(reinterpret_cast<Record *>(const_cast<char *>(slot) + L::TextOffset(schema)),
			static_cast<int>(L::TextCapacity(schema)));
		ValidateRow();
	}

	template <class L>
	void store(char *slot)
	{
		ValidateRow();
		if (members.size() > L::memberCapacity)
			throw std::runtime_error("Help category member count exceeds capacity");
		if (static_cast<size_t>(textRow.GetSize()) > L::TextCapacity(schema))
			throw std::runtime_error("Help category text exceeds slot capacity");
		slotfile::WriteU32(slot, id);
		slotfile::WriteU16(slot + 4, index);
		slotfile::WriteU16(slot + 6, static_cast<uint16_t>(members.size()));
		for (size_t i = 0; i < members.size(); ++i)
			slotfile::WriteU32(slot + L::membersOffset + i * 4, members[i]);
		textRow.WriteRow(reinterpret_cast<Record *>(slot + L::TextOffset(schema)),
			static_cast<int>(L::TextCapacity(schema)));
	}

private:
	Row textRow;
	std::vector<char> rawBytes;

	void ValidateRow() const
	{
		const auto &cells = textRow.GetCellsConst();
		const bool japanese = cells.size() == 2 &&
			cells[0].GetType() == 0 && cells[1].GetType() == 0;
		const bool english = cells.size() == 5 && cells[0].GetType() == 0 &&
			cells[1].GetType() == 1 && cells[2].GetType() == 0 &&
			cells[3].GetType() == 0 && cells[4].GetType() == 0;
		if (!japanese && !english)
			throw std::runtime_error("Invalid Japanese or English help category text layout");
	}
};

struct Format
{
	using Entry = hlcfmt::Entry;
	using Datum = hlcfmt::Datum;
	using Schema = hlcfmt::Schema;

	static constexpr std::string_view id = "v10";
	static constexpr std::string_view scope = "ROM/332/47 and ROM/332/49 help category tables";
	static constexpr size_t slotSize = sizeof(Entry);
	static constexpr size_t cipherSpan = slotSize;
	static constexpr size_t currencySlots = 1;
	static constexpr size_t blobLengthOffset = 0;
	static constexpr size_t blobDataOffset = 0;
	static constexpr size_t blobCapacity = 0;
	static constexpr size_t textEnd = offsetof(Entry, terminator);
	static constexpr size_t membersOffset = offsetof(Entry, members);
	static constexpr size_t memberCapacity = 64;
	static constexpr Schema DEFAULT_SCHEMA = Schema::CATEGORY;
	static constexpr bool IsCurrencySchema(Schema) { return false; }
	static constexpr size_t TextOffset(Schema) { return offsetof(Entry, text); }
	static constexpr size_t TextCapacity(Schema) { return sizeof(Entry::text); }
};

static_assert(slotfile::SlotLayoutInvariants<Format>);
} // namespace hlcfmt
