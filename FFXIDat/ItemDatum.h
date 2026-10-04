#pragma once

// The item datum: the record of one slot, owned by one layout version.
//
// Every version of the family keeps its own typed view: `DatumBase<EntryT>` is
// parameterised by that version's Entry, and `load` makes the typed view a plain
// memory image of the slot, so a record read with v10 / v20 / v30 is typed as
// that version. There is no family level Datum and no version test anywhere in
// this file: the layout template parameter of `load` / `store` supplies the
// offsets, the text position, the blob position and the capacity.
//
// The datum owns both the exact slot bytes (`rawBytes`) and the semantic view
// (header, text row, image). The container starts every rewrite from `raw()`, so
// every byte the layout does not model keeps its original value and a rewrite is
// byte identical; `store` only overwrites the modelled fields.
//
// The structs of the three versions name the same members (header, spec,
// image_length, image_data, end_marker) and the header does too (id, the two
// flag bytes, stack_size, item_type, resource_id, valid_targets), so the shared
// code below needs no CRTP and no candidate names.

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Image.h"
#include "Record.h"
#include "RecordFormat.h"
#include "SlotFile.h"
#include "ItemSpecType.h"

namespace itmfmt
{

// The file level context of a CSV export: which kind of record the file holds
// and whether its text rows use the English cell layout. The column set of a
// layout follows from it.
struct CsvContext
{
	SpecType schema = SpecType::NORMAL;
	bool english = false;
};

// Compresses an image to the byte form stored in the tail blob of a slot.
inline std::vector<char> ImageBytes(const Image &image, size_t capacity)
{
	std::vector<char> bytes(capacity);
	size_t size = capacity;
	image.WriteToMemory(bytes.data(), size);
	if (size > capacity)
		throw std::runtime_error("Item image exceeds record capacity");
	bytes.resize(size);
	return bytes;
}

template <class EntryT>
class DatumBase
{
public:
	// Typed view of the slot, owned by this version: the whole record, as the
	// layout of this version defines it.
	EntryT originalEntry{};

	uint32_t id = 0;
	Image image;

	// Exact slot bytes as stored in the file, already decrypted.
	std::vector<char> rawBytes;

	// Format::id of the layout that parsed this record ("v10" / "v20" / "v30").
	std::string_view layoutId;

	// Store the original Row structure - THIS IS THE SOURCE OF TRUTH for text fields
	// Row doesn't know what it contains - Datum is responsible for interpreting it
	Row originalRow;
	bool hasOriginalRow = false;

	// Optional: for debugging/logging only, not used in core logic
	RecordFormat recordFormat = RecordFormat::Unknown;

	// Store the spec type for this item
	SpecType schema = SpecType::NORMAL;

	// The container protocol asks for `raw()`; the member keeps the longer name
	// because `raw` is already the name of the untranslated spec area of Entry.
	const std::vector<char> &raw() const { return rawBytes; }

	// ============ Reading and writing one slot ============

	// Parses a decrypted slot of the layout L. The slot must have been checked
	// by the container (end marker and text table).
	template <class L>
	void load(const char *slot, typename L::Schema schema)
	{
		layoutId = L::id;
		this->schema = schema;
		rawBytes.assign(slot, slot + L::slotSize);

		// The typed view is the memory image of this slot: every field the
		// layout of this version models stays readable, including the spec area.
		std::memcpy(&originalEntry, slot, sizeof(EntryT));
		id = originalEntry.header.id;

		// Text table: the container validated it, so this cannot read out of the slot.
		const size_t TextOffset = L::TextOffset(schema);
		originalRow.ReadRow(reinterpret_cast<Record *>(const_cast<char *>(slot) + TextOffset),
			static_cast<int>(L::textEnd - TextOffset));
		hasOriginalRow = true;
		detectFormat();

		// Tail blob (item image). A layout without a blob must have a zero length.
		const uint32_t imageLength = slotfile::ReadU32(slot + L::blobLengthOffset);
		if (imageLength > L::blobCapacity)
			throw std::runtime_error("Invalid image length: " + std::to_string(imageLength) +
				", maximum allowed: " + std::to_string(L::blobCapacity));
		image = Image();
		if (imageLength > 0)
		{
			Image parsed;
			parsed.ReadFromMemory(slot + L::blobDataOffset, imageLength);
			image = std::move(parsed);
		}
	}

