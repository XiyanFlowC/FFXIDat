#include "EjrefToleranceProcessor.h"
#include "../Config.h"
#include "../TranslationDatabase.h"
#include "../ProcessorUtils.h"
#include <EventStringBase.h>
#include <DMsg.h>
#include <xystring.h>
#include <iostream>

bool EjrefToleranceProcessor::Process(const FileProcessDef& fileDef,
    const std::filesystem::path& inputPath,
    const std::filesystem::path& outputPath,
    const std::map<std::u8string, FileProcessDef>& jpDefsByComment)
{
    // Only active in ejref_tolerance mode
    if (!(Config::Instance().IsEnglishMode() && 
          Config::Instance().IsEnAsJa() && 
          Config::Instance().IsEjrefTolerance()))
    {
        return false;
    }

    // Check if this is a special comment that needs ejref_tolerance processing
    if (!ProcessorUtils::IsEjrefSpecialComment(fileDef.comment))
    {
        return false;
    }

    // Try each specific handler
    if (fileDef.type == u8"evsb")
    {
        if (fileDef.comment == u8"gev/status")
        {
            return TryProcessGevStatus(fileDef, inputPath, outputPath, jpDefsByComment);
        }
        else if (fileDef.comment == u8"gev/action")
        {
            return TryProcessGevAction(fileDef, inputPath, outputPath, jpDefsByComment);
        }
        else if (fileDef.comment == u8"evx/Aht Urhgan Whitegate")
        {
            return TryProcessAhtUrhganWhitegate(fileDef, inputPath, outputPath, jpDefsByComment);
        }
    }
    else if (fileDef.type == u8"dmsg")
    {
        if (ProcessorUtils::IsEjrefShorterReferenceComment(fileDef.comment))
        {
            return TryProcessShorterReference(fileDef, inputPath, outputPath, jpDefsByComment);
        }
        else if (ProcessorUtils::IsEjrefSameRowCell0Comment(fileDef.comment))
        {
            return TryProcessSameRowCell0(fileDef, inputPath, outputPath, jpDefsByComment);
        }
        else if (fileDef.comment == u8"sys/key_item")
        {
            return TryProcessKeyItem(fileDef, inputPath, outputPath, jpDefsByComment);
        }
    }

    return false;
}

bool EjrefToleranceProcessor::TryProcessGevStatus(const FileProcessDef& fileDef,
    const std::filesystem::path& inputPath,
    const std::filesystem::path& outputPath,
    const std::map<std::u8string, FileProcessDef>& jpDefsByComment)
{
    auto jpItr = jpDefsByComment.find(fileDef.comment);
    if (jpItr == jpDefsByComment.end() || jpItr->second.type != fileDef.type)
        return false;

    auto jpDatPath = Config::Instance().GetGameRoot() / (jpItr->second.path + u8".DAT");
    if (!std::filesystem::exists(jpDatPath))
        return false;

    EventStringBase evsb(inputPath);
    EventStringBase jpEvsb(jpDatPath);
    evsb.Read();
    jpEvsb.Read();

    // English version has 2x the records of Japanese
    if (jpEvsb.size() * 2 != evsb.size() || jpEvsb.size() == 0)
        return false;

    auto& db = TranslationDatabase::Instance();
    for (size_t i = 0; i < evsb.size(); ++i)
    {
        evsb[i] = db.GetTranslationFromReference(evsb[i], jpEvsb[i % jpEvsb.size()]);
    }

    evsb.path = outputPath;
    evsb.Write();
    return true;
}

bool EjrefToleranceProcessor::TryProcessGevAction(const FileProcessDef& fileDef,
    const std::filesystem::path& inputPath,
    const std::filesystem::path& outputPath,
    const std::map<std::u8string, FileProcessDef>& jpDefsByComment)
{
    auto jpItr = jpDefsByComment.find(fileDef.comment);
    if (jpItr == jpDefsByComment.end() || jpItr->second.type != fileDef.type)
        return false;

    auto jpDatPath = Config::Instance().GetGameRoot() / (jpItr->second.path + u8".DAT");
    if (!std::filesystem::exists(jpDatPath))
        return false;

    EventStringBase evsb(inputPath);
    EventStringBase jpEvsb(jpDatPath);
    evsb.Read();
    jpEvsb.Read();

    auto& db = TranslationDatabase::Instance();
    for (size_t i = 0; i < evsb.size(); ++i)
    {
        if (i < jpEvsb.size())
            evsb[i] = db.GetTranslationFromReference(evsb[i], jpEvsb[i]);
        else
            evsb[i] = db.GetTranslation(evsb[i]);
    }

    evsb.path = outputPath;
    evsb.Write();
    return true;
}

