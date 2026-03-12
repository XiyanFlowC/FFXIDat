#include "ItemData.h"
#include "Image.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <cstdio>
#include "xystring.h"

// ============================================================================
// Currency File Special Handling
// ============================================================================
// Currency files (ItemSpecType::CURRENCY) have unique characteristics:
// 
// 1. FIXED SIZE: Always exactly 0xC000 (49152) bytes
// 2. SINGLE ENTRY: Contains exactly 1 ItemEntry
// 3. ZERO PADDING: Remaining space after the entry is filled with 0x00
//
// Read behavior:
//   - Validates file size is exactly 0xC000 bytes
//   - Reads only the first ItemEntry
//   - Ignores zero-padding
//   - Asserts exactly 1 entry was read
//
// Write behavior:
//   - Validates data contains exactly 1 entry
//   - Writes the single ItemEntry
//   - Pads with 0x00 to reach 0xC000 bytes
//   - Verifies final file size is exactly 0xC000 bytes
//
// These assertions ensure data integrity and prevent corruption.
// ============================================================================

// Helper function to perform byte-wise right rotation by specified bits
inline uint8_t ror_byte(uint8_t value, int bits) {
	bits &= 7; // Ensure bits is in range 0-7 for byte rotation
	return (value >> bits) | (value << (8 - bits));
}

// Helper function to perform byte-wise left rotation by specified bits
inline uint8_t rol_byte(uint8_t value, int bits) {
	bits &= 7; // Ensure bits is in range 0-7 for byte rotation
	return (value << bits) | (value >> (8 - bits));
}

// Helper function to decrypt a buffer using byte-wise right rotation by 5
void decryptRor5(char* buffer, size_t size) {
	for (size_t i = 0; i < size; ++i) {
		uint8_t byte = static_cast<uint8_t>(buffer[i]);
		// Perform byte-wise right rotation by 5 bits
		buffer[i] = static_cast<char>(ror_byte(byte, 5));
	}
}

// Helper function to encrypt a buffer using byte-wise left rotation by 5 (inverse of ROR 5)
void encryptRol5(char* buffer, size_t size) {
	for (size_t i = 0; i < size; ++i) {
		uint8_t byte = static_cast<uint8_t>(buffer[i]);
		// Perform byte-wise left rotation by 5 bits (inverse of ROR 5)
		buffer[i] = static_cast<char>(rol_byte(byte, 5));
	}
}

// Helper function to decrypt uint32_t using byte-wise ROR 5
uint32_t decryptUint32Ror5(uint32_t value) {
	uint8_t* bytes = reinterpret_cast<uint8_t*>(&value);
	for (int i = 0; i < 4; ++i) {
		bytes[i] = ror_byte(bytes[i], 5);
	}
	return value;
}

// Helper function to encrypt uint32_t using byte-wise ROL 5 (inverse of ROR 5)
uint32_t encryptUint32Rol5(uint32_t value) {
	uint8_t* bytes = reinterpret_cast<uint8_t*>(&value);
	for (int i = 0; i < 4; ++i) {
		bytes[i] = rol_byte(bytes[i], 5);
	}
	return value;
}

