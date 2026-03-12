#include "MonBridge.h"
#include "xystring.h"
#include "CsvFile.h"

#include <fstream>
#include <stdexcept>
#include <cstring>

// Helper function to perform byte-wise right rotation by specified bits
static inline uint8_t ror_byte(uint8_t value, int bits) {
	bits &= 7; // Ensure bits is in range 0-7 for byte rotation
	return (value >> bits) | (value << (8 - bits));
}

// Helper function to perform byte-wise left rotation by specified bits
static inline uint8_t rol_byte(uint8_t value, int bits) {
	bits &= 7; // Ensure bits is in range 0-7 for byte rotation
	return (value << bits) | (value >> (8 - bits));
}

// Helper function to decrypt a buffer using byte-wise right rotation by 5
static void decryptRor5(char* buffer, size_t size) {
	for (size_t i = 0; i < size; ++i) {
		uint8_t byte = static_cast<uint8_t>(buffer[i]);
		// Perform byte-wise right rotation by 5 bits
		buffer[i] = static_cast<char>(ror_byte(byte, 5));
	}
}

// Helper function to encrypt a buffer using byte-wise left rotation by 5 (inverse of ROR 5)
static void encryptRol5(char* buffer, size_t size) {
	for (size_t i = 0; i < size; ++i) {
		uint8_t byte = static_cast<uint8_t>(buffer[i]);
		// Perform byte-wise left rotation by 5 bits (inverse of ROR 5)
		buffer[i] = static_cast<char>(rol_byte(byte, 5));
	}
}

void MonBridge::Read(const char* path)
{
	Read(xybase::string::sys_mbs_to_wcs(std::string(path)));
}

void MonBridge::Read(const std::wstring& path)
{
	data.clear();
	std::ifstream eye(path, std::ios::binary);
	if (!eye.is_open()) {
		throw std::runtime_error("Failed to open file: " + xybase::string::sys_wcs_to_mbs(path));
	}

	// Read entire file and decrypt it with ROR 5
	eye.seekg(0, std::ios::end);
	size_t fileSize = eye.tellg();
	eye.seekg(0, std::ios::beg);
	
	std::unique_ptr<char[]> fileBuffer(new char[fileSize]);
	eye.read(fileBuffer.get(), fileSize);
	eye.close();
	
	// Decrypt the entire file using ROR 5
	decryptRor5(fileBuffer.get(), fileSize);
	
	// Process entries from decrypted buffer
	size_t offset = 0;
	while (offset < fileSize) {
		MBRecord entry;
		
		// Check if we have enough bytes for a complete entry
		if (offset + sizeof(MBRecord) > fileSize) {
			break; // End of file or incomplete entry
		}
		
		// Copy entry from buffer
		memcpy(&entry, fileBuffer.get() + offset, sizeof(MBRecord));
		
		// Verify the terminator (should be 0xFF)
		if (entry.terminator != (char)0xFF) {
			throw std::runtime_error("Invalid terminator found, expected 0xFF but got: " + 
								   std::to_string(static_cast<unsigned>(entry.terminator)));
		}
		
		// Create a MonBridgeDatum and store the complete original entry
		MonBridgeDatum datum;
		datum.originalEntry = entry;  // Preserve ALL original data including unknown fields
		datum.id = entry.id;
		datum.idx = entry.idx;
		
		// Extract internal name from fixed-size char array (ASCII identifier)
		datum.internalName = xybase::string::to_utf8(std::string(entry.name, strnlen(entry.name, 32)));
		
		// Parse the record data to extract display name (first cell only)
		Row row;
		row.ReadRow(&entry.rec.info_rec, sizeof(entry.rec.raw));
		
		if (row.GetCells().size() >= 1) {
			// First cell contains the display name shown in game
			if (row.GetCells()[0].GetType() == 0) { // string type
				auto displayStr = row.GetCells()[0].Get<std::u8string>();
				datum.displayName = displayStr;
			}
		}
		
		// Process icon data if present (similar to ItemData image handling)
		// Note: icon_data size is fixed at 2431 bytes
		if (entry.icon_size) {  // Basic check if icon data exists
			Image img;
			// Assuming icon_data contains image data, try to read it
			// The actual size might need to be determined from the data
			try {
				img.ReadFromMemory(entry.icon_data, entry.icon_size);
				datum.image = std::move(img);
			} catch (const std::exception&) {
				// If image reading fails, just leave image empty
			}
		}
		
		data.push_back(std::move(datum));
		offset += sizeof(MBRecord);
	}
}

