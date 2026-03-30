#pragma once

/* Using sqlite, 3.48.0 */
#include "sqlite3/sqlite3.h"
#include <string>
#include <exception>
#include "CsvFile.h"

// Forward declarations for ItemData structures
struct ItemWeaponSpec;
struct ItemArmourSpec;
struct ItemUsableSpec;
struct ItemNormalSpec;
struct ItemEquipSlot;
struct ItemRaceApplicability;
struct ItemJobApplicability;

class SQLException : public std::runtime_error {
public:
	explicit SQLException(const std::string &msg) : std::runtime_error(msg.c_str()) {}

	explicit SQLException(const char *msg) : std::runtime_error(msg) {}
};

class SQLiteDataSource
{
	sqlite3 *db;
	void (*ring)(const char8_t *msg);

	void Ring(const char8_t *msg);
public:
	SQLiteDataSource();
	~SQLiteDataSource();
	
	void Initialise();

	void InitialiseFileDefinition(CsvFile &csv);

	void DumpTranslationData();

	void ExportNoTranslation();

	void ImportTranslation();

	void Purge();

	void DropFile(const char *path);

	void DatToDatabase(const char *lang, const char *type, const char *path);

	void ImportDat(const std::string &path, const std::string &type);

	void TransAndOut();

    std::u8string GetTranslation(const std::u8string &text);

	void Execute(const std::string &qry);

	void SetRing(void (*callback)(const char8_t *msg));
protected:

	void InsertText(const char * text, int file_id, int rowNum, int colNum);

	void TranslateDat(int file_id, const char *file_path, const char *type);
	std::u8string GetFileLang(int file_id);

	// ItemData support methods
	void ImportItemDat(const int file_id, const std::wstring &path, const std::wstring &type);
	void TranslateItemDat(int file_id, const wchar_t *file_path, const char *type);
	int InsertOrGetItemRecord(int file_id, uint32_t item_id, const std::wstring &type);
	int InsertOrGetText(const std::u8string &text);
	
	// Spec-specific insertion methods
	void InsertWeaponSpec(int item_id, const ItemWeaponSpec &spec);
	void InsertArmourSpec(int item_id, const ItemArmourSpec &spec);
	void InsertUsableSpec(int item_id, const ItemUsableSpec &spec);
	void InsertNormalSpec(int item_id, const ItemNormalSpec &spec);
	void InsertEquipSlots(int item_id, const ItemEquipSlot &slots);
	void InsertRaceApplicability(int item_id, const ItemRaceApplicability &races);
	void InsertJobApplicability(int item_id, const ItemJobApplicability &jobs);
	
	// MonBridge support methods
	void ImportMonBridgeDat(const int file_id, const std::wstring &path);
	void TranslateMonBridgeDat(int file_id, const wchar_t *file_path);
	int InsertOrGetMonBridgeRecord(int file_id, uint32_t mb_id);
	
	// RecordsOfEminence support methods
	void ImportRoeCategoryDat(const int file_id, const std::wstring &path);
	void TranslateRoeCategoryDat(int file_id, const wchar_t *file_path);
	int InsertOrGetRoeCategoryRecord(uint32_t roe_id);

	// Quest/Mission DMsg support methods
	int InsertOrGetQuestDMsgRecord(const std::u8string &category, int quest_id);
	void UpdateQuestDMsgRecord(const std::u8string &lang, int record_id, const std::u8string &name, const std::u8string &description);
	
	void ImportRoeQuestDat(const int file_id, const std::wstring &path);
	void TranslateRoeQuestDat(int file_id, const wchar_t *file_path);
	int InsertOrGetRoeQuestRecord(uint32_t roe_id);
};

