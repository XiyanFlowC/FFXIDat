#include "CsvTranslationLoader.h"
#include "Config.h"
#include <CsvFile.h>
#include <xystring.h>
#include <stdexcept>

CsvTranslationLoader& CsvTranslationLoader::Instance()
{
    static CsvTranslationLoader instance;
    return instance;
}

bool CsvTranslationLoader::HasTranslatedCsv(const std::u8string& comment) const
{
    namespace fs = std::filesystem;
    const auto& progRoot = Config::Instance().GetProgRoot();
    fs::path csvPath = progRoot / L"text" / L"tgt" / xybase::string::to_wstring(comment + u8".csv");
    return fs::exists(csvPath) && fs::is_regular_file(csvPath);
}

std::filesystem::path CsvTranslationLoader::GetTranslatedCsvPath(const std::u8string& comment) const
{
    const auto& progRoot = Config::Instance().GetProgRoot();
    return progRoot / L"text" / L"tgt" / xybase::string::to_wstring(comment + u8".csv");
}

void CsvTranslationLoader::ValidateCsvHeader(CsvFile& csv, const std::vector<std::u8string>& expectedHeader, const std::filesystem::path& csvPath)
{
    if (csv.IsEof())
        throw std::runtime_error("CSV is empty > " + csvPath.string());

    std::vector<std::u8string> actualHeader;
    actualHeader.reserve(expectedHeader.size());

    for (size_t i = 0; i < expectedHeader.size(); ++i)
    {
        actualHeader.push_back(csv.NextCell());
        if (csv.IsEol() && i + 1 < expectedHeader.size())
            break;
    }
    csv.NextLine();

    if (actualHeader.size() != expectedHeader.size())
        throw std::runtime_error("CSV header column count mismatch > " + csvPath.string());

    for (size_t i = 0; i < expectedHeader.size(); ++i)
    {
        if (actualHeader[i] != expectedHeader[i])
            throw std::runtime_error("CSV header mismatch > " + csvPath.string());
    }
}

std::map<uint32_t, ItemCsvTranslation> CsvTranslationLoader::LoadItemCsvTranslations(const std::filesystem::path& csvPath)
{
    std::map<uint32_t, ItemCsvTranslation> result;
    CsvFile csv(csvPath, std::ios::in | std::ios::binary);
    ValidateCsvHeader(csv, { u8"ID", u8"Name", u8"Description" }, csvPath);

    while (!csv.IsEof())
    {
        std::u8string idStr = csv.NextCell();
        std::u8string name = csv.IsEol() ? u8"" : csv.NextCell();
        std::u8string description = csv.IsEol() ? u8"" : csv.NextCell();
        csv.NextLine();

        if (idStr.empty())
            continue;

        try
        {
            uint32_t id = static_cast<uint32_t>(xybase::string::stoi(idStr));
            result[id] = { name, description };
        }
        catch (const std::exception&)
        {
        }
    }

    return result;
}

std::map<uint32_t, RoeQuestCsvTranslation> CsvTranslationLoader::LoadRoeQuestCsvTranslations(const std::filesystem::path& csvPath)
{
    std::map<uint32_t, RoeQuestCsvTranslation> result;
    CsvFile csv(csvPath, std::ios::in | std::ios::binary);
    ValidateCsvHeader(csv, { u8"ID", u8"QuestName", u8"Description", u8"Note" }, csvPath);

    while (!csv.IsEof())
    {
        std::u8string idStr = csv.NextCell();
        std::u8string questName = csv.IsEol() ? u8"" : csv.NextCell();
        std::u8string description = csv.IsEol() ? u8"" : csv.NextCell();
        std::u8string note = csv.IsEol() ? u8"" : csv.NextCell();
        csv.NextLine();

        if (idStr.empty())
            continue;

        try
        {
            uint32_t id = static_cast<uint32_t>(xybase::string::stoi(idStr));
            result[id] = { questName, description, note };
        }
        catch (const std::exception&)
        {
        }
    }

    return result;
}

std::map<int, QuestDMsgCsvTranslation> CsvTranslationLoader::LoadQuestDMsgCsvTranslations(const std::filesystem::path& csvPath)
{
    std::map<int, QuestDMsgCsvTranslation> result;
    CsvFile csv(csvPath, std::ios::in | std::ios::binary);
    ValidateCsvHeader(csv, { u8"ID", u8"Name", u8"Desc" }, csvPath);

    while (!csv.IsEof())
    {
        std::u8string idStr = csv.NextCell();
        std::u8string name = csv.IsEol() ? u8"" : csv.NextCell();
        std::u8string description = csv.IsEol() ? u8"" : csv.NextCell();
        csv.NextLine();

        if (idStr.empty())
            continue;

        try
        {
            int factor = 1;
            if (idStr.starts_with(u8"-"))
            {
                idStr = idStr.substr(1);
                factor = -1;
            }
            int id = xybase::string::stoi(idStr) * factor;
            result[id] = { name, description };
        }
        catch (const std::exception&)
        {
        }
    }

    return result;
}

std::map<uint32_t, std::u8string> CsvTranslationLoader::LoadRoeCategoryCsvTranslations(const std::filesystem::path& csvPath)
{
    std::map<uint32_t, std::u8string> result;
    CsvFile csv(csvPath, std::ios::in | std::ios::binary);
    ValidateCsvHeader(csv, { u8"ID", u8"CategoryName" }, csvPath);

    while (!csv.IsEof())
    {
        std::u8string idStr = csv.NextCell();
        std::u8string categoryName = csv.IsEol() ? u8"" : csv.NextCell();
        csv.NextLine();

        if (idStr.empty())
            continue;

        try
        {
            uint32_t id = static_cast<uint32_t>(xybase::string::stoi(idStr));
            result[id] = categoryName;
        }
        catch (const std::exception&)
        {
        }
    }

    return result;
}
