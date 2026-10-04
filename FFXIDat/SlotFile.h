#pragma once

// SlotFile: the fixed size slot container used by several DAT families, and the
// shared vocabulary of the record format model.
//
// Recognition signal (no annotation needed): the file size is a whole number of
// slots AND every slot ends with 0xFF. Verified on the live install for items
// (5120), RoE quest / category (5120), MonBridge (5120) and StatusData (6144);
// it does not match xis / evsb / evfx payloads.
//
// The model separates three independent axes:
//
//   container  SlotFile<Layout, Cipher>  slot stride, terminator, optional blob
//                                        (length + payload), currency padding,
//                                        strict text validation, iteration
//   cipher     SlotCipher                transforms [0, Layout::cipherSpan) of
//                                        every slot; ror5 for items / RoE /
//                                        MonBridge, a per slot variable ror for
//                                        StatusData (not implemented here)
//   layout     SlotLayout                per family and version structs (Header,
//                                        Specs, Entry), the offsets derived from
//                                        those structs and the datum semantics
//
// The CSV view of the same layouts lives in the adapter layer (SlotFileCsv.h),
// which is also where the CSV hooks of a layout are required: the container must
// not depend on CsvFile.
//
// `RecordFormat.h` enumerates text row cell layouts (RecordFormat::ItemJapanese,
// ...) for debug logging; it is unrelated to the layout types of this model.

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "Record.h"

