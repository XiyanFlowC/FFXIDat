#include "SQLiteDataSource.h"

#include <stdexcept>
#include "xystring.h"
#include <iostream>
#include "DataManager.h"
#include "ItemData.h"
#include "StatusData.h"
#include "MonBridge.h"
#include "RecordsOfEminence.h"

void SQLiteDataSource::Ring(const char8_t *msg)
{
    if (ring) ring(msg);
    else std::wcout << xybase::string::to_wstring(msg) << std::endl;
}

SQLiteDataSource::SQLiteDataSource()
	: db(nullptr), ring(nullptr)
{
    std::filesystem::path gp = PathUtil::progRootPath;
    if (sqlite3_open((gp / "text.db").string().c_str(), &db) != SQLITE_OK) {
		throw std::runtime_error("Cannot open database text.db!!!");
	}
    Execute("PRAGMA foreign_keys = ON;");
}

SQLiteDataSource::~SQLiteDataSource()
{
	if (db) sqlite3_close(db);
}

void SQLiteDataSource::Initialise()
{
	Execute(R"(
        CREATE TABLE IF NOT EXISTS file (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            path TEXT NOT NULL UNIQUE,
            type TEXT NOT NULL,
            lang TEXT NOT NULL,
            comment TEXT,
            cols TEXT
        );
    )");
    Execute(R"(
        CREATE TABLE IF NOT EXISTS text (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            text TEXT NOT NULL UNIQUE
        );
    )");
    Execute(R"(
        CREATE TABLE IF NOT EXISTS rela (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_id INTEGER NOT NULL,
            file_row INTEGER NOT NULL,
            file_col INTEGER NOT NULL,
            text_id INTEGER NOT NULL,
            FOREIGN KEY (file_id) REFERENCES file(id) ON DELETE CASCADE,
            FOREIGN KEY (text_id) REFERENCES text(id)
        );
    )");
    Execute(R"(
        CREATE TABLE IF NOT EXISTS trans (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            text_id INTEGER NOT NULL UNIQUE,
            text TEXT NOT NULL,
            FOREIGN KEY (text_id) REFERENCES text(id) ON DELETE CASCADE
        );
    )");
    
    // Main items table with header fields
    Execute(R"(
        CREATE TABLE IF NOT EXISTS items (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_id INTEGER NOT NULL,
            item_id INTEGER NOT NULL,
            spec_type TEXT NOT NULL,
            name_text_id INTEGER,
            description_text_id INTEGER,
            
            -- Header fields
            stack_size INTEGER,
            item_type INTEGER,
            resource_id INTEGER,
            valid_targets INTEGER,
            
            -- Header flags (flag set 1)
            is_scroll INTEGER,
            is_not_listable INTEGER,
            is_inscribable INTEGER,
            is_alt INTEGER,
            ukn_flg1 INTEGER,
            is_in_mystery_box INTEGER,
            is_gm_item INTEGER,
            is_wall_decoration INTEGER,
            
            -- Header flags (flag set 2)
            is_rare INTEGER,
            is_unsellable INTEGER,
            is_unmailable INTEGER,
            is_ex INTEGER,
            is_equipment INTEGER,
            is_npc_tradeable INTEGER,
            is_usable INTEGER,
            is_linkshell INTEGER,
            
            -- Image data
            image_length INTEGER,
            image_data BLOB,
            end_marker INTEGER,
            
            FOREIGN KEY (file_id) REFERENCES file(id) ON DELETE CASCADE,
            FOREIGN KEY (name_text_id) REFERENCES text(id),
            FOREIGN KEY (description_text_id) REFERENCES text(id),
            UNIQUE(file_id, item_id)
        );
    )");
    
    // Equipment slots table (for weapons and armour)
    Execute(R"(
        CREATE TABLE IF NOT EXISTS item_equip_slots (
            item_id INTEGER PRIMARY KEY,
            main_hand INTEGER,
            sub_hand INTEGER,
            ranged INTEGER,
            ammo INTEGER,
            head INTEGER,
            body INTEGER,
            hands INTEGER,
            legs INTEGER,
            feet INTEGER,
            neck INTEGER,
            waist INTEGER,
            left_ear INTEGER,
            right_ear INTEGER,
            left_ring INTEGER,
            right_ring INTEGER,
            back INTEGER,
            FOREIGN KEY (item_id) REFERENCES items(id) ON DELETE CASCADE
        );
    )");
    
    // Race applicability table (for weapons and armour)
    Execute(R"(
        CREATE TABLE IF NOT EXISTS item_race_applicability (
            item_id INTEGER PRIMARY KEY,
            none INTEGER,
            hume_male INTEGER,
            hume_female INTEGER,
            elvaan_male INTEGER,
            elvaan_female INTEGER,
            taru_male INTEGER,
            taru_female INTEGER,
            mithra INTEGER,
            galka INTEGER,
            rsv INTEGER,
            FOREIGN KEY (item_id) REFERENCES items(id) ON DELETE CASCADE
        );
    )");
    
    // Job applicability table (for weapons and armour)
    Execute(R"(
        CREATE TABLE IF NOT EXISTS item_job_applicability (
            item_id INTEGER PRIMARY KEY,
            pld INTEGER,
            thf INTEGER,
            rdm INTEGER,
            blm INTEGER,
            whm INTEGER,
            mnk INTEGER,
            war INTEGER,
            rsv1 INTEGER,
            smn INTEGER,
            drg INTEGER,
            nin INTEGER,
            sam INTEGER,
            rng INTEGER,
            brd INTEGER,
            bst INTEGER,
            drk INTEGER,
            mon INTEGER,
            run INTEGER,
            geo INTEGER,
            sch INTEGER,
            dnc INTEGER,
            pup INTEGER,
            cor INTEGER,
            blu INTEGER,
            rsv2 INTEGER,
            FOREIGN KEY (item_id) REFERENCES items(id) ON DELETE CASCADE
        );
    )");
    
    // Weapon-specific fields
    Execute(R"(
        CREATE TABLE IF NOT EXISTS item_weapon_spec (
            item_id INTEGER PRIMARY KEY,
            level INTEGER,
            ukn INTEGER,
            ukn2 INTEGER,
            dmg INTEGER,
            delay INTEGER,
            ukn5 INTEGER,
            ukn6 INTEGER,
            ukn12 INTEGER,
            ukn7 INTEGER,
            ukn9 INTEGER,
            max_charges INTEGER,
            cast_factor INTEGER,
            use_time INTEGER,
            reuse_time INTEGER,
            ukn20 INTEGER,
            ukn21 INTEGER,
            ilvl INTEGER,
            ukn22 INTEGER,
            ukn23 INTEGER,
            FOREIGN KEY (item_id) REFERENCES items(id) ON DELETE CASCADE
        );
    )");
    
    // Armour-specific fields
    Execute(R"(
        CREATE TABLE IF NOT EXISTS item_armour_spec (
            item_id INTEGER PRIMARY KEY,
            level INTEGER,
            ukn INTEGER,
            shield_size INTEGER,
            max_charges INTEGER,
            cast_factor INTEGER,
            use_time INTEGER,
            reuse_time INTEGER,
            ukn1 INTEGER,
            ukn2 INTEGER,
            ilvl INTEGER,
            ukn3 INTEGER,
            ukn4 INTEGER,
            FOREIGN KEY (item_id) REFERENCES items(id) ON DELETE CASCADE
        );
    )");
    
    // Usable-specific fields
    Execute(R"(
        CREATE TABLE IF NOT EXISTS item_usable_spec (
            item_id INTEGER PRIMARY KEY,
            cast_factor INTEGER,
            ukn1 INTEGER,
            ukn2 INTEGER,
            ukn3 INTEGER,
            FOREIGN KEY (item_id) REFERENCES items(id) ON DELETE CASCADE
        );
    )");
    
    // Normal-specific fields
    Execute(R"(
        CREATE TABLE IF NOT EXISTS item_normal_spec (
            item_id INTEGER PRIMARY KEY,
            ukn1 INTEGER,
            ukn2 INTEGER,
            ukn3 INTEGER,
            ukn4 INTEGER,
            ukn5 INTEGER,
            FOREIGN KEY (item_id) REFERENCES items(id) ON DELETE CASCADE
        );
    )");
    
    // MonBridge table
    Execute(R"(
        CREATE TABLE IF NOT EXISTS monbridge (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_id INTEGER NOT NULL,
            mb_id INTEGER NOT NULL,
            mb_idx INTEGER NOT NULL,
            display_name_text_id INTEGER,
            
            -- Icon data
            icon_data BLOB,
            
            FOREIGN KEY (file_id) REFERENCES file(id) ON DELETE CASCADE,
            FOREIGN KEY (display_name_text_id) REFERENCES text(id),
            UNIQUE(file_id, mb_id)
        );
    )");
    
    // RecordsOfEminence Quest table (ROM/307/15)
    Execute(R"(
        CREATE TABLE IF NOT EXISTS roe_quest (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_id INTEGER NOT NULL,
            roe_id INTEGER NOT NULL,
            roe_release_date INTEGER NOT NULL,
            quest_name_text_id INTEGER,
            description_text_id INTEGER,
            
            -- Quest configuration fields
            repeatable INTEGER NOT NULL DEFAULT 0,
            target_count INTEGER NOT NULL DEFAULT 0,
            emi_reward INTEGER NOT NULL DEFAULT 0,
            exp_reward INTEGER NOT NULL DEFAULT 0,
            cap_reward INTEGER NOT NULL DEFAULT 0,
            uni_reward INTEGER NOT NULL DEFAULT 0,
            
            FOREIGN KEY (file_id) REFERENCES file(id) ON DELETE CASCADE,
            FOREIGN KEY (quest_name_text_id) REFERENCES text(id),
            FOREIGN KEY (description_text_id) REFERENCES text(id),
            UNIQUE(file_id, roe_id)
        );
    )");
    
    // RecordsOfEminence Category table (ROM/307/23)
    Execute(R"(
        CREATE TABLE IF NOT EXISTS roe_category (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_id INTEGER NOT NULL,
            roe_id INTEGER NOT NULL,
            category_name_text_id INTEGER,
            
            FOREIGN KEY (file_id) REFERENCES file(id) ON DELETE CASCADE,
            FOREIGN KEY (category_name_text_id) REFERENCES text(id),
            UNIQUE(file_id, roe_id)
        );
    )");
    
    // RecordsOfEminence Category-Child relationship table (preserves tree structure)
    Execute(R"(
        CREATE TABLE IF NOT EXISTS roe_category_children (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            category_id INTEGER NOT NULL,
            child_index INTEGER NOT NULL,
            child_id INTEGER NOT NULL,
            quest_flag INTEGER NOT NULL,
            ukn1 INTEGER DEFAULT 0,
            ukn2 INTEGER DEFAULT 0,
            ukn3 INTEGER DEFAULT 0,
            
            FOREIGN KEY (category_id) REFERENCES roe_category(id) ON DELETE CASCADE,
            UNIQUE(category_id, child_index)
        );
    )");
    
    // Index for faster tree traversal
    Execute("CREATE INDEX IF NOT EXISTS idx_roe_category_children_parent ON roe_category_children(category_id);");
    Execute("CREATE INDEX IF NOT EXISTS idx_roe_category_children_child ON roe_category_children(child_id, quest_flag);");
}

