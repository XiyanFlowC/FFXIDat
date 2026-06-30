#pragma once
#include "../FileProcessor.h"
#include <filesystem>
#include <map>
#include <string>

class EventProcessor : public FileProcessor
{
	static bool aliasMapLoaded_;
    static std::map<std::string, std::string> aliasMap_; // alias path
	static void LoadAliasMap();
	static std::filesystem::path ResolveEventSrcPath(const std::string& actorName, int actorId, int eventIndex, const std::string& zoneName);
    static std::filesystem::path ResolveEventTgtPath(const std::string& actorName, int actorId, int eventIndex, const std::string& zoneName);

    static bool IsSelectPrompt(const std::u8string& text);
	static bool StripEndingCtrlSeq(const std::u8string& text, std::u8string& strippedText, std::u8string& endingCtrlSeq);
    /**
     * @brief Build rosetta text by combining original and translated text with a separator.
     * @param originalText 
     * @param translatedText 
	 * @param seperator the separator used to seperate the original and translated text, e.g. "<lf>"
     * @param insMode 0 - before, 1 - after
	 * @return built rosetta text, e.g. "original<lf>translated" or "translated<lf>original"
     */
    static std::u8string MakeRosettaText(const std::u8string originalText, const std::u8string translatedText, const std::u8string& seperator, int insMode);


public:
    bool Process(
        const FileProcessDef& fileDef,
        const std::filesystem::path& datPath,
        const std::filesystem::path& outPath,
        const std::map<std::u8string, FileProcessDef>& jpDefsByComment) override;

    std::u8string GetSupportedType() const override { return u8"evsb"; }
};
