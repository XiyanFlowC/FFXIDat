#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <stdexcept>

#include "Record.h"
#include "Image.h"
#include "RecordFormat.h"

// Sample Files: 0/6 0/7 0/8 286/72 0/5 0/4 （整个文件ror5）

#pragma pack(push, 1)

struct ItemHeader
{
	uint32_t id;
	
	// flag set 1
	uint8_t is_wall_decoration : 1; // can be hung on wall
	uint8_t is_gm_item : 1;
	uint8_t is_in_mystery_box : 1; // can be yielded by mystery box
	uint8_t ukn_flg1 : 1;
	uint8_t is_alt : 1; // can be sent to another character hold by same account
	uint8_t is_inscribable : 1; // can be inscribed
	uint8_t is_not_listable : 1; // cannot be listed in auction house
	uint8_t is_scroll : 1;

	// flag set 2
	uint8_t is_linkshell : 1;
	uint8_t is_usable : 1; // can be used
	uint8_t is_npc_tradeable : 1; // can be traded with NPCs
	uint8_t is_equipment : 1; // can be equipped
	uint8_t is_unsellable : 1; // cannot be sold to vendor
	uint8_t is_unmailable : 1; // cannot be mailed
	uint8_t is_ex : 1; // cannot be traded
	uint8_t is_rare : 1;

	uint16_t stack_size;
	uint16_t item_type;
	uint16_t resource_id;
	uint16_t valid_targets;
};


struct ItemJobApplicability
{
	uint32_t rsv1 : 1; // Bit 0
	uint32_t war : 1;  // Bit 1
	uint32_t mnk : 1;  // Bit 2
	uint32_t whm : 1;  // Bit 3
	uint32_t blm : 1;  // Bit 4
	uint32_t rdm : 1;  // Bit 5
	uint32_t thf : 1;  // Bit 6
	uint32_t pld : 1;  // Bit 7

	uint32_t drk : 1;  // Bit 8
	uint32_t bst : 1;  // Bit 9
	uint32_t brd : 1;  // Bit 10
	uint32_t rng : 1;  // Bit 11
	uint32_t sam : 1;  // Bit 12
	uint32_t nin : 1;  // Bit 13
	uint32_t drg : 1;  // Bit 14
	uint32_t smn : 1;  // Bit 15

	uint32_t blu : 1;  // Bit 16
	uint32_t cor : 1;  // Bit 17
	uint32_t pup : 1;  // Bit 18
	uint32_t dnc : 1;  // Bit 19
	uint32_t sch : 1;  // Bit 20
	uint32_t geo : 1;  // Bit 21
	uint32_t run : 1;  // Bit 22
	uint32_t mon : 1;  // Bit 23
	uint32_t rsv2 : 8; // Remaining bits
};

struct ItemEquipSlot
{
	uint16_t main_hand : 1;
	uint16_t sub_hand : 1;
	uint16_t ranged : 1;
	uint16_t ammo : 1;
	uint16_t head : 1;
	uint16_t body : 1;
	uint16_t hands : 1;
	uint16_t legs : 1;

	uint16_t feet : 1;
	uint16_t neck : 1;
	uint16_t waist : 1;
	uint16_t left_ear : 1;
	uint16_t right_ear : 1;
	uint16_t left_ring : 1;
	uint16_t right_ring : 1;
	uint16_t back : 1;
};

struct ItemRaceApplicability
{
	uint16_t None : 1;
	uint16_t HumeMale : 1;
	uint16_t HumeFemale : 1;
	uint16_t ElvaanMale : 1;
	uint16_t ElvaanFemale : 1;
	uint16_t TaruMale : 1;
	uint16_t TaruFemale : 1;
	uint16_t Mithra : 1;
	uint16_t Galka : 1;
	uint16_t Rsv : 7;
};

struct ItemArmourSpec
{
	int16_t level;
	ItemEquipSlot equip_slots;
	ItemRaceApplicability equip_races;
	ItemJobApplicability equip_jobs;
	uint16_t slvl;
	uint16_t shield_size;
	uint8_t max_charges;
	uint8_t cast_factor; // 使用后到效果生效的延迟时间系数，1/4秒，动画硬直时间
	uint16_t use_time;
	uint16_t reuse_time;
	uint16_t ukn1;
	uint16_t related_item_id;
	uint16_t ilvl;
	uint16_t ukn3;
	uint16_t ukn4;
	Record info_rec;
};

