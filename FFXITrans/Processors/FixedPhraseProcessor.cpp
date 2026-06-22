#include "FixedPhraseProcessor.h"
#include "../FinalTextProcessor.h"
#include "../TranslationDatabase.h"
#include "../Config.h"
#include "../Logger.h"
#include "../CsvTranslationLoader.h"
#include "../ChsToSJis.h"
#include <FixedPhrase.h>

bool FixedPhraseProcessor::Process(
	const FileProcessDef& fileDef,
	const std::filesystem::path& datPath,
	const std::filesystem::path& outPath,
	const std::map<std::u8string, FileProcessDef>& jpDefsByComment)
{
	FixedPhrase fixedPhrase;
	fixedPhrase.Read(datPath);
	FinalTextProcessor finalTextProcessor(fileDef.comment, fileDef.type);

	auto& csvLoader = CsvTranslationLoader::Instance();

	// Check for CSV translation
	if (csvLoader.HasTranslatedCsv(fileDef.comment))
	{
		// Source validation: check if original DAT content matches SRC CSV
		bool srcValidationEnabled = Config::Instance().IsSrcValidationEnabled();
		if (srcValidationEnabled && csvLoader.HasSrcCsv(fileDef.comment))
		{
			FixedPhrase srcPhrase;
			srcPhrase.Read(datPath);
			FixedPhrase srcCsv;
			srcCsv.FromCsv(csvLoader.GetSrcCsvPath(fileDef.comment).wstring());

			// Compare original DAT categories with SRC CSV categories
			auto& srcCsvCat = srcCsv.categories;
			auto& srcDatCat = srcPhrase.categories;
			bool srcMatch = true;
			if (srcCsvCat.size() != srcDatCat.size())
			{
				srcMatch = false;
			}
			else
			{
				for (size_t i = 0; i < srcCsvCat.size() && srcMatch; ++i)
				{
					if (srcCsvCat[i].categoryName != srcDatCat[i].categoryName ||
						srcCsvCat[i].categoryPron != srcDatCat[i].categoryPron ||
						srcCsvCat[i].entries.size() != srcDatCat[i].entries.size())
					{
						srcMatch = false;
						break;
					}
					for (size_t j = 0; j < srcCsvCat[i].entries.size() && srcMatch; ++j)
					{
						if (srcCsvCat[i].entries[j].text != srcDatCat[i].entries[j].text ||
							srcCsvCat[i].entries[j].pron != srcDatCat[i].entries[j].pron)
						{
							srcMatch = false;
						}
					}
				}
			}

			if (!srcMatch)
			{
				Logger::Instance().Warning(
					"FixedPhraseProcessor src validation failed for " + Logger::ToUtf8(fileDef.comment)
					+ " — falling back to TransDB");
				goto fallback_to_transdb;
			}
		}

		{
		std::filesystem::path csvPath = csvLoader.GetTranslatedCsvPath(fileDef.comment);
		fixedPhrase.FromCsv(csvPath.wstring());

		for (size_t categoryIndex = 0; categoryIndex < fixedPhrase.categories.size(); ++categoryIndex)
		{
			auto& category = fixedPhrase.categories[categoryIndex];
			category.categoryName = finalTextProcessor.Process(
				category.categoryName,
				category.categoryName,
				static_cast<int64_t>(category.cat.cat),
				1);
			category.categoryPron = finalTextProcessor.Process(
				category.categoryPron,
				category.categoryPron,
				static_cast<int64_t>(category.cat.cat),
				2);
			for (auto& entry : category.entries)
			{
				entry.text = finalTextProcessor.Process(
					entry.text,
					entry.text,
					static_cast<int64_t>(entry.cat.ent),
					1);
				entry.pron = finalTextProcessor.Process(
					entry.pron,
					entry.pron,
					static_cast<int64_t>(entry.cat.ent),
					2);
			}
		}

		fixedPhrase.Write(outPath);
		return true;
		}
	}

fallback_to_transdb:
	// Try to get Japanese reference if en_as_ja is enabled
	std::vector<std::u8string> referenceTexts;
	bool useJaReference = TryGetJapaneseReference(fileDef, jpDefsByComment, referenceTexts);

	auto& db = TranslationDatabase::Instance();
	size_t textIdx = 0;

	for (auto& category : fixedPhrase.categories)
	{
		if (useJaReference && textIdx < referenceTexts.size())
		{
			category.categoryName = finalTextProcessor.Process(
				db.GetTranslationFromReference(category.categoryName, referenceTexts[textIdx++]),
				category.categoryName,
				static_cast<int64_t>(category.cat.cat),
				1);
		}
		else
		{
			category.categoryName = finalTextProcessor.Process(
				db.GetTranslation(category.categoryName),
				category.categoryName,
				static_cast<int64_t>(category.cat.cat),
				1);
			++textIdx;
		}

		if (useJaReference && textIdx < referenceTexts.size())
		{
			category.categoryPron = finalTextProcessor.Process(
				db.GetTranslationFromReference(category.categoryPron, referenceTexts[textIdx++]),
				category.categoryPron,
				static_cast<int64_t>(category.cat.cat),
				2);
		}
		else
		{
			category.categoryPron = finalTextProcessor.Process(
				db.GetTranslation(category.categoryPron),
				category.categoryPron,
				static_cast<int64_t>(category.cat.cat),
				2);
			++textIdx;
		}

		for (auto& entry : category.entries)
		{
			if (!entry.text.empty())
			{
				if (useJaReference && textIdx < referenceTexts.size())
				{
					entry.text = finalTextProcessor.Process(
						db.GetTranslationFromReference(entry.text, referenceTexts[textIdx++]),
						entry.text,
						static_cast<int64_t>(entry.cat.ent),
						1);
				}
				else
				{
					entry.text = finalTextProcessor.Process(
						db.GetTranslation(entry.text),
						entry.text,
						static_cast<int64_t>(entry.cat.ent),
						1);
					++textIdx;
				}
			}

			if (!entry.pron.empty())
			{
				if (useJaReference && textIdx < referenceTexts.size())
				{
					entry.pron = finalTextProcessor.Process(
						db.GetTranslationFromReference(entry.pron, referenceTexts[textIdx++]),
						entry.pron,
						static_cast<int64_t>(entry.cat.ent),
						2);
				}
				else
				{
					entry.pron = finalTextProcessor.Process(
						db.GetTranslation(entry.pron),
						entry.pron,
						static_cast<int64_t>(entry.cat.ent),
						2);
					++textIdx;
				}
			}
		}
	}

	fixedPhrase.Write(outPath);
	return true;
}
