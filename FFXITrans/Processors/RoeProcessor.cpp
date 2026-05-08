#include "RoeProcessor.h"
#include "../TranslationDatabase.h"
#include "../Config.h"
#include "../CsvTranslationLoader.h"
#include "../ProcessorUtils.h"
#include "../ChsToSJis.h"
#include <RecordsOfEminence.h>
#include <xystring.h>

bool RoeProcessor::Process(
    const FileProcessDef& fileDef,
    const std::filesystem::path& datPath,
    const std::filesystem::path& outPath,
    const std::map<std::u8string, FileProcessDef>& jpDefsByComment)
{
    if (fileDef.type == u8"erq")
    {
        return ProcessQuestData(fileDef, datPath, outPath, jpDefsByComment);
    }
    else if (fileDef.type == u8"erc")
    {
        return ProcessCategoryData(fileDef, datPath, outPath, jpDefsByComment);
    }
    return false;
}

bool RoeProcessor::ProcessQuestData(
    const FileProcessDef& fileDef,
    const std::filesystem::path& datPath,
    const std::filesystem::path& outPath,
    const std::map<std::u8string, FileProcessDef>& jpDefsByComment)
{
    RecordsOfEminence roe;
    roe.ReadQuest(datPath);

    std::map<uint32_t, std::u8string> alternateNamesById;
    if (Config::Instance().IsBabelAlternateOriginalEnabled())
    {
        FileProcessDef alternateDef;
        if (ProcessorUtils::TryGetFileDef(fileDef.comment, fileDef.type, ProcessorUtils::GetAlternateLanguageCode(), alternateDef))
        {
            auto alternateDatPath = Config::Instance().GetGameRoot() / (alternateDef.path + u8".DAT");
            if (std::filesystem::exists(alternateDatPath))
            {
                RecordsOfEminence alternateRoe;
                alternateRoe.ReadQuest(alternateDatPath);
                for (const auto& alternateDatum : alternateRoe.questData)
                {
                    alternateNamesById[alternateDatum.id] = alternateDatum.questName();
                }
            }
        }
    }

    auto& csvLoader = CsvTranslationLoader::Instance();

    // Check for CSV translation
    if (csvLoader.HasTranslatedCsv(fileDef.comment))
    {
        auto csvTranslations = csvLoader.LoadRoeQuestCsvTranslations(
            csvLoader.GetTranslatedCsvPath(fileDef.comment));

        for (auto& datum : roe.questData)
        {
            auto itrCsv = csvTranslations.find(datum.id);
            if (itrCsv == csvTranslations.end())
            {
                // Fallback to regular translation
                auto& config = Config::Instance();
                std::u8string originalName = datum.questName();
                std::u8string alternateOriginalName;
                if (const auto altItr = alternateNamesById.find(datum.id); altItr != alternateNamesById.end())
                    alternateOriginalName = altItr->second;

                if (!config.IsNoName())
                    datum.setQuestName(ChsToSJis::Instance().ReplaceHanzi(
                        xybase::string::unescape(TranslationDatabase::Instance().GetTranslation(
                            xybase::string::escape(originalName)))));

                std::u8string translatedDesc = ChsToSJis::Instance().ReplaceHanzi(
                    xybase::string::unescape(TranslationDatabase::Instance().GetTranslation(
                        xybase::string::escape(datum.description()))));
                translatedDesc = ProcessorUtils::PrependBabelText(translatedDesc, originalName, alternateOriginalName);
                datum.setDescription(translatedDesc);

                datum.setNote(ChsToSJis::Instance().ReplaceHanzi(
                    xybase::string::unescape(TranslationDatabase::Instance().GetTranslation(
                        xybase::string::escape(datum.note())))));
                continue;
            }

            auto& config = Config::Instance();
            std::u8string originalName = datum.questName();
            std::u8string alternateOriginalName;
            if (const auto altItr = alternateNamesById.find(datum.id); altItr != alternateNamesById.end())
                alternateOriginalName = altItr->second;

            if (!config.IsNoName())
            {
                if (!itrCsv->second.questName.empty())
                    datum.setQuestName(ChsToSJis::Instance().ReplaceHanzi(itrCsv->second.questName));
                else
                    datum.setQuestName(ChsToSJis::Instance().ReplaceHanzi(
                        xybase::string::unescape(TranslationDatabase::Instance().GetTranslation(
                            xybase::string::escape(originalName)))));
            }

            std::u8string translatedDesc;
            if (!itrCsv->second.description.empty())
                translatedDesc = ChsToSJis::Instance().ReplaceHanzi(itrCsv->second.description);
            else
                translatedDesc = ChsToSJis::Instance().ReplaceHanzi(
                    xybase::string::unescape(TranslationDatabase::Instance().GetTranslation(
                        xybase::string::escape(datum.description()))));
            translatedDesc = ProcessorUtils::PrependBabelText(translatedDesc, originalName, alternateOriginalName);
            datum.setDescription(translatedDesc);

            if (!itrCsv->second.note.empty())
                datum.setNote(ChsToSJis::Instance().ReplaceHanzi(itrCsv->second.note));
            else
                datum.setNote(ChsToSJis::Instance().ReplaceHanzi(
                    xybase::string::unescape(TranslationDatabase::Instance().GetTranslation(
                        xybase::string::escape(datum.note())))));
        }

        roe.WriteQuest(outPath);
        return true;
    }

    // Regular translation with cell-based processing
    std::set<int> targetCells = ProcessorUtils::ParseCellIndices(fileDef.cellIndicesStr);

    // Try to get Japanese reference if en_as_ja is enabled
    std::vector<std::u8string> referenceTexts;
    bool useJaReference = TryGetJapaneseReference(fileDef, jpDefsByComment, referenceTexts);

    auto& db = TranslationDatabase::Instance();
    size_t textIdx = 0;

    for (auto& datum : roe.questData)
    {
        int cellIndex = 1;
        for (auto& cell : datum.row())
        {
            if (cell.GetType() == 0) // string cell
            {
                bool shouldTranslate = targetCells.empty() || targetCells.count(cellIndex) > 0;

                if (shouldTranslate)
                {
                    std::u8string text = xybase::string::escape(cell.Get<std::u8string>());
                    std::u8string translated;

                    if (useJaReference && textIdx < referenceTexts.size())
                    {
                        translated = db.GetTranslationFromReference(text, referenceTexts[textIdx]);
                    }
                    else
                    {
                        translated = db.GetTranslation(text);
                    }

                    cell.Set(xybase::string::unescape(translated));
                    ++textIdx;
                }
            }
            ++cellIndex;
        }
    }

    roe.WriteQuest(outPath);
    return true;
}