void ItemData::Read(std::wstring path, ItemSpecType defaultSpecType)
{
	// 每次读取一块直到结束
	data.clear();
	
	std::ifstream file(path, std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file: " + xybase::string::sys_wcs_to_mbs(path));
	}

	// Read entire file and decrypt it with ROR 5
	file.seekg(0, std::ios::end);
	size_t fileSize = file.tellg();
	file.seekg(0, std::ios::beg);
	
	// Currency files have a fixed size of 0xC000 bytes
	const size_t CURRENCY_FILE_SIZE = 0xC000;
	if (defaultSpecType == ItemSpecType::CURRENCY) {
		if (fileSize != CURRENCY_FILE_SIZE) {
			throw std::runtime_error("Currency file size must be exactly 0xC000 bytes, got: " + 
								   std::to_string(fileSize));
		}
	}
	
	std::unique_ptr<char[]> fileBuffer(new char[fileSize]);
	file.read(fileBuffer.get(), fileSize);
	file.close();
	
	// Decrypt the entire file using ROR 5
	if (!encryptionSuppression)
		decryptRor5(fileBuffer.get(), fileSize);
	
	// Process entries from decrypted buffer
	size_t offset = 0;
	
	// Currency files: only read the first entry, rest is zero-padding
	size_t maxEntries = (defaultSpecType == ItemSpecType::CURRENCY) ? 1 : fileSize / sizeof(ItemEntry);
	size_t entryCount = 0;
	
	while (offset < fileSize && entryCount < maxEntries) {
		ItemEntry entry;
		
		// Check if we have enough bytes for a complete entry
		if (offset + sizeof(ItemEntry) > fileSize) {
			break; // End of file or incomplete entry
		}
		
		// Copy entry from buffer
		memcpy(&entry, fileBuffer.get() + offset, sizeof(ItemEntry));
		
		// Verify the end marker (should be 0xFF)
		if (entry.end_marker != 0xFF) {
			throw std::runtime_error("Invalid end marker found, expected 0xFF but got: " + 
								   std::to_string(static_cast<unsigned>(entry.end_marker)));
		}
		
		// Create an ItemDatum and store the complete original entry
		ItemDatum datum;
		datum.originalEntry = entry;  // Preserve ALL original data including unknown fields
		datum.id = entry.header.id;
		datum.spec_type = defaultSpecType;
		
		// Parse the record data to extract name and description
		// Use the specified spec type to choose the appropriate record
		Record* rec = nullptr;
		switch (defaultSpecType) {
			case ItemSpecType::WEAPON:
				rec = &entry.spec.weapon.info_rec;
				break;
			case ItemSpecType::ARMOUR:
				rec = &entry.spec.armour.info_rec;
				break;
			case ItemSpecType::USABLE:
				rec = &entry.spec.usable.info_rec;
				break;
			case ItemSpecType::NORMAL:
				rec = &entry.spec.normal.info_rec;
				break;
			case ItemSpecType::PUPPET:
				rec = &entry.spec.puppet.info_rec;
				break;
			case ItemSpecType::SLIP:
				rec = &entry.spec.slip.info_rec;
				break;
			case ItemSpecType::CURRENCY:
				rec = &entry.spec.currency.info_rec;
				break;
		}
		
		if (rec && rec->cellCount > 0) {
			// Read and save the complete original Row
			datum.originalRow.ReadRow(rec, sizeof(entry.spec.raw));
			datum.hasOriginalRow = true;
			
			// Auto-detect format (optional, for debugging)
			datum.detectFormat();
			
			// That's it! No manual field extraction needed.
			// Users access via datum.name(), datum.description(), etc.
		}
		
		// Process image data if present
		if (entry.image_length > 0) {
			// Validate image length against the actual image_data array size
			if (entry.image_length > sizeof(entry.image_data)) {
				throw std::runtime_error("Invalid image length: " + std::to_string(entry.image_length) + 
									   ", maximum allowed: " + std::to_string(sizeof(entry.image_data)));
			}

			Image img;
			img.ReadFromMemory(entry.image_data, entry.image_length);
			datum.image = std::move(img);
		}
		
		data.push_back(std::move(datum));
		offset += sizeof(ItemEntry);
		entryCount++;
		
		// Currency files: break after first entry
		if (defaultSpecType == ItemSpecType::CURRENCY && entryCount >= 1) {
			break;
		}
	}
	
	// Currency files: verify we read exactly 1 entry
	if (defaultSpecType == ItemSpecType::CURRENCY) {
		if (data.size() != 1) {
			throw std::runtime_error("Currency file must contain exactly 1 entry, got: " + 
								   std::to_string(data.size()));
		}
	}
}