namespace slotfile
{

// --------------------------------------------------------------------- versions

// Opaque labels of the record versions a family can have, in the order we have
// observed them. They are used for routing only (the data side spells the newest
// known layout as "" and the oldest one as "_o"); they are never compared as
// numbers and they carry no promise about release dates. A family with a single
// known layout only uses the oldest label.
enum class Version
{
	V10, // oldest known layout of the family
	V20, // layout used before the 2026-09 update (some families have no V20)
	V30, // newest known layout
};

// ------------------------------------------------------------------ byte access

// All known layouts are little endian and unaligned.
inline uint8_t ReadU8(const char *bytes) { return static_cast<uint8_t>(bytes[0]); }
inline int8_t ReadI8(const char *bytes) { return static_cast<int8_t>(bytes[0]); }
inline uint16_t ReadU16(const char *bytes) { uint16_t v = 0; std::memcpy(&v, bytes, 2); return v; }
inline int16_t ReadI16(const char *bytes) { int16_t v = 0; std::memcpy(&v, bytes, 2); return v; }
inline uint32_t ReadU32(const char *bytes) { uint32_t v = 0; std::memcpy(&v, bytes, 4); return v; }
inline int32_t ReadI32(const char *bytes) { int32_t v = 0; std::memcpy(&v, bytes, 4); return v; }

inline void WriteU8(char *bytes, uint8_t v) { bytes[0] = static_cast<char>(v); }
inline void WriteI8(char *bytes, int8_t v) { bytes[0] = static_cast<char>(v); }
inline void WriteU16(char *bytes, uint16_t v) { std::memcpy(bytes, &v, 2); }
inline void WriteI16(char *bytes, int16_t v) { std::memcpy(bytes, &v, 2); }
inline void WriteU32(char *bytes, uint32_t v) { std::memcpy(bytes, &v, 4); }
inline void WriteI32(char *bytes, int32_t v) { std::memcpy(bytes, &v, 4); }

inline std::string Hex(uint64_t value)
{
	char buffer[32] = { 0 };
	std::snprintf(buffer, sizeof(buffer), "0x%llX", static_cast<unsigned long long>(value));
	return std::string(buffer);
}

// Path text for diagnostics only: a DAT path that cannot be represented must
// still produce a readable message instead of throwing.
inline std::string DescribePath(const std::wstring &path)
{
	std::string text;
	text.reserve(path.size());
	for (wchar_t c : path)
		text.push_back(c > 0 && c < 0x80 ? static_cast<char>(c) : '?');
	return text;
}

// -------------------------------------------------------------- text validation

// The wire layout of a text Record (Record.h) in bytes. They are named here
// because the validator below walks the bytes instead of the structs.
inline constexpr size_t TEXT_RECORD_HEADER_SIZE = 4;  // Record::cellCount
inline constexpr size_t TEXT_CELL_SPACE_SIZE = 8;      // sizeof(RecordSpec)
inline constexpr size_t STRING_CELL_HEADER_SIZE = 28; // RecordString::one + zero[6]
inline constexpr size_t INT_CELL_SIZE = 4;           // one int32 payload

// Validates the text Record that starts at `textOffset` inside a slot whose text
// area ends at `textEnd`.
//
// The loaders double as file type sniffers, so arbitrary bytes reach them: this
// check must never read past `textEnd` and must throw instead of guessing. It
// therefore walks the cell table by offset and length (`base + offset`, bounded
// by `limit`) instead of dereferencing the record structures, which would read
// out of the text area as soon as a field is malformed.
//
// What makes a record structurally valid, and nothing more: the cell table (the
// count at the head of the record plus one 8 byte RecordSpec per cell) fits into
// the text area, every cell offset is at or behind the end of that table, the
// payload the offset points at (28 header bytes plus the bytes up to and
// including the NUL for a string cell, 4 bytes for an int cell) stays inside the
// text area, every type code is 0 or 1, and a string cell has a NUL inside the
// area. That is the whole contract, so all of the following are accepted on
// purpose:
//
//   - cells that are not back to back, i.e. a hole between two payloads,
//   - holes of any content, nonzero bytes included,
//   - offsets that are neither ascending nor distinct (two cells may point at
//     one shared payload), and any order of the cells after the table.
//
// The rewrite of such a record is therefore not byte identical: it lays the
// cells out canonically (Row::WriteRow pads every string cell to a four byte
// boundary and puts the next cell right behind it). Byte level round trip is
// promised for the canonical records the installed client writes, and for those
// only; see docs/FILE_FORMATS.md.
//
// Everything the container refuses on top of this (stride, end marker, currency
// padding, cell count bounds) lives in ParseInto and ReadRow.
inline void ValidateTextRecord(const char *slot, size_t TextOffset, size_t textEnd)
{
	if (TextOffset > textEnd || textEnd - TextOffset < TEXT_RECORD_HEADER_SIZE)
		throw std::runtime_error("Text record starts past the text area");

	const char *base = slot + TextOffset;
	const size_t limit = textEnd - TextOffset;

	const int32_t cellCount = ReadI32(base);
	if (cellCount <= 0 || static_cast<size_t>(cellCount) >
		(limit - TEXT_RECORD_HEADER_SIZE) / TEXT_CELL_SPACE_SIZE)
		throw std::runtime_error("Invalid text cell count");

	const size_t tableEnd = TEXT_RECORD_HEADER_SIZE +
		static_cast<size_t>(cellCount) * TEXT_CELL_SPACE_SIZE;
	for (int32_t i = 0; i < cellCount; ++i)
	{
		const char *cell = base + TEXT_RECORD_HEADER_SIZE +
			static_cast<size_t>(i) * TEXT_CELL_SPACE_SIZE;
		const int32_t offset = ReadI32(cell);
		const int32_t type = ReadI32(cell + 4);
		if (type != 0 && type != 1)
			throw std::runtime_error("Invalid text cell type");
		if (offset < 0 || static_cast<size_t>(offset) < tableEnd)
			throw std::runtime_error("Invalid text cell offset");

		const size_t cellOffset = static_cast<size_t>(offset);
		if (cellOffset > limit)
			throw std::runtime_error("Text cell exceeds the text area");
		const size_t available = limit - cellOffset;

		if (type == 0)
		{
			// A string cell is a 28 byte header (the marker `one == 1` and six
			// zero words) followed by the NUL terminated text.
			if (available < STRING_CELL_HEADER_SIZE + 1)
				throw std::runtime_error("Text cell exceeds the text area");
			if (!std::memchr(base + cellOffset + STRING_CELL_HEADER_SIZE, 0,
				available - STRING_CELL_HEADER_SIZE))
				throw std::runtime_error("Unterminated text cell");
		}
		else if (available < INT_CELL_SIZE)
			throw std::runtime_error("Text cell exceeds the text area");
	}
}

// --------------------------------------------------------------------- concepts

// One record of that layout, parsed from and written back to raw slot bytes.
//
// `load` reads a decrypted slot, `store` rewrites a slot buffer that the
// container filled with the record's own original bytes, so everything a
// layout does not model keeps its exact original value.
template <class D, class L>
concept SlotDatum = requires (D d, const char *slot, char *out, typename L::Schema schema) {
	{ d.template load<L>(slot, schema) };
	{ d.template store<L>(out) };
	{ d.schema } -> std::convertible_to<typename L::Schema>;
	{ d.row() } -> std::convertible_to<Row &>;
	{ d.raw() } -> std::convertible_to<const std::vector<char> &>;
};

// One layout of one family and version: own structs and the offsets derived from
// those structs. The CSV view of a layout is a separate axis and lives in the
// adapter layer (SlotFileCsv.h, concept SlotCsvLayout).
template <class L>
concept SlotLayout = requires (typename L::Schema schema) {
	typename L::Entry;
	typename L::Datum;
	typename L::Schema;
	{ L::id } -> std::convertible_to<std::string_view>;
	{ L::scope } -> std::convertible_to<std::string_view>;
	{ L::slotSize } -> std::convertible_to<size_t>;
	{ L::cipherSpan } -> std::convertible_to<size_t>;
	{ L::currencySlots } -> std::convertible_to<size_t>;
	{ L::blobLengthOffset } -> std::convertible_to<size_t>;
	{ L::blobDataOffset } -> std::convertible_to<size_t>;
	{ L::blobCapacity } -> std::convertible_to<size_t>;
	{ L::textEnd } -> std::convertible_to<size_t>;
	{ L::DEFAULT_SCHEMA } -> std::convertible_to<typename L::Schema>;
	{ L::IsCurrencySchema(schema) } -> std::convertible_to<bool>;
	{ L::TextOffset(schema) } -> std::convertible_to<size_t>;
	requires SlotDatum<typename L::Datum, L>;
};

// The layout invariants the container relies on. They are constraints of the
// container, so a layout that breaks one fails where the container is
// instantiated; they only pin what the container itself reads, the per family
// structs are pinned by the static_asserts of their own headers.
template <class L>
concept SlotLayoutInvariants = SlotLayout<L> && requires {
	requires L::slotSize == sizeof(typename L::Entry);
	requires (L::cipherSpan > 0);
	requires (L::cipherSpan <= L::slotSize);
	requires (L::currencySlots >= 1);
	requires (L::blobLengthOffset + 4 <= L::slotSize);
	requires (L::blobCapacity == 0 ||
		L::blobDataOffset + L::blobCapacity < L::slotSize);
	requires (L::textEnd < L::slotSize);
};

// Transformation applied to the first Layout::cipherSpan bytes of every slot.
template <class C>
concept SlotCipher = requires (char *slot, size_t span) {
	{ C::decrypt(slot, span) };
	{ C::encrypt(slot, span) };
};

// --------------------------------------------------------------------- ciphers

// Whole slot rotation by a fixed amount. A byte rotation leaves 0xFF a fixed
// point, so slot end markers stay detectable in the encrypted file.
struct Ror5Cipher
{
	static void decrypt(char *slot, size_t span)
	{
		for (size_t i = 0; i < span; ++i)
		{
			const auto v = static_cast<uint8_t>(slot[i]);
			slot[i] = static_cast<char>((v >> 5) | (v << 3));
		}
	}
	static void encrypt(char *slot, size_t span)
	{
		for (size_t i = 0; i < span; ++i)
		{
			const auto v = static_cast<uint8_t>(slot[i]);
			slot[i] = static_cast<char>((v << 5) | (v >> 3));
		}
	}
};

// ------------------------------------------------------------------- container

// The container is a codec over a datum store: it owns `data` when used on its
// own, and the family facades (ItemData, RecordsOfEminence, MonBridge) call the
// span overloads with their own vector so that they keep a single storage.
//
// Both the datum protocol and the layout invariants are constraints of the
// layout, so a layout that does not satisfy them fails at the point where the
// container is instantiated, not somewhere inside it.
template <SlotLayoutInvariants Layout, SlotCipher Cipher = Ror5Cipher>
class SlotFile
{
public:
	using Datum = typename Layout::Datum;
	using Schema = typename Layout::Schema;

