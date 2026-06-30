#include "RoeProcessor.h"
#include "../FinalTextProcessor.h"
#include "../TranslationDatabase.h"
#include "../Config.h"
#include "../Logger.h"
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
	FinalTextProcessor finalTextProcessor(fileDef.comment, fileDef.type);

	auto processText = [&](const std::u8string& translated, const std::u8string& original, int64_t rowOrId, int64_t colOrColId)
	{
		return finalTextProcessor.Process(translated, original, rowOrId, colOrColId);
	};

	auto processEscaped = [&](const std::u8string& translated, const std::u8string& original, int64_t rowOrId, int64_t colOrColId)
	{
		return xybase::string::unescape(finalTextProcessor.ProcessEscaped(translated, original, rowOrId, colOrColId));
	};

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

		// Source validation
		bool srcValidationEnabled = Config::Instance().IsSrcValidationEnabled();
		std::map<uint32_t, RoeQuestCsvTranslation> srcCsvTranslations;
		if (srcValidationEnabled && csvLoader.HasSrcCsv(fileDef.comment))
		{
			srcCsvTranslations = csvLoader.LoadRoeQuestCsvTranslations(
				csvLoader.GetSrcCsvPath(fileDef.comment));
		}

		for (auto& datum : roe.questData)
		{
			bool skipCsv = false;
			if (srcValidationEnabled && !srcCsvTranslations.empty())
			{
				auto itrSrc = srcCsvTranslations.find(datum.id);
				if (itrSrc != srcCsvTranslations.end())
				{
					if (datum.questName() != itrSrc->second.questName ||
						datum.description() != itrSrc->second.description ||
						datum.note() != itrSrc->second.note)
					{
						Logger::Instance().Warning(
							"RoeProcessor src validation failed for quest id=" + std::to_string(datum.id)
							+ " - falling back to TransDB");
						Logger::Instance().Note(
							"Original: " + Logger::ToUtf8(datum.questName()) + " | "
							+ Logger::ToUtf8(datum.description()) + " | "
							+ Logger::ToUtf8(datum.note()));
						skipCsv = true;
					}
				}
			}

			auto itrCsv = csvTranslations.find(datum.id);
			if (itrCsv == csvTranslations.end() || skipCsv)
			{
				// Fallback to regular translation
				auto& config = Config::Instance();
				std::u8string originalName = datum.questName();
				std::u8string alternateOriginalName;
				if (const auto altItr = alternateNamesById.find(datum.id); altItr != alternateNamesById.end())
					alternateOriginalName = altItr->second;

				if (!config.IsNoName())
					datum.setQuestName(processEscaped(
						TranslationDatabase::Instance().GetTranslation(xybase::string::escape(originalName)),
						xybase::string::escape(originalName),
						datum.id,
						1));

				std::u8string translatedDesc = processEscaped(
					TranslationDatabase::Instance().GetTranslation(xybase::string::escape(datum.description())),
					xybase::string::escape(datum.description()),
					datum.id,
					2);
				translatedDesc = ProcessorUtils::PrependBabelText(translatedDesc, originalName, alternateOriginalName);
				datum.setDescription(translatedDesc);

				datum.setNote(processEscaped(
					TranslationDatabase::Instance().GetTranslation(xybase::string::escape(datum.note())),
					xybase::string::escape(datum.note()),
					datum.id,
					3));
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
					datum.setQuestName(processText(itrCsv->second.questName, originalName, datum.id, 1));
				else
					datum.setQuestName(processEscaped(
						TranslationDatabase::Instance().GetTranslation(xybase::string::escape(originalName)),
						xybase::string::escape(originalName),
						datum.id,
						1));
			}

			std::u8string translatedDesc;
			if (!itrCsv->second.description.empty())
				translatedDesc = processText(itrCsv->second.description, datum.description(), datum.id, 2);
			else
				translatedDesc = processEscaped(
					TranslationDatabase::Instance().GetTranslation(xybase::string::escape(datum.description())),
					xybase::string::escape(datum.description()),
					datum.id,
					2);
			translatedDesc = ProcessorUtils::PrependBabelText(translatedDesc, originalName, alternateOriginalName);
			datum.setDescription(translatedDesc);

			if (!itrCsv->second.note.empty())
				datum.setNote(processText(itrCsv->second.note, datum.note(), datum.id, 3));
			else
				datum.setNote(processEscaped(
					TranslationDatabase::Instance().GetTranslation(xybase::string::escape(datum.note())),
					xybase::string::escape(datum.note()),
					datum.id,
					3));
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

					cell.Set(xybase::string::unescape(finalTextProcessor.ProcessEscaped(
						translated,
						text,
						datum.id,
						cellIndex)));
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
	FinalTextProcessor finalTextProcessor(fileDef.comment, fileDef.type);

	auto processText = [&](const std::u8string& translated, const std::u8string& original, int64_t rowOrId, int64_t colOrColId)
	{
		return finalTextProcessor.Process(translated, original, rowOrId, colOrColId);
	};

	auto processEscaped = [&](const std::u8string& translated, const std::u8string& original, int64_t rowOrId, int64_t colOrColId)
	{
		return xybase::string::unescape(finalTextProcessor.ProcessEscaped(translated, original, rowOrId, colOrColId));
	};

	auto& csvLoader = CsvTranslationLoader::Instance();

	// Check for CSV translation
	if (csvLoader.HasTranslatedCsv(fileDef.comment))
	{
		auto csvTranslations = csvLoader.LoadRoeCategoryCsvTranslations(
			csvLoader.GetTranslatedCsvPath(fileDef.comment));

		// Source validation
		bool srcValidationEnabled = Config::Instance().IsSrcValidationEnabled();
		std::map<uint32_t, std::u8string> srcCsvTranslations;
		if (srcValidationEnabled && csvLoader.HasSrcCsv(fileDef.comment))
		{
			srcCsvTranslations = csvLoader.LoadRoeCategoryCsvTranslations(
				csvLoader.GetSrcCsvPath(fileDef.comment));
		}

		for (auto& datum : roe.categoryData)
		{
			bool skipCsv = false;
			if (srcValidationEnabled && !srcCsvTranslations.empty())
			{
				auto itrSrc = srcCsvTranslations.find(datum.id);
				if (itrSrc != srcCsvTranslations.end())
				{
					if (datum.categoryName() != itrSrc->second)
					{
						Logger::Instance().Warning(
							"RoeProcessor src validation failed for category id=" + std::to_string(datum.id)
							+ " - falling back to TransDB");
						Logger::Instance().Note(
							"Original category name: " + Logger::ToUtf8(datum.categoryName())
							+ ", SRC CSV category name: " + Logger::ToUtf8(itrSrc->second));
						skipCsv = true;
					}
				}
			}

			auto itrCsv = csvTranslations.find(datum.id);
			if (itrCsv == csvTranslations.end() || skipCsv)
			{
				datum.setCategoryName(processEscaped(
					TranslationDatabase::Instance().GetTranslation(xybase::string::escape(datum.categoryName())),
					xybase::string::escape(datum.categoryName()),
					datum.id,
					1));
				continue;
			}

			if (!itrCsv->second.empty())
				datum.setCategoryName(processText(itrCsv->second, datum.categoryName(), datum.id, 1));
			else
				datum.setCategoryName(processEscaped(
					TranslationDatabase::Instance().GetTranslation(xybase::string::escape(datum.categoryName())),
					xybase::string::escape(datum.categoryName()),
					datum.id,
					1));
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

				datum.setCategoryName(xybase::string::unescape(finalTextProcessor.ProcessEscaped(
					translated,
					text,
					datum.id,
					1)));
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
