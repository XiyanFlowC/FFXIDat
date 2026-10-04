#pragma once
#pragma once
#include <string>
#include <vector>
#include <set>
#include <map>
#include <filesystem>

// File definition structure
struct FileProcessDef
{
    std::u8string path;
    std::u8string type;
    std::u8string lang;
    std::u8string comment;
    std::u8string cellIndicesStr;
};

// Helper functions
namespace ProcessorUtils
{
    // Parse cell indices like "2|3" to {2, 3}
    std::set<int> ParseCellIndices(const std::u8string& cellIndicesStr);

    // Comment type checking
    bool IsQuestDMsg(const std::u8string& comment);
    bool IsEjrefShorterReferenceComment(const std::u8string& comment);
    bool IsEjrefSameRowCell0Comment(const std::u8string& comment);
    bool IsEjrefSpecialComment(const std::u8string& comment);
    std::u8string GetCurrentLanguageCode();
    std::u8string GetAlternateLanguageCode();
    bool TryGetFileDef(const std::u8string& comment, const std::u8string& type, const std::u8string& lang, FileProcessDef& fileDef);
    std::u8string PrependBabelText(const std::u8string& translatedText, const std::u8string& currentOriginalText, const std::u8string& alternateOriginalText);

    // InsToken handling
    struct InsToken
    {
        size_t start = 0;
        size_t end = 0;
        std::vector<std::u8string> parts;
    };

    std::vector<InsToken> ParseInsTokens(const std::u8string& text);
    std::u8string BuildInsToken(const std::vector<std::u8string>& parts);
    std::u8string BuildInsKey(const std::vector<std::u8string>& parts);
    bool TryAdaptInsCategoryForEnglish(const std::u8string& englishSource, std::u8string& translated);
    bool TryAdaptInsCategoryForCurrentLanguage(const std::u8string& sourceText, std::u8string& foreignText);

    // Collect strings from various file types
    std::vector<std::u8string> CollectStrings(const std::filesystem::path& datPath, const std::u8string& type, const std::u8string& cellIndicesStr);
    std::map<uint32_t, std::vector<std::u8string>> CollectItemTextsById(const std::filesystem::path& datPath, const std::u8string& type);
    std::map<int, std::vector<std::u8string>> CollectDMsgTextsById(const std::filesystem::path& datPath, const std::u8string& cellIndicesStr);
    std::map<uint32_t, std::u8string> CollectMonBridgeTextsById(const std::filesystem::path& datPath);

    // Records of Eminence text indexed by entry id, so that an entry of one
    // language can be looked up in another language without relying on the
    // order of the records. Every text is escaped, like the other Collect*
    // helpers do, so callers compare and feed them without extra converting.
    struct RoeQuestTextById
    {
        std::u8string questName;
        std::u8string description;
        std::u8string note;
    };

    std::map<uint32_t, RoeQuestTextById> CollectRoeQuestTextsById(const std::filesystem::path& datPath, const std::u8string& type);
    std::map<uint32_t, std::u8string> CollectRoeCategoryTextsById(const std::filesystem::path& datPath, const std::u8string& type);

    // The Japanese reference text of one Records of Eminence quest cell: 0 the
    // name, 1 the description, 2 the note. The value is always fetched from the
    // current ja table by id, so it cannot be a stale pairing, and an id the ja
    // table does not hold yields an empty string.
    std::u8string GetRoeQuestReferenceAt(const std::map<uint32_t, RoeQuestTextById>& jaTextsById, uint32_t id, int cellIndex);

    // Id staleness detection of an ID indexed translation CSV.
    //
    // The translation is keyed by entry id, so it stands for the record the id
    // held when the CSV was exported. The source (text/src) CSV side of the
    // pair is a copy of the source language table at that moment, so an id
    // whose source text differs between the copy and the table on disk has been
    // relaid out or rewritten since the export: the translation next to it
    // belongs to another record and must not be applied.
    //
    // Only the ja/en records of Eminence pair has a defined source side for
    // this check. Every other language pair of the same comment has no source
    // table to compare against, and is left alone.
    std::set<uint32_t> FindStaleRoeQuestIds(
        const std::filesystem::path& jaDatPath,
        const std::u8string& jaType,
        const std::map<uint32_t, RoeQuestTextById>& srcRows);

    std::set<uint32_t> FindStaleRoeCategoryIds(
        const std::filesystem::path& jaDatPath,
        const std::u8string& jaType,
        const std::map<uint32_t, std::u8string>& srcRows);
}

