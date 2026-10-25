#pragma once
#include <map>
#include <filesystem>
#include <string>

#include "ProcessorUtils.h"

// One row of an item CSV: the file holds one name and one description per item
// id, the shape ProcessorUtils::ItemTextById also gives the item table on disk,
// so the loader, the id staleness check and the processors share it.
using ItemCsvTranslation = ProcessorUtils::ItemTextById;

// One row of a Records of Eminence quest CSV: the file holds one column set per
// schema, so the shape is shared by the loader, the id staleness check and the
// processors instead of being repeated next to each of them.
using RoeQuestCsvTranslation = ProcessorUtils::RoeQuestTextById;

struct QuestDMsgCsvTranslation
{
    std::u8string name;
    std::u8string description;
};

class CsvTranslationLoader
{
public:
    static CsvTranslationLoader& Instance();

    // Check if CSV exists
    bool HasTranslatedCsv(const std::u8string& comment) const;
    std::filesystem::path GetTranslatedCsvPath(const std::u8string& comment) const;

    // SRC CSV for validation
    bool HasSrcCsv(const std::u8string& comment) const;
    std::filesystem::path GetSrcCsvPath(const std::u8string& comment) const;

    // Load various CSV types
    std::map<uint32_t, ItemCsvTranslation> LoadItemCsvTranslations(const std::filesystem::path& csvPath);
    std::map<uint32_t, RoeQuestCsvTranslation> LoadRoeQuestCsvTranslations(const std::filesystem::path& csvPath);
    std::map<int, QuestDMsgCsvTranslation> LoadQuestDMsgCsvTranslations(const std::filesystem::path& csvPath);
    std::map<uint32_t, std::u8string> LoadRoeCategoryCsvTranslations(const std::filesystem::path& csvPath);

private:
    CsvTranslationLoader() = default;
    CsvTranslationLoader(const CsvTranslationLoader&) = delete;
    CsvTranslationLoader& operator=(const CsvTranslationLoader&) = delete;

    void ValidateCsvHeader(class CsvFile& csv, const std::vector<std::u8string>& expectedHeader, const std::filesystem::path& csvPath);
};
