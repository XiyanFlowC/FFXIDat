#include "ItemData.h"
#include "Image.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include "xystring.h"

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
    
    std::unique_ptr<char[]> fileBuffer(new char[fileSize]);
    file.read(fileBuffer.get(), fileSize);
    file.close();
    
    // Decrypt the entire file using ROR 5
    decryptRor5(fileBuffer.get(), fileSize);
    
    // Process entries from decrypted buffer
    size_t offset = 0;
    while (offset < fileSize) {
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
            default:
                rec = &entry.spec.normal.info_rec;
                break;
        }
        
        if (rec && rec->cellCount > 0) {
            Row row;
            row.ReadRow(rec, sizeof(entry.spec.raw));
            
            // Typically item records have: name, description, and other fields
            if (row.GetCells().size() >= 1) {
                // Convert name
                auto nameStr = row.GetCells()[0].Get<std::u8string>();
                datum.name = nameStr;
            }
            
            if (row.GetCells().size() >= 2) {
                // Convert description
                auto descStr = row.GetCells()[1].Get<std::u8string>();
                datum.description = descStr;
            }
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
    }
}

void ItemData::Write(std::wstring path)
{
    std::ofstream file(path, std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + xybase::string::sys_wcs_to_mbs(path));
    }

    // Prepare buffer for all entries
    std::vector<char> buffer;
    
    for (const ItemDatum &datum : data) {
        // Use the preserved original entry as the base, which maintains all unknown fields
        ItemEntry entry = datum.originalEntry;
        
        // Update the ID field that might have been modified
        entry.header.id = datum.id;
        
        // Prepare the record data for name and description
        Row row;
        row.GetCells().emplace_back(datum.name);
        row.GetCells().emplace_back(datum.description);
        
        // Use the stored spec type to update the appropriate record
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
            default:
                rec = &entry.spec.normal.info_rec;
                break;
        }
        
        if (rec) {
            int recSize = row.GetSize();
            if (recSize > sizeof(entry.spec.raw)) {
                throw std::runtime_error("Record size exceeds maximum allowed size");
            }
            
            // Write the record data into the spec
            row.WriteRow(rec, sizeof(entry.spec.raw));
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
    
    // Encrypt the entire buffer using ROL 5 before writing
    encryptRol5(buffer.data(), buffer.size());
    
    // Write the encrypted buffer to file
    file.write(buffer.data(), buffer.size());
    
    if (!file) {
        throw std::runtime_error("Failed to write data to file");
    }
    
    file.close();
}