void ItemData::Write(std::wstring path)
{
	std::ofstream file(path, std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file for writing: " + xybase::string::sys_wcs_to_mbs(path));
	}
	
	// Currency files: must have exactly 1 entry
	bool isCurrency = !data.empty() && data[0].spec_type == ItemSpecType::CURRENCY;
	if (isCurrency && data.size() != 1) {
		throw std::runtime_error("Currency file must contain exactly 1 entry, got: " + 
							   std::to_string(data.size()));
	}

	// Prepare buffer for all entries
	std::vector<char> buffer;
	
	for (ItemDatum &datum : data) {  // 注意：需要非 const 引用，因为要 WriteRow
		// Use the preserved original entry as the base, which maintains all unknown fields
		ItemEntry entry = datum.originalEntry;
		
		// Update the ID field that might have been modified
		entry.header.id = datum.id;
		
		// Use the stored spec type to write the appropriate record
		Record* rec = nullptr;
		switch (datum.spec_type) {
			case ItemSpecType::WEAPON:
				rec = &entry.spec.weapon.info_rec;
				break;
			case ItemSpecType::ARMOUR:
				rec = &entry.spec.armour.info_rec;
				break;
			case ItemSpecType::USABLE:
				rec = &entry.spec.usable.info_rec;
				break;
			case ItemSpecType::NORMAL:
				rec = &entry.spec.normal.info_rec;
				break;
			case ItemSpecType::PUPPET:
				rec = &entry.spec.puppet.info_rec;
				break;
			case ItemSpecType::SLIP:
				rec = &entry.spec.slip.info_rec;
				break;
			case ItemSpecType::CURRENCY:
				rec = &entry.spec.currency.info_rec;
				break;
		}
		
		if (rec && datum.hasOriginalRow) {
			// Simply write the Row as-is (it's already updated via setters)
			datum.originalRow.WriteRow(rec, sizeof(entry.spec.raw));
		}
		
		// Handle image data if present
		if (datum.image.texture) {
			size_t imgSize = sizeof(entry.image_data);
			datum.image.WriteToMemory(entry.image_data, imgSize);
			if (imgSize > sizeof(entry.image_data)) {
				throw std::runtime_error("Image size exceeds maximum allowed size");
			}
			entry.image_length = static_cast<uint32_t>(imgSize);
		} else {
			entry.image_length = datum.originalEntry.image_length;  // Keep original if no image changes
		}
		
		entry.end_marker = 0xFF; // Ensure end marker is correct
		
		// Add entry to buffer
		size_t oldSize = buffer.size();
		buffer.resize(oldSize + sizeof(ItemEntry));
		memcpy(buffer.data() + oldSize, &entry, sizeof(ItemEntry));
	}
	
	// Currency files: pad to exactly 0xC000 bytes with zeros
	const size_t CURRENCY_FILE_SIZE = 0xC000;
	if (isCurrency) {
		if (buffer.size() > CURRENCY_FILE_SIZE) {
			throw std::runtime_error("Currency entry size exceeds maximum: " + 
								   std::to_string(buffer.size()) + " > " + std::to_string(CURRENCY_FILE_SIZE));
		}
		
		// Pad with zeros to reach exactly 0xC000 bytes
		size_t paddingSize = CURRENCY_FILE_SIZE - buffer.size();
		buffer.resize(CURRENCY_FILE_SIZE, 0x00);
	}
	
	// Encrypt the entire buffer using ROL 5 before writing
	if (!encryptionSuppression)
		encryptRol5(buffer.data(), buffer.size());
	
	// Write the encrypted buffer to file
	file.write(buffer.data(), buffer.size());
	
	if (!file) {
		throw std::runtime_error("Failed to write data to file");
	}
	
	// Currency files: verify written size
	if (isCurrency) {
		file.seekp(0, std::ios::end);
		size_t writtenSize = file.tellp();
		if (writtenSize != CURRENCY_FILE_SIZE) {
			throw std::runtime_error("Currency file write failed: expected " + 
								   std::to_string(CURRENCY_FILE_SIZE) + " bytes, wrote " + 
								   std::to_string(writtenSize) + " bytes");
		}
	}
	
	file.close();
}

#include "CsvFile.h"