void SQLiteDataSource::InitialiseFileDefinition(CsvFile &csv)
{
    sqlite3_stmt *stmt;
    const char *qry = "INSERT INTO file (path, type, lang, comment, cols) VALUES (?, ?, ?, ?, ?);";

    if (sqlite3_prepare_v2(db, qry, -1, &stmt, nullptr) != SQLITE_OK)
    {
        throw SQLException(sqlite3_errmsg(db));
    }

    while (!csv.IsEof())
    {
        auto path = csv.NextCell();
        auto type = csv.NextCell();
        auto lang = csv.NextCell();
        auto comment = csv.NextCell();

        sqlite3_bind_text(stmt, 1, reinterpret_cast<const char *>(path.c_str()), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, reinterpret_cast<const char *>(type.c_str()), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, reinterpret_cast<const char *>(lang.c_str()), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, reinterpret_cast<const char *>(comment.c_str()), -1, SQLITE_TRANSIENT);

        if (!csv.IsEol())
        {
            auto cols = csv.NextCell();
            sqlite3_bind_text(stmt, 5, reinterpret_cast<const char *>(cols.c_str()), -1, SQLITE_TRANSIENT);
        }
        csv.NextLine();

        if (sqlite3_step(stmt) != SQLITE_DONE)
        {
            std::u8string err = u8"Insert definition for " + path + u8" failed." + (char8_t *)sqlite3_errmsg(db);
            Ring(err.c_str());
        }
        sqlite3_reset(stmt);
    }
    sqlite3_finalize(stmt);
}

void SQLiteDataSource::DumpTranslationData()
{
    sqlite3_stmt *stmt = nullptr;

    try
    {
        if (sqlite3_prepare_v2(db, "SELECT text.text, trans.text FROM text JOIN trans ON text.id = trans.text_id;", -1, &stmt, nullptr) != SQLITE_OK) {
            throw SQLException("failed to prepare");
        }

        std::ofstream oPen("text.txt", std::ios::out | std::ios::binary),
            tPen("text_translated.txt", std::ios::out | std::ios::binary);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::u8string text = (const char8_t *)sqlite3_column_text(stmt, 0);
            std::u8string trans = (const char8_t *)sqlite3_column_text(stmt, 1);

            oPen.write((const char *)text.c_str(), text.size());
            oPen.put('\n');
            tPen.write((const char *)trans.c_str(), trans.size());
            tPen.put('\n');
        }

        sqlite3_finalize(stmt);
    }
    catch (SQLException &ex)
    {
        if (stmt) sqlite3_finalize(stmt);
        throw;
    }
}

void SQLiteDataSource::ExportNoTranslation()
{
    sqlite3_stmt *stmt = nullptr;

    try
    {
        if (sqlite3_prepare_v2(db, "SELECT text.text FROM text WHERE text.id NOT IN (SELECT text_id FROM trans);", -1, &stmt, nullptr) != SQLITE_OK) {
            throw SQLException("failed to prepare");
        }

        std::ofstream oPen("text.txt", std::ios::out | std::ios::binary);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::u8string text = (const char8_t *)sqlite3_column_text(stmt, 0);

            oPen.write((const char *)text.c_str(), text.size());
            oPen.put('\n');
        }
    }
    catch (SQLException &ex)
    {
        if (stmt) sqlite3_finalize(stmt);
        throw;
    }
}

