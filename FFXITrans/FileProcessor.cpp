#include "FileProcessor.h"
#include "Config.h"
#include <filesystem>
#include <iostream>
#include <xystring.h>

bool FileProcessor::TryGetJapaneseReference(
    const FileProcessDef& fileDef,
    const std::map<std::u8string, FileProcessDef>& jpDefsByComment,
    std::vector<std::u8string>& referenceTexts) const
{
    namespace fs = std::filesystem;

    if (!Config::Instance().IsEnglishMode() || !Config::Instance().IsEnAsJa())
        return false;

    auto jpItr = jpDefsByComment.find(fileDef.comment);
    if (jpItr == jpDefsByComment.end() || jpItr->second.type != fileDef.type)
        return false;

    fs::path jpDatPath = Config::Instance().GetGameRoot() / (jpItr->second.path + u8".DAT");
    if (!fs::exists(jpDatPath))
        return false;

    auto currentTexts = ProcessorUtils::CollectStrings(
        Config::Instance().GetGameRoot() / (fileDef.path + u8".DAT"),
        fileDef.type,
        fileDef.cellIndicesStr);

    auto jpTexts = ProcessorUtils::CollectStrings(
        jpDatPath,
        jpItr->second.type,
        jpItr->second.cellIndicesStr);

    if (currentTexts.size() != jpTexts.size())
    {
        if (Config::Instance().IsVerbose())
        {
            std::wcout << L"\n检测到 en_as_ja，但同 comment 的 EN/JA 记录数不同，回退普通处理：["
                << xybase::string::to_wstring(fileDef.comment) << L"] EN=" << currentTexts.size()
                << L" JA=" << jpTexts.size() << std::endl;
        }
        return false;
    }

    referenceTexts = std::move(jpTexts);
    return true;
}
