#include "RecordsOfEminence.h"
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
		buffer[i] = static_cast<char>(ror_byte(byte, 5));
	}
}

// Helper function to encrypt a buffer using byte-wise left rotation by 5 (inverse of ROR 5)
static void encryptRol5(char* buffer, size_t size) {
	for (size_t i = 0; i < size; ++i) {
		uint8_t byte = static_cast<uint8_t>(buffer[i]);
		buffer[i] = static_cast<char>(rol_byte(byte, 5));
	}
}

// ============= ROM/307/15 (Quest Entry) Methods =============

void RecordsOfEminence::ReadQuest(const char* path)
{
	ReadQuest(xybase::string::sys_mbs_to_wcs(std::string(path)));
}

void RecordsOfEminence::ReadQuest(const std::wstring& path)
{
	questData.clear();
	std::ifstream eye(path, std::ios::binary);
	if (!eye.is_open()) {
		throw std::runtime_error("Failed to open ROE quest file: " + xybase::string::sys_wcs_to_mbs(path));
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
		RoeQuestEntry entry;
		
		// Check if we have enough bytes for a complete entry
		if (offset + sizeof(RoeQuestEntry) > fileSize) {
			break; // End of file or incomplete entry
		}
		
		// Copy entry from buffer
		memcpy(&entry, fileBuffer.get() + offset, sizeof(RoeQuestEntry));
		
		// Verify the terminator (should be 0xFF)
		if (entry.terminator != (char)0xFF) {
			throw std::runtime_error("Invalid terminator found, expected 0xFF but got: " + 
								   std::to_string(static_cast<unsigned>(entry.terminator)));
		}
		
		// Create a RoeQuestDatum and store the complete original entry
		RoeQuestDatum datum;
		datum.originalEntry = entry;  // Preserve ALL original data
		datum.id = entry.id;
		datum.release_date = entry.release_date;
		
		// Read and save the complete original Row
		datum.originalRow.ReadRow(&entry.info.info_rec, sizeof(entry.info.raw));
		datum.hasOriginalRow = true;
		
		// That's it! Text fields accessed via datum.questName(), datum.description()
		
		questData.push_back(std::move(datum));
		offset += sizeof(RoeQuestEntry);
	}
}

void RecordsOfEminence::WriteQuest(const char* path)
{
	WriteQuest(xybase::string::sys_mbs_to_wcs(std::string(path)));
}

void RecordsOfEminence::WriteQuest(const std::wstring& path)
{
	std::ofstream file(path, std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file for writing: " + xybase::string::sys_wcs_to_mbs(path));
	}
	
	// Prepare buffer for all entries
	std::vector<char> buffer;
	
	for (RoeQuestDatum &datum : questData) {  // 注意：非 const，因为 WriteRow 需要
		// Use the preserved original entry as the base
		RoeQuestEntry entry = datum.originalEntry;
		
		// Update the fields that might have been modified
		entry.id = datum.id;
		entry.release_date = datum.release_date;
		
		// Simply write the Row as-is (it's already updated via setters)
		if (datum.hasOriginalRow) {
			datum.originalRow.WriteRow(&entry.info.info_rec, sizeof(entry.info.raw));
		}
		
		entry.terminator = 0xFF; // Ensure terminator is correct
		
		// Encrypt the entry using ROL 5
		char entryBuffer[sizeof(RoeQuestEntry)];
		memcpy(entryBuffer, &entry, sizeof(RoeQuestEntry));
		encryptRol5(entryBuffer, sizeof(RoeQuestEntry));
		
		// Add encrypted entry to buffer
		size_t oldSize = buffer.size();
		buffer.resize(oldSize + sizeof(RoeQuestEntry));
		memcpy(buffer.data() + oldSize, entryBuffer, sizeof(RoeQuestEntry));
	}
	
	// Write the encrypted buffer to file
	file.write(buffer.data(), buffer.size());
	
	if (!file) {
		throw std::runtime_error("Failed to write data to file");
	}

	file.close();
}

void RecordsOfEminence::QuestToICsv(const char* path)
{
	std::wstring wpath = xybase::string::sys_mbs_to_wcs(std::string(path));
	CsvFile csv(wpath, std::ios::out | std::ios::binary);
	
	// Write header
	csv.NewCell(u8"ID");
	csv.NewCell(u8"Release_Date");
	csv.NewCell(u8"Repeatable");
	csv.NewCell(u8"Target_Count");
	csv.NewCell(u8"EMI_Reward");
	csv.NewCell(u8"EXP_Reward");
	csv.NewCell(u8"CAP_Reward");
	csv.NewCell(u8"UNI_Reward");
	csv.NewCell(u8"Quest_Name");
	csv.NewLine();
	
	for (const auto& datum : questData) {
		// Write basic fields
		csv.NewCell(xybase::string::to_utf8(std::to_string(datum.id)));
		csv.NewCell(xybase::string::to_utf8(std::to_string(datum.release_date)));
		csv.NewCell(xybase::string::to_utf8(std::to_string(datum.originalEntry.repeatable)));
		csv.NewCell(xybase::string::to_utf8(std::to_string(datum.originalEntry.target_count)));
		csv.NewCell(xybase::string::to_utf8(std::to_string(datum.originalEntry.emi_reward)));
		csv.NewCell(xybase::string::to_utf8(std::to_string(datum.originalEntry.exp_reward)));
		csv.NewCell(xybase::string::to_utf8(std::to_string(datum.originalEntry.cap_reward)));
		csv.NewCell(xybase::string::to_utf8(std::to_string(datum.originalEntry.uni_reward)));
		
		// Write text fields
		for (const auto& cell : datum.originalRow) {
			try {
				csv.NewCell(cell.ToString());
			} catch (...) {
				csv.NewCell(u8"");
			}
		}

		csv.NewLine();
	}
	
	csv.Close();
}

