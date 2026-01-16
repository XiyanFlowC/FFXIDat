#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <stdexcept>

#include "Record.h"
#include "Image.h"

#pragma pack(push, 1)

// All of these structures are encrypted with ROR 5 for the entire file
// ROM/307/15 - Individual quest/objective entries
struct RoeQuestEntry
{
	uint32_t id;
	uint32_t release_date; // Date in numeric format (e.g., 20141005 = 5 Oct 2014)
	uint32_t repeatable; // 0 = no, 1 = yes
	uint32_t target_count; 
	uint32_t emi_reward;
	uint32_t exp_reward;
	uint32_t cap_reward;
	uint32_t uni_reward;
	union {
		char raw[3039];
		Record info_rec;
		// For Japanese, 3 cells in total, cell 0 is quest name, cell 1 is description, cell 2 is empty
		// For English, 5 cells in total, cell 0&1 are quest name (seems identical, or singular/plural form?),
		// cell 2 is empty, cell 3 is description, cell 4 is empty
	} info;
	char terminator; // must be 0xFF
};

// ROM/307/23 - Category entries that organize quests
struct RoeCategoryEntry
{
	uint32_t id;
	uint32_t count_of_children;
	struct {
		uint32_t child_id; // refers to RoeQuestEntry.id or RoeCategoryEntry.id
		uint32_t quest_flag; // 0 = category, non-zero = actual quest
		uint32_t ukn[3]; // unknown, seems to be always 0
	} children[28];
	union {
		char raw[2503];
		Record info_rec; // Cell 0 is category name
	} info;
	char terminator; // must be 0xFF
};
#pragma pack(pop)

class RecordsOfEminence
{
public:
	// Data structure for ROM/307/15 (Quest entries)
	class RoeQuestDatum
	{
	public:
		uint32_t id;
		uint32_t release_date; // Date in numeric format (e.g., 20141005 = Oct 5, 2014)
		
		// Store the complete original entry to preserve ALL fields
		RoeQuestEntry originalEntry;
		
		// Store the original Row structure - THIS IS THE SOURCE OF TRUTH for text fields
		Row originalRow;
		bool hasOriginalRow = false;
		
		// ============ Text Field Accessors ============
		
		// Get quest name (Cell 0)
		// Throws: std::out_of_range if cell doesn't exist
		std::u8string questName() const {
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
		
		// Set quest name (Cell 0)
		// For English format, also updates Cell 1 (duplicate name)
		// Returns: true if successful, false if cell doesn't exist
		bool setQuestName(const std::u8string& newName) {
			if (!hasOriginalRow) return false;
			
			auto& cells = originalRow.GetCells();
			if (cells.empty()) return false;
			
			// Set cell 0 (primary name)
			cells[0].Set(newName);
			
			// For English format, also set cell 1 (duplicate name)
			if (cells.size() >= 2 && cells[1].GetType() == 0) {
				cells[1].Set(newName);
			}
			
			return true;
		}
		
		// Get description (auto-detect Japanese/English format)
		// Japanese: Cell 1, English: Cell 3
		std::u8string description() const {
			if (!hasOriginalRow) {
				throw std::runtime_error("No original row data");
			}
			
			const auto& cells = originalRow.GetCellsConst();
			
			// Try English format (cell 3)
			if (cells.size() >= 4 && cells[3].GetType() == 0) {
				return cells[3].Get<std::u8string>();
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
			
			// Try English format (cell 3)
			if (cells.size() >= 4) {
				cells[3].Set(newDesc);
				return true;
			}
			
			// Try Japanese format (cell 1)
			if (cells.size() >= 2) {
				cells[1].Set(newDesc);
				return true;
			}
			
			return false;
		}
		
		// ============ Direct Row Access ============
		
		Row& row() { return originalRow; }
		const Row& row() const { return originalRow; }
		
		size_t cellCount() const {
			return hasOriginalRow ? originalRow.GetCellsConst().size() : 0;
		}
		
		RoeQuestDatum() : id(0), release_date(0), originalEntry{}
		{
			// Initialize originalEntry with default values
			originalEntry.id = 0;
			originalEntry.release_date = 0;
			originalEntry.repeatable = 0;
			originalEntry.target_count = 0;
			originalEntry.emi_reward = 0;
			originalEntry.exp_reward = 0;
			originalEntry.cap_reward = 0;
			originalEntry.uni_reward = 0;
			memset(&originalEntry.info, 0, sizeof(originalEntry.info));
			originalEntry.terminator = '\xFF';
		}
	};
	
	// Data structure for ROM/307/23 (Category entries)
	class RoeCategoryDatum
	{
	public:
		uint32_t id;
		
		// Store the complete original entry to preserve ALL fields
		RoeCategoryEntry originalEntry;
		
		// Store the original Row structure - THIS IS THE SOURCE OF TRUTH for text fields
		Row originalRow;
		bool hasOriginalRow = false;
		
		// ============ Text Field Accessors ============
		
		// Get category name (Cell 0)
		// Throws: std::out_of_range if cell doesn't exist
		std::u8string categoryName() const {
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
		
		// Set category name (Cell 0)
		// Returns: true if successful, false if cell doesn't exist
		bool setCategoryName(const std::u8string& newName) {
			if (!hasOriginalRow) return false;
			
			auto& cells = originalRow.GetCells();
			if (cells.empty()) return false;
			
			cells[0].Set(newName);
			return true;
		}
		
		// ============ Direct Row Access ============
		
		Row& row() { return originalRow; }
		const Row& row() const { return originalRow; }
		
		size_t cellCount() const {
			return hasOriginalRow ? originalRow.GetCellsConst().size() : 0;
		}
		
		RoeCategoryDatum() : id(0), originalEntry{}
		{
			// Initialize originalEntry with default values
			originalEntry.id = 0;
			originalEntry.count_of_children = 0;
			memset(originalEntry.children, 0, sizeof(originalEntry.children));
			memset(&originalEntry.info, 0, sizeof(originalEntry.info));
			originalEntry.terminator = '\xFF';
		}
	};

	// ROM/307/15 methods (Quest entries)
	void ReadQuest(const char* path);
	void ReadQuest(const std::wstring& path);
	void WriteQuest(const char* path);
	void WriteQuest(const std::wstring& path);

	void QuestToICsv(const char* path);
	
	// ROM/307/23 methods (Category entries)
	void ReadCategory(const char* path);
	void ReadCategory(const std::wstring& path);
	void WriteCategory(const char* path);
	void WriteCategory(const std::wstring& path);

	void CategoryToICsv(const char* path);

	std::vector<RoeQuestDatum> questData;     // ROM/307/15 data
	std::vector<RoeCategoryDatum> categoryData; // ROM/307/23 data
};