void ItemData::ToICsv(const std::wstring& path) const
{
	CsvFile csv(path, std::ios::out | std::ios::binary);

	if (data.empty()) {
		csv.Close();
		return; // No data to write
	}

	const auto& firstDatum = data.front();
	const bool isEnglish = std::any_of(data.begin(), data.end(), [](const ItemDatum& datum) {
		if (!datum.hasOriginalRow) return false;
		const auto& cells = datum.originalRow.GetCellsConst();
		return cells.size() >= 5 && cells[0].GetType() == 0 && cells[1].GetType() == 1;
	});

	auto toU8 = [](auto v) {
		return xybase::string::to_utf8(std::to_string(v));
	};
	auto boolToU8 = [](bool v) {
		return v ? u8"1" : u8"0";
	};
	auto specTypeToU8 = [](ItemSpecType specType) {
		switch (specType) {
		case ItemSpecType::WEAPON: return std::u8string(u8"WEAPON");
		case ItemSpecType::ARMOUR: return std::u8string(u8"ARMOUR");
		case ItemSpecType::USABLE: return std::u8string(u8"USABLE");
		case ItemSpecType::PUPPET: return std::u8string(u8"PUPPET");
		case ItemSpecType::SLIP: return std::u8string(u8"SLIP");
		case ItemSpecType::CURRENCY: return std::u8string(u8"CURRENCY");
		case ItemSpecType::NORMAL:
		default: return std::u8string(u8"NORMAL");
		}
	};
	auto flagsToDescription = [](const ItemHeader& header) {
		std::stringstream ss;
		if (header.is_ex) ss << "Ex ";
		if (header.is_rare) ss << "Rare ";
		if (header.is_alt) ss << "Alt ";
		if (header.is_wall_decoration) ss << "WallDecoration ";
		if (header.is_gm_item) ss << "GMItem ";
		if (header.is_in_mystery_box) ss << "MysteryBox ";
		if (header.is_inscribable) ss << "Inscribable ";
		if (header.is_not_listable) ss << "NotListable ";
		if (header.is_scroll) ss << "Scroll ";
		if (header.is_linkshell) ss << "Linkshell ";
		if (header.is_usable) ss << "Usable ";
		if (header.is_npc_tradeable) ss << "NPCTradeable ";
		if (header.is_equipment) ss << "Equipment ";
		if (header.is_unsellable) ss << "Unsellable ";
		if (header.is_unmailable) ss << "Unmailable ";
		return xybase::string::to_utf8(ss.str());
	};
	auto joinU8 = [](const std::vector<const char8_t*>& parts) {
		std::u8string result;
		for (auto part : parts) {
			if (!part || *part == 0) continue;
			if (!result.empty()) result += u8" ";
			result += part;
		}
		return result;
	};
	auto equipSlotText = [&joinU8](const ItemEquipSlot& slots) {
		std::vector<const char8_t*> parts;
		if (slots.main_hand) parts.push_back(u8"MainHand");
		if (slots.sub_hand) parts.push_back(u8"SubHand");
		if (slots.ranged) parts.push_back(u8"Ranged");
		if (slots.ammo) parts.push_back(u8"Ammo");
		if (slots.head) parts.push_back(u8"Head");
		if (slots.body) parts.push_back(u8"Body");
		if (slots.hands) parts.push_back(u8"Hands");
		if (slots.legs) parts.push_back(u8"Legs");
		if (slots.feet) parts.push_back(u8"Feet");
		if (slots.neck) parts.push_back(u8"Neck");
		if (slots.waist) parts.push_back(u8"Waist");
		if (slots.left_ear) parts.push_back(u8"LeftEar");
		if (slots.right_ear) parts.push_back(u8"RightEar");
		if (slots.left_ring) parts.push_back(u8"LeftRing");
		if (slots.right_ring) parts.push_back(u8"RightRing");
		if (slots.back) parts.push_back(u8"Back");
		return joinU8(parts);
	};
	auto raceText = [&joinU8](const ItemRaceApplicability& races) -> std::u8string {
		std::vector<const char8_t*> parts;

		// if all
		if (races.HumeMale&&races.HumeFemale&&races.ElvaanMale&&races.ElvaanFemale&&races.TaruMale&&races.TaruFemale&&races.Mithra&&races.Galka) {
			return u8"All";
		}

		// if female
		if (races.ElvaanFemale && races.HumeFemale && races.TaruFemale && races.Mithra) {
			return u8"Female";
		}

		// if male
		if (races.ElvaanMale && races.HumeMale && races.TaruMale && races.Galka) {
			return u8"Male";
		}

		if (races.None) parts.push_back(u8"None");
		if (races.HumeMale) parts.push_back(u8"HumeMale");
		if (races.HumeFemale) parts.push_back(u8"HumeFemale");
		if (races.ElvaanMale) parts.push_back(u8"ElvaanMale");
		if (races.ElvaanFemale) parts.push_back(u8"ElvaanFemale");
		if (races.TaruMale) parts.push_back(u8"TaruMale");
		if (races.TaruFemale) parts.push_back(u8"TaruFemale");
		if (races.Mithra) parts.push_back(u8"Mithra");
		if (races.Galka) parts.push_back(u8"Galka");
		return joinU8(parts);
	};
	auto jobText = [&joinU8](const ItemJobApplicability& jobs) {
		std::vector<const char8_t*> parts;
		if (jobs.pld) parts.push_back(u8"PLD");
		if (jobs.thf) parts.push_back(u8"THF");
		if (jobs.rdm) parts.push_back(u8"RDM");
		if (jobs.blm) parts.push_back(u8"BLM");
		if (jobs.whm) parts.push_back(u8"WHM");
		if (jobs.mnk) parts.push_back(u8"MNK");
		if (jobs.war) parts.push_back(u8"WAR");
		if (jobs.smn) parts.push_back(u8"SMN");
		if (jobs.drg) parts.push_back(u8"DRG");
		if (jobs.nin) parts.push_back(u8"NIN");
		if (jobs.sam) parts.push_back(u8"SAM");
		if (jobs.rng) parts.push_back(u8"RNG");
		if (jobs.brd) parts.push_back(u8"BRD");
		if (jobs.bst) parts.push_back(u8"BST");
		if (jobs.drk) parts.push_back(u8"DRK");
		if (jobs.mon) parts.push_back(u8"MON");
		if (jobs.run) parts.push_back(u8"RUN");
		if (jobs.geo) parts.push_back(u8"GEO");
		if (jobs.sch) parts.push_back(u8"SCH");
		if (jobs.dnc) parts.push_back(u8"DNC");
		if (jobs.pup) parts.push_back(u8"PUP");
		if (jobs.cor) parts.push_back(u8"COR");
		if (jobs.blu) parts.push_back(u8"BLU");
		return joinU8(parts);
	};

	csv.NewCell(u8"ID");
	csv.NewCell(u8"Name");
	csv.NewCell(u8"Description");
	csv.NewCell(u8"Flags");
	csv.NewCell(u8"Stack");
	csv.NewCell(u8"Type");
	// csv.NewCell(u8"SpecType");
	csv.NewCell(u8"ResID");
	csv.NewCell(u8"Targets");
	csv.NewCell(u8"ImageLength");
	if (isEnglish) {
		csv.NewCell(u8"LogFlag");
		csv.NewCell(u8"Name_Singular");
		csv.NewCell(u8"Name_Plural");
	}

	switch (firstDatum.spec_type) {
	case ItemSpecType::WEAPON:
		csv.NewCell(u8"Level");
		csv.NewCell(u8"Slots");
		csv.NewCell(u8"Races");
		csv.NewCell(u8"Jobs");
		for (auto&& h : { u8"Ukn", u8"Ukn2", u8"DMG", u8"Delay", u8"Ukn5", u8"Ukn6", u8"Ukn12", u8"Ukn7", u8"Ukn9",
			u8"MaxCharges", u8"CastFactor", u8"UseTime", u8"ReuseTime", u8"Ukn20", u8"Ukn21", u8"iLvl", u8"Ukn22", u8"Ukn23" }) {
			csv.NewCell(h);
		}
		break;
	case ItemSpecType::ARMOUR:
		csv.NewCell(u8"Level");
		csv.NewCell(u8"Slots");
		csv.NewCell(u8"Races");
		csv.NewCell(u8"Jobs");
		for (auto&& h : { u8"Ukn", u8"ShieldSize", u8"MaxCharges", u8"CastFactor", u8"UseTime", u8"ReuseTime", u8"Ukn1", u8"Ukn2", u8"iLvl", u8"Ukn3", u8"Ukn4" }) {
			csv.NewCell(h);
		}
		break;
	case ItemSpecType::USABLE:
		for (auto&& h : { u8"CastFactor", u8"Ukn1", u8"Ukn2", u8"Ukn3" }) {
			csv.NewCell(h);
		}
		break;
	case ItemSpecType::NORMAL:
		for (auto&& h : { u8"Ukn1", u8"Ukn2", u8"Ukn3", u8"Ukn4", u8"Ukn5" }) {
			csv.NewCell(h);
		}
		break;
	case ItemSpecType::PUPPET:
		for (auto&& h : { u8"Slot_Head", u8"Slot_Body", u8"Slot_Attachment", u8"Ukn1", u8"Ukn2" }) {
			csv.NewCell(h);
		}
		break;
	case ItemSpecType::SLIP:
		for (int i = 0; i < 70; ++i) {
			csv.NewCell(xybase::string::to_utf8(std::string("Ukn") + std::to_string(i)));
		}
		break;
	case ItemSpecType::CURRENCY:
		csv.NewCell(u8"Ukn");
		break;
	}
	csv.NewLine();

	for (const auto& datum : data) {
		std::u8string name;
		std::u8string desc;
		std::u8string nameSg;
		std::u8string namePl;
		std::u8string logFlag;

		if (datum.hasOriginalRow) {
			try { name = datum.name(); } catch (...) {}
			try { desc = datum.description(); } catch (...) {}
			if (isEnglish) {
				try { logFlag = toU8(datum.logFlag()); } catch (...) {}
				try { nameSg = datum.name_sg(); } catch (...) {}
				try { namePl = datum.name_pl(); } catch (...) {}
			}
		}

		csv.NewCell(toU8(datum.id));
		csv.NewCell(name);
		csv.NewCell(desc);
		csv.NewCell(flagsToDescription(datum.flags()));
		csv.NewCell(toU8(datum.stack_size()));
		csv.NewCell(toU8(datum.item_type()));
		// csv.NewCell(specTypeToU8(datum.spec_type));
		csv.NewCell(toU8(datum.resource_id()));
		csv.NewCell(toU8(datum.valid_targets()));
		csv.NewCell(toU8(datum.originalEntry.image_length));
		if (isEnglish) {
			csv.NewCell(logFlag);
			csv.NewCell(nameSg);
			csv.NewCell(namePl);
		}

		switch (datum.spec_type) {
		case ItemSpecType::WEAPON:
		{
			const auto& spec = datum.originalEntry.spec.weapon;
			csv.NewCell(toU8(spec.level));
			csv.NewCell(equipSlotText(spec.equip_slots));
			csv.NewCell(raceText(spec.races));
			csv.NewCell(jobText(spec.jobs));
			csv.NewCell(toU8(spec.ukn));
			csv.NewCell(toU8(spec.ukn2));
			csv.NewCell(toU8(spec.dmg));
			csv.NewCell(toU8(spec.delay));
			csv.NewCell(toU8(spec.ukn5));
			csv.NewCell(toU8(spec.ukn6));
			csv.NewCell(toU8(spec.ukn12));
			csv.NewCell(toU8(spec.ukn7));
			csv.NewCell(toU8(spec.ukn9));
			csv.NewCell(toU8(spec.max_charges));
			csv.NewCell(toU8(spec.cast_factor));
			csv.NewCell(toU8(spec.use_time));
			csv.NewCell(toU8(spec.reuse_time));
			csv.NewCell(toU8(spec.ukn20));
			csv.NewCell(toU8(spec.ukn21));
			csv.NewCell(toU8(spec.ilvl));
			csv.NewCell(toU8(spec.ukn22));
			csv.NewCell(toU8(spec.ukn23));
			break;
		}
		case ItemSpecType::ARMOUR:
		{
			const auto& spec = datum.originalEntry.spec.armour;
			csv.NewCell(toU8(spec.level));
			csv.NewCell(equipSlotText(spec.equip_slots));
			csv.NewCell(raceText(spec.equip_races));
			csv.NewCell(jobText(spec.equip_jobs));
			csv.NewCell(toU8(spec.ukn));
			csv.NewCell(toU8(spec.shield_size));
			csv.NewCell(toU8(spec.max_charges));
			csv.NewCell(toU8(spec.cast_factor));
			csv.NewCell(toU8(spec.use_time));
			csv.NewCell(toU8(spec.reuse_time));
			csv.NewCell(toU8(spec.ukn1));
			csv.NewCell(toU8(spec.ukn2));
			csv.NewCell(toU8(spec.ilvl));
			csv.NewCell(toU8(spec.ukn3));
			csv.NewCell(toU8(spec.ukn4));
			break;
		}
		case ItemSpecType::USABLE:
		{
			const auto& spec = datum.originalEntry.spec.usable;
			csv.NewCell(toU8(spec.cast_factor));
			csv.NewCell(toU8(spec.ukn1));
			csv.NewCell(toU8(spec.ukn2));
			csv.NewCell(toU8(spec.ukn3));
			break;
		}
		case ItemSpecType::NORMAL:
		{
			const auto& spec = datum.originalEntry.spec.normal;
			csv.NewCell(toU8(spec.ukn1));
			csv.NewCell(toU8(spec.ukn2));
			csv.NewCell(toU8(spec.ukn3));
			csv.NewCell(toU8(spec.ukn4));
			csv.NewCell(toU8(spec.ukn5));
			break;
		}
		case ItemSpecType::PUPPET:
		{
			const auto& spec = datum.originalEntry.spec.puppet;
			csv.NewCell(boolToU8(spec.head));
			csv.NewCell(boolToU8(spec.body));
			csv.NewCell(boolToU8(spec.attachment));
			csv.NewCell(toU8(spec.ukn1));
			csv.NewCell(toU8(spec.ukn2));
			break;
		}
		case ItemSpecType::SLIP:
		{
			const auto& spec = datum.originalEntry.spec.slip;
			for (uint8_t b : spec.ukn) {
				csv.NewCell(toU8(static_cast<unsigned>(b)));
			}
			break;
		}
		case ItemSpecType::CURRENCY:
		{
			const auto& spec = datum.originalEntry.spec.currency;
			csv.NewCell(toU8(spec.ukn));
			break;
		}
		}

		csv.NewLine();
	}

	csv.Close();
}