// ============= ROM/307/23 (Category Entry) Methods =============

void RecordsOfEminence::ReadCategory(const char* path)
{
	ReadCategory(xybase::string::sys_mbs_to_wcs(std::string(path)));
}

void RecordsOfEminence::ReadCategory(const std::wstring& path)
{
	categoryData.clear();
	std::ifstream eye(path, std::ios::binary);
	if (!eye.is_open()) {
		throw std::runtime_error("Failed to open ROE category file: " + xybase::string::sys_wcs_to_mbs(path));
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
		RoeCategoryEntry entry;
		
		// Check if we have enough bytes for a complete entry
		if (offset + sizeof(RoeCategoryEntry) > fileSize) {
			break; // End of file or incomplete entry
		}
		
		// Copy entry from buffer
		memcpy(&entry, fileBuffer.get() + offset, sizeof(RoeCategoryEntry));
		
		// Verify the terminator (should be 0xFF)
		if (entry.terminator != (char)0xFF) {
			throw std::runtime_error("Invalid terminator found, expected 0xFF but got: " + 
								   std::to_string(static_cast<unsigned>(entry.terminator)));
		}
		
		// Create a RoeCategoryDatum and store the complete original entry
		RoeCategoryDatum datum;
		datum.originalEntry = entry;  // Preserve ALL original data including children
		datum.id = entry.id;
		
		// Read and save the complete original Row
		datum.originalRow.ReadRow(&entry.info.info_rec, sizeof(entry.info.raw));
		datum.hasOriginalRow = true;
		
		// That's it! Text fields accessed via datum.categoryName()
		
		categoryData.push_back(std::move(datum));
		offset += sizeof(RoeCategoryEntry);
	}
}

void RecordsOfEminence::WriteCategory(const char* path)
{
	WriteCategory(xybase::string::sys_mbs_to_wcs(std::string(path)));
}

void RecordsOfEminence::WriteCategory(const std::wstring& path)
{
	std::ofstream file(path, std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file for writing: " + xybase::string::sys_wcs_to_mbs(path));
	}
	
	// Prepare buffer for all entries
	std::vector<char> buffer;
	
	for (RoeCategoryDatum &datum : categoryData) {  // 注意：非 const
		// Use the preserved original entry as the base
		RoeCategoryEntry entry = datum.originalEntry;
		
		// Update the fields that might have been modified
		entry.id = datum.id;
		
		// Simply write the Row as-is (it's already updated via setters)
		if (datum.hasOriginalRow) {
			datum.originalRow.WriteRow(&entry.info.info_rec, sizeof(entry.info.raw));
		}
		
		entry.terminator = 0xFF; // Ensure terminator is correct
		
		// Encrypt the entry using ROL 5
		char entryBuffer[sizeof(RoeCategoryEntry)];
		memcpy(entryBuffer, &entry, sizeof(RoeCategoryEntry));
		encryptRol5(entryBuffer, sizeof(RoeCategoryEntry));
		
		// Add encrypted entry to buffer
		size_t oldSize = buffer.size();
		buffer.resize(oldSize + sizeof(RoeCategoryEntry));
		memcpy(buffer.data() + oldSize, entryBuffer, sizeof(RoeCategoryEntry));
	}
	
	// Write the encrypted buffer to file
	file.write(buffer.data(), buffer.size());
	
	if (!file) {
		throw std::runtime_error("Failed to write data to file");
	}

	file.close();
}

void RecordsOfEminence::CategoryToICsv(const char* path)
{
	std::wstring wpath = xybase::string::sys_mbs_to_wcs(std::string(path));
	CsvFile csv(wpath, std::ios::out | std::ios::binary);
	
	// Write header
	csv.NewCell(u8"ID");
	csv.NewCell(u8"Child_Count");
	csv.NewCell(u8"Category_Name");
	csv.NewCell(u8"Cell_Count");
	csv.NewCell(u8"Children_Info");
	csv.NewLine();
	
	for (const auto& datum : categoryData) {
		// Write basic fields
		csv.NewCell(xybase::string::to_utf8(std::to_string(datum.id)));
		csv.NewCell(xybase::string::to_utf8(std::to_string(datum.originalEntry.count_of_children)));
		
		// Write category name
		try {
			csv.NewCell(datum.categoryName());
		} catch (...) {
			csv.NewCell(u8"");
		}
		
		// Write cell count
		csv.NewCell(xybase::string::to_utf8(std::to_string(datum.cellCount())));
		
		// Write children info as a concatenated string
		std::u8string childrenInfo;
		for (uint32_t i = 0; i < datum.originalEntry.count_of_children && i < 20; ++i) {
			if (i > 0) childrenInfo += u8"; ";
			childrenInfo += u8"[" + xybase::string::to_utf8(std::to_string(datum.originalEntry.children[i].child_id));
			childrenInfo += u8"," + xybase::string::to_utf8(std::to_string(datum.originalEntry.children[i].quest_flag)) + u8"]";
		}
		csv.NewCell(childrenInfo);
		csv.NewLine();
	}
	
	csv.Close();
}