	// Rewrites the modelled fields of a slot buffer. The container filled the
	// buffer with this record's own original bytes (or with zeroes when the
	// record has none), so every byte this layout does not model stays as it
	// was; `store` must not reset the buffer. Throws before the caller opens
	// any output file.
	//
	// The text row is written back canonically (Row::WriteRow puts every string
	// cell on its four byte boundary and the next cell right behind it), so byte
	// level round trip is guaranteed for the canonical records the installed
	// client writes and for those only: a non canonical layout the parser accepts
	// (discontinuous cells, a shared cell offset, a nonzero hole between two
	// payloads) is rearranged here, and its byte for byte losslessness is out of
	// scope. See the contract of slotfile::ValidateTextRecord and
	// docs/FILE_FORMATS.md.
	template <class L>
	void store(char *slot)
	{
		originalEntry.header.id = id;
		std::memcpy(slot, &originalEntry.header, 6);
		if constexpr (L::hasExtendedFlags)
			slotfile::WriteU16(slot + L::headerExtendedFlagsOffset, originalEntry.header.extended_flags);
		slotfile::WriteU16(slot + L::headerStackSizeOffset, originalEntry.header.stack_size);
		slotfile::WriteU16(slot + L::headerItemTypeOffset, originalEntry.header.item_type);
		slotfile::WriteU16(slot + L::headerResourceIdOffset, originalEntry.header.resource_id);
		slotfile::WriteU16(slot + L::headerValidTargetsOffset, originalEntry.header.valid_targets);

		const size_t TextOffset = L::TextOffset(schema);
		const size_t TextCapacity = L::textEnd - TextOffset;
		if (hasOriginalRow)
		{
			if (static_cast<size_t>(originalRow.GetSize()) > TextCapacity)
				throw std::runtime_error("Item text exceeds record capacity for id=" + std::to_string(id));
			originalRow.WriteRow(reinterpret_cast<Record *>(slot + TextOffset),
				static_cast<int>(TextCapacity));
		}

		uint32_t imageLength = slotfile::ReadU32(slot + L::blobLengthOffset);
		if (image.texture)
		{
			const auto bytes = ImageBytes(image, L::blobCapacity);
			std::memcpy(slot + L::blobDataOffset, bytes.data(), bytes.size());
			imageLength = static_cast<uint32_t>(bytes.size());
		}
		slotfile::WriteU32(slot + L::blobLengthOffset, imageLength);
		originalEntry.image_length = imageLength;
	}

	// ============ Text Field Accessors ============

	// Get primary item name (Cell 0)
	// Throws: std::out_of_range if cell doesn't exist
	//         std::runtime_error if cell is not string type
	std::u8string name() const {
		if (!hasOriginalRow) {
			throw std::runtime_error("No original row data");
		}
		
		const auto& cells = originalRow.GetCellsConst();
		if (cells.empty()) {
			throw std::out_of_range("Cell 0 does not exist");
		}
		
		if (cells[0].GetType() != 0) {
			throw std::runtime_error("Cell 0 is not a string");
		}
		
		return cells[0].Get<std::u8string>();
	}
	
	// Set primary item name (Cell 0)
	// Returns: true if successful, false if cell doesn't exist
	bool setName(const std::u8string& newName) {
		if (!hasOriginalRow) return false;
		
		auto& cells = originalRow.GetCells();
		if (cells.empty()) {
			// Allow creating cell 0 for new items
			cells.emplace_back(newName);
			return true;
		}
		
		cells[0].Set(newName);
		return true;
	}
	
