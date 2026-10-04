#pragma once

// MonBridge layout v10: the 0xC00 byte record still used by the de/fr tables of
// the live client. Compared with v30 it lacks the two 16 bit fields that were
// inserted at 6 and 50, so name / para1 / para2 keep the old offsets and the
// text record and the icon area are one and two bytes shorter.

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "MonBridgeFormatV30.h"

namespace mbfmt
{
namespace v10
{

#pragma pack(push, 1)

struct Entry
{
	uint32_t id;
	uint16_t idx;
	char name[32];
	int16_t para1[5];
	int8_t para2[64];

	union {
		char raw[528];
		Record info_rec;
	} rec;

	uint32_t icon_size;
	char icon_data[2427];

	char terminator; // must be 0xFF
};

#pragma pack(pop)

static_assert(sizeof(Entry) == 0xC00, "v10 record size mismatch");
static_assert(offsetof(Entry, rec) == 112, "v10 text record offset mismatch");
static_assert(offsetof(Entry, icon_size) == 640, "v10 icon size offset mismatch");
static_assert(offsetof(Entry, icon_data) == 644, "v10 icon data offset mismatch");

struct Format
{
	using Entry = v10::Entry;
	using Datum = mbfmt::Datum;
	using Schema = mbfmt::Schema;
	using CsvContext = mbfmt::CsvContext;

	static constexpr Version version = Version::V10;
	static constexpr std::string_view id = "v10";
	static constexpr std::string_view scope = "de/fr MonBridge tables, oldest known layout";

	static constexpr size_t slotSize = sizeof(Entry);
	static constexpr size_t cipherSpan = sizeof(Entry);
	static constexpr size_t currencySlots = 1;
	static constexpr size_t blobLengthOffset = offsetof(Entry, icon_size);
	static constexpr size_t blobDataOffset = offsetof(Entry, icon_data);
	static constexpr size_t blobCapacity = sizeof(Entry::icon_data);
	static constexpr size_t textEnd = offsetof(Entry, icon_size);
	static constexpr Schema DEFAULT_SCHEMA = Schema::BRIDGE;

	static constexpr size_t idxOffset = offsetof(Entry, idx);
	static constexpr size_t nameOffset = offsetof(Entry, name);
	static constexpr size_t nameSize = sizeof(Entry::name);
	static constexpr size_t para1Offset = offsetof(Entry, para1);
	static constexpr size_t para2Offset = offsetof(Entry, para2);
	static constexpr size_t recOffset = offsetof(Entry, rec);
	static constexpr size_t recSize = sizeof(Entry::rec.raw);

	static constexpr bool IsCurrencySchema(Schema) { return false; }
	static constexpr size_t TextOffset(Schema) { return offsetof(Entry, rec); }
	static constexpr size_t TextCapacity(Schema schema) { return textEnd - TextOffset(schema); }

	static CsvContext GetCsvContext(std::span<const Datum> records);
	static void WriteCsvHeader(CsvFile &csv, const CsvContext &context);
	static void WriteCsvRow(CsvFile &csv, const CsvContext &context, const Datum &datum);
};

static_assert(Format::TextOffset(Schema::BRIDGE) == 112, "v10 text offset mismatch");
static_assert(slotfile::SlotDatum<mbfmt::Datum, Format>, "v10 datum does not match the container protocol");

} // namespace v10

// The ja/en tables before the 2026-09 update used the same record as the de/fr
// tables of that era: this family has two labels for one layout.
namespace v20 = v10;
static_assert(std::is_same_v<v20::Format, v10::Format>, "v20 must alias v10 in this family");

} // namespace mbfmt
