#include "FixedPhraseProcessor.h"
#include "../TranslationDatabase.h"
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

    auto& csvLoader = CsvTranslationLoader::Instance();

    // Check for CSV translation
    if (csvLoader.HasTranslatedCsv(fileDef.comment))
    {
        std::filesystem::path csvPath = csvLoader.GetTranslatedCsvPath(fileDef.comment);
        fixedPhrase.FromCsv(csvPath.wstring());

        for (auto& category : fixedPhrase.categories)
        {
            category.categoryName = ChsToSJis::Instance().ReplaceHanzi(category.categoryName);
            category.categoryPron = ChsToSJis::Instance().ReplaceHanzi(category.categoryPron);
            for (auto& entry : category.entries)
            {
                entry.text = ChsToSJis::Instance().ReplaceHanzi(entry.text);
                entry.pron = ChsToSJis::Instance().ReplaceHanzi(entry.pron);
            }
        }

        fixedPhrase.Write(outPath);
        return true;
    }

    // Try to get Japanese reference if en_as_ja is enabled
    std::vector<std::u8string> referenceTexts;
    bool useJaReference = TryGetJapaneseReference(fileDef, jpDefsByComment, referenceTexts);

    auto& db = TranslationDatabase::Instance();
    size_t textIdx = 0;

    for (auto& category : fixedPhrase.categories)
    {
        if (useJaReference && textIdx < referenceTexts.size())
        {
            category.categoryName = db.GetTranslationFromReference(category.categoryName, referenceTexts[textIdx++]);
        }
        else
        {
            category.categoryName = db.GetTranslation(category.categoryName);
            ++textIdx;
        }

        if (useJaReference && textIdx < referenceTexts.size())
        {
            category.categoryPron = db.GetTranslationFromReference(category.categoryPron, referenceTexts[textIdx++]);
        }
        else
        {
            category.categoryPron = db.GetTranslation(category.categoryPron);
            ++textIdx;
        }

        for (auto& entry : category.entries)
        {
            if (!entry.text.empty())
            {
                if (useJaReference && textIdx < referenceTexts.size())
                {
                    entry.text = db.GetTranslationFromReference(entry.text, referenceTexts[textIdx++]);
                }
                else
                {
                    entry.text = db.GetTranslation(entry.text);
                    ++textIdx;
                }
            }

            if (!entry.pron.empty())
            {
                if (useJaReference && textIdx < referenceTexts.size())
                {
                    entry.pron = db.GetTranslationFromReference(entry.pron, referenceTexts[textIdx++]);
                }
                else
                {
                    entry.pron = db.GetTranslation(entry.pron);
                    ++textIdx;
                }
            }
        }
    }

    fixedPhrase.Write(outPath);
    return true;
}