	// The layout this store was built for: the CSV adapter entry point needs it,
	// because a CSV view belongs to a layout rather than to a record store.
	using LayoutType = Layout;

	std::vector<Datum> data;
	bool encryptionSuppression = false;

	// ---------------------------------------------------------------- reading

	void Read(const std::wstring &path, Schema schema = Layout::DEFAULT_SCHEMA)
	{
		std::ifstream file(path, std::ios::binary);
		if (!file.is_open())
			throw std::runtime_error("Failed to open file: " + DescribePath(path));

		file.seekg(0, std::ios::end);
		const auto end = file.tellg();
		if (end < 0)
			throw std::runtime_error("Failed to determine slot file size: " + DescribePath(path));
		const size_t fileSize = static_cast<size_t>(end);
		file.seekg(0, std::ios::beg);

		std::vector<char> buffer(fileSize);
		if (fileSize > 0)
		{
			file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
			if (!file)
				throw std::runtime_error("Failed to read the whole slot file: " + DescribePath(path));
		}
		file.close();

		if (!encryptionSuppression)
			Decrypt(buffer);

		std::vector<Datum> records;
		ParseInto(records, buffer, schema);
		data = std::move(records);
	}

	// Parses an already decrypted buffer. Exposed for tests and sniffers.
	void Parse(std::span<const char> buffer, Schema schema = Layout::DEFAULT_SCHEMA)
	{
		std::vector<Datum> records;
		ParseInto(records, buffer, schema);
		data = std::move(records);
	}

