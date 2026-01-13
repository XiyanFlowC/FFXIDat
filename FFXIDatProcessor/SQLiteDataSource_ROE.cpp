#include "SQLiteDataSource.h"
#include "RecordsOfEminence.h"
#include "xystring.h"
#include "DataManager.h"

// ============================================
// ROM/307/15 - Quest Entry Support (type: erc)
// ============================================

void SQLiteDataSource::ImportRoeQuestDat(const int file_id, const std::wstring &path)
{
    sqlite3_stmt *stmt = nullptr;
    
    RecordsOfEminence roe;
    roe.ReadQuest(path);
    
    for (const auto &datum : roe.questData) {
        int roe_record_id = -1;
        try
        {
            roe_record_id = InsertOrGetRoeQuestRecord(file_id, datum.id);
        }
        catch (SQLException &ex)
        {
            Ring(xybase::string::to_utf8(std::string("Failed to insert or get ROE Quest record for ID ") + std::to_string(datum.id) + ": " + ex.what()).c_str());
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
                int quest_name_text_id = InsertOrGetText(xybase::string::escape(qName));
                if (sqlite3_prepare_v2(db, "UPDATE roe_quest SET quest_name_text_id = ? WHERE id = ?", -1, &stmt, nullptr) == SQLITE_OK)
                {
                    sqlite3_bind_int(stmt, 1, quest_name_text_id);
                    sqlite3_bind_int(stmt, 2, roe_record_id);
                    sqlite3_step(stmt);
                }
                sqlite3_finalize(stmt);
            }
        } catch (...) { /* Ignore missing fields */ }
        
        // Insert description text if not empty
        try {
            std::u8string desc = datum.description();
            if (!desc.empty()) {
                int description_text_id = InsertOrGetText(xybase::string::escape(desc));
                if (sqlite3_prepare_v2(db, "UPDATE roe_quest SET description_text_id = ? WHERE id = ?", -1, &stmt, nullptr) == SQLITE_OK)
                {
                    sqlite3_bind_int(stmt, 1, description_text_id);
                    sqlite3_bind_int(stmt, 2, roe_record_id);
                    sqlite3_step(stmt);
                }
                sqlite3_finalize(stmt);
            }
        } catch (...) { /* Ignore missing fields */ }
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
    
    RecordsOfEminence roe;
    
    // Read original data first (preserves all game configuration fields)
    roe.ReadQuest(datPath);
    
    sqlite3_stmt *stmt = nullptr;
    
    // Get translations for each entry (only translate text fields)
    for (auto &datum : roe.questData) {
        // Get quest name translation
        if (sqlite3_prepare_v2(db, 
            "SELECT tr.text FROM roe_quest rq "
            "JOIN text t ON rq.quest_name_text_id = t.id "
            "JOIN trans tr ON t.id = tr.text_id "
            "WHERE rq.file_id = ? AND rq.roe_id = ?", 
            -1, &stmt, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_int(stmt, 1, file_id);
            sqlite3_bind_int(stmt, 2, datum.id);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* translatedName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                if (translatedName) {
                    std::u8string transName(reinterpret_cast<const char8_t*>(translatedName));
                    datum.setQuestName(transName);
                }
            }
        }
        sqlite3_finalize(stmt);
        
        // Get description translation
        if (sqlite3_prepare_v2(db, 
            "SELECT tr.text FROM roe_quest rq "
            "JOIN text t ON rq.description_text_id = t.id "
            "JOIN trans tr ON t.id = tr.text_id "
            "WHERE rq.file_id = ? AND rq.roe_id = ?", 
            -1, &stmt, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_int(stmt, 1, file_id);
            sqlite3_bind_int(stmt, 2, datum.id);
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

int SQLiteDataSource::InsertOrGetRoeQuestRecord(int file_id, uint32_t roe_id)
{
    sqlite3_stmt *stmt = nullptr;
    int record_id = -1;
    
    // Try to get existing record
    if (sqlite3_prepare_v2(db, "SELECT id FROM roe_quest WHERE file_id = ? AND roe_id = ?", -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, file_id);
        sqlite3_bind_int(stmt, 2, roe_id);
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            record_id = sqlite3_column_int(stmt, 0);
        }
    }
    sqlite3_finalize(stmt);
    
    if (record_id == -1)
    {
        // Insert new record with default values for quest configuration fields
        const char *insertSQL = R"(
            INSERT INTO roe_quest 
            (file_id, roe_id, roe_release_date, repeatable, target_count, emi_reward, exp_reward, cap_reward, uni_reward) 
            VALUES (?, ?, 0, 0, 0, 0, 0, 0, 0)
        )";
        
        if (sqlite3_prepare_v2(db, insertSQL, -1, &stmt, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_int(stmt, 1, file_id);
            sqlite3_bind_int(stmt, 2, roe_id);
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
    
    RecordsOfEminence roe;
    roe.ReadCategory(path);
    
    for (const auto &datum : roe.categoryData) {
        int roe_record_id = -1;
        try
        {
            roe_record_id = InsertOrGetRoeCategoryRecord(file_id, datum.id);
        }
        catch (SQLException &ex)
        {
            Ring(xybase::string::to_utf8(std::string("Failed to insert or get ROE Category record for ID ") + std::to_string(datum.id) + ": " + ex.what()).c_str());
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
                int category_name_text_id = InsertOrGetText(xybase::string::escape(catName));
                if (sqlite3_prepare_v2(db, "UPDATE roe_category SET category_name_text_id = ? WHERE id = ?", -1, &stmt, nullptr) == SQLITE_OK)
                {
                    sqlite3_bind_int(stmt, 1, category_name_text_id);
                    sqlite3_bind_int(stmt, 2, roe_record_id);
                    sqlite3_step(stmt);
                }
                sqlite3_finalize(stmt);
            }
        } catch (...) { /* Ignore missing fields */ }
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
    
    RecordsOfEminence roe;
    
    // Read original data first (preserves all children relationships and other fields)
    roe.ReadCategory(datPath);
    
    sqlite3_stmt *stmt = nullptr;
    
    // Get translations for each entry (only translate text fields)
    for (auto &datum : roe.categoryData) {
        // Get category name translation
        if (sqlite3_prepare_v2(db, 
            "SELECT tr.text FROM roe_category rc "
            "JOIN text t ON rc.category_name_text_id = t.id "
            "JOIN trans tr ON t.id = tr.text_id "
            "WHERE rc.file_id = ? AND rc.roe_id = ?", 
            -1, &stmt, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_int(stmt, 1, file_id);
            sqlite3_bind_int(stmt, 2, datum.id);
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

int SQLiteDataSource::InsertOrGetRoeCategoryRecord(int file_id, uint32_t roe_id)
{
    sqlite3_stmt *stmt = nullptr;
    int record_id = -1;
    
    // Try to get existing record
    if (sqlite3_prepare_v2(db, "SELECT id FROM roe_category WHERE file_id = ? AND roe_id = ?", -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, file_id);
        sqlite3_bind_int(stmt, 2, roe_id);
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            record_id = sqlite3_column_int(stmt, 0);
        }
    }
    sqlite3_finalize(stmt);
    
    if (record_id == -1)
    {
        // Insert new record
        if (sqlite3_prepare_v2(db, "INSERT INTO roe_category (file_id, roe_id) VALUES (?, ?)", -1, &stmt, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_int(stmt, 1, file_id);
            sqlite3_bind_int(stmt, 2, roe_id);
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
