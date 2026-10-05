#pragma once

#include "SlotFile.h"

namespace helpfmt
{
enum class Schema { HELP };

#pragma pack(push, 1)
struct Entry
{
	uint32_t id;
	uint8_t index;
	uint8_t unknown;
	uint16_t reserved;
	char text[0x1400 - 9];
	char terminator;
};
#pragma pack(pop)

static_assert(sizeof(Entry) == 0x1400);
static_assert(offsetof(Entry, text) == 8);

class Datum
{
public:
	uint32_t id = 0;
	uint8_t index = 0;
	uint8_t unknown = 0;
	uint16_t reserved = 0;
	Schema schema = Schema::HELP;

	Row &row() { return textRow; }
	const Row &row() const { return textRow; }
	const std::vector<char> &raw() const { return rawBytes; }

	template <class L>
	void load(const char *slot, typename L::Schema value)
	{
		schema = value;
		id = slotfile::ReadU32(slot);
		index = slotfile::ReadU8(slot + 4);
		unknown = slotfile::ReadU8(slot + 5);
		reserved = slotfile::ReadU16(slot + 6);
		rawBytes.assign(slot, slot + L::slotSize);
		textRow.ReadRow(reinterpret_cast<Record *>(const_cast<char *>(slot) + L::TextOffset(schema)),
			static_cast<int>(L::TextCapacity(schema)));
		ValidateRow();
	}

	template <class L>
	void store(char *slot)
	{
		ValidateRow();
		if (static_cast<size_t>(textRow.GetSize()) > L::TextCapacity(schema))
			throw std::runtime_error("Help text exceeds slot capacity");
		slotfile::WriteU32(slot, id);
		slotfile::WriteU8(slot + 4, index);
		slotfile::WriteU8(slot + 5, unknown);
		slotfile::WriteU16(slot + 6, reserved);
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
			throw std::runtime_error("Invalid Japanese or English help text layout");
	}
};

struct Format
{
	using Entry = helpfmt::Entry;
	using Datum = helpfmt::Datum;
	using Schema = helpfmt::Schema;

	static constexpr std::string_view id = "v10";
	static constexpr std::string_view scope = "ROM/332/46 and ROM/332/48 help tables, only observed slot layout";
	static constexpr size_t slotSize = sizeof(Entry);
	static constexpr size_t cipherSpan = slotSize;
	static constexpr size_t currencySlots = 1;
	static constexpr size_t blobLengthOffset = 0;
	static constexpr size_t blobDataOffset = 0;
	static constexpr size_t blobCapacity = 0;
	static constexpr size_t textEnd = offsetof(Entry, terminator);
	static constexpr Schema DEFAULT_SCHEMA = Schema::HELP;
	static constexpr bool IsCurrencySchema(Schema) { return false; }
	static constexpr size_t TextOffset(Schema) { return offsetof(Entry, text); }
	static constexpr size_t TextCapacity(Schema) { return sizeof(Entry::text); }
};

static_assert(slotfile::SlotLayoutInvariants<Format>);
} // namespace helpfmt