bool EjrefToleranceProcessor::TryProcessAhtUrhganWhitegate(const FileProcessDef& fileDef,
    const std::filesystem::path& inputPath,
    const std::filesystem::path& outputPath,
    const std::map<std::u8string, FileProcessDef>& jpDefsByComment)
{
    // Find JP evx definition
    auto jpItr = jpDefsByComment.find(u8"evx/Aht Urhgan Whitegate");
    if (jpItr == jpDefsByComment.end() || jpItr->second.type != fileDef.type)
        return false;

    auto jpDatPath = Config::Instance().GetGameRoot() / (jpItr->second.path + u8".DAT");
    if (!std::filesystem::exists(jpDatPath))
        return false;

    // Load JP evx strings
    EventStringBase jpEvsb(jpDatPath);
    jpEvsb.Read();

    // Load ROM reference strings
    auto romPath = Config::Instance().GetGameRoot() / u8"ROM/186/97.DAT";
    if (!std::filesystem::exists(romPath))
        return false;

    EventStringBase romEvsb(romPath);
    romEvsb.Read();

    // Build mapping rom -> jp by index
    std::map<std::u8string, std::u8string> romToJp;
    size_t n = std::min(romEvsb.size(), jpEvsb.size());
    for (size_t i = 0; i < n; ++i)
    {
        romToJp[ romEvsb[i] ] = jpEvsb[i];
    }

    // Now process English file using mapping
    EventStringBase evsb(inputPath);
    evsb.Read();

    auto& db = TranslationDatabase::Instance();
    for (size_t i = 0; i < evsb.size(); ++i)
    {
        std::u8string enText = evsb[i];
        // try find jp via ROM mapping
        auto itr = romToJp.find(enText);
        if (itr != romToJp.end())
        {
            std::u8string jpText = itr->second;
            std::u8string translated = db.GetTranslationFromReference(enText, jpText);
            evsb[i] = translated;
        }
        else
        {
            // fallback to normal translation
            evsb[i] = db.GetTranslation(enText);
        }
    }

    evsb.path = outputPath;
    evsb.Write();
    return true;
}

bool EjrefToleranceProcessor::TryProcessShorterReference(const FileProcessDef& fileDef,
    const std::filesystem::path& inputPath,
    const std::filesystem::path& outputPath,
    const std::map<std::u8string, FileProcessDef>& jpDefsByComment)
{
    auto jpItr = jpDefsByComment.find(fileDef.comment);
    if (jpItr == jpDefsByComment.end() || jpItr->second.type != fileDef.type)
        return false;

    auto jpDatPath = Config::Instance().GetGameRoot() / (jpItr->second.path + u8".DAT");
    if (!std::filesystem::exists(jpDatPath))
        return false;

    DMsg dmsg(inputPath);
    dmsg.Read();
    auto jpTexts = ProcessorUtils::CollectStrings(jpDatPath, jpItr->second.type, jpItr->second.cellIndicesStr);
    std::set<int> targetCells = ProcessorUtils::ParseCellIndices(fileDef.cellIndicesStr);
    bool translateAllCells = targetCells.empty();
    size_t textIdx = 0;

    auto& db = TranslationDatabase::Instance();
    for (auto& row : dmsg)
    {
        int colNum = 1;
        for (auto& cell : row)
        {
            if (cell.GetType() == 0)
            {
                bool shouldTranslate = translateAllCells || targetCells.count(colNum) > 0;
                if (shouldTranslate)
                {
                    std::u8string text = xybase::string::escape(cell.Get<std::u8string>());
                    std::u8string translated = textIdx < jpTexts.size()
                        ? db.GetTranslationFromReference(text, jpTexts[textIdx])
                        : db.GetTranslation(text);
                    cell.Set(xybase::string::unescape(translated));
                    ++textIdx;
                }
            }
            ++colNum;
        }
    }

    dmsg.path = outputPath;
    dmsg.Write();
    return true;
}

bool EjrefToleranceProcessor::TryProcessSameRowCell0(const FileProcessDef& fileDef,
    const std::filesystem::path& inputPath,
    const std::filesystem::path& outputPath,
    const std::map<std::u8string, FileProcessDef>& jpDefsByComment)
{
    auto jpItr = jpDefsByComment.find(fileDef.comment);
    if (jpItr == jpDefsByComment.end() || jpItr->second.type != fileDef.type)
        return false;

    auto jpDatPath = Config::Instance().GetGameRoot() / (jpItr->second.path + u8".DAT");
    if (!std::filesystem::exists(jpDatPath))
        return false;

    DMsg dmsg(inputPath);
    DMsg jpDmsg(jpDatPath);
    dmsg.Read();
    jpDmsg.Read();

    auto& db = TranslationDatabase::Instance();
    auto jpRowItr = jpDmsg.begin();
    for (auto& row : dmsg)
    {
        if (jpRowItr == jpDmsg.end())
            break;

        const auto& jpCells = jpRowItr->GetCellsConst();
        if (!jpCells.empty() && jpCells[0].GetType() == 0)
        {
            std::u8string jpReference = xybase::string::escape(jpCells[0].Get<std::u8string>());
            auto& cells = row.GetCells();
            for (size_t i = 0; i < cells.size() && i < 2; ++i)
            {
                if (cells[i].GetType() != 0)
                    continue;
                std::u8string text = xybase::string::escape(cells[i].Get<std::u8string>());
                cells[i].Set(xybase::string::unescape(db.GetTranslationFromReference(text, jpReference)));
            }
        }
        ++jpRowItr;
    }

    dmsg.path = outputPath;
    dmsg.Write();
    return true;
}

