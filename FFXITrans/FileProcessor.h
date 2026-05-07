#pragma once
#include "ProcessorUtils.h"
#include <filesystem>
#include <map>

// Base class for file processors
class FileProcessor
{
public:
    virtual ~FileProcessor() = default;

    // Process a single file
    virtual bool Process(
        const FileProcessDef& fileDef,
        const std::filesystem::path& datPath,
        const std::filesystem::path& outPath,
        const std::map<std::u8string, FileProcessDef>& jpDefsByComment) = 0;

    // Get supported file type
    virtual std::u8string GetSupportedType() const = 0;

protected:
    // Helper: Get Japanese reference texts if en_as_ja is enabled
    bool TryGetJapaneseReference(
        const FileProcessDef& fileDef,
        const std::map<std::u8string, FileProcessDef>& jpDefsByComment,
        std::vector<std::u8string>& referenceTexts) const;
};
