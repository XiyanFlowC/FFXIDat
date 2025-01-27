#pragma once

/* Using sqlite, 3.48.0 */
#include "sqlite3/sqlite3.h"
#include <string>
#include <exception>
#include "CsvFile.h"

class SQLException : std::runtime_error {
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

};

