#include "XisProcessor.h"
#include "../TranslationDatabase.h"
#include "../Config.h"
#include <XiString.h>
#include <xystring.h>

bool XisProcessor::Process(
    const FileProcessDef& fileDef,
    const std::filesystem::path& datPath,
    const std::filesystem::path& outPath,
    const std::map<std::u8string, FileProcessDef>& jpDefsByComment)
{
    XiString xis(datPath);
    xis.Read();

    // Try to get Japanese reference if en_as_ja is enabled
    std::vector<std::u8string> referenceTexts;
    bool useJaReference = TryGetJapaneseReference(fileDef, jpDefsByComment, referenceTexts);

    auto& db = TranslationDatabase::Instance();
    size_t textIdx = 0;

    for (auto& str : xis)
    {
        std::u8string text = xybase::string::escape(xybase::string::to_utf8(xis.Decode(str.str)));
        std::u8string translated;

        if (useJaReference && textIdx < referenceTexts.size())
        {
            translated = db.GetTranslationFromReference(text, referenceTexts[textIdx]);
        }
        else
        {
            translated = db.GetTranslation(text);
        }

        str.str = xis.Encode(xybase::string::to_string(xybase::string::unescape(translated)));
        ++textIdx;
    }

    xis.path = outPath;
    xis.Write();
    return true;
}
