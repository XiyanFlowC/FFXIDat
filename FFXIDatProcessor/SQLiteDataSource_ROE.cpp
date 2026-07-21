#include "SQLiteDataSource.h"
#include "RecordsOfEminence.h"
#include "xystring.h"
#include "DataManager.h"

namespace
{
	bool IsRoeLang(const std::u8string &lang)
	{
		return lang == u8"ja" || lang == u8"en";
	}

	const char *GetRoeQuestNameColumn(const std::u8string &lang)
	{
		return lang == u8"ja" ? "quest_name_ja_text_id" : "quest_name_en_text_id";
	}

	const char *GetRoeQuestDescriptionColumn(const std::u8string &lang)
	{
		return lang == u8"ja" ? "description_ja_text_id" : "description_en_text_id";
	}

	const char* GetRoeQuestNoteColumn(const std::u8string& lang)
	{
		return lang == u8"ja" ? "note_ja_text_id" : "note_en_text_id";
	}

	const char *GetRoeCategoryNameColumn(const std::u8string &lang)
	{
		return lang == u8"ja" ? "category_name_ja_text_id" : "category_name_en_text_id";
	}
}

// ============================================
// ROM/307/15 - Quest Entry Support (type: erc)
// ============================================

void SQLiteDataSource::ImportRoeQuestDat(const int file_id, const std::wstring &path)
{
	sqlite3_stmt *stmt = nullptr;
	std::u8string fileLang = GetFileLang(file_id);
	
	RecordsOfEminence roe;
	roe.ReadQuest(path);
	
	int rowCounter = 1;
	for (const auto &datum : roe.questData) {
		int roe_record_id = -1;
		try
		{
			roe_record_id = InsertOrGetRoeQuestRecord(datum.id);
		}
		catch (SQLException &ex)
		{
			Ring(xybase::string::to_utf8(std::string("Failed to insert or get ROE Quest record for ID ") + std::to_string(datum.id) + ": " + ex.what()).c_str());
			rowCounter++;
			continue;
		}
		
		// Update main roe_quest table with all quest fields
		const char *updateMainSQL = R"(
			UPDATE roe_quest SET 
				roe_release_date = ?,
				repeatable = ?,
				target_count = ?,
				emi_reward = ?,
				exp_reward = ?,
				cap_reward = ?,
				uni_reward = ?
			WHERE id = ?
		)";
		
		if (sqlite3_prepare_v2(db, updateMainSQL, -1, &stmt, nullptr) == SQLITE_OK)
		{
			sqlite3_bind_int(stmt, 1, datum.release_date);
			sqlite3_bind_int(stmt, 2, datum.originalEntry.repeatable);
			sqlite3_bind_int(stmt, 3, datum.originalEntry.target_count);
			sqlite3_bind_int(stmt, 4, datum.originalEntry.emi_reward);
			sqlite3_bind_int(stmt, 5, datum.originalEntry.exp_reward);
			sqlite3_bind_int(stmt, 6, datum.originalEntry.cap_reward);
			sqlite3_bind_int(stmt, 7, datum.originalEntry.uni_reward);
			sqlite3_bind_int(stmt, 8, roe_record_id);
			sqlite3_step(stmt);
		}
		sqlite3_finalize(stmt);
		
		// Insert quest name text if not empty
		try {
			std::u8string qName = datum.questName();
			if (!qName.empty()) {
				int col = 1; 
				int row = rowCounter;

				for (auto &cell : datum.row()) {
					if (cell.GetType() == 0) {
						InsertText(reinterpret_cast<const char*>(xybase::string::escape(cell.Get<std::u8string>()).c_str()), file_id, row, col);
					}
					col++;
				}

				if (IsRoeLang(fileLang))
				{
					int quest_name_text_id = InsertOrGetText(xybase::string::escape(qName));
					std::string sql = std::string("UPDATE roe_quest SET ") + GetRoeQuestNameColumn(fileLang) + " = ? WHERE id = ?";
					if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
					{
						sqlite3_bind_int(stmt, 1, quest_name_text_id);
						sqlite3_bind_int(stmt, 2, roe_record_id);
						sqlite3_step(stmt);
					}
					sqlite3_finalize(stmt);
				}
			}
			else if (IsRoeLang(fileLang))
			{
				std::string sql = std::string("UPDATE roe_quest SET ") + GetRoeQuestNameColumn(fileLang) + " = NULL WHERE id = ?";
				if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
				{
					sqlite3_bind_int(stmt, 1, roe_record_id);
					sqlite3_step(stmt);
				}
				sqlite3_finalize(stmt);
			}
		} catch (...) { /* Ignore missing fields */ }
		
		// Insert description text if not empty
		try {
			std::u8string desc = datum.description();
			if (!desc.empty()) {
				std::string descStr = reinterpret_cast<const char*>(xybase::string::escape(desc).c_str());
				if (IsRoeLang(fileLang))
				{
					int description_text_id = InsertOrGetText(xybase::string::escape(desc));
					std::string sql = std::string("UPDATE roe_quest SET ") + GetRoeQuestDescriptionColumn(fileLang) + " = ? WHERE id = ?";
					if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
					{
						sqlite3_bind_int(stmt, 1, description_text_id);
						sqlite3_bind_int(stmt, 2, roe_record_id);
						sqlite3_step(stmt);
					}
					sqlite3_finalize(stmt);
				}
			}
			else if (IsRoeLang(fileLang))
			{
				std::string sql = std::string("UPDATE roe_quest SET ") + GetRoeQuestDescriptionColumn(fileLang) + " = NULL WHERE id = ?";
				if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
				{
					sqlite3_bind_int(stmt, 1, roe_record_id);
					sqlite3_step(stmt);
				}
				sqlite3_finalize(stmt);
			}
		} catch (...) { /* Ignore missing fields */ }

		// Insert note text if not empty
		try {
			std::u8string note = datum.note();
			if (!note.empty()) {
				std::string descStr = reinterpret_cast<const char*>(xybase::string::escape(note).c_str());
				if (IsRoeLang(fileLang))
				{
					int description_text_id = InsertOrGetText(xybase::string::escape(note));
					std::string sql = std::string("UPDATE roe_quest SET ") + GetRoeQuestNoteColumn(fileLang) + " = ? WHERE id = ?";
					if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
					{
						sqlite3_bind_int(stmt, 1, description_text_id);
						sqlite3_bind_int(stmt, 2, roe_record_id);
						sqlite3_step(stmt);
					}
					sqlite3_finalize(stmt);
				}
			}
			else {
				std::string sql = std::string("UPDATE roe_quest SET ") + GetRoeQuestNoteColumn(fileLang) + " = NULL WHERE id = ?";
				if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
				{
					sqlite3_bind_int(stmt, 1, roe_record_id);
					sqlite3_step(stmt);
				}
				sqlite3_finalize(stmt);
			}
		}
		catch (...) { /* Ignore missing fields */ }
		
		rowCounter++;
	}
}

