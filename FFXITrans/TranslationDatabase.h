#pragma once
#include <map>
#include <string>
#include <unordered_set>
#include <fstream>
#include <filesystem>

class TranslationDatabase
{
public:
    static TranslationDatabase& Instance();

    // Load translations from text file pairs
    int LoadTextPair(const std::filesystem::path& textPath, const std::filesystem::path& transPath);
    int LoadText(int seq);
    int LoadSourceData();

    // Get translation
    std::u8string GetTranslation(const std::u8string& text);
    std::u8string GetTranslationFromReference(const std::u8string& sourceText, const std::u8string& referenceText);
    bool TryGetTranslationFromReference(const std::u8string& sourceText, const std::u8string& referenceText, std::u8string& translation);

    // Mismatch tracking
    void InitializeMismatchLog(const std::filesystem::path& logPath);
    void CloseMismatchLog();
    int GetMismatchCount() const { return mismatchCount; }

    // Clear data
    void Clear();

    size_t GetTranslationCount() const { return textMapping.size(); }

private:
    TranslationDatabase() = default;
    TranslationDatabase(const TranslationDatabase&) = delete;
    TranslationDatabase& operator=(const TranslationDatabase&) = delete;

    bool PrepareTextStream(std::ifstream& eye);
    void RecordMismatch(const std::u8string& text);

    std::map<std::u8string, std::u8string> textMapping;
    std::unordered_set<std::u8string> mismatchSet;
    int mismatchCount = 0;
    std::ofstream mismatchFile;
};