void SQLiteDataSource::ImportTranslation()
{
    sqlite3_stmt *qryStmt = nullptr;
    sqlite3_stmt *insStmt = nullptr;

    std::ifstream oEye("text.txt", std::ios::in | std::ios::binary),
        tEye("text_translated.txt", std::ios::in | std::ios::binary);
    Execute("BEGIN;");
    try
    {
        std::string text;
        std::string trans;

        if (sqlite3_prepare_v2(db, "SELECT id FROM text WHERE text = ?;", -1, &qryStmt, nullptr) != SQLITE_OK) {
            throw SQLException(std::string("exist query prepare failed. ") + sqlite3_errmsg(db));
        }

        if (sqlite3_prepare_v2(db, "INSERT INTO trans (text_id, text) VALUES (:text_id, :text) ON CONFLICT(text_id) DO UPDATE SET text = :text WHERE text_id = :text_id;", -1, &insStmt, nullptr) != SQLITE_OK)
        {
            throw SQLException(std::string("insert prepare failed.") + sqlite3_errmsg(db));
        }
        int tidId = sqlite3_bind_parameter_index(insStmt, ":text_id");
        int tId = sqlite3_bind_parameter_index(insStmt, ":text");

        while (std::getline(oEye, text)) {
            if (!std::getline(tEye, trans))
            {
                throw std::runtime_error("text.txt, text_translated.txt number of lines mismatch!!!");
            }
            
            sqlite3_bind_text(qryStmt, 1, text.c_str(), -1, SQLITE_TRANSIENT);

            if (sqlite3_step(qryStmt) != SQLITE_ROW) {

                sqlite3_reset(qryStmt);
                continue;
                // throw SQLException(std::string("failed to query index for ") + xybase::string::to_string((char8_t *)text.c_str()));
            }

            int text_id = sqlite3_column_int(qryStmt, 0);
            
            sqlite3_bind_int(insStmt, tidId, text_id);
            sqlite3_bind_text(insStmt, tId, trans.c_str(), -1, SQLITE_TRANSIENT);

            if (sqlite3_step(insStmt) != SQLITE_DONE)
            {
                throw SQLException(sqlite3_errmsg(db));
            }
            sqlite3_reset(qryStmt);
            sqlite3_reset(insStmt);
        }

        sqlite3_finalize(qryStmt);
        sqlite3_finalize(insStmt);

        Execute("COMMIT;");
    }
    catch ( SQLException &ex)
    {
        Execute("ROLLBACK;");
        if (qryStmt) sqlite3_finalize(qryStmt);
        if (insStmt) sqlite3_finalize(insStmt);
        throw;
    }
}

void SQLiteDataSource::Purge()
{
    Execute("CREATE TEMP VIEW text_ids AS SELECT text_id FROM rela");
    Execute(R"(DELETE FROM text WHERE id NOT IN text_ids;)");
}

void SQLiteDataSource::DropFile(const char *path)
{
    // Prepare SQL statements
    const std::string deleteRelaSQL = R"(
        DELETE FROM rela
        WHERE file_id = (
            SELECT id FROM file WHERE path = ?
        );
    )";

    const std::string deleteFileSQL = R"(
        DELETE FROM file WHERE path = ?;
    )";

    sqlite3_stmt *stmt = nullptr;

    try {
        // Delete from rela
        if (sqlite3_prepare_v2(db, deleteRelaSQL.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            throw SQLException("Failed to prepare delete from rela statement");
        }
        sqlite3_bind_text(stmt, 1, path, -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            throw SQLException("Failed to execute delete from rela statement");
        }
        sqlite3_finalize(stmt);
        stmt = nullptr;

        // Delete from file
        if (sqlite3_prepare_v2(db, deleteFileSQL.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            throw SQLException("Failed to prepare delete from file statement");
        }
        sqlite3_bind_text(stmt, 1, path, -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            throw SQLException("Failed to execute delete from file statement");
        }
        sqlite3_finalize(stmt);
    }
    catch (const std::exception &e) {
        if (stmt) sqlite3_finalize(stmt);
        throw; // Rethrow the exception
    }
}

void SQLiteDataSource::DatToDatabase(const char *lang, const char *type, const char *path)
{
    std::string query;
    sqlite3_stmt *stmt = nullptr;

    if (path) {
        // Search by path only
        query = "SELECT path, type FROM file WHERE path = ?";
    }
    else {
        // Search by lang and optionally type
        query = "SELECT path, type FROM file WHERE 1 = 1";
        if (lang) {
            query += " AND lang = ?";
        }
        if (type) {
            query += " AND type = ?";
        }
    }

    try {
        if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error("Failed to prepare SQL statement");
        }

        // Bind parameters
        if (path) {
            sqlite3_bind_text(stmt, 1, path, -1, SQLITE_TRANSIENT);
        }
        else {
            if (lang) {
                sqlite3_bind_text(stmt, 1, lang, -1, SQLITE_TRANSIENT);
                if (type) {
                    sqlite3_bind_text(stmt, 2, type, -1, SQLITE_TRANSIENT);
                }
            }
            else if (type) {
                sqlite3_bind_text(stmt, 1, type, -1, SQLITE_TRANSIENT);
            }
        }

        // Execute and process results
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *filePath = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
            const char *fileType = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));

            if (filePath && fileType) {
                ImportDat(filePath, fileType);
            }
        }

        sqlite3_finalize(stmt);
    }
    catch (const std::exception &e) {
        if (stmt) sqlite3_finalize(stmt);
        throw; // Rethrow the exception for further handling
    }
}

#include "DMsg.h"
#include "XiString.h"
#include "EventStringBase.h"
#include "DataManager.h"
#include "ItemData.h"

