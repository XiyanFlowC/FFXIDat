#include "StatusDataProcessor.h"
#include "../TranslationDatabase.h"
#include <StatusData.h>
#include <xystring.h>

bool StatusDataProcessor::Process(
    const FileProcessDef& fileDef,
    const std::filesystem::path& datPath,
    const std::filesystem::path& outPath,
    const std::map<std::u8string, FileProcessDef>& jpDefsByComment)
{
    StatusData statusData;
    statusData.Read(datPath);

    // Try to get Japanese reference if en_as_ja is enabled
    std::vector<std::u8string> referenceTexts;
    bool useJaReference = TryGetJapaneseReference(fileDef, jpDefsByComment, referenceTexts);

    auto& db = TranslationDatabase::Instance();
    size_t textIdx = 0;

    for (auto& datum : statusData.data)
    {
        if (!datum.description.empty())
        {
            std::u8string text = xybase::string::escape(datum.description);
            std::u8string translated;

            if (useJaReference && textIdx < referenceTexts.size())
            {
                translated = db.GetTranslationFromReference(text, referenceTexts[textIdx]);
            }
            else
            {
                translated = db.GetTranslation(text);
            }

            datum.description = xybase::string::unescape(translated);
            ++textIdx;
        }
    }

    statusData.Write(outPath);
    return true;
}
