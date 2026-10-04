#pragma once

// SlotFileCsv: the CSV adapter of the slot container.
//
// The container (SlotFile.h) is a byte codec and knows nothing about CSV; the
// layouts (ItemFormat*.h, RoeFormat*.h, MonBridgeFormat*.h) own the column sets
// of their family. This header is the only place where the two meet: it requires
// the CSV hooks of a layout (`SlotCsvLayout`) and dumps a record store through
// them. Nothing here is a member of SlotFile, so the container keeps no CSV
// dependency and the CSV view can be tested, replaced or dropped on its own.
//
// The adapter is deliberately thin: it owns the file, the header row and the
// loop, while the layout owns the columns. A layout that exports CSV must
// declare its hooks in its own header (never in an anonymous namespace), because
// the definitions live in the per family *FormatsCsv.cpp translation units.

#include <cstring>
#include <filesystem>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

#include <xystring.h>

#include "CsvFile.h"
#include "SlotFile.h"

namespace slotfile
{

// A layout whose records can be written as CSV: one file level context, one
// header row and one row per record, all of them from the same column list.
template <class L>
concept SlotCsvLayout = SlotLayout<L> && requires (typename L::Schema schema,
	std::span<const typename L::Datum> records, CsvFile &csv, const typename L::Datum &datum) {
	{ L::GetCsvContext(records) };
	{ L::WriteCsvHeader(csv, L::GetCsvContext(records)) };
	{ L::WriteCsvRow(csv, L::GetCsvContext(records), datum) };
};

// The whole CSV view: a header row followed by one row per record.
//
// The overloads below are the entry point of the adapter: one for a record store
// the caller passes, one for the store a SlotFile already holds. The columns of
// a layout follow from its records (the schema axis is per file), so an empty
// store produces an empty file instead of a header that no row could match.
template <SlotCsvLayout Layout, class Datum>
	requires std::same_as<Datum, typename Layout::Datum>
void WriteICsv(const std::wstring &path, std::span<const Datum> records)
{
	CsvFile csv(path, std::ios::out | std::ios::binary);
	if (records.empty())
	{
		csv.Close();
		return;
	}

	const auto context = Layout::GetCsvContext(records);
	Layout::WriteCsvHeader(csv, context);
	for (const Datum &datum : records)
		Layout::WriteCsvRow(csv, context, datum);
	csv.Close();
}

template <SlotCsvLayout Layout, class Datum>
	requires std::same_as<Datum, typename Layout::Datum>
void WriteICsv(const std::wstring &path, const std::vector<Datum> &records)
{
	WriteICsv<Layout>(path, std::span<const Datum>(records));
}

// One column of a layout: the header text and the value of one record. Keeping
// both in one place is what makes a header and its rows line up by construction.
// A family with one fixed column set for all of its versions derives both from
// one field table; a family whose fields differ per version uses one entry per
// column as well, only in its own header/row pair.

// Cell writers shared by the per family column tables.

inline std::u8string CsvInt(int64_t value)
{
	return xybase::string::to_utf8(std::to_string(value));
}

inline std::u8string CsvBool(bool value)
{
	return value ? u8"1" : u8"0";
}

inline std::u8string CsvEnum(uint64_t value)
{
	return CsvInt(static_cast<int64_t>(value));
}

// Reinterprets one field of a packed record as an unsigned integer of its own
// size, so that a flag block can be read without naming its bit width.
template <class T>
uint64_t CsvBits(T value)
{
	using U = std::make_unsigned_t<T>;
	U raw = 0;
	std::memcpy(&raw, &value, sizeof(raw));
	return static_cast<uint64_t>(raw);
}

} // namespace slotfile