	// ---------------------------------------------------------------- writing

	// Assembles the slots. A failed assembly (an oversized translation, for
	// instance) throws before the caller opens any file.
	std::vector<char> Serialize(std::span<Datum> records) const
	{
		const size_t slotSize = Layout::slotSize;
		if constexpr (Layout::currencySlots > 1)
		{
			// A currency file holds exactly one entry; the remaining slots are the
			// zero padding. Writing a different number of entries is a caller bug.
			for (const Datum &datum : records)
				if (Layout::IsCurrencySchema(datum.schema) && records.size() != 1)
					throw std::runtime_error("Currency file must contain exactly 1 entry, got: " +
						std::to_string(records.size()));
		}
		const bool currency = IsCurrency(records);

		std::vector<char> buffer(records.size() * slotSize, 0);
		for (size_t i = 0; i < records.size(); ++i)
		{
			char *slot = buffer.data() + i * slotSize;

			// The record's own original bytes are the base of the rewrite: a
			// datum that keeps them dictates every byte its layout does not
			// model, and `store` only overwrites the modelled fields. A datum
			// without them (a record built from scratch) starts from zeroes.
			if (records[i].raw().size() == slotSize)
				std::memcpy(slot, records[i].raw().data(), slotSize);
			else
				std::memset(slot, 0, slotSize);

			records[i].template store<Layout>(slot);
			slot[slotSize - 1] = static_cast<char>(0xFF);
		}

		if (currency)
			buffer.resize(slotSize * Layout::currencySlots, 0x00);

		if (!encryptionSuppression)
			Encrypt(buffer);

		return buffer;
	}

