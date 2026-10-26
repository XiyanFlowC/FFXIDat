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
	const std::u8string base = StripLegacyTypeSuffix(type);
	return base == u8"iab" || base == u8"iwb" || base == u8"iub" || base == u8"inb"
		|| base == u8"ipb" || base == u8"isb" || base == u8"icb" || base == u8"iib";
}

ItemSpecType GetItemSpecType(const std::u8string& type)
{
	const std::u8string base = StripLegacyTypeSuffix(type);
	if (base == u8"iab") return ItemSpecType::ARMOUR;
	if (base == u8"iwb") return ItemSpecType::WEAPON;
	if (base == u8"iub") return ItemSpecType::USABLE;
	if (base == u8"ipb") return ItemSpecType::PUPPET;
	if (base == u8"isb") return ItemSpecType::SLIP;
	if (base == u8"icb") return ItemSpecType::CURRENCY;
	if (base == u8"iib") return ItemSpecType::INSTINCT;
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
	itemData.Read(datPath, specType, VersionForTypeCode(fileDef.type));

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
					if (!texts.name.empty())
						alternateNamesById[id] = xybase::string::unescape(texts.name);
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

		// Id staleness detection, the item counterpart of the Records of
		// Eminence check in RoeProcessor. The CSV is keyed by item id, so a row
		// stands for the item the id held when the CSV was exported; comparing
		// the source side of the pair against the current ja item table tells
		// which ids have been relaid out or rewritten since then, and those rows
		// must not be applied.
		//
		// The same ja table is what the fallback needs as well: an English run's
		// translation database may hold ja keys, so a field whose CSV row is
		// missing or stale is first looked up by the ja text of its id, with the
		// English text as the last resort instead of writing the ja text into the
		// English DAT.
		std::map<uint32_t, ProcessorUtils::ItemTextById> jaTextsById;

		std::set<uint32_t> staleIds;
		if (Config::Instance().IsEnglishMode() && Config::Instance().IsEnAsJa() && csvLoader.HasSrcCsv(fileDef.comment))
		{
			auto jpItr = jpDefsByComment.find(fileDef.comment);
			if (jpItr != jpDefsByComment.end() && jpItr->second.type == fileDef.type)
			{
				std::filesystem::path jaDatPath = Config::Instance().GetGameRoot() / (jpItr->second.path + u8".DAT");
				if (std::filesystem::exists(jaDatPath))
				{
					jaTextsById = ProcessorUtils::CollectItemTextsById(jaDatPath, jpItr->second.type);

					auto jaSrcRows = csvLoader.LoadItemCsvTranslations(
						csvLoader.GetSrcCsvPath(fileDef.comment));

					staleIds = ProcessorUtils::FindStaleItemIds(jaDatPath, jpItr->second.type, jaSrcRows);

					Logger::Instance().Info("ItemProcessor id staleness check on item id. comment=" + Logger::ToUtf8(fileDef.comment)
						+ ", srcRows=" + std::to_string(jaSrcRows.size())
						+ ", staleIds=" + std::to_string(staleIds.size()));
				}
			}
		}

		// Resolve one field of one item through the translation database: the ja
		// reference of this very id is preferred, because that is the key an
		// English run's database may hold, and the English text is the fallback
		// when the database does not know it.
		auto resolveTranslation = [&](const std::u8string& englishText, uint32_t id, int cellIndex)
		{
			std::u8string reference;
			if (auto jaItr = jaTextsById.find(id); jaItr != jaTextsById.end())
				reference = cellIndex == 1 ? jaItr->second.name : jaItr->second.description;

			return TranslationDatabase::Instance().GetTranslationPreferringReference(
				xybase::string::escape(englishText), reference);
		};

		for (auto datum : itemData.data)
		{
			bool skipCsv = staleIds.count(datum.id()) > 0;
			if (skipCsv)
			{
				Logger::Instance().Warning(
					"ItemProcessor id staleness rejected item id=" + std::to_string(datum.id())
					+ " - falling back to TransDB");
			}

			if (srcValidationEnabled && !srcCsvTranslations.empty())
			{
				auto itrSrc = srcCsvTranslations.find(datum.id());
				if (itrSrc != srcCsvTranslations.end())
				{
					std::u8string originalName = datum.name();
					std::u8string originalDesc = datum.description();
					if (originalName != itrSrc->second.name ||
						originalDesc != itrSrc->second.description)
					{
						Logger::Instance().Warning(
							"ItemProcessor src validation failed for id=" + std::to_string(datum.id())
							+ " - falling back to TransDB");
						Logger::Instance().Note(
							"Original name: " + Logger::ToUtf8(originalName)
							+ ", SRC CSV name: " + Logger::ToUtf8(itrSrc->second.name)
							+ ", Original desc: " + Logger::ToUtf8(originalDesc)
							+ ", SRC CSV desc: " + Logger::ToUtf8(itrSrc->second.description));
						skipCsv = true;
					}
				}
			}

			auto itrCsv = csvTranslations.find(datum.id());

			if (itrCsv == csvTranslations.end() || skipCsv)
			{
				// Fallback to regular translation
				auto& config = Config::Instance();
				std::u8string originalName = datum.name();
				std::u8string alternateOriginalName;
				if (const auto altItr = alternateNamesById.find(datum.id()); altItr != alternateNamesById.end())
					alternateOriginalName = altItr->second;

				if ((translateAllCells || targetCells.count(1)) && !config.IsNoName())
				{
					auto res = processEscaped(
						resolveTranslation(originalName, datum.id(), 1),
						xybase::string::escape(originalName),
						datum.id(),
						1);
					datum.setName(res);
					datum.setName_pl(res);
					datum.setName_sg(res);
				}

				if (translateAllCells || targetCells.count(2))
				{
					std::u8string translatedDesc = processEscaped(
						resolveTranslation(datum.description(), datum.id(), 2),
						xybase::string::escape(datum.description()),
						datum.id(),
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
			if (const auto altItr = alternateNamesById.find(datum.id()); altItr != alternateNamesById.end())
				alternateOriginalName = altItr->second;

			if (!config.IsNoName())
			{
				if (!itrCsv->second.name.empty() && (translateAllCells || targetCells.count(1)))
				{
					auto convertedName = processText(itrCsv->second.name, originalName, datum.id(), 1);
					datum.setName(convertedName);
					datum.setName_sg(convertedName);
					datum.setName_pl(convertedName);
				}
				else if (translateAllCells || targetCells.count(1))
				{
					auto res = processEscaped(
						resolveTranslation(originalName, datum.id(), 1),
						xybase::string::escape(originalName),
						datum.id(),
						1);
					datum.setName(res);
					datum.setName_pl(res);
					datum.setName_sg(res);
				}
			}

			if (translateAllCells || targetCells.count(2))
			{
				std::u8string translatedDesc;
				if (!itrCsv->second.description.empty())
				{
					translatedDesc = processText(itrCsv->second.description, datum.description(), datum.id(), 2);
				}
				else
				{
					translatedDesc = processEscaped(
						resolveTranslation(datum.description(), datum.id(), 2),
						xybase::string::escape(datum.description()),
						datum.id(),
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
	std::map<uint32_t, ProcessorUtils::ItemTextById> jpTextsById;
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

	for (auto datum : itemData.data)
	{
		// Use ID-mapped reference if available
		auto& config = Config::Instance();
		std::u8string originalName = datum.name();
		std::u8string alternateOriginalName;
		if (const auto altItr = alternateNamesById.find(datum.id()); altItr != alternateNamesById.end())
			alternateOriginalName = altItr->second;

		if (!jpTextsById.empty())
		{
			auto jpTextItr = jpTextsById.find(datum.id());
			if (jpTextItr != jpTextsById.end())
			{
				if (!jpTextItr->second.name.empty() && (translateAllCells || targetCells.count(1)) && !config.IsNoName())
				{
					auto translatedName = db.GetTranslationFromReference(
						xybase::string::escape(originalName), jpTextItr->second.name);
					auto unescapedName = processEscaped(
						translatedName,
						xybase::string::escape(originalName),
						datum.id(),
						1);
					datum.setName(unescapedName);
					datum.setName_sg(unescapedName);
					datum.setName_pl(unescapedName);
				}
				if (!jpTextItr->second.description.empty() && (translateAllCells || targetCells.count(2)))
				{
					auto translatedDesc = db.GetTranslationFromReference(
						xybase::string::escape(datum.description()), jpTextItr->second.description);
					std::u8string finalDesc = processEscaped(
						translatedDesc,
						xybase::string::escape(datum.description()),
						datum.id(),
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
						datum.id(),
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
