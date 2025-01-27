#include "SQLiteDataSource.h"

#include <stdexcept>
#include "xystring.h"
#include <iostream>
#include "DataManager.h"

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
                throw SQLException("query for index failed.");
            }

            int text_id = sqlite3_column_int(qryStmt, 0);
            
            sqlite3_bind_int(insStmt, tidId, text_id);
            sqlite3_bind_text(insStmt, tId, trans.c_str(), -1, SQLITE_TRANSIENT);

            if (sqlite3_step(insStmt) != SQLITE_DONE)
            {
                throw SQLException("insertion failed.");
            }
            sqlite3_reset(qryStmt);
            sqlite3_reset(insStmt);
        }

        sqlite3_finalize(qryStmt);
        sqlite3_finalize(insStmt);
    }
    catch (SQLException &ex)
    {
        if (qryStmt) sqlite3_finalize(qryStmt);
        if (insStmt) sqlite3_finalize(insStmt);
        throw;
    }
}

void SQLiteDataSource::Purge()
{
    Execute(R"(DELETE FROM text WHERE id NOT IN (SELECT text_id FROM rela);)");
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
        query = "SELECT path, type FROM file WHERE lang = ?";
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
            sqlite3_bind_text(stmt, 1, lang, -1, SQLITE_TRANSIENT);
            if (type) {
                sqlite3_bind_text(stmt, 2, type, -1, SQLITE_TRANSIENT);
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
                InsertText((const char *)xybase::string::to_utf8(xybase::string::escape(xis.Decode(e.str))).c_str(), file_id, rowNum++, 1);
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
    if (t == "xis")
    {
        XiString xis(datPath);
        xis.Read();
        int rowNum = 1;
        for (auto &str : xis)
        {
            std::u8string text = xybase::string::escape(xybase::string::to_utf8(str.str));

            str.str = xis.Encode(xybase::string::to_string(xybase::string::unescape(GetTranslation(text))));
        }
        xis.path = outPath;
        xis.Write();
    }
    if (t == "evsb")
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