	// Get singular form (Cell 2, English only)
	// Throws: std::out_of_range if cell doesn't exist
	std::u8string name_sg() const {
		if (!hasOriginalRow) {
			throw std::runtime_error("No original row data");
		}
		
		const auto& cells = originalRow.GetCellsConst();

		if (cells.size() >= 9) {
			if (cells[4].GetType() == 0)
				return cells[4].Get<std::u8string>();
			throw std::runtime_error("Cell 4 is not a string - de");
		}

		if (cells.size() >= 6) {
			if (cells[3].GetType() == 0)
				return cells[3].Get<std::u8string>();
			throw std::runtime_error("Cell 3 is not a string - fr");
		}

		if (cells.size() < 3) {
			throw std::out_of_range("Cell 2 (singular form) does not exist");
		}
		
		if (cells[2].GetType() != 0) {
			throw std::runtime_error("Cell 2 is not a string");
		}
		
		return cells[2].Get<std::u8string>();
	}
	
	// Set singular form (Cell 2, English only)
	// Returns: true if successful, false if cell doesn't exist
	bool setName_sg(const std::u8string& newName) {
		if (!hasOriginalRow) return false;
		
		auto& cells = originalRow.GetCells();

		// de format
		if (cells.size() >= 9) {
			cells[4].Set(newName);
			return true;
		}

		// fr format
		if (cells.size() >= 6) {
			cells[3].Set(newName);
			return true;
		}

		if (cells.size() < 3) return false;  // Not English format
		
		cells[2].Set(newName);
		return true;
	}
	
	// Get plural form (Cell 3, English only)
	std::u8string name_pl() const {
		if (!hasOriginalRow) {
			throw std::runtime_error("No original row data");
		}
		
		const auto& cells = originalRow.GetCellsConst();

		if (cells.size() >= 9) {
			if (cells[7].GetType() == 0)
				return cells[7].Get<std::u8string>();
			throw std::runtime_error("Cell 7 is not a string - de");
		}

		// fr format
		if (cells.size() >= 6) {
			if (cells[4].GetType() == 0)
				return cells[4].Get<std::u8string>();
			throw std::runtime_error("Cell 4 is not a string - fr");
		}

		if (cells.size() < 4) {
			throw std::out_of_range("Cell 3 (plural form) does not exist");
		}
		
		if (cells[3].GetType() != 0) {
			throw std::runtime_error("Cell 3 is not a string");
		}
		
		return cells[3].Get<std::u8string>();
	}
	
	// Set plural form (Cell 3, English only)
	bool setName_pl(const std::u8string& newName) {
		if (!hasOriginalRow) return false;
		
		auto& cells = originalRow.GetCells();

		// try DE
		if (cells.size() >= 9)
		{
			cells[7].Set(newName);
			return true;
		}

		// FR
		if (cells.size() >= 6)
		{
			cells[4].Set(newName);
			return true;
		}

		if (cells.size() < 4) return false;  // Not English format
		
		cells[3].Set(newName);
		return true;
	}
	
	// Get description (auto-detect Japanese/English format)
	// Japanese: Cell 1, English: Cell 4
	std::u8string description() const {
		if (!hasOriginalRow) {
			throw std::runtime_error("No original row data");
		}
		
		const auto& cells = originalRow.GetCellsConst();

		if (cells.size() >= 9 && cells[8].GetType() == 0) {
			return cells[8].Get<std::u8string>();
		}

		// try French format (cell 5)
		if (cells.size() >= 6 && cells[5].GetType() == 0) {
			return cells[5].Get<std::u8string>();
		}
		
		// Try English format first (cell 4)
		if (cells.size() >= 5 && cells[4].GetType() == 0) {
			return cells[4].Get<std::u8string>();
		}
		
		// Try Japanese format (cell 1)
		if (cells.size() >= 2 && cells[1].GetType() == 0) {
			return cells[1].Get<std::u8string>();
		}
		
		throw std::out_of_range("Description cell not found");
	}
	