bool EjrefToleranceProcessor::TryProcessKeyItem(const FileProcessDef& fileDef,
    const std::filesystem::path& inputPath,
    const std::filesystem::path& outputPath,
    const std::map<std::u8string, FileProcessDef>& jpDefsByComment)
{
    auto jpItr = jpDefsByComment.find(fileDef.comment);
    if (jpItr == jpDefsByComment.end() || jpItr->second.type != fileDef.type)
        return false;

    auto jpDatPath = Config::Instance().GetGameRoot() / (jpItr->second.path + u8".DAT");
    if (!std::filesystem::exists(jpDatPath))
        return false;

    DMsg dmsg(inputPath);
    DMsg jpDmsg(jpDatPath);
    dmsg.Read();
    jpDmsg.Read();

    // Build JP reference mapping
    struct KeyItemReference
    {
        std::u8string name;
        std::u8string description;
    };
    std::map<int, KeyItemReference> jpById;

    for (auto& row : jpDmsg)
    {
        const auto& cells = row.GetCellsConst();
        if (cells.size() < 3 || cells[0].GetType() != 1)
            continue;

        int id = cells[0].Get<int>();
        KeyItemReference ref;
        if (cells[1].GetType() == 0)
            ref.name = xybase::string::escape(cells[1].Get<std::u8string>());
        if (cells[2].GetType() == 0)
            ref.description = xybase::string::escape(cells[2].Get<std::u8string>());
        if (id != 0)
            jpById[id] = std::move(ref);
    }

    // Apply translations
    auto& db = TranslationDatabase::Instance();
    for (auto& row : dmsg)
    {
        auto& cells = row.GetCells();
        if (cells.size() < 7 || cells[0].GetType() != 1)
            continue;

        int id = cells[0].Get<int>();
        if (id == 0)
        {
            // ID=0: translate cells 4, 5, 6 directly
            if (cells[4].GetType() == 0)
            {
                std::u8string text = xybase::string::escape(cells[4].Get<std::u8string>());
                cells[4].Set(xybase::string::unescape(db.GetTranslation(text)));
            }
            if (cells[5].GetType() == 0)
            {
                std::u8string text = xybase::string::escape(cells[5].Get<std::u8string>());
                cells[5].Set(xybase::string::unescape(db.GetTranslation(text)));
            }
            if (cells[6].GetType() == 0)
            {
                std::u8string text = xybase::string::escape(cells[6].Get<std::u8string>());
                cells[6].Set(xybase::string::unescape(db.GetTranslation(text)));
            }
            continue;
        }

        auto refItr = jpById.find(id);
        if (refItr == jpById.end())
            continue;
        const KeyItemReference* ref = &refItr->second;

        std::u8string originalName;
        try
        {
			if (cells[4].GetType() == 0)
                originalName = cells[4].Get<std::u8string>();
        }
        catch (const std::exception&)
        {
            // Not a string, skip bilingual name prefix
		}

        // Use JP name as reference for cells 4 and 5
        if (!ref->name.empty() && !Config::Instance().IsNoName())
        {
            if (cells[4].GetType() == 0)
            {
                std::u8string text = xybase::string::escape(cells[4].Get<std::u8string>());
                cells[4].Set(xybase::string::unescape(db.GetTranslationFromReference(text, ref->name)));
            }
            if (cells[5].GetType() == 0)
            {
                std::u8string text = xybase::string::escape(cells[5].Get<std::u8string>());
                cells[5].Set(xybase::string::unescape(db.GetTranslationFromReference(text, ref->name)));
            }
        }

        // Use JP description as reference for cell 6
        if (!ref->description.empty() && cells[6].GetType() == 0)
        {
            std::u8string text = xybase::string::escape(cells[6].Get<std::u8string>());
			std::u8string translated = xybase::string::unescape(db.GetTranslationFromReference(text, ref->description));
            if (Config::Instance().IsBilingual() && !originalName.empty())
				translated = u8"(" + originalName + u8")\n" + translated;
            cells[6].Set(translated);
        }
    }

    dmsg.path = outputPath;
    dmsg.Write();
    return true;
}
