#pragma once
#include "../FileProcessor.h"
#include <map>

// Special processor for ejref_tolerance mode
// Handles complex English-to-Japanese reference translation logic
class EjrefToleranceProcessor : public FileProcessor
{
public:
    bool Process(const FileProcessDef& fileDef,
        const std::filesystem::path& inputPath,
        const std::filesystem::path& outputPath,
        const std::map<std::u8string, FileProcessDef>& jpDefsByComment) override;

    std::u8string GetSupportedType() const override { return u8"ejref_special"; }

private:
    // EVSB special handlers
    bool TryProcessGevStatus(const FileProcessDef& fileDef,
        const std::filesystem::path& inputPath,
        const std::filesystem::path& outputPath,
        const std::map<std::u8string, FileProcessDef>& jpDefsByComment);

    bool TryProcessGevAction(const FileProcessDef& fileDef,
        const std::filesystem::path& inputPath,
        const std::filesystem::path& outputPath,
        const std::map<std::u8string, FileProcessDef>& jpDefsByComment);

    // EVX special handlers
    bool TryProcessAhtUrhganWhitegate(const FileProcessDef& fileDef,
        const std::filesystem::path& inputPath,
        const std::filesystem::path& outputPath,
        const std::map<std::u8string, FileProcessDef>& jpDefsByComment);

    // DMsg special handlers
    bool TryProcessShorterReference(const FileProcessDef& fileDef,
        const std::filesystem::path& inputPath,
        const std::filesystem::path& outputPath,
        const std::map<std::u8string, FileProcessDef>& jpDefsByComment);

    bool TryProcessSameRowCell0(const FileProcessDef& fileDef,
        const std::filesystem::path& inputPath,
        const std::filesystem::path& outputPath,
        const std::map<std::u8string, FileProcessDef>& jpDefsByComment);

    bool TryProcessKeyItem(const FileProcessDef& fileDef,
        const std::filesystem::path& inputPath,
        const std::filesystem::path& outputPath,
        const std::map<std::u8string, FileProcessDef>& jpDefsByComment);
};