	// Set description (auto-detect format)
	// Returns: true if successful, false if appropriate cell doesn't exist
	bool setDescription(const std::u8string& newDesc) {
		if (!hasOriginalRow) return false;
		
		auto& cells = originalRow.GetCells();

		// de format
		if (cells.size() >= 9) {
			cells[8].Set(newDesc);
			return true;
		}

		// fr format
		if (cells.size() >= 6) {
			cells[5].Set(newDesc);
			return true;
		}
		
		// Try English format first (cell 4)
		if (cells.size() >= 5) {
			cells[4].Set(newDesc);
			return true;
		}
		
		// Try Japanese format (cell 1)
		if (cells.size() >= 2) {
			cells[1].Set(newDesc);
			return true;
		}
		
		return false;
	}
	
	// Get log flag (Cell 1, English only, integer)
	int logFlag() const {
		if (!hasOriginalRow) {
			throw std::runtime_error("No original row data");
		}
		
		const auto& cells = originalRow.GetCellsConst();
		if (cells.size() < 2) {
			throw std::out_of_range("Cell 1 (log flag) does not exist");
		}
		
		if (cells[1].GetType() != 1) {
			throw std::runtime_error("Cell 1 is not an integer");
		}
		
		return cells[1].Get<int>();
	}
	
	// Set log flag (Cell 1, English only)
	bool setLogFlag(int flag) {
		if (!hasOriginalRow) return false;
		
		auto& cells = originalRow.GetCells();
		if (cells.size() < 2) return false;
		if (cells[1].GetType() != 1) return false;  // Not an int cell
		
		cells[1].Set(flag);
		return true;
	}
	
	// ============ Direct Row Access (for advanced users/debugging) ============
	
	Row& row() { return originalRow; }
	const Row& row() const { return originalRow; }
	
	size_t cellCount() const {
		return hasOriginalRow ? originalRow.GetCellsConst().size() : 0;
	}
	
	const Cell& cell(size_t index) const {
		if (!hasOriginalRow) {
			throw std::runtime_error("No original row data");
		}
		if (index >= originalRow.GetCellsConst().size()) {
			throw std::out_of_range("Cell index out of range");
		}
		return originalRow.GetCellsConst()[index];
	}
	
	// ============ Format Detection (optional, for debugging) ============
	
	void detectFormat() {
		if (!hasOriginalRow) {
			recordFormat = RecordFormat::Unknown;
			return;
		}
		
		const auto& cells = originalRow.GetCells();
		size_t count = cells.size();
		
		// Japanese format: 2 cells, both strings
		if (count == 2 && cells[0].GetType() == 0 && cells[1].GetType() == 0) {
			recordFormat = RecordFormat::ItemJapanese;
			return;
		}
		
		// English format: 5+ cells, specific pattern
		if (count >= 5 && 
			cells[0].GetType() == 0 &&  // name (string)
			cells[1].GetType() == 1 &&  // logFlag (int)
			cells[2].GetType() == 0 &&  // singular (string)
			cells[3].GetType() == 0 &&  // plural (string)
			cells[4].GetType() == 0) {  // description (string)
			recordFormat = RecordFormat::ItemEnglish;
			return;
		}
		
		recordFormat = RecordFormat::Unknown;
	}
	
	// ============ Header Accessors ============
	
	uint16_t& stack_size() { return originalEntry.header.stack_size; }
	const uint16_t& stack_size() const { return originalEntry.header.stack_size; }
	uint16_t& item_type() { return originalEntry.header.item_type; }
	const uint16_t& item_type() const { return originalEntry.header.item_type; }
	uint16_t& resource_id() { return originalEntry.header.resource_id; }
	const uint16_t& resource_id() const { return originalEntry.header.resource_id; }
	uint16_t& valid_targets() { return originalEntry.header.valid_targets; }
	const uint16_t& valid_targets() const { return originalEntry.header.valid_targets; }
	
	// The flag block of the header of this version.
	auto& flags() { return originalEntry.header; }
	const auto& flags() const { return originalEntry.header; }
};

} // namespace itmfmt