void SQLiteDataSource::TranslateRoeQuestDat(int file_id, const wchar_t *file_path)
{
	std::wstring inputPath = file_path;
	if (!inputPath.ends_with(L".DAT")) {
		inputPath += L".DAT";
	}
	auto datPath = PathUtil::GetPath(inputPath);
	auto outPath = PathUtil::GetOutPathConf(inputPath);
	std::u8string fileLang = GetFileLang(file_id);
	
	RecordsOfEminence roe;
	
	// Read original data first (preserves all game configuration fields)
	roe.ReadQuest(datPath);
	if (!IsRoeLang(fileLang))
	{
		roe.WriteQuest(outPath);
		return;
	}
	
	sqlite3_stmt *stmt = nullptr;
	
	// Get translations for each entry (only translate text fields)
	for (auto &datum : roe.questData) {
		std::string nameSql = std::string("SELECT tr.text FROM roe_quest rq ")
			+ "JOIN text t ON rq." + GetRoeQuestNameColumn(fileLang) + " = t.id "
			+ "JOIN trans tr ON t.id = tr.text_id "
			+ "WHERE rq.roe_id = ?";
		if (sqlite3_prepare_v2(db, nameSql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
		{
			sqlite3_bind_int(stmt, 1, datum.id);
			if (sqlite3_step(stmt) == SQLITE_ROW) {
				const char* translatedName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
				if (translatedName) {
					std::u8string transName(reinterpret_cast<const char8_t*>(translatedName));
					datum.setQuestName(transName);
				}
			}
		}
		sqlite3_finalize(stmt);
		
		std::string descSql = std::string("SELECT tr.text FROM roe_quest rq ")
			+ "JOIN text t ON rq." + GetRoeQuestDescriptionColumn(fileLang) + " = t.id "
			+ "JOIN trans tr ON t.id = tr.text_id "
			+ "WHERE rq.roe_id = ?";
		if (sqlite3_prepare_v2(db, descSql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
		{
			sqlite3_bind_int(stmt, 1, datum.id);
			if (sqlite3_step(stmt) == SQLITE_ROW) {
				const char* translatedDesc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
				if (translatedDesc) {
					std::u8string transDesc(reinterpret_cast<const char8_t*>(translatedDesc));
					datum.setDescription(transDesc);
				}
			}
		}
		sqlite3_finalize(stmt);
	}
	
	// Write with translated text but original game configuration
	roe.WriteQuest(outPath);
}

int SQLiteDataSource::InsertOrGetRoeQuestRecord(uint32_t roe_id)
{
	sqlite3_stmt *stmt = nullptr;
	int record_id = -1;
	
	// Try to get existing record
	if (sqlite3_prepare_v2(db, "SELECT id FROM roe_quest WHERE roe_id = ?", -1, &stmt, nullptr) == SQLITE_OK)
	{
		sqlite3_bind_int(stmt, 1, roe_id);
		if (sqlite3_step(stmt) == SQLITE_ROW)
		{
			record_id = sqlite3_column_int(stmt, 0);
		}
	}
	sqlite3_finalize(stmt);
	stmt = nullptr;
	
	if (record_id == -1)
	{
		// Insert new record with default values for quest configuration fields
		const char *insertSQL = R"(
			INSERT INTO roe_quest 
			(roe_id, roe_release_date, repeatable, target_count, emi_reward, exp_reward, cap_reward, uni_reward) 
			VALUES (?, 0, 0, 0, 0, 0, 0, 0)
		)";
		
		if (sqlite3_prepare_v2(db, insertSQL, -1, &stmt, nullptr) == SQLITE_OK)
		{
			sqlite3_bind_int(stmt, 1, roe_id);
			if (sqlite3_step(stmt) == SQLITE_DONE)
			{
				record_id = static_cast<int>(sqlite3_last_insert_rowid(db));
			}
			else
			{
				sqlite3_finalize(stmt);
				throw SQLException(sqlite3_errmsg(db));
			}
		}
		else
		{
			throw SQLException(sqlite3_errmsg(db));
		}
		sqlite3_finalize(stmt);
	}
	
	return record_id;
}

// ================================================
// ROM/307/23 - Category Entry Support (type: erq)
// ================================================

void SQLiteDataSource::ImportRoeCategoryDat(const int file_id, const std::wstring &path)
{
	sqlite3_stmt *stmt = nullptr;
	std::u8string fileLang = GetFileLang(file_id);
	
	RecordsOfEminence roe;
	roe.ReadCategory(path);
	
	int rowCounter = 1;
	for (const auto &datum : roe.categoryData) {
		int roe_record_id = -1;
		try
		{
			roe_record_id = InsertOrGetRoeCategoryRecord(datum.id);
		}
		catch (SQLException &ex)
		{
			Ring(xybase::string::to_utf8(std::string("Failed to insert or get ROE Category record for ID ") + std::to_string(datum.id) + ": " + ex.what()).c_str());
			rowCounter++;
			continue;
		}
		
		// Delete existing children entries for this category
		if (sqlite3_prepare_v2(db, "DELETE FROM roe_category_children WHERE category_id = ?", -1, &stmt, nullptr) == SQLITE_OK)
		{
			sqlite3_bind_int(stmt, 1, roe_record_id);
			sqlite3_step(stmt);
		}
		sqlite3_finalize(stmt);
		
		// Insert all children relationships
		const char *insertChildSQL = R"(
			INSERT INTO roe_category_children 
			(category_id, child_index, child_id, quest_flag, ukn1, ukn2, ukn3)
			VALUES (?, ?, ?, ?, ?, ?, ?)
		)";
		
		if (sqlite3_prepare_v2(db, insertChildSQL, -1, &stmt, nullptr) == SQLITE_OK)
		{
			for (uint32_t i = 0; i < datum.originalEntry.count_of_children && i < 20; ++i)
			{
				const auto &child = datum.originalEntry.children[i];
				
				sqlite3_bind_int(stmt, 1, roe_record_id);
				sqlite3_bind_int(stmt, 2, i); // child_index
				sqlite3_bind_int(stmt, 3, child.child_id);
				sqlite3_bind_int(stmt, 4, child.quest_flag);
				sqlite3_bind_int(stmt, 5, child.ukn[0]);
				sqlite3_bind_int(stmt, 6, child.ukn[1]);
				sqlite3_bind_int(stmt, 7, child.ukn[2]);
				
				if (sqlite3_step(stmt) != SQLITE_DONE)
				{
					Ring(xybase::string::to_utf8(std::string("Failed to insert child relationship for category ") + std::to_string(datum.id)).c_str());
				}
				
				sqlite3_reset(stmt);
			}
		}
		sqlite3_finalize(stmt);
		
		// Insert category name text if not empty
		try {
			std::u8string catName = datum.categoryName();
			if (!catName.empty()) {
				int col = 1; 
				int row = rowCounter;

				for (auto& cell : datum.row())
				{
					if (cell.GetType() == 0) // str
					{
						InsertText(reinterpret_cast<const char*>(xybase::string::escape(cell.Get<std::u8string>()).c_str()), file_id, row, col);
					}
					col++;
				}

				if (IsRoeLang(fileLang))
				{
					int category_name_text_id = InsertOrGetText(xybase::string::escape(catName));
					std::string sql = std::string("UPDATE roe_category SET ") + GetRoeCategoryNameColumn(fileLang) + " = ? WHERE id = ?";
					if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
					{
						sqlite3_bind_int(stmt, 1, category_name_text_id);
						sqlite3_bind_int(stmt, 2, roe_record_id);
						sqlite3_step(stmt);
					}
					sqlite3_finalize(stmt);
				}
			}
			else if (IsRoeLang(fileLang))
			{
				std::string sql = std::string("UPDATE roe_category SET ") + GetRoeCategoryNameColumn(fileLang) + " = NULL WHERE id = ?";
				if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
				{
					sqlite3_bind_int(stmt, 1, roe_record_id);
					sqlite3_step(stmt);
				}
				sqlite3_finalize(stmt);
			}
		} catch (...) { /* Ignore missing fields */ }
		
		rowCounter++;
	}
}

void SQLiteDataSource::TranslateRoeCategoryDat(int file_id, const wchar_t *file_path)
{
	std::wstring inputPath = file_path;
	if (!inputPath.ends_with(L".DAT")) {
		inputPath += L".DAT";
	}
	auto datPath = PathUtil::GetPath(inputPath);
	auto outPath = PathUtil::GetOutPathConf(inputPath);
	std::u8string fileLang = GetFileLang(file_id);
	
	RecordsOfEminence roe;
	
	// Read original data first (preserves all children relationships and other fields)
	roe.ReadCategory(datPath);
	if (!IsRoeLang(fileLang))
	{
		roe.WriteCategory(outPath);
		return;
	}
	
	sqlite3_stmt *stmt = nullptr;
	
	// Get translations for each entry (only translate text fields)
	for (auto &datum : roe.categoryData) {
		std::string sql = std::string("SELECT tr.text FROM roe_category rc ")
			+ "JOIN text t ON rc." + GetRoeCategoryNameColumn(fileLang) + " = t.id "
			+ "JOIN trans tr ON t.id = tr.text_id "
			+ "WHERE rc.roe_id = ?";
		if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
		{
			sqlite3_bind_int(stmt, 1, datum.id);
			if (sqlite3_step(stmt) == SQLITE_ROW) {
				const char* translatedName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
				if (translatedName) {
					std::u8string transName(reinterpret_cast<const char8_t*>(translatedName));
					datum.setCategoryName(transName);
				}
			}
		}
		sqlite3_finalize(stmt);
	}
	
	// Write with translated text but original children relationships
	roe.WriteCategory(outPath);
}

int SQLiteDataSource::InsertOrGetRoeCategoryRecord(uint32_t roe_id)
{
	sqlite3_stmt *stmt = nullptr;
	int record_id = -1;
	
	// Try to get existing record
	if (sqlite3_prepare_v2(db, "SELECT id FROM roe_category WHERE roe_id = ?", -1, &stmt, nullptr) == SQLITE_OK)
	{
		sqlite3_bind_int(stmt, 1, roe_id);
		if (sqlite3_step(stmt) == SQLITE_ROW)
		{
			record_id = sqlite3_column_int(stmt, 0);
		}
	}
	sqlite3_finalize(stmt);
	
	if (record_id == -1)
	{
		// Insert new record
		if (sqlite3_prepare_v2(db, "INSERT INTO roe_category (roe_id) VALUES (?)", -1, &stmt, nullptr) == SQLITE_OK)
		{
			sqlite3_bind_int(stmt, 1, roe_id);
			if (sqlite3_step(stmt) == SQLITE_DONE)
			{
				record_id = static_cast<int>(sqlite3_last_insert_rowid(db));
			}
			else
			{
				sqlite3_finalize(stmt);
				throw SQLException(sqlite3_errmsg(db));
			}
		}
		else
		{
			throw SQLException(sqlite3_errmsg(db));
		}
		sqlite3_finalize(stmt);
	}
	
	return record_id;
}
