#include "DMsgProcessor.h"
#include "../TranslationDatabase.h"
#include "../Config.h"
#include "../ProcessorUtils.h"
#include "../CsvTranslationLoader.h"
#include "../ChsToSJis.h"
#include <DMsg.h>
#include <xystring.h>

bool DMsgProcessor::Process(
    const FileProcessDef& fileDef,
    const std::filesystem::path& datPath,
    const std::filesystem::path& outPath,
    const std::map<std::u8string, FileProcessDef>& jpDefsByComment)
{
    // Check if this is a Quest DMsg with CSV translation
    if (ProcessorUtils::IsQuestDMsg(fileDef.comment) 
        && CsvTranslationLoader::Instance().HasTranslatedCsv(fileDef.comment))
    {
        return ProcessQuestDMsg(fileDef, datPath, outPath);
    }

    return ProcessRegularDMsg(fileDef, datPath, outPath, jpDefsByComment);
}

bool DMsgProcessor::ProcessQuestDMsg(
    const FileProcessDef& fileDef,
    const std::filesystem::path& datPath,
    const std::filesystem::path& outPath)
{
    DMsg dmsg(datPath);
    dmsg.Read();

    std::map<int, std::u8string> alternateNamesById;
    if (Config::Instance().IsBabelAlternateOriginalEnabled())
    {
        FileProcessDef alternateDef;
        if (ProcessorUtils::TryGetFileDef(fileDef.comment, fileDef.type, ProcessorUtils::GetAlternateLanguageCode(), alternateDef))
        {
            auto alternateDatPath = Config::Instance().GetGameRoot() / (alternateDef.path + u8".DAT");
            if (std::filesystem::exists(alternateDatPath))
            {
                auto alternateTextsById = ProcessorUtils::CollectDMsgTextsById(alternateDatPath, alternateDef.cellIndicesStr);
                for (const auto& [id, texts] : alternateTextsById)
                {
                    if (!texts.empty())
                        alternateNamesById[id] = xybase::string::unescape(texts.front());
                }
            }
        }
    }

    auto csvTranslations = CsvTranslationLoader::Instance().LoadQuestDMsgCsvTranslations(
        CsvTranslationLoader::Instance().GetTranslatedCsvPath(fileDef.comment));

    auto& db = TranslationDatabase::Instance();

    // Special case for sys/mis/ad (section headers with negative IDs)
    if (fileDef.comment == u8"sys/mis/ad")
    {
        for (auto& row : dmsg)
        {
            const auto& cells = row.GetCellsConst();
            if (cells.empty() || cells[0].GetType() != 1)
                continue;

            int rowId = cells[0].Get<int>();
            const int sourceRowId = rowId;

            // Section header detection
            if (cells.size() >= 2 && cells[1].GetType() == 0 
                && !cells[1].Get<std::u8string>().starts_with(u8"__"))
            {
                rowId = -rowId; // Use negative ID for section headers
            }

            auto itrCsv = csvTranslations.find(rowId);
            auto& mutableCells = row.GetCells();

            if (itrCsv == csvTranslations.end())
            {
                // Fallback to regular translation
                auto& config = Config::Instance();
                std::u8string originalName;
                if (mutableCells.size() >= 2 && mutableCells[1].GetType() == 0)
                    originalName = mutableCells[1].Get<std::u8string>();
                std::u8string alternateOriginalName;
                if (const auto altItr = alternateNamesById.find(sourceRowId); altItr != alternateNamesById.end())
                    alternateOriginalName = altItr->second;

                if (mutableCells.size() >= 2 && mutableCells[1].GetType() == 0 && !config.IsNoName())
                    mutableCells[1].Set(ChsToSJis::Instance().ReplaceHanzi(
                        xybase::string::unescape(db.GetTranslation(
                            xybase::string::escape(originalName)))));

                if (mutableCells.size() >= 3 && mutableCells[2].GetType() == 0)
                {
                    std::u8string translatedDesc = ChsToSJis::Instance().ReplaceHanzi(
                        xybase::string::unescape(db.GetTranslation(
                            xybase::string::escape(mutableCells[2].Get<std::u8string>()))));
                    translatedDesc = ProcessorUtils::PrependBabelText(translatedDesc, originalName, alternateOriginalName);
                    mutableCells[2].Set(translatedDesc);
                }
                continue;
            }

            // Apply CSV translation
            auto& config = Config::Instance();
            std::u8string originalName;
            if (mutableCells.size() >= 2 && mutableCells[1].GetType() == 0)
                originalName = mutableCells[1].Get<std::u8string>();
            std::u8string alternateOriginalName;
            if (const auto altItr = alternateNamesById.find(sourceRowId); altItr != alternateNamesById.end())
                alternateOriginalName = altItr->second;

            if (mutableCells.size() >= 2 && mutableCells[1].GetType() == 0 && !config.IsNoName())
            {
                if (!itrCsv->second.name.empty())
                    mutableCells[1].Set(ChsToSJis::Instance().ReplaceHanzi(itrCsv->second.name));
                else
                    mutableCells[1].Set(ChsToSJis::Instance().ReplaceHanzi(
                        xybase::string::unescape(db.GetTranslation(
                            xybase::string::escape(originalName)))));
            }

            if (mutableCells.size() >= 3 && mutableCells[2].GetType() == 0)
            {
                std::u8string translatedDesc;
                if (!itrCsv->second.description.empty())
                    translatedDesc = ChsToSJis::Instance().ReplaceHanzi(itrCsv->second.description);
                else
                    translatedDesc = ChsToSJis::Instance().ReplaceHanzi(
                        xybase::string::unescape(db.GetTranslation(
                            xybase::string::escape(mutableCells[2].Get<std::u8string>()))));
                translatedDesc = ProcessorUtils::PrependBabelText(translatedDesc, originalName, alternateOriginalName);
                mutableCells[2].Set(translatedDesc);
            }
        }
    }
    else
    {
        // Regular Quest DMsg processing
        for (auto& row : dmsg)
        {
            const auto& cells = row.GetCellsConst();
            if (cells.empty() || cells[0].GetType() != 1)
                continue;

            int rowId = cells[0].Get<int>();
            auto itrCsv = csvTranslations.find(rowId);
            auto& mutableCells = row.GetCells();

            if (itrCsv == csvTranslations.end())
            {
                // Fallback to regular translation
                auto& config = Config::Instance();
                std::u8string originalName;
                if (mutableCells.size() >= 2 && mutableCells[1].GetType() == 0)
                    originalName = mutableCells[1].Get<std::u8string>();
                std::u8string alternateOriginalName;
                if (const auto altItr = alternateNamesById.find(rowId); altItr != alternateNamesById.end())
                    alternateOriginalName = altItr->second;

                if (mutableCells.size() >= 2 && mutableCells[1].GetType() == 0 && !config.IsNoName())
                    mutableCells[1].Set(ChsToSJis::Instance().ReplaceHanzi(
                        xybase::string::unescape(db.GetTranslation(
                            xybase::string::escape(originalName)))));

                if (mutableCells.size() >= 3 && mutableCells[2].GetType() == 0)
                {
                    std::u8string translatedDesc = ChsToSJis::Instance().ReplaceHanzi(
                        xybase::string::unescape(db.GetTranslation(
                            xybase::string::escape(mutableCells[2].Get<std::u8string>()))));
                    translatedDesc = ProcessorUtils::PrependBabelText(translatedDesc, originalName, alternateOriginalName);
                    mutableCells[2].Set(translatedDesc);
                }
                continue;
            }

            // Apply CSV translation
            auto& config = Config::Instance();
            std::u8string originalName;
            if (mutableCells.size() >= 2 && mutableCells[1].GetType() == 0)
                originalName = mutableCells[1].Get<std::u8string>();
            std::u8string alternateOriginalName;
            if (const auto altItr = alternateNamesById.find(rowId); altItr != alternateNamesById.end())
                alternateOriginalName = altItr->second;

            if (mutableCells.size() >= 2 && mutableCells[1].GetType() == 0 && !config.IsNoName())
            {
                if (!itrCsv->second.name.empty())
                    mutableCells[1].Set(ChsToSJis::Instance().ReplaceHanzi(itrCsv->second.name));
                else
                    mutableCells[1].Set(ChsToSJis::Instance().ReplaceHanzi(
                        xybase::string::unescape(db.GetTranslation(
                            xybase::string::escape(originalName)))));
            }

            if (mutableCells.size() >= 3 && mutableCells[2].GetType() == 0)
            {
                std::u8string translatedDesc;
                if (!itrCsv->second.description.empty())
                    translatedDesc = ChsToSJis::Instance().ReplaceHanzi(itrCsv->second.description);
                else
                    translatedDesc = ChsToSJis::Instance().ReplaceHanzi(
                        xybase::string::unescape(db.GetTranslation(
                            xybase::string::escape(mutableCells[2].Get<std::u8string>()))));
                translatedDesc = ProcessorUtils::PrependBabelText(translatedDesc, originalName, alternateOriginalName);
                mutableCells[2].Set(translatedDesc);
            }
        }
    }

    dmsg.path = outPath;
    dmsg.Write();
    return true;
}