void SQLiteDataSource::ImportDat(const std::string &path, const std::string &type)
{
    sqlite3_stmt *stmt = nullptr;
    int file_id = -1;
    try
    {
        if (sqlite3_prepare_v2(db, "SELECT id FROM file WHERE path = ?", -1, &stmt, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_text(stmt, 1, reinterpret_cast<const char *>(path.c_str()), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(stmt) == SQLITE_ROW)
            {
                file_id = sqlite3_column_int(stmt, 0);
            }
        }
        sqlite3_finalize(stmt);

        if (sqlite3_prepare_v2(db, "DELETE FROM rela WHERE file_id = ?", -1, &stmt, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_int(stmt, 1, file_id);
            if (sqlite3_step(stmt) != SQLITE_DONE)
            {
                throw SQLException(sqlite3_errmsg(db));
            }
        }
        sqlite3_finalize(stmt);

        // Also clean up items data for this file
        if (type.starts_with("i")) {
            if (sqlite3_prepare_v2(db, "DELETE FROM items WHERE file_id = ?", -1, &stmt, nullptr) == SQLITE_OK)
            {
                sqlite3_bind_int(stmt, 1, file_id);
                sqlite3_step(stmt);
            }
            sqlite3_finalize(stmt);
        }
        
        // Also clean up monbridge data for this file
        if (type == "mbd") {
            if (sqlite3_prepare_v2(db, "DELETE FROM monbridge WHERE file_id = ?", -1, &stmt, nullptr) == SQLITE_OK)
            {
                sqlite3_bind_int(stmt, 1, file_id);
                sqlite3_step(stmt);
            }
            sqlite3_finalize(stmt);
        }
        
        // Also clean up ROE quest data for this file (ROM/307/15)
        if (type == "erq") {
            if (sqlite3_prepare_v2(db, "DELETE FROM roe_quest WHERE file_id = ?", -1, &stmt, nullptr) == SQLITE_OK)
            {
                sqlite3_bind_int(stmt, 1, file_id);
                sqlite3_step(stmt);
            }
            sqlite3_finalize(stmt);
        }
        
        // Also clean up ROE category data for this file (ROM/307/23)
        if (type == "erc") {
            if (sqlite3_prepare_v2(db, "DELETE FROM roe_category WHERE file_id = ?", -1, &stmt, nullptr) == SQLITE_OK)
            {
                sqlite3_bind_int(stmt, 1, file_id);
                sqlite3_step(stmt);
            }
            sqlite3_finalize(stmt);
        }
    }
    catch (SQLException &ex)
    {
        sqlite3_finalize(stmt);
        throw;
    }

    auto datPath = PathUtil::GetPath(xybase::string::sys_mbs_to_wcs(path + ".DAT"));
    Ring(xybase::string::to_utf8(datPath).c_str());

    Execute("BEGIN;");
    try
    {
        if (type == "dmsg")
        {
            DMsg dmsg(datPath);
            dmsg.Read();
            int rowNum = 1;
            for (auto &row : dmsg)
            {
                int colNum = 1;
                for (auto &cell : row)
                {
                    if (cell.GetType() == 0) // str
                    {
                        std::u8string text = xybase::string::escape(cell.Get<std::u8string>());

                        InsertText(reinterpret_cast<const char *>(text.c_str()), file_id, rowNum, colNum);
                    }
                    ++colNum;
                }
                ++rowNum;
            }
        }
        else if (type == "xis")
        {
            XiString xis(datPath);
            xis.Read();
            int rowNum = 1;
            for (auto &e : xis)
            {
                InsertText((const char *)xybase::string::escape(xybase::string::to_utf8(xis.Decode(e.str))).c_str(), file_id, rowNum++, 1);
            }
        }
        else if (type == "evsb")
        {
            EventStringBase evsb(datPath);
            evsb.Read();
            int rowNum = 1;
            for (auto &str : evsb)
            {
                InsertText(reinterpret_cast<const char *>(str.c_str()), file_id, rowNum++, 1);
            }
        }
        else if (type == "sd")
        {
            StatusData statusData;
            statusData.Read(datPath);
            int rowNum = 1;
            for (const auto &datum : statusData.data) {
                if (!datum.description.empty()) {
                    std::string descStr = reinterpret_cast<const char*>(xybase::string::escape(datum.description).c_str());
                    InsertText(descStr.c_str(), file_id, rowNum, 1);
                }
                ++rowNum;
            }
        }
        else if (type == "ieb" || type == "inb" || type == "iub" || type == "iwb" || type == "iab" || type == "ipb" || type == "isb" || type == "icb")
        {
            ImportItemDat(file_id, datPath, xybase::string::sys_mbs_to_wcs(type));
        }
        else if (type == "mbd")
        {
            ImportMonBridgeDat(file_id, datPath);
        }
        else if (type == "erq") // ROM/307/15 - Quest entries
        {
            ImportRoeQuestDat(file_id, datPath);
        }
        else if (type == "erc") // ROM/307/23 - Category entries
        {
            ImportRoeCategoryDat(file_id, datPath);
        }
    }
    catch (std::exception &ex)
    {
        Execute("ROLLBACK;");
        throw;
    }
    Execute("COMMIT;");
}

void SQLiteDataSource::InsertText(const char * text, int file_id, int rowNum, int colNum)
{
    int text_id = -1;
    sqlite3_stmt *stmt;

    try
    {
        if (sqlite3_prepare_v2(db, "SELECT id FROM text WHERE text = ?", -1, &stmt, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_text(stmt, 1, text, -1, SQLITE_TRANSIENT);
            if (sqlite3_step(stmt) == SQLITE_ROW)
            {
                text_id = sqlite3_column_int(stmt, 0);
            }
        }
        sqlite3_finalize(stmt);

        if (text_id == -1)
        {
            if (sqlite3_prepare_v2(db, "INSERT INTO text (text) VALUES (?)", -1, &stmt, nullptr) == SQLITE_OK)
            {
                sqlite3_bind_text(stmt, 1, text, -1, SQLITE_TRANSIENT);
                if (sqlite3_step(stmt) == SQLITE_DONE)
                {
                    text_id = static_cast<int>(sqlite3_last_insert_rowid(db));
                }
                else
                {
                    throw SQLException(sqlite3_errmsg(db));
                }
            }
            sqlite3_finalize(stmt);
        }

        if (sqlite3_prepare_v2(db,
            "INSERT INTO rela (file_id, file_row, file_col, text_id) VALUES (?, ?, ?, ?)",
            -1, &stmt, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_int(stmt, 1, file_id);
            sqlite3_bind_int(stmt, 2, rowNum);
            sqlite3_bind_int(stmt, 3, colNum);
            sqlite3_bind_int(stmt, 4, text_id);
            if (sqlite3_step(stmt) != SQLITE_DONE)
            {
                throw SQLException(sqlite3_errmsg(db));
            }
        }
        sqlite3_finalize(stmt);
    }
    catch (SQLException &ex)
    {
        sqlite3_finalize(stmt);
        throw;
    }
}

void SQLiteDataSource::TransAndOut()
{
    sqlite3_stmt *stmt = nullptr;

    try {
        if (sqlite3_prepare_v2(db, "select path, type, id from file;", -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error("Failed to prepare SQL statement");
        }

        // Execute and process results
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *filePath = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
            const char *fileType = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
            int id = sqlite3_column_int(stmt, 2);

            if (filePath && fileType) {
                TranslateDat(id, filePath, fileType);
            }
        }

        sqlite3_finalize(stmt);
    }
    catch (const std::exception &e) {
        if (stmt) sqlite3_finalize(stmt);
        throw; // Rethrow the exception for further handling
    }
}

#include "ChsToSJis.h"

std::u8string SQLiteDataSource::GetTranslation(const std::u8string &text) {
    sqlite3_stmt *stmt = nullptr;
    int textId = -1;
    std::u8string translation;

    const char *findTextSQL = "SELECT id FROM text WHERE text = ?";
    if (sqlite3_prepare_v2(db, findTextSQL, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare statement for text lookup");
    }

    sqlite3_bind_text(stmt, 1, (const char *)text.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        textId = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    if (textId == -1) {
        return text;
    }

    const char *findTranslationSQL = "SELECT text FROM trans WHERE text_id = ?";
    if (sqlite3_prepare_v2(db, findTranslationSQL, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare statement for translation lookup");
    }

    sqlite3_bind_int(stmt, 1, textId);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *translatedText = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        if (translatedText != nullptr) {
            translation = (const char8_t *)translatedText;
#ifdef CHS_SPECIFIED_MOD
            translation = ChsToSJis::Instance().ReplaceHanzi(translation);
#endif
        }
    }
    else {
        translation = text;
    }
    sqlite3_finalize(stmt);

    return translation;
}

void SQLiteDataSource::TranslateDat(int file_id, const char *file_path, const char *type)
{
    std::string t(type);
    auto datPath = PathUtil::GetPath(xybase::string::sys_mbs_to_wcs(file_path) + L".DAT");
    auto outPath = PathUtil::GetOutPathConf(xybase::string::sys_mbs_to_wcs(file_path) + L".DAT");
    Ring(xybase::string::to_utf8(datPath).c_str());

    if (t == "dmsg")
    {
        DMsg dmsg(datPath);
        dmsg.Read();
        int rowNum = 1;
        for (auto &row : dmsg)
        {
            int colNum = 1;
            for (auto &cell : row)
            {
                if (cell.GetType() == 0) // str
                {
                    std::u8string text = xybase::string::escape(cell.Get<std::u8string>());

                    cell.Set(xybase::string::unescape(GetTranslation(text)));
                }
                ++colNum;
            }
            ++rowNum;
        }
        dmsg.path = outPath;
        dmsg.Write();
    }
    else if (t == "xis")
    {
        XiString xis(datPath);
        xis.Read();
        int rowNum = 1;
        for (auto &str : xis)
        {
            std::u8string text = xybase::string::escape(xybase::string::to_utf8(xis.Decode(str.str)));

            str.str = xis.Encode(xybase::string::to_string(xybase::string::unescape(GetTranslation(text))));
        }
        xis.path = outPath;
        xis.Write();
    }
    else if (t == "evsb")
    {
        EventStringBase evsb(datPath);
        evsb.Read();
        for (auto &str : evsb)
        {
            auto res = GetTranslation(str);
            str = res;
        }
        evsb.path = outPath;
        evsb.Write();
    }
    else if (t == "sd")
    {
        StatusData statusData;
        statusData.Read(datPath);
        int rowNum = 1;
        for (auto &datum : statusData.data) {
            if (!datum.description.empty()) {
                datum.description = xybase::string::unescape(GetTranslation((datum.description)));
            }
            ++rowNum;
        }
        statusData.Write(outPath);
    }
    else if (t == "ieb" || t == "inb" || t == "iub" || t == "iwb" || t == "iab" || type == "ipb" || type == "isb" || type == "icb")
    {
        TranslateItemDat(file_id, xybase::string::sys_mbs_to_wcs(file_path).c_str(), t.c_str());
    }
    else if (t == "mbd")
    {
        TranslateMonBridgeDat(file_id, xybase::string::sys_mbs_to_wcs(file_path).c_str());
    }
    else if (t == "erq") // ROM/307/15 - Quest entries
    {
        TranslateRoeQuestDat(file_id, xybase::string::sys_mbs_to_wcs(file_path).c_str());
    }
    else if (t == "erc") // ROM/307/23 - Category entries
    {
        TranslateRoeCategoryDat(file_id, xybase::string::sys_mbs_to_wcs(file_path).c_str());
    }
}

void SQLiteDataSource::Execute(const std::string &qry)
{
	char *errorMessage = nullptr;
	if (sqlite3_exec(db, qry.c_str(), nullptr, nullptr, &errorMessage) != SQLITE_OK) {
		throw SQLException(errorMessage);
		sqlite3_free(errorMessage);
	}
}

void SQLiteDataSource::SetRing(void(*callback)(const char8_t *msg))
{
    ring = callback;
}

void SQLiteDataSource::ImportItemDat(const int file_id, const std::wstring &path, const std::wstring &type)
{
    sqlite3_stmt *stmt = nullptr;
    
    ItemData itemData;
    ItemSpecType specType = ItemSpecType::NORMAL;
    
    // Determine spec type based on type parameter
    if (type == L"ieb") specType = ItemSpecType::ARMOUR;
    else if (type == L"inb") specType = ItemSpecType::NORMAL;
    else if (type == L"iub") specType = ItemSpecType::USABLE;
    else if (type == L"iwb") specType = ItemSpecType::WEAPON;
    else if (type == L"iab") specType = ItemSpecType::ARMOUR;
    else if (type == L"isb") specType = ItemSpecType::SLIP;
    else if (type == L"ipb") specType = ItemSpecType::PUPPET;
    else if (type == L"icb") specType = ItemSpecType::CURRENCY;
    
    itemData.Read(path, specType);
    
    for (const auto &datum : itemData.data) {
		int item_record_id = -1;
        try
        {
            item_record_id = InsertOrGetItemRecord(file_id, datum.id, type);
        }
        catch (SQLException &ex)
        {
            Ring(xybase::string::to_utf8(std::string("Failed to insert or get item record for item ID ") + std::to_string(datum.id) + ": " + ex.what()).c_str());
            continue;
		}
        
        // Update main items table with all header fields
        const char *updateMainSQL = R"(
            UPDATE items SET 
                spec_type = ?, stack_size = ?, item_type = ?, resource_id = ?, valid_targets = ?,
                is_scroll = ?, is_not_listable = ?, is_inscribable = ?, is_alt = ?, ukn_flg1 = ?,
                is_in_mystery_box = ?, is_gm_item = ?, is_wall_decoration = ?,
                is_rare = ?, is_unsellable = ?, is_unmailable = ?, is_ex = ?,
                is_equipment = ?, is_npc_tradeable = ?, is_usable = ?, is_linkshell = ?,
                image_length = ?, image_data = ?, end_marker = ?
            WHERE id = ?
        )";
        
        if (sqlite3_prepare_v2(db, updateMainSQL, -1, &stmt, nullptr) == SQLITE_OK)
        {
            std::string specTypeStr;
            switch (datum.spec_type) {
                case ItemSpecType::WEAPON: specTypeStr = "WEAPON"; break;
                case ItemSpecType::ARMOUR: specTypeStr = "ARMOUR"; break;
                case ItemSpecType::USABLE: specTypeStr = "USABLE"; break;
				case ItemSpecType::PUPPET: specTypeStr = "PUPPET"; break;
				case ItemSpecType::SLIP: specTypeStr = "SLIP"; break;
                case ItemSpecType::NORMAL: default: specTypeStr = "NORMAL"; break;
            }
            
            sqlite3_bind_text(stmt, 1, specTypeStr.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 2, datum.stack_size());
            sqlite3_bind_int(stmt, 3, datum.item_type());
            sqlite3_bind_int(stmt, 4, datum.resource_id());
            sqlite3_bind_int(stmt, 5, datum.valid_targets());
            sqlite3_bind_int(stmt, 6, datum.flags().is_scroll ? 1 : 0);
            sqlite3_bind_int(stmt, 7, datum.flags().is_not_listable ? 1 : 0);
            sqlite3_bind_int(stmt, 8, datum.flags().is_inscribable ? 1 : 0);
            sqlite3_bind_int(stmt, 9, datum.flags().is_alt ? 1 : 0);
            sqlite3_bind_int(stmt, 10, datum.flags().ukn_flg1 ? 1 : 0);
            sqlite3_bind_int(stmt, 11, datum.flags().is_in_mystery_box ? 1 : 0);
            sqlite3_bind_int(stmt, 12, datum.flags().is_gm_item ? 1 : 0);
            sqlite3_bind_int(stmt, 13, datum.flags().is_wall_decoration ? 1 : 0);
            sqlite3_bind_int(stmt, 14, datum.flags().is_rare ? 1 : 0);
            sqlite3_bind_int(stmt, 15, datum.flags().is_unsellable ? 1 : 0);
            sqlite3_bind_int(stmt, 16, datum.flags().is_unmailable ? 1 : 0);
            sqlite3_bind_int(stmt, 17, datum.flags().is_ex ? 1 : 0);
            sqlite3_bind_int(stmt, 18, datum.flags().is_equipment ? 1 : 0);
            sqlite3_bind_int(stmt, 19, datum.flags().is_npc_tradeable ? 1 : 0);
            sqlite3_bind_int(stmt, 20, datum.flags().is_usable ? 1 : 0);
            sqlite3_bind_int(stmt, 21, datum.flags().is_linkshell ? 1 : 0);
            sqlite3_bind_int(stmt, 22, datum.originalEntry.image_length);
            
            // Handle image data
            if (datum.originalEntry.image_length > 0) {
                sqlite3_bind_blob(stmt, 23, datum.originalEntry.image_data, datum.originalEntry.image_length, SQLITE_STATIC);
            } else {
                sqlite3_bind_null(stmt, 23);
            }
            
            sqlite3_bind_int(stmt, 24, datum.originalEntry.end_marker);
            sqlite3_bind_int(stmt, 25, item_record_id);
            sqlite3_step(stmt);
        }
        sqlite3_finalize(stmt);
        
        // Insert spec-specific data based on type
        switch (datum.spec_type) {
            case ItemSpecType::WEAPON:
                InsertWeaponSpec(item_record_id, datum.originalEntry.spec.weapon);
                InsertEquipSlots(item_record_id, datum.originalEntry.spec.weapon.equip_slots);
                InsertRaceApplicability(item_record_id, datum.originalEntry.spec.weapon.races);
                InsertJobApplicability(item_record_id, datum.originalEntry.spec.weapon.jobs);
                break;
                
            case ItemSpecType::ARMOUR:
                InsertArmourSpec(item_record_id, datum.originalEntry.spec.armour);
                InsertEquipSlots(item_record_id, datum.originalEntry.spec.armour.equip_slots);
                InsertRaceApplicability(item_record_id, datum.originalEntry.spec.armour.equip_races);
                InsertJobApplicability(item_record_id, datum.originalEntry.spec.armour.equip_jobs);
                break;
                
            case ItemSpecType::USABLE:
                InsertUsableSpec(item_record_id, datum.originalEntry.spec.usable);
                break;
                
            case ItemSpecType::NORMAL:
            default:
                InsertNormalSpec(item_record_id, datum.originalEntry.spec.normal);
                break;
        }
        
        // Insert name text if not empty
        try {
            std::u8string itemName = datum.name();
            if (!itemName.empty()) {
                int name_text_id = InsertOrGetText(xybase::string::escape(itemName));
                if (sqlite3_prepare_v2(db, "UPDATE items SET name_text_id = ? WHERE id = ?", -1, &stmt, nullptr) == SQLITE_OK)
                {
                    sqlite3_bind_int(stmt, 1, name_text_id);
                    sqlite3_bind_int(stmt, 2, item_record_id);
                    sqlite3_step(stmt);
                }
                sqlite3_finalize(stmt);
            }
        } catch (...) { /* Ignore missing fields */ }
        
        // Insert description text if not empty
        try {
            std::u8string itemDesc = datum.description();
            if (!itemDesc.empty()) {
                int desc_text_id = InsertOrGetText(xybase::string::escape(itemDesc));
                if (sqlite3_prepare_v2(db, "UPDATE items SET description_text_id = ? WHERE id = ?", -1, &stmt, nullptr) == SQLITE_OK)
                {
                    sqlite3_bind_int(stmt, 1, desc_text_id);
                    sqlite3_bind_int(stmt, 2, item_record_id);
                    sqlite3_step(stmt);
                }
                sqlite3_finalize(stmt);
            }
        } catch (...) { /* Ignore missing fields */ }
    }
}

void SQLiteDataSource::InsertWeaponSpec(int item_id, const ItemWeaponSpec &spec)
{
    sqlite3_stmt *stmt = nullptr;
    
    const char *sql = R"(
        INSERT OR REPLACE INTO item_weapon_spec (
            item_id, level, ukn, ukn2, dmg, delay, ukn5, ukn6, ukn12, ukn7, ukn9,
            max_charges, cast_factor, use_time, reuse_time, ukn20, ukn21, ilvl, ukn22, ukn23
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, item_id);
        sqlite3_bind_int(stmt, 2, spec.level);
        sqlite3_bind_int(stmt, 3, spec.ukn);
        sqlite3_bind_int(stmt, 4, spec.ukn2);
        sqlite3_bind_int(stmt, 5, spec.dmg);
        sqlite3_bind_int(stmt, 6, spec.delay);
        sqlite3_bind_int(stmt, 7, spec.ukn5);
        sqlite3_bind_int(stmt, 8, spec.ukn6);
        sqlite3_bind_int(stmt, 9, spec.ukn12);
        sqlite3_bind_int(stmt, 10, spec.ukn7);
        sqlite3_bind_int(stmt, 11, spec.ukn9);
        sqlite3_bind_int(stmt, 12, spec.max_charges);
        sqlite3_bind_int(stmt, 13, spec.cast_factor);
        sqlite3_bind_int(stmt, 14, spec.use_time);
        sqlite3_bind_int(stmt, 15, spec.reuse_time);
        sqlite3_bind_int(stmt, 16, spec.ukn20);
        sqlite3_bind_int(stmt, 17, spec.ukn21);
        sqlite3_bind_int(stmt, 18, spec.ilvl);
        sqlite3_bind_int(stmt, 19, spec.ukn22);
        sqlite3_bind_int(stmt, 20, spec.ukn23);
        sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
}

void SQLiteDataSource::InsertArmourSpec(int item_id, const ItemArmourSpec &spec)
{
    sqlite3_stmt *stmt = nullptr;
    
    const char *sql = R"(
        INSERT OR REPLACE INTO item_armour_spec (
            item_id, level, ukn, shield_size, max_charges, cast_factor, 
            use_time, reuse_time, ukn1, ukn2, ilvl, ukn3, ukn4
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, item_id);
        sqlite3_bind_int(stmt, 2, spec.level);
        sqlite3_bind_int(stmt, 3, spec.ukn);
        sqlite3_bind_int(stmt, 4, spec.shield_size);
        sqlite3_bind_int(stmt, 5, spec.max_charges);
        sqlite3_bind_int(stmt, 6, spec.cast_factor);
        sqlite3_bind_int(stmt, 7, spec.use_time);
        sqlite3_bind_int(stmt, 8, spec.reuse_time);
        sqlite3_bind_int(stmt, 9, spec.ukn1);
        sqlite3_bind_int(stmt, 10, spec.ukn2);
        sqlite3_bind_int(stmt, 11, spec.ilvl);
        sqlite3_bind_int(stmt, 12, spec.ukn3);
        sqlite3_bind_int(stmt, 13, spec.ukn4);
        sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
}

void SQLiteDataSource::InsertUsableSpec(int item_id, const ItemUsableSpec &spec)
{
    sqlite3_stmt *stmt = nullptr;
    
    const char *sql = R"(
        INSERT OR REPLACE INTO item_usable_spec (
            item_id, cast_factor, ukn1, ukn2, ukn3
        ) VALUES (?, ?, ?, ?, ?)
    )";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, item_id);
        sqlite3_bind_int(stmt, 2, spec.cast_factor);
        sqlite3_bind_int(stmt, 3, spec.ukn1);
        sqlite3_bind_int(stmt, 4, spec.ukn2);
        sqlite3_bind_int(stmt, 5, spec.ukn3);
        sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
}

void SQLiteDataSource::InsertNormalSpec(int item_id, const ItemNormalSpec &spec)
{
    sqlite3_stmt *stmt = nullptr;
    
    const char *sql = R"(
        INSERT OR REPLACE INTO item_normal_spec (
            item_id, ukn1, ukn2, ukn3, ukn4, ukn5
        ) VALUES (?, ?, ?, ?, ?, ?)
    )";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, item_id);
        sqlite3_bind_int(stmt, 2, spec.ukn1);
        sqlite3_bind_int(stmt, 3, spec.ukn2);
        sqlite3_bind_int(stmt, 4, spec.ukn3);
        sqlite3_bind_int(stmt, 5, spec.ukn4);
        sqlite3_bind_int(stmt, 6, spec.ukn5);
        sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
}

void SQLiteDataSource::InsertEquipSlots(int item_id, const ItemEquipSlot &slots)
{
    sqlite3_stmt *stmt = nullptr;
    
    const char *sql = R"(
        INSERT OR REPLACE INTO item_equip_slots (
            item_id, main_hand, sub_hand, ranged, ammo, head, body, hands, legs,
            feet, neck, waist, left_ear, right_ear, left_ring, right_ring, back
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, item_id);
        sqlite3_bind_int(stmt, 2, slots.main_hand ? 1 : 0);
        sqlite3_bind_int(stmt, 3, slots.sub_hand ? 1 : 0);
        sqlite3_bind_int(stmt, 4, slots.ranged ? 1 : 0);
        sqlite3_bind_int(stmt, 5, slots.ammo ? 1 : 0);
        sqlite3_bind_int(stmt, 6, slots.head ? 1 : 0);
        sqlite3_bind_int(stmt, 7, slots.body ? 1 : 0);
        sqlite3_bind_int(stmt, 8, slots.hands ? 1 : 0);
        sqlite3_bind_int(stmt, 9, slots.legs ? 1 : 0);
        sqlite3_bind_int(stmt, 10, slots.feet ? 1 : 0);
        sqlite3_bind_int(stmt, 11, slots.neck ? 1 : 0);
        sqlite3_bind_int(stmt, 12, slots.waist ? 1 : 0);
        sqlite3_bind_int(stmt, 13, slots.left_ear ? 1 : 0);
        sqlite3_bind_int(stmt, 14, slots.right_ear ? 1 : 0);
        sqlite3_bind_int(stmt, 15, slots.left_ring ? 1 : 0);
        sqlite3_bind_int(stmt, 16, slots.right_ring ? 1 : 0);
        sqlite3_bind_int(stmt, 17, slots.back ? 1 : 0);
        sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
}

void SQLiteDataSource::InsertRaceApplicability(int item_id, const ItemRaceApplicability &races)
{
    sqlite3_stmt *stmt = nullptr;
    
    const char *sql = R"(
        INSERT OR REPLACE INTO item_race_applicability (
            item_id, none, hume_male, hume_female, elvaan_male, elvaan_female,
            taru_male, taru_female, mithra, galka, rsv
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, item_id);
        sqlite3_bind_int(stmt, 2, races.None ? 1 : 0);
        sqlite3_bind_int(stmt, 3, races.HumeMale ? 1 : 0);
        sqlite3_bind_int(stmt, 4, races.HumeFemale ? 1 : 0);
        sqlite3_bind_int(stmt, 5, races.ElvaanMale ? 1 : 0);
        sqlite3_bind_int(stmt, 6, races.ElvaanFemale ? 1 : 0);
        sqlite3_bind_int(stmt, 7, races.TaruMale ? 1 : 0);
        sqlite3_bind_int(stmt, 8, races.TaruFemale ? 1 : 0);
        sqlite3_bind_int(stmt, 9, races.Mithra ? 1 : 0);
        sqlite3_bind_int(stmt, 10, races.Galka ? 1 : 0);
        sqlite3_bind_int(stmt, 11, races.Rsv);
        sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
}

void SQLiteDataSource::InsertJobApplicability(int item_id, const ItemJobApplicability &jobs)
{
    sqlite3_stmt *stmt = nullptr;
    
    const char *sql = R"(
        INSERT OR REPLACE INTO item_job_applicability (
            item_id, pld, thf, rdm, blm, whm, mnk, war, rsv1,
            smn, drg, nin, sam, rng, brd, bst, drk,
            mon, run, geo, sch, dnc, pup, cor, blu, rsv2
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, item_id);
        sqlite3_bind_int(stmt, 2, jobs.pld ? 1 : 0);
        sqlite3_bind_int(stmt, 3, jobs.thf ? 1 : 0);
        sqlite3_bind_int(stmt, 4, jobs.rdm ? 1 : 0);
        sqlite3_bind_int(stmt, 5, jobs.blm ? 1 : 0);
        sqlite3_bind_int(stmt, 6, jobs.whm ? 1 : 0);
        sqlite3_bind_int(stmt, 7, jobs.mnk ? 1 : 0);
        sqlite3_bind_int(stmt, 8, jobs.war ? 1 : 0);
        sqlite3_bind_int(stmt, 9, jobs.rsv1 ? 1 : 0);
        sqlite3_bind_int(stmt, 10, jobs.smn ? 1 : 0);
        sqlite3_bind_int(stmt, 11, jobs.drg ? 1 : 0);
        sqlite3_bind_int(stmt, 12, jobs.nin ? 1 : 0);
        sqlite3_bind_int(stmt, 13, jobs.sam ? 1 : 0);
        sqlite3_bind_int(stmt, 14, jobs.rng ? 1 : 0);
        sqlite3_bind_int(stmt, 15, jobs.brd ? 1 : 0);
        sqlite3_bind_int(stmt, 16, jobs.bst ? 1 : 0);
        sqlite3_bind_int(stmt, 17, jobs.drk ? 1 : 0);
        sqlite3_bind_int(stmt, 18, jobs.mon ? 1 : 0);
        sqlite3_bind_int(stmt, 19, jobs.run ? 1 : 0);
        sqlite3_bind_int(stmt, 20, jobs.geo ? 1 : 0);
        sqlite3_bind_int(stmt, 21, jobs.sch ? 1 : 0);
        sqlite3_bind_int(stmt, 22, jobs.dnc ? 1 : 0);
        sqlite3_bind_int(stmt, 23, jobs.pup ? 1 : 0);
        sqlite3_bind_int(stmt, 24, jobs.cor ? 1 : 0);
        sqlite3_bind_int(stmt, 25, jobs.blu ? 1 : 0);
        sqlite3_bind_int(stmt, 26, jobs.rsv2);
        sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
}

void SQLiteDataSource::TranslateItemDat(int file_id, const wchar_t *file_path, const char *type)
{
    std::wstring inputPath = file_path;
    if (!inputPath.ends_with(L".DAT")) {
        inputPath += L".DAT";
    }
    auto datPath = PathUtil::GetPath(inputPath);
    auto outPath = PathUtil::GetOutPathConf(inputPath);
    
    ItemData itemData;
    ItemSpecType specType = ItemSpecType::NORMAL;
    
    // Determine spec type based on type parameter
    std::string typeStr(type);
    if (typeStr == "ieb") specType = ItemSpecType::ARMOUR;
    else if (typeStr == "inb") specType = ItemSpecType::NORMAL;
    else if (typeStr == "iub") specType = ItemSpecType::USABLE;
    else if (typeStr == "iwb") specType = ItemSpecType::WEAPON;
    else if (typeStr == "iab") specType = ItemSpecType::ARMOUR;
    else if (typeStr == "isb") specType = ItemSpecType::SLIP;
    else if (typeStr == "ipb") specType = ItemSpecType::PUPPET;
    else if (typeStr == "icb") specType = ItemSpecType::CURRENCY;
    
    // Read original data first
    itemData.Read(datPath, specType);
    
    sqlite3_stmt *stmt = nullptr;
    
    // Get translations for each item
    for (auto &datum : itemData.data) {
        // Get name translation
        if (sqlite3_prepare_v2(db, 
            "SELECT tr.text FROM items i "
            "JOIN text t ON i.name_text_id = t.id "
            "JOIN trans tr ON t.id = tr.text_id "
            "WHERE i.file_id = ? AND i.item_id = ?", 
            -1, &stmt, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_int(stmt, 1, file_id);
            sqlite3_bind_int(stmt, 2, datum.id);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* translatedName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                if (translatedName) {
                    std::u8string transName(reinterpret_cast<const char8_t*>(translatedName));
                    datum.setName(transName);
                }
            }
        }
        sqlite3_finalize(stmt);
        
        // Get description translation
        if (sqlite3_prepare_v2(db, 
            "SELECT tr.text FROM items i "
            "JOIN text t ON i.description_text_id = t.id "
            "JOIN trans tr ON t.id = tr.text_id "
            "WHERE i.file_id = ? AND i.item_id = ?", 
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
    
    itemData.Write(outPath);
}

int SQLiteDataSource::InsertOrGetItemRecord(int file_id, uint32_t item_id, const std::wstring &type)
{
    sqlite3_stmt *stmt = nullptr;
    int record_id = -1;
    
    // Try to get existing record
    if (sqlite3_prepare_v2(db, "SELECT id FROM items WHERE file_id = ? AND item_id = ?", -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, file_id);
        sqlite3_bind_int(stmt, 2, item_id);
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            record_id = sqlite3_column_int(stmt, 0);
        }
    }
    sqlite3_finalize(stmt);
    
    if (record_id == -1)
    {
        // Insert new record
        if (sqlite3_prepare_v2(db, "INSERT INTO items (file_id, item_id, spec_type) VALUES (?, ?, '')", -1, &stmt, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_int(stmt, 1, file_id);
            sqlite3_bind_int(stmt, 2, item_id);
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

int SQLiteDataSource::InsertOrGetText(const std::u8string &text)
{
    sqlite3_stmt *stmt = nullptr;
    int text_id = -1;
    
    std::string textStr = reinterpret_cast<const char*>(text.c_str());
    
    // Try to get existing text
    if (sqlite3_prepare_v2(db, "SELECT id FROM text WHERE text = ?", -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_text(stmt, 1, textStr.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            text_id = sqlite3_column_int(stmt, 0);
        }
    }
    sqlite3_finalize(stmt);
    
    if (text_id == -1)
    {
        // Insert new text
        if (sqlite3_prepare_v2(db, "INSERT INTO text (text) VALUES (?)", -1, &stmt, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_text(stmt, 1, textStr.c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(stmt) == SQLITE_DONE)
            {
                text_id = static_cast<int>(sqlite3_last_insert_rowid(db));
            }
        }
        sqlite3_finalize(stmt);
    }
    
    return text_id;
}

void SQLiteDataSource::ImportMonBridgeDat(const int file_id, const std::wstring &path)
{
    sqlite3_stmt *stmt = nullptr;
    
    MonBridge monBridge;
    monBridge.Read(path);
    
    for (const auto &datum : monBridge.data) {
		int mb_record_id = -1;
        try
        {
            mb_record_id = InsertOrGetMonBridgeRecord(file_id, datum.id);
        }
        catch (SQLException &ex)
        {
            Ring(xybase::string::to_utf8(std::string("Failed to insert or get MonBridge record for ID ") + std::to_string(datum.id) + ": " + ex.what()).c_str());
            continue;
		}
        
        // Update main monbridge table
        const char *updateMainSQL = R"(
            UPDATE monbridge SET 
                mb_idx = ?, icon_data = ?
            WHERE id = ?
        )";
        
        if (sqlite3_prepare_v2(db, updateMainSQL, -1, &stmt, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_int(stmt, 1, datum.idx);
            
            // Handle icon data
            if (datum.originalEntry.icon_data[0] != 0) {
                sqlite3_bind_blob(stmt, 2, datum.originalEntry.icon_data, sizeof(datum.originalEntry.icon_data), SQLITE_STATIC);
            } else {
                sqlite3_bind_null(stmt, 2);
            }
            
            sqlite3_bind_int(stmt, 3, mb_record_id);
            sqlite3_step(stmt);
        }
        sqlite3_finalize(stmt);
        
        // Insert display name text if not empty (internal name is NOT translated)
        if (!datum.displayName.empty()) {
            int display_name_text_id = InsertOrGetText(xybase::string::escape(datum.displayName));
            if (sqlite3_prepare_v2(db, "UPDATE monbridge SET display_name_text_id = ? WHERE id = ?", -1, &stmt, nullptr) == SQLITE_OK)
            {
                sqlite3_bind_int(stmt, 1, display_name_text_id);
                sqlite3_bind_int(stmt, 2, mb_record_id);
                sqlite3_step(stmt);
            }
            sqlite3_finalize(stmt);
        }
    }
}

void SQLiteDataSource::TranslateMonBridgeDat(int file_id, const wchar_t *file_path)
{
    std::wstring inputPath = file_path;
    if (!inputPath.ends_with(L".DAT")) {
        inputPath += L".DAT";
    }
    auto datPath = PathUtil::GetPath(inputPath);
    auto outPath = PathUtil::GetOutPathConf(inputPath);
    
    MonBridge monBridge;
    
    // Read original data first
    monBridge.Read(datPath);
    
    sqlite3_stmt *stmt = nullptr;
    
    // Get translations for each entry
    for (auto &datum : monBridge.data) {
        // Only translate display name (internal name must remain unchanged for game to find entries)
        if (sqlite3_prepare_v2(db, 
            "SELECT tr.text FROM monbridge mb "
            "JOIN text t ON mb.display_name_text_id = t.id "
            "JOIN trans tr ON t.id = tr.text_id "
            "WHERE mb.file_id = ? AND mb.mb_id = ?", 
            -1, &stmt, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_int(stmt, 1, file_id);
            sqlite3_bind_int(stmt, 2, datum.id);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* translatedDisplay = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                if (translatedDisplay) {
                    std::u8string transDisplay(reinterpret_cast<const char8_t*>(translatedDisplay));
                    datum.displayName = transDisplay;
                }
            }
        }
        sqlite3_finalize(stmt);
    }
    
    monBridge.Write(outPath);
}

int SQLiteDataSource::InsertOrGetMonBridgeRecord(int file_id, uint32_t mb_id)
{
    sqlite3_stmt *stmt = nullptr;
    int record_id = -1;
    
    // Try to get existing record
    if (sqlite3_prepare_v2(db, "SELECT id FROM monbridge WHERE file_id = ? AND mb_id = ?", -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, file_id);
        sqlite3_bind_int(stmt, 2, mb_id);
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            record_id = sqlite3_column_int(stmt, 0);
        }
    }
    sqlite3_finalize(stmt);
    
    if (record_id == -1)
    {
        // Insert new record
        if (sqlite3_prepare_v2(db, "INSERT INTO monbridge (file_id, mb_id, mb_idx) VALUES (?, ?, 0)", -1, &stmt, nullptr) == SQLITE_OK)
        {
            sqlite3_bind_int(stmt, 1, file_id);
            sqlite3_bind_int(stmt, 2, mb_id);
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
