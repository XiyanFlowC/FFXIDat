#pragma once

// MonBridge layout v30: the 0x1400 byte record of ROM/288/66.
//
// The leading fields were verified against the de/fr tables of the same client:
// idx stays at 4, name moved from 6 to 8 and para2 from 48 to 52 because two new
// 16 bit fields were inserted (both always zero in the observed data).
//
// The family shares one datum type (mbfmt::Datum, defined below): the internal
// name, the parameters, the text record, the icon and the raw slot bytes have the
// same shape in every version, only the offsets differ.

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "Image.h"
#include "Record.h"
#include "SlotFile.h"
#include "CsvFile.h"

namespace mbfmt
{

enum class Schema
{
	BRIDGE,
};

using Version = slotfile::Version;

// The file level context of a CSV export. This family has a single fixed column
// set, so the context carries no column selection.
struct CsvContext
{
};

// ASCII internal identifier <-> the fixed size name field.
std::u8string InternalNameFromBytes(const char *bytes, size_t size);
std::string InternalNameToBytes(const std::u8string &name);

namespace v30
{

#pragma pack(push, 1)

struct Entry
{
	uint32_t id;
	uint16_t idx;
	uint16_t ukn0; // always zero in observed data
	char name[32];
	int16_t para1[5];
	uint16_t ukn1; // always zero in observed data
	int8_t para2[64];

	union {
		char raw[524];
		Record info_rec;
	} rec;

	uint32_t icon_size; // size of icon_data, 0 if no icon
	char icon_data[4475];

	char terminator; // must be 0xFF
};

#pragma pack(pop)

static_assert(sizeof(Entry) == 0x1400, "v30 record size mismatch");
static_assert(offsetof(Entry, rec) == 116, "v30 text record offset mismatch");
static_assert(offsetof(Entry, icon_size) == 640, "v30 icon size offset mismatch");
static_assert(offsetof(Entry, icon_data) == 644, "v30 icon data offset mismatch");

} // namespace v30

// ---------------------------------------------------------------- datum

// One MonBridge record, whatever version it was read with.
class Datum
{
public:
	uint32_t id = 0;
	uint16_t idx = 0;

	// Internal ASCII identifier, from the fixed size name field.
	std::u8string internalName;

	// Display name, from cell 0 of the text record.
	std::u8string displayName;

	Image image;

	// Exact slot bytes as stored in the file, already decrypted.
	std::vector<char> rawBytes;

	// Format::id of the layout that parsed this record.
	std::string_view layoutId;

	// Typed view of the newest known layout. A record read with an older layout
	// mirrors the fields that layout defines and leaves the rest zeroed; the real
	// bytes stay in raw().
	v30::Entry originalEntry;

	Row originalRow;
	bool hasOriginalRow = false;

	Schema schema = Schema::BRIDGE;

	// Convenience accessors for commonly used fields
	uint16_t &index() { return originalEntry.idx; }
	const uint16_t &index() const { return originalEntry.idx; }

	const std::vector<char> &raw() const { return rawBytes; }
	Row &row() { return originalRow; }
	const Row &row() const { return originalRow; }

	template <class L>
	void load(const char *slot, typename L::Schema)
	{
		layoutId = L::id;
		rawBytes.assign(slot, slot + L::slotSize);

		if constexpr (std::is_same_v<typename L::Entry, v30::Entry>)
			std::memcpy(&originalEntry, slot, sizeof(v30::Entry));
		else
		{
			std::memset(&originalEntry, 0, sizeof(v30::Entry));
			originalEntry.terminator = static_cast<char>(0xFF);
		}

		originalEntry.id = slotfile::ReadU32(slot);
		originalEntry.idx = slotfile::ReadU16(slot + L::idxOffset);
		std::memcpy(originalEntry.name, slot + L::nameOffset, sizeof(v30::Entry::name));
		std::memcpy(originalEntry.para1, slot + L::para1Offset, sizeof(v30::Entry::para1));
		std::memcpy(originalEntry.para2, slot + L::para2Offset, sizeof(v30::Entry::para2));
		id = originalEntry.id;
		idx = originalEntry.idx;
		internalName = InternalNameFromBytes(originalEntry.name, sizeof(originalEntry.name));

		originalRow.ReadRow(reinterpret_cast<Record *>(const_cast<char *>(slot) + L::recOffset),
			static_cast<int>(L::recSize));
		hasOriginalRow = true;
		const auto &cells = originalRow.GetCellsConst();
		displayName = (!cells.empty() && cells[0].GetType() == 0)
			? cells[0].Get<std::u8string>() : std::u8string();

		const uint32_t iconSize = slotfile::ReadU32(slot + L::blobLengthOffset);
		if (iconSize > L::blobCapacity)
			throw std::runtime_error("Invalid icon size: " + std::to_string(iconSize) +
				", maximum allowed: " + std::to_string(L::blobCapacity));
		originalEntry.icon_size = iconSize;
		std::memcpy(originalEntry.icon_data, slot + L::blobDataOffset, L::blobCapacity);
		if (iconSize > 0)
		{
			Image parsed;
			try
			{
				parsed.ReadFromMemory(slot + L::blobDataOffset, iconSize);
				image = std::move(parsed);
			}
			catch (const std::exception &)
			{
				// If image reading fails, just leave the image empty.
			}
		}
	}

	template <class L>
	void store(char *slot)
	{
		slotfile::WriteU32(slot, id);
		slotfile::WriteU16(slot + L::idxOffset, idx);

		const std::string name = InternalNameToBytes(internalName);
		std::memset(slot + L::nameOffset, 0, L::nameSize);
		const size_t copy = name.size() < L::nameSize ? name.size() : L::nameSize - 1;
		std::memcpy(slot + L::nameOffset, name.data(), copy);

		if (!displayName.empty())
		{
			Row row;
			row.GetCells().emplace_back(displayName);
			if (static_cast<size_t>(row.GetSize()) > L::recSize)
				throw std::runtime_error("Record size exceeds maximum allowed size");
			row.WriteRow(reinterpret_cast<Record *>(slot + L::recOffset), static_cast<int>(L::recSize));
		}

		uint32_t iconSize = slotfile::ReadU32(slot + L::blobLengthOffset);
		if (image.texture)
		{
			size_t size = L::blobCapacity;
			image.WriteToMemory(slot + L::blobDataOffset, size);
			if (size > L::blobCapacity)
				throw std::runtime_error("Image size exceeds maximum allowed size");
			iconSize = static_cast<uint32_t>(size);
		}
		slotfile::WriteU32(slot + L::blobLengthOffset, iconSize);
		originalEntry.icon_size = iconSize;
	}

	Datum()
	{
		std::memset(&originalEntry, 0, sizeof(originalEntry));
		originalEntry.terminator = static_cast<char>(0xFF);
	}
};

namespace v30
{

struct Format
{
	using Entry = v30::Entry;
	using Datum = mbfmt::Datum;
	using Schema = mbfmt::Schema;

	static constexpr Version version = Version::V30;
	static constexpr std::string_view id = "v30";
	static constexpr std::string_view scope = "ja/en MonBridge tables, 2026-09 update onwards";

	static constexpr size_t slotSize = sizeof(Entry);
	static constexpr size_t cipherSpan = sizeof(Entry);
	static constexpr size_t currencySlots = 1; // no currency form in this family
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

static_assert(slotfile::SlotDatum<mbfmt::Datum, Format>, "v30 datum does not match the container protocol");
} // namespace v30
} // namespace mbfmt
