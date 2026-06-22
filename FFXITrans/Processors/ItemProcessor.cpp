#include "ItemProcessor.h"
#include "../FinalTextProcessor.h"
#include "../TranslationDatabase.h"
#include "../Config.h"
#include "../Logger.h"
#include "../ProcessorUtils.h"
#include "../CsvTranslationLoader.h"
#include "../ChsToSJis.h"
#include <ItemData.h>
#include <xystring.h>

bool ItemProcessor::SupportsType(const std::u8string& type) const
{
	return type == u8"iab" || type == u8"iwb" || type == u8"iub" || type == u8"inb"
		|| type == u8"ipb" || type == u8"isb" || type == u8"icb" || type == u8"iib";
}

ItemSpecType GetItemSpecType(const std::u8string& type)
{
	if (type == u8"iab") return ItemSpecType::ARMOUR;
	if (type == u8"iwb") return ItemSpecType::WEAPON;
	if (type == u8"iub") return ItemSpecType::USABLE;
	if (type == u8"ipb") return ItemSpecType::PUPPET;
	if (type == u8"isb") return ItemSpecType::SLIP;
	if (type == u8"icb") return ItemSpecType::CURRENCY;
	if (type == u8"iib") return ItemSpecType::INSTINCT;
	return ItemSpecType::NORMAL;
}

