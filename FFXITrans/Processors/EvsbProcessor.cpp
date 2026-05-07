#include "EvsbProcessor.h"
#include "../TranslationDatabase.h"
#include "../Config.h"
#include "../ProcessorUtils.h"
#include <EventStringBase.h>
#include <xystring.h>

bool EvsbProcessor::Process(
    const FileProcessDef& fileDef,
    const std::filesystem::path& datPath,
    const std::filesystem::path& outPath,
    const std::map<std::u8string, FileProcessDef>& jpDefsByComment)
{
    EventStringBase evsb(datPath);
    evsb.Read();

    // Try to get Japanese reference if en_as_ja is enabled
    std::vector<std::u8string> referenceTexts;
    bool useJaReference = TryGetJapaneseReference(fileDef, jpDefsByComment, referenceTexts);

    auto& db = TranslationDatabase::Instance();
    size_t textIdx = 0;

    for (auto& s : evsb)
    {
        std::u8string translated;

        if (useJaReference && textIdx < referenceTexts.size())
        {
            std::u8string refTranslated;
            if (db.TryGetTranslationFromReference(s, referenceTexts[textIdx], refTranslated) 
                && ProcessorUtils::TryAdaptInsCategoryForEnglish(s, refTranslated))
            {
                translated = refTranslated;
            }
            else
            {
                translated = db.GetTranslation(s);
            }
        }
        else
        {
            translated = db.GetTranslation(s);
        }

        s = translated;
        ++textIdx;
    }

    evsb.path = outPath;
    evsb.Write();
    return true;
}
