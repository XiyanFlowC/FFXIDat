#include "StatusData.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
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

// Helper function to count number of 1 bits in a byte
inline int ones(uint8_t byte) {
    int count = 0;
    while (byte) {
        count += byte & 1;
        byte >>= 1;
    }
    return count;
}

// Rotation functions for different rotation amounts
inline uint8_t ror_byte_n(uint8_t value, int bits) {
    bits &= 7; // Ensure bits is in range 0-7 for byte rotation
    return (value >> bits) | (value << (8 - bits));
}

inline uint8_t rol_byte_n(uint8_t value, int bits) {
    bits &= 7; // Ensure bits is in range 0-7 for byte rotation
    return (value << bits) | (value >> (8 - bits));
}

// Helper function to decrypt a buffer using variable rotation
void decryptVariable(char* buffer, size_t size, int rotationBits) {
    for (size_t i = 0; i < size; ++i) {
        uint8_t byte = static_cast<uint8_t>(buffer[i]);
        buffer[i] = static_cast<char>(ror_byte_n(byte, rotationBits));
    }
}

// Helper function to encrypt a buffer using variable rotation (inverse)
void encryptVariable(char* buffer, size_t size, int rotationBits) {
    for (size_t i = 0; i < size; ++i) {
        uint8_t byte = static_cast<uint8_t>(buffer[i]);
        buffer[i] = static_cast<char>(rol_byte_n(byte, rotationBits));
    }
}

// Helper function to decrypt uint16_t using variable rotation
uint16_t decryptUint16Variable(uint16_t value, int rotationBits) {
    uint8_t* bytes = reinterpret_cast<uint8_t*>(&value);
    for (int i = 0; i < 2; ++i) {
        bytes[i] = ror_byte_n(bytes[i], rotationBits);
    }
    return value;
}

// Helper function to encrypt uint16_t using variable rotation (inverse)
uint16_t encryptUint16Variable(uint16_t value, int rotationBits) {
    uint8_t* bytes = reinterpret_cast<uint8_t*>(&value);
    for (int i = 0; i < 2; ++i) {
        bytes[i] = rol_byte_n(bytes[i], rotationBits);
    }
    return value;
}

void StatusData::Read(std::wstring path)
{
    data.clear();
    
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + xybase::string::to_string(path));
    }

    // 每次读取一块直到读取到文件末尾
    // Read chunks until reaching end of file, each chunk ends with 0xFF marker
    while (!file.eof()) {
        StatusEntry entry;
        
        // Read the entry structure
        file.read(reinterpret_cast<char*>(&entry), sizeof(StatusEntry));
        
        if (file.gcount() == 0) {
            break; // End of file
        }
        
        if (file.gcount() != sizeof(StatusEntry)) {
            throw std::runtime_error("Incomplete entry read from file");
        }
        
        // Verify the end marker (should be 0xFF)
        if (entry.end_marker != 0xFF) {
            throw std::runtime_error("Invalid end marker found, expected 0xFF but got: " + 
                                   std::to_string(static_cast<unsigned>(entry.end_marker)));
        }

        // 解密
        // 根据块地址[2][11][12]中，1的个数决定ror数
        // 1的个数 abs(ones([2])-ones([11])+ones([12]))，对5取余
        // 0-4分别对应ror7 1 6 2 5

        uint8_t* stp = reinterpret_cast<uint8_t*>(&entry);
        
        // Calculate index using encrypted bytes
        int idx = abs(ones(stp[2]) - ones(stp[11]) + ones(stp[12])) % 5;
        
        // Map idx to rotation amounts: 0-4分别对应ror7 1 6 2 5
        int rotationBits[] = {7, 1, 6, 2, 5};
        int rorBits = rotationBits[idx];
        
        // Decrypt the id and flg using calculated rotation
        uint16_t decryptedId = decryptUint16Variable(entry.id, rorBits);
        uint16_t decryptedFlg = decryptUint16Variable(entry.flg, rorBits);
        
        // Decrypt the spec data using the same rotation (NOT the entire entry)
        decryptVariable(entry.spec.raw, sizeof(entry.spec.raw), rorBits);
        
        // Parse the decrypted data into a StatusDatum
        StatusDatum datum;
        datum.id = decryptedId;
        datum.flg = decryptedFlg;
        
        // Parse the record data to extract description (Cell 0 only)
        if (entry.spec.rec.cellCount > 0) {
            Row row;
            row.ReadRow(&entry.spec.rec, sizeof(entry.spec.raw));
            
            // Status records only have description in Cell 0
            if (row.GetCells().size() >= 1) {
                // Convert description from Cell 0
                auto descStr = row.GetCells()[0].Get<std::u8string>();
                datum.description = descStr;
            }
        }
        
        // Process image data if present (image data is stored raw, not encrypted)
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
    }
    
    file.close();
}

void StatusData::Write(std::wstring path)
{
    std::ofstream file(path, std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + xybase::string::to_string(path));
    }

    for (const StatusDatum &datum : data) {
        StatusEntry entry = {};
        
        // Set basic values first
        entry.id = static_cast<uint16_t>(datum.id);
        entry.flg = datum.flg;
        
        // Prepare the record data with only description in Cell 0
        Row row;
        row.GetCells().emplace_back(datum.description);
        
        int recSize = row.GetSize();
        if (recSize > sizeof(entry.spec.raw)) {
            throw std::runtime_error("Record size exceeds maximum allowed size");
        }
        
        // Write the record data into the spec
        row.WriteRow(&entry.spec.rec, sizeof(entry.spec.raw));
        
        // Handle image data if present
        if (datum.image.texture) {
            size_t imgSize = sizeof(entry.image_data);
            datum.image.WriteToMemory(entry.image_data, imgSize);
            if (imgSize > sizeof(entry.image_data)) {
                throw std::runtime_error("Image size exceeds maximum allowed size");
            }
            entry.image_length = static_cast<uint32_t>(imgSize);
        } else {
            entry.image_length = 0;
        }
        
        entry.end_marker = 0xFF; // Set end marker
        
        // Now calculate rotation bits using the same logic as Read
        // Need to use bytes at positions [2], [11], [12] of the entry structure
        uint8_t* stp = reinterpret_cast<uint8_t*>(&entry);
        
        // Calculate index using the same bytes as in Read: [2], [11], [12]
        int idx = abs(ones(stp[2]) - ones(stp[11]) + ones(stp[12])) % 5;
        
        // Map idx to rotation amounts: 0-4分别对应ror7 1 6 2 5
        int rotationBits[] = {7, 1, 6, 2, 5};
        int rolBits = rotationBits[idx]; // Use same rotation for encryption (will be reversed)
        
        // Encrypt the id and flg using calculated rotation (inverse of ror is rol)
        entry.id = encryptUint16Variable(entry.id, rolBits);
        entry.flg = encryptUint16Variable(entry.flg, rolBits);
        
        // Encrypt the spec data using calculated rotation
        encryptVariable(entry.spec.raw, sizeof(entry.spec.raw), rolBits);
        
        // Write the entry to file
        file.write(reinterpret_cast<const char*>(&entry), sizeof(StatusEntry));
        
        if (!file) {
            throw std::runtime_error("Failed to write entry to file");
        }
    }
    file.close();
}
