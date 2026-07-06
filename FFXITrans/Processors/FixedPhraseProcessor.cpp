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
					+ " - SKIP processing due to the nature of fixed phrase file.");
				return false;
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
					u8"",
					static_cast<int64_t>(entry.cat.ent),
					1);
				entry.pron = finalTextProcessor.Process(
					entry.pron,
					u8"",
					static_cast<int64_t>(entry.cat.ent),
					2);
			}
		}

		fixedPhrase.Write(outPath);
		return true;
		}
	}

	fixedPhrase.Write(outPath);
	return true;
}
