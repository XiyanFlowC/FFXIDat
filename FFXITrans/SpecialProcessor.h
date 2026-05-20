#pragma once
#include "FileProcessor.h"

class SpecialProcessor :
    public FileProcessor
{
public:
    // Í¨¹ý FileProcessor ¼Ì³Ð
    bool Process(const FileProcessDef& fileDef, const std::filesystem::path& datPath, const std::filesystem::path& outPath, const std::map<std::u8string, FileProcessDef>& jpDefsByComment) override;
    std::u8string GetSupportedType() const override;

    bool TryProcessJobName(const FileProcessDef& fileDef, const std::filesystem::path& inputPath, const std::filesystem::path& outputPath, const std::map<std::u8string, FileProcessDef>& jpDefsByComment);
};