bool ItemProcessor::Process(
	const FileProcessDef& fileDef,
	const std::filesystem::path& datPath,
	const std::filesystem::path& outPath,
	const std::map<std::u8string, FileProcessDef>& jpDefsByComment)
{
	// Parse cell indices
	std::set<int> targetCells = ProcessorUtils::ParseCellIndices(fileDef.cellIndicesStr);
	bool translateAllCells = targetCells.empty();
	FinalTextProcessor finalTextProcessor(fileDef.comment, fileDef.type);

	auto processText = [&](const std::u8string& translated, const std::u8string& original, int64_t rowOrId, int64_t colOrColId)
	{
		return finalTextProcessor.Process(translated, original, rowOrId, colOrColId);
	};

	auto processEscaped = [&](const std::u8string& translated, const std::u8string& original, int64_t rowOrId, int64_t colOrColId)
	{
		return xybase::string::unescape(finalTextProcessor.ProcessEscaped(translated, original, rowOrId, colOrColId));
	};

	ItemData itemData;
	ItemSpecType specType = GetItemSpecType(fileDef.type);
	itemData.Read(datPath, specType);

	std::map<uint32_t, std::u8string> alternateNamesById;
	if (Config::Instance().IsBabelAlternateOriginalEnabled())
	{
		FileProcessDef alternateDef;
		if (ProcessorUtils::TryGetFileDef(fileDef.comment, fileDef.type, ProcessorUtils::GetAlternateLanguageCode(), alternateDef))
		{
			auto alternateDatPath = Config::Instance().GetGameRoot() / (alternateDef.path + u8".DAT");
			if (std::filesystem::exists(alternateDatPath))
			{
				auto alternateTextsById = ProcessorUtils::CollectItemTextsById(alternateDatPath, fileDef.type);
				for (const auto& [id, texts] : alternateTextsById)
				{
					if (!texts.empty())
						alternateNamesById[id] = xybase::string::unescape(texts.front());
				}
			}
		}
	}

	auto& db = TranslationDatabase::Instance();
	auto& csvLoader = CsvTranslationLoader::Instance();

	// Check for CSV translation
	if (csvLoader.HasTranslatedCsv(fileDef.comment))
	{
		auto csvTranslations = csvLoader.LoadItemCsvTranslations(
			csvLoader.GetTranslatedCsvPath(fileDef.comment));

		// Source validation: check if original DAT text matches SRC CSV
		bool srcValidationEnabled = Config::Instance().IsSrcValidationEnabled();
		std::map<uint32_t, ItemCsvTranslation> srcCsvTranslations;
		if (srcValidationEnabled && csvLoader.HasSrcCsv(fileDef.comment))
		{
			srcCsvTranslations = csvLoader.LoadItemCsvTranslations(
				csvLoader.GetSrcCsvPath(fileDef.comment));
		}

		for (auto& datum : itemData.data)
		{
			bool skipCsv = false;
			if (srcValidationEnabled && !srcCsvTranslations.empty())
			{
				auto itrSrc = srcCsvTranslations.find(datum.id);
				if (itrSrc != srcCsvTranslations.end())
				{
					std::u8string originalName = datum.name();
					std::u8string originalDesc = datum.description();
					if (xybase::string::escape(originalName) != itrSrc->second.name ||
						xybase::string::escape(originalDesc) != itrSrc->second.description)
					{
						Logger::Instance().Warning(
							"ItemProcessor src validation failed for id=" + std::to_string(datum.id)
							+ " — falling back to TransDB");
						skipCsv = true;
					}
				}
			}

			if (skipCsv)
			{
				// Fallback to regular translation
				auto& config = Config::Instance();
				std::u8string originalName = datum.name();
				std::u8string alternateOriginalName;
				if (const auto altItr = alternateNamesById.find(datum.id); altItr != alternateNamesById.end())
					alternateOriginalName = altItr->second;

				if ((translateAllCells || targetCells.count(1)) && !config.IsNoName())
					datum.setName(processEscaped(
						db.GetTranslation(xybase::string::escape(originalName)),
						xybase::string::escape(originalName),
						datum.id,
						1));

				if (translateAllCells || targetCells.count(2))
				{
					std::u8string translatedDesc = processEscaped(
						db.GetTranslation(xybase::string::escape(datum.description())),
						xybase::string::escape(datum.description()),
						datum.id,
						2);
					translatedDesc = ProcessorUtils::PrependBabelText(translatedDesc, originalName, alternateOriginalName);
					datum.setDescription(translatedDesc);
				}
				continue;
			}

			auto itrCsv = csvTranslations.find(datum.id);

			if (itrCsv == csvTranslations.end())
			{
				// Fallback to regular translation
				auto& config = Config::Instance();
				std::u8string originalName = datum.name();
				std::u8string alternateOriginalName;
				if (const auto altItr = alternateNamesById.find(datum.id); altItr != alternateNamesById.end())
					alternateOriginalName = altItr->second;

				if ((translateAllCells || targetCells.count(1)) && !config.IsNoName())
					datum.setName(processEscaped(
						db.GetTranslation(xybase::string::escape(originalName)),
						xybase::string::escape(originalName),
						datum.id,
						1));

				if (translateAllCells || targetCells.count(2))
				{
					std::u8string translatedDesc = processEscaped(
						db.GetTranslation(xybase::string::escape(datum.description())),
						xybase::string::escape(datum.description()),
						datum.id,
						2);
					translatedDesc = ProcessorUtils::PrependBabelText(translatedDesc, originalName, alternateOriginalName);
					datum.setDescription(translatedDesc);
				}
				continue;
			}

			// Apply CSV translation
			auto& config = Config::Instance();
			std::u8string originalName = datum.name();
			std::u8string alternateOriginalName;
			if (const auto altItr = alternateNamesById.find(datum.id); altItr != alternateNamesById.end())
				alternateOriginalName = altItr->second;

			if (!config.IsNoName())
			{
				if (!itrCsv->second.name.empty() && (translateAllCells || targetCells.count(1)))
				{
					auto convertedName = processText(itrCsv->second.name, originalName, datum.id, 1);
					datum.setName(convertedName);
					datum.setName_sg(convertedName);
					datum.setName_pl(convertedName);
				}
				else if (translateAllCells || targetCells.count(1))
				{
					datum.setName(processEscaped(
						db.GetTranslation(xybase::string::escape(originalName)),
						xybase::string::escape(originalName),
						datum.id,
						1));
				}
			}

			if (translateAllCells || targetCells.count(2))
			{
				std::u8string translatedDesc;
				if (!itrCsv->second.description.empty())
				{
					translatedDesc = processText(itrCsv->second.description, datum.description(), datum.id, 2);
				}
				else
				{
					translatedDesc = processEscaped(
						db.GetTranslation(xybase::string::escape(datum.description())),
						xybase::string::escape(datum.description()),
						datum.id,
						2);
				}

				translatedDesc = ProcessorUtils::PrependBabelText(translatedDesc, originalName, alternateOriginalName);

				datum.setDescription(translatedDesc);
			}
		}

		itemData.Write(outPath);
		return true;
	}

	// Check for ID-mapped reference (en_as_ja mode)
	std::map<uint32_t, std::vector<std::u8string>> jpTextsById;
	if (Config::Instance().IsEnglishMode() && Config::Instance().IsEnAsJa())
	{
		auto jpItr = jpDefsByComment.find(fileDef.comment);
		if (jpItr != jpDefsByComment.end() && jpItr->second.type == fileDef.type)
		{
			std::filesystem::path jpDatPath = Config::Instance().GetGameRoot() / (jpItr->second.path + u8".DAT");
			if (std::filesystem::exists(jpDatPath))
			{
				jpTextsById = ProcessorUtils::CollectItemTextsById(jpDatPath, fileDef.type);
			}
		}
	}

	// Try regular Japanese reference
	std::vector<std::u8string> referenceTexts;
	bool useJaReference = jpTextsById.empty() && TryGetJapaneseReference(fileDef, jpDefsByComment, referenceTexts);

	size_t textIdx = 0;

	for (auto& datum : itemData.data)
	{
		// Use ID-mapped reference if available
		auto& config = Config::Instance();
		std::u8string originalName = datum.name();
		std::u8string alternateOriginalName;
		if (const auto altItr = alternateNamesById.find(datum.id); altItr != alternateNamesById.end())
			alternateOriginalName = altItr->second;

		if (!jpTextsById.empty())
		{
			auto jpTextItr = jpTextsById.find(datum.id);
			if (jpTextItr != jpTextsById.end())
			{
				if (!jpTextItr->second.empty() && (translateAllCells || targetCells.count(1)) && !config.IsNoName())
				{
					auto translatedName = db.GetTranslationFromReference(
						xybase::string::escape(originalName), jpTextItr->second[0]);
					auto unescapedName = processEscaped(
						translatedName,
						xybase::string::escape(originalName),
						datum.id,
						1);
					datum.setName(unescapedName);
					datum.setName_sg(unescapedName);
					datum.setName_pl(unescapedName);
				}
				if (jpTextItr->second.size() >= 2 && (translateAllCells || targetCells.count(2)))
				{
					auto translatedDesc = db.GetTranslationFromReference(
						xybase::string::escape(datum.description()), jpTextItr->second.back());
					std::u8string finalDesc = processEscaped(
						translatedDesc,
						xybase::string::escape(datum.description()),
						datum.id,
						2);
					finalDesc = ProcessorUtils::PrependBabelText(finalDesc, originalName, alternateOriginalName);
					datum.setDescription(finalDesc);
				}
				continue;
			}
		}

		// Use cell-based processing
		int cellIndex = 1;
		for (auto& cell : datum.row())
		{
			if (cell.GetType() == 0) // string cell
			{
				bool shouldTranslate = translateAllCells || targetCells.count(cellIndex) > 0;

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

	itemData.Write(outPath);
	return true;
}