struct ItemPuppetSlot
{
	uint16_t head : 1;
	uint16_t body : 1;
	uint16_t attachment : 1;
	uint16_t rsv : 13;
	uint32_t ukn1;
	uint32_t ukn2;
	Record info_rec;
};

struct ItemPuppetSpec
{
	ItemPuppetSlot equip_slots;
	uint8_t fire : 4;
	uint8_t ice : 4;
	uint8_t air : 4;
	uint8_t earth : 4;
	uint8_t thunder : 4;
	uint8_t water : 4;
	uint8_t light : 4;
	uint8_t dark : 4;
	uint8_t ukn;
	uint16_t ukn2;
	uint16_t ukn3;
	Record info_rec;
};

struct ItemNormalSpec
{
	int16_t element;
	int16_t storage;
	int16_t related_item_id;
	int16_t ukn4;
	int16_t ukn5;
	Record info_rec;
};

struct ItemUsableSpec
{
	int16_t cast_factor;
	int32_t ukn1;
	int32_t ukn2;
	int32_t ukn3;

	Record info_rec;
};

struct ItemUsableOldSpec
{
	int16_t cast_factor;
	int32_t ukn1;
	int32_t ukn2;

	Record info_rec;
};

struct ItemWeaponSpec
{
	uint16_t level;
	ItemEquipSlot equip_slots;
	ItemRaceApplicability races;
	ItemJobApplicability jobs;
	uint16_t slvl;

	uint16_t ukn2;
	uint16_t dmg;
	uint16_t delay;
	uint16_t dps;
	uint8_t skill;
	uint8_t ukn12;
	uint16_t ukn7;
	uint16_t ukn9;

	uint8_t max_charges;
	uint8_t cast_factor; // 使用后到效果生效的延迟时间系数，1/4秒，动画硬直时间
	uint16_t use_time;
	uint16_t reuse_time;
	uint16_t ukn20;
	uint16_t related_item_id;
	uint16_t ilvl;
	uint16_t ukn22;
	uint16_t ukn23;

	Record info_rec;
};

enum class SkillType : uint8_t
{
	None = 0,
	HandToHand = 1,
	Dagger = 2,
	Sword = 3,
	GreatSword = 4,
	Axe = 5,
	GreatAxe = 6,
	Scythe = 7,
	Polearm = 8,
	Katana = 9,
	GreatKatana = 10,
	Club = 11,
	Staff = 12,
	Weapon12 = 13,
	Weapon11 = 14,
	Weapon10 = 15,
	Weapon9 = 16,
	Weapon8 = 17,
	Weapon7 = 18,
	Weapon6 = 19,
	Weapon5 = 20,
	Weapon4 = 21,
	AutomatonMelee = 22,
	AutomatonArchery = 23,
	AutomatonMagic = 24,
	Archery = 25,
	Marksmanship = 26,
	Throwing = 27,
	Guard = 28,
	Evasion = 29,
	Shield = 30,
	Parrying = 31,
	DivineMagic = 32,
	HealingMagic = 33,
	EnhancingMagic = 34,
	EnfeeblingMagic = 35,
	ElementalMagic = 36,
	DarkMagic = 37,
	SummoningMagic = 38,
	Ninjutsu = 39,
	Singing = 40,
	StringedInstrument = 41,
	WindInstrument = 42,
	BlueMagic = 43,
	Geomancy = 44,
	Handbell = 45,
	Magic2 = 46,
	Magic1 = 47,
	Fishing = 48,
	Woodworking = 49,
	Smithing = 50,
	Goldsmithing = 51,
	Clothcraft = 52,
	Leatherworking = 53,
	Bonecraft = 54,
	Alchemy = 55,
	Cooking = 56,
	Synergy = 57,
	Synthesis6 = 58,
	Synthesis5 = 59,
	Synthesis4 = 60,
	Synthesis3 = 61,
	Synthesis2 = 62,
	Synthesis1 = 63
};