	void Write(const std::wstring &path) { Write(path, data); }

	void Write(const std::wstring &path, std::span<Datum> records)
	{
		const std::vector<char> buffer = Serialize(records);

		std::ofstream file(path, std::ios::binary);
		if (!file.is_open())
			throw std::runtime_error("Failed to open file for writing: " + DescribePath(path));
		if (!buffer.empty())
			file.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
		if (!file)
			throw std::runtime_error("Failed to write the whole slot file: " + DescribePath(path));
	}

	// The CSV view of the records is not a container operation: it lives in the
	// adapter layer, see SlotFileCsv.h.

private:
	static bool IsCurrency(std::span<const Datum> records)
	{
		if constexpr (Layout::currencySlots <= 1)
			return false;
		else
		{
			if (records.size() != 1)
				return false;
			return Layout::IsCurrencySchema(records[0].schema);
		}
	}

	static void Decrypt(std::span<char> buffer)
	{
		for (size_t offset = 0; offset + Layout::slotSize <= buffer.size(); offset += Layout::slotSize)
			Cipher::decrypt(buffer.data() + offset, Layout::cipherSpan);
	}

	static void Encrypt(std::span<char> buffer)
	{
		for (size_t offset = 0; offset + Layout::slotSize <= buffer.size(); offset += Layout::slotSize)
			Cipher::encrypt(buffer.data() + offset, Layout::cipherSpan);
	}

	static void ParseInto(std::vector<Datum> &records, std::span<const char> buffer, Schema schema)
	{
		const size_t slotSize = Layout::slotSize;
		if (buffer.empty() || buffer.size() % slotSize != 0)
			throw std::runtime_error("Slot file size must be a multiple of " + Hex(slotSize) +
				" bytes, got: " + std::to_string(buffer.size()));

		const size_t slots = buffer.size() / slotSize;
		const bool currency = Layout::currencySlots > 1 && Layout::IsCurrencySchema(schema);
		if (currency && buffer.size() != slotSize * Layout::currencySlots)
			throw std::runtime_error("Currency file size must be exactly " +
				Hex(slotSize * Layout::currencySlots) + " bytes, got: " + std::to_string(buffer.size()));

		const size_t TextOffset = Layout::TextOffset(schema);
		// The text position depends on the schema, so it can only be checked
		// here: a layout whose schema puts its text record past the text area
		// would make the validator below read outside the slot.
		if (TextOffset > Layout::textEnd)
			throw std::runtime_error("Text record offset of this schema is past the text area: " +
				Hex(TextOffset) + " > " + Hex(Layout::textEnd));
		const size_t count = currency ? 1 : slots;

		std::vector<Datum> parsed;
		parsed.reserve(count);
		for (size_t i = 0; i < count; ++i)
		{
			const char *slot = buffer.data() + i * slotSize;
			const uint8_t endMarker = static_cast<uint8_t>(slot[slotSize - 1]);
			if (endMarker != 0xFF)
				throw std::runtime_error("Invalid end marker found, expected 0xFF but got: " +
					std::to_string(static_cast<unsigned>(endMarker)));

			ValidateTextRecord(slot, TextOffset, Layout::textEnd);

			Datum datum;
			datum.template load<Layout>(slot, schema);
			parsed.push_back(std::move(datum));
		}

		if (currency)
		{
			if (parsed.size() != 1)
				throw std::runtime_error("Currency file must contain exactly 1 entry, got: " +
					std::to_string(parsed.size()));
			for (size_t offset = slotSize; offset < buffer.size(); ++offset)
				if (buffer[offset] != 0)
					throw std::runtime_error("Currency file has nonzero padding");
		}

		records = std::move(parsed);
	}
};

} // namespace slotfile
