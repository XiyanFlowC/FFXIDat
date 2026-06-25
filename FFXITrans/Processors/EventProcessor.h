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


public:
    bool Process(
        const FileProcessDef& fileDef,
        const std::filesystem::path& datPath,
        const std::filesystem::path& outPath,
        const std::map<std::u8string, FileProcessDef>& jpDefsByComment) override;

    std::u8string GetSupportedType() const override { return u8"evsb"; }
};
