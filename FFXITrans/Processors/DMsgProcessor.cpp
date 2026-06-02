#include "DMsgProcessor.h"
#include "../FinalTextProcessor.h"
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
	FinalTextProcessor finalTextProcessor(fileDef.comment, fileDef.type);

	auto processText = [&](const std::u8string& translated, const std::u8string& original, int64_t rowOrId, int64_t colOrColId)
	{
		return finalTextProcessor.Process(translated, original, rowOrId, colOrColId);
	};

	auto processEscaped = [&](const std::u8string& translated, const std::u8string& original, int64_t rowOrId, int64_t colOrColId)
	{
		return xybase::string::unescape(finalTextProcessor.ProcessEscaped(translated, original, rowOrId, colOrColId));
	};

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
					// 部分任务名有 _ 前缀，需要剔除
					auto str = xybase::string::unescape(texts.front());
					size_t startPos = str.find_first_not_of(u8'_');
					if (startPos != std::u8string::npos)
						str = str.substr(str.find_first_not_of(u8'_'));
					if (!texts.empty())
						alternateNamesById[id] = str;
				}
			}
		}
	}

	auto csvTranslations = CsvTranslationLoader::Instance().LoadQuestDMsgCsvTranslations(
		CsvTranslationLoader::Instance().GetTranslatedCsvPath(fileDef.comment));

	auto& db = TranslationDatabase::Instance();

	// Special case for sys/mis/ad and sys/mis/rov (section headers with negative IDs)
	if (fileDef.comment == u8"sys/mis/ad" || fileDef.comment == u8"sys/mis/rov")
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
					mutableCells[1].Set(processEscaped(
						db.GetTranslation(xybase::string::escape(originalName)),
						xybase::string::escape(originalName),
						sourceRowId,
						2));

				if (mutableCells.size() >= 3 && mutableCells[2].GetType() == 0)
				{
					std::u8string translatedDesc = processEscaped(
						db.GetTranslation(xybase::string::escape(mutableCells[2].Get<std::u8string>())),
						xybase::string::escape(mutableCells[2].Get<std::u8string>()),
						sourceRowId,
						3);
					if (originalName.starts_with(u8"__")) // Section header, prepend babel text to description instead of name
					{
						auto pos = originalName.find_first_not_of(u8'_');
						if (pos != std::u8string::npos)
							originalName = originalName.substr();
					}
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
					mutableCells[1].Set(processText(itrCsv->second.name, originalName, sourceRowId, 2));
				else
					mutableCells[1].Set(processEscaped(
						db.GetTranslation(xybase::string::escape(originalName)),
						xybase::string::escape(originalName),
						sourceRowId,
						2));
			}

			if (mutableCells.size() >= 3 && mutableCells[2].GetType() == 0)
			{
				std::u8string translatedDesc;
				if (!itrCsv->second.description.empty())
					translatedDesc = processText(itrCsv->second.description, mutableCells[2].Get<std::u8string>(), sourceRowId, 3);
				else
					translatedDesc = processEscaped(
						db.GetTranslation(xybase::string::escape(mutableCells[2].Get<std::u8string>())),
						xybase::string::escape(mutableCells[2].Get<std::u8string>()),
						sourceRowId,
						3);
				// Some of the original name has indent prefix like __, which should be removed when prepending to description
				if (originalName.starts_with(u8"__"))
				{
					auto pos = originalName.find_first_not_of(u8'_');
					if (pos != std::u8string::npos)
						originalName = originalName.substr(pos);
				}
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
					mutableCells[1].Set(processEscaped(
						db.GetTranslation(xybase::string::escape(originalName)),
						xybase::string::escape(originalName),
						rowId,
						2));

				if (mutableCells.size() >= 3 && mutableCells[2].GetType() == 0)
				{
					std::u8string translatedDesc = processEscaped(
						db.GetTranslation(xybase::string::escape(mutableCells[2].Get<std::u8string>())),
						xybase::string::escape(mutableCells[2].Get<std::u8string>()),
						rowId,
						3);
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
					mutableCells[1].Set(processText(itrCsv->second.name, originalName, rowId, 2));
				else
					mutableCells[1].Set(processEscaped(
						db.GetTranslation(xybase::string::escape(originalName)),
						xybase::string::escape(originalName),
						rowId,
						2));
			}

			if (mutableCells.size() >= 3 && mutableCells[2].GetType() == 0)
			{
				std::u8string translatedDesc;
				if (!itrCsv->second.description.empty())
					translatedDesc = processText(itrCsv->second.description, mutableCells[2].Get<std::u8string>(), rowId, 3);
				else
					translatedDesc = processEscaped(
						db.GetTranslation(xybase::string::escape(mutableCells[2].Get<std::u8string>())),
						xybase::string::escape(mutableCells[2].Get<std::u8string>()),
						rowId,
						3);
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
	FinalTextProcessor finalTextProcessor(fileDef.comment, fileDef.type);

	auto processEscaped = [&](const std::u8string& translated, const std::u8string& original, int64_t rowOrId, int64_t colOrColId)
	{
		return xybase::string::unescape(finalTextProcessor.ProcessEscaped(translated, original, rowOrId, colOrColId));
	};

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
					cells[1].Set(processEscaped(
						db.GetTranslation(xybase::string::escape(originalName)),
						xybase::string::escape(originalName),
						rowId,
						2));
			   translatedDesc = ProcessorUtils::PrependBabelText(finalTextProcessor.Process(translatedDesc, cells[2].Get<std::u8string>(), rowId, 3), originalName, alternateOriginalName);
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

					cell.Set(processEscaped(translated, text, rowId, colNum));
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
