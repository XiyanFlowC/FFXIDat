#pragma once
#include "../FileProcessor.h"

class ItemProcessor : public FileProcessor
{
public:
    bool Process(
        const FileProcessDef& fileDef,
        const std::filesystem::path& datPath,
        const std::filesystem::path& outPath,
        const std::map<std::u8string, FileProcessDef>& jpDefsByComment) override;

    std::u8string GetSupportedType() const override { return u8"item"; }

    // Support all item types
    bool SupportsType(const std::u8string& type) const;
};