bool DMsgProcessor::ProcessRegularDMsg(
    const FileProcessDef& fileDef,
    const std::filesystem::path& datPath,
    const std::filesystem::path& outPath,
    const std::map<std::u8string, FileProcessDef>& jpDefsByComment)
{
    DMsg dmsg(datPath);
    dmsg.Read();

    // Parse cell indices
    std::set<int> targetCells = ProcessorUtils::ParseCellIndices(fileDef.cellIndicesStr);

	bool keyItemSpecialCase = fileDef.comment == u8"sys/key_item";
    if (keyItemSpecialCase)
    {
        if (Config::Instance().IsNoName()) {
			if (targetCells.contains(2)) targetCells.erase(2); // Don't translate name column
        }
	}

    bool translateAllCells = targetCells.empty();

    // Try regular Japanese reference
    std::vector<std::u8string> referenceTexts;
    bool useJaReference = TryGetJapaneseReference(fileDef, jpDefsByComment, referenceTexts);

    auto& db = TranslationDatabase::Instance();
    size_t textIdx = 0;

  // The following process only need when translating key items with babel enabled, as we need to append original name to description. For other cases we can just translate as normal without worrying about the order of text.
    if (keyItemSpecialCase && Config::Instance().IsBabelEnabled()) {
        std::map<int, std::u8string> alternateNamesById;
        if (Config::Instance().IsBabelAlternateOriginalEnabled())
        {
            FileProcessDef alternateDef;
            if (ProcessorUtils::TryGetFileDef(fileDef.comment, fileDef.type, ProcessorUtils::GetAlternateLanguageCode(), alternateDef))
            {
                auto alternateDatPath = Config::Instance().GetGameRoot() / (alternateDef.path + u8".DAT");
                if (std::filesystem::exists(alternateDatPath))
                {
                    auto alternateTextsById = ProcessorUtils::CollectDMsgTextsById(alternateDatPath, alternateDef.cellIndicesStr);
                    for (const auto& [id, texts] : alternateTextsById)
                    {
                        if (!texts.empty())
                            alternateNamesById[id] = xybase::string::unescape(texts.front());
                    }
                }
            }
        }

        for (auto& row : dmsg)
        {
            int rowId = 0;
            bool hasRowId = !row.GetCellsConst().empty() && row.GetCellsConst()[0].GetType() == 1;
            if (hasRowId)
            {
                rowId = row.GetCellsConst()[0].Get<int>();
            }
            auto& cells = row.GetCells();
            if (cells.size() >= 3 && cells[1].GetType() == 0 && cells[2].GetType() == 0)
            {
                std::u8string originalName = cells[1].Get<std::u8string>();
                std::u8string alternateOriginalName;
                if (const auto altItr = alternateNamesById.find(rowId); altItr != alternateNamesById.end())
                    alternateOriginalName = altItr->second;
                std::u8string translatedDesc;
				translatedDesc = xybase::string::unescape(db.GetTranslation(xybase::string::escape(cells[2].Get<std::u8string>())));
				if (!Config::Instance().IsNoName())
                    cells[1].Set(ChsToSJis::Instance().ReplaceHanzi(
                        xybase::string::unescape(db.GetTranslation(
						    xybase::string::escape(originalName)))));
               translatedDesc = ProcessorUtils::PrependBabelText(ChsToSJis::Instance().ReplaceHanzi(translatedDesc), originalName, alternateOriginalName);
                cells[2].Set(translatedDesc);
			}
        }

		dmsg.path = outPath;
        dmsg.Write();
		return true;
    }

    for (auto& row : dmsg)
    {
        int rowId = 0;
        bool hasRowId = !row.GetCellsConst().empty() && row.GetCellsConst()[0].GetType() == 1;
        if (hasRowId)
        {
            rowId = row.GetCellsConst()[0].Get<int>();
        }

        size_t rowTextIdx = 0;
        int colNum = 1;

        for (auto& cell : row)
        {
            if (cell.GetType() == 0) // string cell
            {
                bool shouldTranslate = translateAllCells || targetCells.count(colNum) > 0;

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
                    ++rowTextIdx;
                }
            }
            ++colNum;
        }
    }

    dmsg.path = outPath;
    dmsg.Write();
    return true;
}
