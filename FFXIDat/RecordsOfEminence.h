#pragma once

// RecordsOfEminence: the facade of the Records of Eminence family.
//
// It keeps one table per schema (quests and categories); the record version is
// routed to the right layout.

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

#include "Record.h"
#include "Image.h"
#include "RoeFormats.h"

// Compatibility names of the structs that moved into their layout namespaces.
using RoeQuestEntry = roefmt::v30::QuestEntry;
using RoeQuestEntryLegacy = roefmt::v10::QuestEntry;
using RoeCategoryEntry = roefmt::v30::CategoryEntry;
using RoeCategoryEntryLegacy = roefmt::v10::CategoryEntry;

class RecordsOfEminence
{
public:
	using RoeQuestDatum = roefmt::QuestDatum;
	using RoeCategoryDatum = roefmt::CategoryDatum;

	// ROM/307/15 methods (Quest entries). The version comes from the data side
	// annotation (VersionForTypeCode); the pre-update corpus (LocCNTxtOld) asks
	// for its version explicitly.
	void ReadQuest(const char* path, roefmt::Version version = roefmt::CURRENT_VERSION);
	void ReadQuest(const std::wstring& path, roefmt::Version version = roefmt::CURRENT_VERSION);
	void WriteQuest(const char* path);
	void WriteQuest(const std::wstring& path);

	void QuestToICsv(const char* path);

	// ROM/307/23 methods (Category entries)
	void ReadCategory(const char* path, roefmt::Version version = roefmt::CURRENT_VERSION);
	void ReadCategory(const std::wstring& path, roefmt::Version version = roefmt::CURRENT_VERSION);
	void WriteCategory(const char* path);
	void WriteCategory(const std::wstring& path);

	void CategoryToICsv(const char* path);

	// Record version of the data currently held.
	roefmt::Version version() const { return layoutVersion; }

	std::vector<RoeQuestDatum> questData;       // ROM/307/15 data
	std::vector<RoeCategoryDatum> categoryData; // ROM/307/23 data

private:
	roefmt::Version layoutVersion = roefmt::CURRENT_VERSION;
};