bool RoeProcessor::ProcessCategoryData(
    const FileProcessDef& fileDef,
    const std::filesystem::path& datPath,
    const std::filesystem::path& outPath,
    const std::map<std::u8string, FileProcessDef>& jpDefsByComment)
{
    RecordsOfEminence roe;
    roe.ReadCategory(datPath);

    auto& csvLoader = CsvTranslationLoader::Instance();

    // Check for CSV translation
    if (csvLoader.HasTranslatedCsv(fileDef.comment))
    {
        auto csvTranslations = csvLoader.LoadRoeCategoryCsvTranslations(
            csvLoader.GetTranslatedCsvPath(fileDef.comment));

        for (auto& datum : roe.categoryData)
        {
            auto itrCsv = csvTranslations.find(datum.id);
            if (itrCsv == csvTranslations.end())
            {
                datum.setCategoryName(ChsToSJis::Instance().ReplaceHanzi(
                    xybase::string::unescape(TranslationDatabase::Instance().GetTranslation(
                        xybase::string::escape(datum.categoryName())))));
                continue;
            }

            if (!itrCsv->second.empty())
                datum.setCategoryName(ChsToSJis::Instance().ReplaceHanzi(itrCsv->second));
            else
                datum.setCategoryName(ChsToSJis::Instance().ReplaceHanzi(
                    xybase::string::unescape(TranslationDatabase::Instance().GetTranslation(
                        xybase::string::escape(datum.categoryName())))));
        }

        roe.WriteCategory(outPath);
        return true;
    }

    // Try to get Japanese reference if en_as_ja is enabled
    std::vector<std::u8string> referenceTexts;
    bool useJaReference = TryGetJapaneseReference(fileDef, jpDefsByComment, referenceTexts);

    auto& db = TranslationDatabase::Instance();
    size_t textIdx = 0;

    for (auto& datum : roe.categoryData)
    {
        try
        {
            std::u8string catName = datum.categoryName();
            if (!catName.empty())
            {
                std::u8string text = xybase::string::escape(catName);
                std::u8string translated;

                if (useJaReference && textIdx < referenceTexts.size())
                {
                    translated = db.GetTranslationFromReference(text, referenceTexts[textIdx]);
                }
                else
                {
                    translated = db.GetTranslation(text);
                }

                datum.setCategoryName(xybase::string::unescape(translated));
                ++textIdx;
            }
        }
        catch (...)
        {
            // Ignore if field doesn't exist
        }
    }

    roe.WriteCategory(outPath);
    return true;
}
