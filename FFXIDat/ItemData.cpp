#include "ItemData.h"
#include "Image.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cstring>
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
    CsvFile csv(path, std::ios::out);

    if (data.empty()) {
        return; // No data to write
	}

	// Get first datum for header extraction
	auto&& first_datum = data.front();
	// Write header
	csv.NewCell(u8"ID");
	csv.NewCell(u8"Name");
	csv.NewCell(u8"Description");
    // TODO: Implement this
    switch (first_datum.item_type()) {
        
    }

    for (auto &&datum : data) {

	}
}
