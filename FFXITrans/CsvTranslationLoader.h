#pragma once
#include <map>
#include <filesystem>
#include <string>

// CSV translation structures
struct ItemCsvTranslation
{
    std::u8string name;
    std::u8string description;
};

struct RoeQuestCsvTranslation
{
    std::u8string questName;
    std::u8string description;
    std::u8string note;
};

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
