#pragma once
#include "../FileProcessor.h"

class DMsgProcessor : public FileProcessor
{
public:
    bool Process(
        const FileProcessDef& fileDef,
        const std::filesystem::path& datPath,
        const std::filesystem::path& outPath,
        const std::map<std::u8string, FileProcessDef>& jpDefsByComment) override;

    std::u8string GetSupportedType() const override { return u8"dmsg"; }

private:
    bool ProcessQuestDMsg(
        const FileProcessDef& fileDef,
        const std::filesystem::path& datPath,
        const std::filesystem::path& outPath);

    bool ProcessRegularDMsg(
        const FileProcessDef& fileDef,
        const std::filesystem::path& datPath,
        const std::filesystem::path& outPath,
        const std::map<std::u8string, FileProcessDef>& jpDefsByComment);
};