void MonBridge::Write(const char* path)
{
	Write(xybase::string::sys_mbs_to_wcs(std::string(path)));
}

void MonBridge::Write(const std::wstring& path)
{
	std::ofstream file(path, std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file for writing: " + xybase::string::sys_wcs_to_mbs(path));
	}
	
	// Prepare buffer for all entries
	std::vector<char> buffer;
	
	for (const MonBridgeDatum &datum : data) {
		// Use the preserved original entry as the base, which maintains all unknown fields
		MBRecord entry = datum.originalEntry;
		
		// Update the fields that might have been modified
		entry.id = datum.id;
		entry.idx = datum.idx;
		
		// Update internal name field (ASCII identifier)
		std::string nameStr = xybase::string::to_string(datum.internalName);
		memset(entry.name, 0, sizeof(entry.name));
		strncpy_s(entry.name, sizeof(entry.name), nameStr.c_str(), std::min(nameStr.length(), size_t(31)));
		
		// Prepare the record data for display name (single cell only)
		if (!datum.displayName.empty()) {
			Row row;
			row.GetCells().emplace_back(datum.displayName);
			
			int recSize = row.GetSize();
			if (recSize > sizeof(entry.rec.raw)) {
				throw std::runtime_error("Record size exceeds maximum allowed size");
			}
			
			// Write the record data into the rec union
			row.WriteRow(&entry.rec.info_rec, sizeof(entry.rec.raw));
		}
		
		// Handle icon data if present
		if (datum.image.texture) {
			size_t imgSize = sizeof(entry.icon_data);
			datum.image.WriteToMemory(entry.icon_data, imgSize);
			if (imgSize > sizeof(entry.icon_data)) {
				throw std::runtime_error("Image size exceeds maximum allowed size");
			}
		} else {
			// Keep original icon data if no image changes
			memcpy(entry.icon_data, datum.originalEntry.icon_data, sizeof(entry.icon_data));
		}
		
		entry.terminator = 0xFF; // Ensure terminator is correct
		
		// Encrypt the entry using ROL 5
		char entryBuffer[sizeof(MBRecord)];
		memcpy(entryBuffer, &entry, sizeof(MBRecord));
		encryptRol5(entryBuffer, sizeof(MBRecord));
		
		// Add encrypted entry to buffer
		size_t oldSize = buffer.size();
		buffer.resize(oldSize + sizeof(MBRecord));
		memcpy(buffer.data() + oldSize, entryBuffer, sizeof(MBRecord));
	}
	
	// Write the encrypted buffer to file
	file.write(buffer.data(), buffer.size());
	
	if (!file) {
		throw std::runtime_error("Failed to write data to file");
	}

	file.close();
}

void MonBridge::ToICsv(const std::wstring& path) const
{
	CsvFile csv(path, std::ios::out | std::ios::binary);
	
	// Write header
	csv.NewCell(u8"ID");
	csv.NewCell(u8"Index");
	csv.NewCell(u8"Internal_Name");
	csv.NewCell(u8"Display_Name");
	csv.NewLine();
	
	for (const auto& datum : data) {
		// Write basic fields
		csv.NewCell(xybase::string::to_utf8(std::to_string(datum.id)));
		csv.NewCell(xybase::string::to_utf8(std::to_string(datum.idx)));
		csv.NewCell(datum.internalName);
		csv.NewCell(datum.displayName);
		
		csv.NewLine();
	}
	
	csv.Close();
}

