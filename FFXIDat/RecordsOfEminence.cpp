#include "RecordsOfEminence.h"

#include <fstream>
#include <stdexcept>
#include <utility>

#include "SlotFileCsv.h"
#include "xystring.h"

namespace
{
template <class Fn>
void WithQuestLayout(roefmt::Version version, Fn&& fn)
{
	switch (version)
	{
	case roefmt::Version::V10: fn(slotfile::SlotFile<roefmt::v10::Quest>{}); return;
	case roefmt::Version::V20: fn(slotfile::SlotFile<roefmt::v20::Quest>{}); return;
	case roefmt::Version::V30: fn(slotfile::SlotFile<roefmt::v30::Quest>{}); return;
	}
	throw std::runtime_error("Unknown Records of Eminence quest version");
}

template <class Fn>
void WithCategoryLayout(roefmt::Version version, Fn&& fn)
{
	switch (version)
	{
	case roefmt::Version::V10:
	case roefmt::Version::V20: fn(slotfile::SlotFile<roefmt::v10::Category>{}); return;
	case roefmt::Version::V30: fn(slotfile::SlotFile<roefmt::v30::Category>{}); return;
	}
	throw std::runtime_error("Unknown Records of Eminence category version");
}
} // namespace

// The CSV view of this family goes through the adapter layer: the column sets
// live in RoeFormatsCsv.cpp, behind the layout hooks declared in RoeFormatV30.h.
// ============= ROM/307/15 (Quest Entry) Methods =============

void RecordsOfEminence::ReadQuest(const char* path, roefmt::Version version)
{
	ReadQuest(xybase::string::sys_mbs_to_wcs(std::string(path)), version);
}

void RecordsOfEminence::ReadQuest(const std::wstring& path, roefmt::Version version)
{
	std::vector<RoeQuestDatum> records;
	WithQuestLayout(version, [&](auto file) {
		file.Read(path, roefmt::Schema::QUEST);
		records = std::move(file.data);
	});

	questData = std::move(records);
	layoutVersion = version;
}

void RecordsOfEminence::WriteQuest(const char* path)
{
	WriteQuest(xybase::string::sys_mbs_to_wcs(std::string(path)));
}

void RecordsOfEminence::WriteQuest(const std::wstring& path)
{
	WithQuestLayout(layoutVersion, [&](auto file) { file.Write(path, questData); });
}

void RecordsOfEminence::QuestToICsv(const char* path)
{
	const std::wstring wpath = xybase::string::sys_mbs_to_wcs(std::string(path));
	WithQuestLayout(layoutVersion, [&](auto file) {
		slotfile::WriteICsv<decltype(file)::LayoutType>(wpath, questData);
	});
}

// ============= ROM/307/23 (Category Entry) Methods =============

void RecordsOfEminence::ReadCategory(const char* path, roefmt::Version version)
{
	ReadCategory(xybase::string::sys_mbs_to_wcs(std::string(path)), version);
}

void RecordsOfEminence::ReadCategory(const std::wstring& path, roefmt::Version version)
{
	std::vector<RoeCategoryDatum> records;
	WithCategoryLayout(version, [&](auto file) {
		file.Read(path, roefmt::Schema::CATEGORY);
		records = std::move(file.data);
	});

	categoryData = std::move(records);
	layoutVersion = version;
}

void RecordsOfEminence::WriteCategory(const char* path)
{
	WriteCategory(xybase::string::sys_mbs_to_wcs(std::string(path)));
}

void RecordsOfEminence::WriteCategory(const std::wstring& path)
{
	WithCategoryLayout(layoutVersion, [&](auto file) { file.Write(path, categoryData); });
}

void RecordsOfEminence::CategoryToICsv(const char* path)
{
	const std::wstring wpath = xybase::string::sys_mbs_to_wcs(std::string(path));
	WithCategoryLayout(layoutVersion, [&](auto file) {
		slotfile::WriteICsv<decltype(file)::LayoutType>(wpath, categoryData);
	});
}