struct ItemSlipSpec
{
	uint8_t ukn[70];
	Record info_rec;
};

struct ItemInstinctSpec
{
	uint16_t ukn[13];

	Record info_rec;
};

struct ItemCurrencySpec
{
	uint16_t ukn;
	Record info_rec;
};

union ItemSpecData
{
	char raw[626];
	ItemArmourSpec armour;
	ItemNormalSpec normal;
	ItemUsableSpec usable;
	ItemPuppetSpec puppet;
	ItemWeaponSpec weapon;
	ItemSlipSpec slip;
	ItemCurrencySpec currency;
	ItemInstinctSpec instinct;
};

struct ItemEntry
{
	ItemHeader header;
	ItemSpecData spec;
	uint32_t image_length;
	char image_data[2427]; // 不确定是否是定数
	uint8_t end_marker; // must be 0xFF
};

#pragma pack(pop)

// Enum to specify the type of item spec data
enum class ItemSpecType {
	NORMAL,
	USABLE,
	WEAPON,
	ARMOUR,
	PUPPET,
	SLIP,
	CURRENCY,  // ?? Special: Currency files have exactly 1 entry and fixed 0xC000 byte size
	INSTINCT,
};

class ItemData
{
public:
	using ValidTarget = uint16_t; // bitmask for valid target types, e.g. player, NPC, etc.
	const ValidTarget VT_SELF = 0x0001,
		VT_PLAYER = 0x0002,
		VT_PARTY = 0x0004,
		VT_ALLY = 0x0008,
		VT_NPC = 0x0010,
		VT_ENEMY = 0x0020,
		VT_CORPSE = 0x0080;

	bool encryptionSuppression = false;

	class ItemDatum
	{
	public:
		uint32_t id;
		Image image;
		
		// Store the complete original entry to preserve ALL fields including unknown ones
		ItemEntry originalEntry;
		
		// Store the original Row structure - THIS IS THE SOURCE OF TRUTH for text fields
		// Row doesn't know what it contains - ItemDatum is responsible for interpreting it
		Row originalRow;
		bool hasOriginalRow = false;
		
		// Optional: for debugging/logging only, not used in core logic
		RecordFormat recordFormat = RecordFormat::Unknown;
		
		// Store the spec type for this item
		ItemSpecType spec_type;
		
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
		
		// ============ Existing Accessors (unchanged) ============
		
		uint16_t& stack_size() { return originalEntry.header.stack_size; }
		const uint16_t& stack_size() const { return originalEntry.header.stack_size; }
		uint16_t& item_type() { return originalEntry.header.item_type; }
		const uint16_t& item_type() const { return originalEntry.header.item_type; }
		uint16_t& resource_id() { return originalEntry.header.resource_id; }
		const uint16_t& resource_id() const { return originalEntry.header.resource_id; }
		uint16_t& valid_targets() { return originalEntry.header.valid_targets; }
		const uint16_t& valid_targets() const { return originalEntry.header.valid_targets; }
		
		ItemHeader& flags() { return originalEntry.header; }
		const ItemHeader& flags() const { return originalEntry.header; }
		
		ItemDatum() : originalEntry{}, spec_type(ItemSpecType::NORMAL)
		{
			originalEntry.header.id = 0;
			originalEntry.header.stack_size = 1;
			originalEntry.header.item_type = 0;
			originalEntry.header.resource_id = 0;
			originalEntry.header.valid_targets = 0;
			originalEntry.image_length = 0;
			originalEntry.end_marker = 0xFF;
			memset(&originalEntry.spec, 0, sizeof(originalEntry.spec));
			memset(originalEntry.image_data, 0, sizeof(originalEntry.image_data));
		}
	};

	void Read(std::wstring path, ItemSpecType defaultSpecType = ItemSpecType::NORMAL);
	void Write(std::wstring path);

	// For inspection only, not designed for full fidelity round-trip
	void ToICsv(const std::wstring &path) const;
	std::vector<ItemDatum> data;
};
