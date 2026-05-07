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

    // Collect strings from various file types
    std::vector<std::u8string> CollectStrings(const std::filesystem::path& datPath, const std::u8string& type, const std::u8string& cellIndicesStr);
    std::map<uint32_t, std::vector<std::u8string>> CollectItemTextsById(const std::filesystem::path& datPath, const std::u8string& type);
    std::map<int, std::vector<std::u8string>> CollectDMsgTextsById(const std::filesystem::path& datPath, const std::u8string& cellIndicesStr);
    std::map<uint32_t, std::u8string> CollectMonBridgeTextsById(const std::filesystem::path& datPath);
}

