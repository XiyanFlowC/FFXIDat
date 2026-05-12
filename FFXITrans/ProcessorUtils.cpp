#include "ProcessorUtils.h"
#include "Config.h"
#include <sstream>
#include <set>
#include <regex>
#include <tuple>
#include <XiString.h>
#include <DMsg.h>
#include <EventStringBase.h>
#include <StatusData.h>
#include <ItemData.h>
#include <FixedPhrase.h>
#include <MonBridge.h>
#include <RecordsOfEminence.h>
#include <CsvFile.h>
#include <xystring.h>

namespace ProcessorUtils
{
    std::set<int> ParseCellIndices(const std::u8string& cellIndicesStr)
    {
        std::set<int> indices;
        if (cellIndicesStr.empty())
            return indices;

        std::string str = reinterpret_cast<const char*>(cellIndicesStr.c_str());
        std::stringstream ss(str);
        std::string token;

        while (std::getline(ss, token, '|'))
        {
            try
            {
                int index = std::stoi(token);
                if (index > 0)
                    indices.insert(index);
            }
            catch (const std::exception&)
            {
                // Ignore invalid indices
            }
        }

        return indices;
    }

    bool IsQuestDMsg(const std::u8string& comment)
    {
        return comment.starts_with(u8"sys/mis/") || comment.starts_with(u8"sys/qst/");
    }

    bool IsEjrefShorterReferenceComment(const std::u8string& comment)
    {
        return comment == u8"gev/action"
            || comment == u8"sys/text_command_help"
			|| comment == u8"sys/weapon_skill" // for backwards compatibility, can be removed later
			|| comment == u8"sys/weapon_skill_help" // for backwards compatibility, can be removed later
			|| comment == u8"sys/ability"
			|| comment == u8"sys/ability_help";
    }

    bool IsEjrefSameRowCell0Comment(const std::u8string& comment)
    {
        return comment == u8"sys/status" || comment == u8"sys/mob_race";
    }

    bool IsEjrefSpecialComment(const std::u8string& comment)
    {
        return comment == u8"gev/status"
            || comment == u8"sys/key_item"
			|| comment == u8"evx/Aht Urhgan Whitegate"
            || IsEjrefShorterReferenceComment(comment)
            || IsEjrefSameRowCell0Comment(comment);
    }

    std::u8string GetCurrentLanguageCode()
    {
        return Config::Instance().IsEnglishMode() ? u8"en" : u8"jp";
    }

    std::u8string GetAlternateLanguageCode()
    {
        return Config::Instance().IsEnglishMode() ? u8"jp" : u8"en";
    }

    bool TryGetFileDef(const std::u8string& comment, const std::u8string& type, const std::u8string& lang, FileProcessDef& fileDef)
    {
        using FileDefKey = std::tuple<std::u8string, std::u8string, std::u8string>;

        static const auto defsByKey = []()
            {
                std::map<FileDefKey, FileProcessDef> defs;
                const auto defsPath = Config::Instance().GetProgRoot() / "defs.csv";
                if (!std::filesystem::exists(defsPath))
                    return defs;

                CsvFile def(defsPath, std::ios::in | std::ios::binary);
                while (!def.IsEof())
                {
                    FileProcessDef loadedDef;
                    loadedDef.path = def.NextCell();
                    loadedDef.type = def.NextCell();
                    loadedDef.lang = def.NextCell();
                    loadedDef.comment = def.NextCell();
                    if (!def.IsEol())
                    {
                        loadedDef.cellIndicesStr = def.NextCell();
                    }
                    def.NextLine();

                    if (loadedDef.path.empty() || loadedDef.type.empty() || loadedDef.lang.empty() || loadedDef.comment.empty())
                        continue;

                    if (loadedDef.path[0] == u8'#')
                    {
                        loadedDef.path.erase(loadedDef.path.begin());
                        if (loadedDef.path.empty())
                            continue;
                    }

                    defs[FileDefKey{ loadedDef.comment, loadedDef.type, loadedDef.lang }] = std::move(loadedDef);
                }

                return defs;
            }();

        const auto itr = defsByKey.find(FileDefKey{ comment, type, lang });
        if (itr == defsByKey.end())
            return false;

        fileDef = itr->second;
        return true;
    }

    std::u8string PrependBabelText(const std::u8string& translatedText, const std::u8string& currentOriginalText, const std::u8string& alternateOriginalText)
    {
        if (!Config::Instance().IsBabelEnabled())
            return translatedText;

        std::vector<std::u8string> originals;
        if (Config::Instance().IsBabelCurrentOriginalEnabled() && !currentOriginalText.empty())
        {
            originals.push_back(currentOriginalText);
        }
        if (Config::Instance().IsBabelAlternateOriginalEnabled()
            && !alternateOriginalText.empty()
            && std::find(originals.begin(), originals.end(), alternateOriginalText) == originals.end())
        {
            originals.push_back(alternateOriginalText);
        }
        if (originals.empty())
            return translatedText;

        std::u8string prefix = u8"(";
        for (size_t i = 0; i < originals.size(); ++i)
        {
            if (i > 0)
                prefix += u8"|";
            prefix += originals[i];
        }
        prefix += u8")\n";
        return prefix + translatedText;
    }

    std::vector<InsToken> ParseInsTokens(const std::u8string& text)
    {
        std::vector<InsToken> result;
        const std::u8string prefix = u8"<ins:";
        size_t pos = 0;

        while ((pos = text.find(prefix, pos)) != std::u8string::npos)
        {
            size_t end = text.find(u8">", pos);
            if (end == std::u8string::npos)
                break;

            InsToken token;
            token.start = pos;
            token.end = end + 1;
            std::u8string body = text.substr(pos + prefix.size(), end - pos - prefix.size());
            std::string bodyStr = xybase::string::to_string(body);
            std::stringstream ss(bodyStr);
            std::string part;

            while (std::getline(ss, part, ':'))
            {
                token.parts.push_back(xybase::string::to_utf8(part));
            }

            result.push_back(std::move(token));
            pos = end + 1;
        }

        return result;
    }

    std::u8string BuildInsToken(const std::vector<std::u8string>& parts)
    {
        std::u8string token = u8"<ins:";
        for (size_t i = 0; i < parts.size(); ++i)
        {
            if (i > 0) token += u8":";
            token += parts[i];
        }
        token += u8">";
        return token;
    }

    std::u8string BuildInsKey(const std::vector<std::u8string>& parts)
    {
        if (parts.size() < 5)
            return {};

        std::u8string key;
        for (size_t i = parts.size() - 4; i < parts.size(); ++i)
        {
            if (!key.empty()) key += u8":";
            key += parts[i];
        }
        return key;
    }

    bool TryAdaptInsCategoryForEnglish(const std::u8string& englishSource, std::u8string& translated)
    {
        auto sourceTokens = ParseInsTokens(englishSource);
        auto translatedTokens = ParseInsTokens(translated);

        if (translatedTokens.empty())
            return true;

        std::map<std::u8string, std::vector<std::u8string>> sourcePartsByVar;
        std::set<std::u8string> sourceShortTokens;

        for (const auto& token : sourceTokens)
        {
            if (token.parts.size() < 5)
            {
                sourceShortTokens.insert(BuildInsToken(token.parts));
                continue;
            }

            auto key = BuildInsKey(token.parts);
            if (!key.empty())
            {
                sourcePartsByVar[key] = token.parts;
            }
        }

        for (const auto& translatedToken : translatedTokens)
        {
            if (translatedToken.parts.size() < 5)
            {
                auto tokenStr = BuildInsToken(translatedToken.parts);
                if (!sourceShortTokens.contains(tokenStr))
                    return false;
            }
        }

        for (auto it = translatedTokens.rbegin(); it != translatedTokens.rend(); ++it)
        {
            auto& token = *it;
            if (token.parts.size() < 5)
                continue;

            auto key = BuildInsKey(token.parts);
            auto sourceItr = sourcePartsByVar.find(key);
            if (sourceItr == sourcePartsByVar.end())
                return false;

            auto sourceParts = sourceItr->second;
            if (sourceParts.size() < 5)
                return false;

            if (sourceParts.size() >= 2)
            {
                // Convert type 24 (English with articles) to type 26 (no articles) for Japanese-like processing
                if (sourceParts[1] == u8"24" || sourceParts[1] == u8"25") // 13 in jp
                {
                    sourceParts[1] = u8"23";
                }
                if (sourceParts[1] == u8"28" || sourceParts[1] == u8"27") { // 14  in jp
					sourceParts[1] = u8"26";
                }
				// Key Item's type replacement (perment key item?)
                if (sourceParts[1] == u8"36"/* || sourceParts[1] == u8"45"*/)
                {
                    sourceParts[1] = u8"33";
                }
                // Key Item's type replacement (temperary key item?)
                if (sourceParts[1] == u8"45")
                {
                    sourceParts[1] = u8"42";
				}
            }

            auto newToken = BuildInsToken(sourceParts);
            translated.replace(token.start, token.end - token.start, newToken);
        }

        return true;
    }

    ItemSpecType GetItemSpecType(const std::u8string& type)
    {
        if (type == u8"iab") return ItemSpecType::ARMOUR;
        if (type == u8"iwb") return ItemSpecType::WEAPON;
        if (type == u8"iub") return ItemSpecType::USABLE;
        if (type == u8"ipb") return ItemSpecType::PUPPET;
        if (type == u8"isb") return ItemSpecType::SLIP;
        if (type == u8"icb") return ItemSpecType::CURRENCY;
        if (type == u8"iib") return ItemSpecType::INSTINCT;
        return ItemSpecType::NORMAL;
    }

    std::map<uint32_t, std::vector<std::u8string>> CollectItemTextsById(const std::filesystem::path& datPath, const std::u8string& type)
    {
        std::map<uint32_t, std::vector<std::u8string>> result;
        ItemData itemData;
        itemData.Read(datPath, GetItemSpecType(type));

        for (const auto& datum : itemData.data)
        {
            auto& texts = result[datum.id];
            try
            {
                auto name = datum.name();
                if (!name.empty()) texts.push_back(xybase::string::escape(name));
            }
            catch (...) {}

            try
            {
                auto desc = datum.description();
                if (!desc.empty()) texts.push_back(xybase::string::escape(desc));
            }
            catch (...) {}
        }

        return result;
    }

    std::map<uint32_t, std::u8string> CollectMonBridgeTextsById(const std::filesystem::path& datPath)
    {
        std::map<uint32_t, std::u8string> result;
        MonBridge monBridge;
        monBridge.Read(datPath);

        for (const auto& datum : monBridge.data)
        {
            if (!datum.displayName.empty())
            {
                result[datum.id] = xybase::string::escape(datum.displayName);
            }
        }

        return result;
    }

    std::map<int, std::vector<std::u8string>> CollectDMsgTextsById(const std::filesystem::path& datPath, const std::u8string& cellIndicesStr)
    {
        std::map<int, std::vector<std::u8string>> result;
        DMsg dmsg(datPath);
        dmsg.Read();
        std::set<int> targetCells = ParseCellIndices(cellIndicesStr);
        bool translateAllCells = targetCells.empty();

        for (auto& row : dmsg)
        {
            const auto& cells = row.GetCellsConst();
            if (cells.empty() || cells[0].GetType() != 1)
                continue;

            int rowId = cells[0].Get<int>();
            auto& texts = result[rowId];
            int colNum = 1;

            for (const auto& cell : cells)
            {
                if (cell.GetType() == 0)
                {
                    bool shouldTranslate = translateAllCells || targetCells.count(colNum) > 0;
                    if (shouldTranslate)
                    {
                        texts.push_back(xybase::string::escape(cell.Get<std::u8string>()));
                    }
                }
                ++colNum;
            }
        }

        return result;
    }

    std::vector<std::u8string> CollectStrings(const std::filesystem::path& datPath, const std::u8string& type, const std::u8string& cellIndicesStr)
    {
        std::vector<std::u8string> result;

        if (type == u8"xis")
        {
            XiString xis(datPath);
            xis.Read();
            for (auto& str : xis)
            {
                result.push_back(xybase::string::escape(xybase::string::to_utf8(xis.Decode(str.str))));
            }
        }
        else if (type == u8"evsb")
        {
            EventStringBase evsb(datPath);
            evsb.Read();
            for (auto& s : evsb)
            {
                result.push_back(s);
            }
        }
        else if (type == u8"dmsg")
        {
            DMsg dmsg(datPath);
            dmsg.Read();
            std::set<int> targetCells = ParseCellIndices(cellIndicesStr);
            bool translateAllCells = targetCells.empty();

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
                            result.push_back(xybase::string::escape(cell.Get<std::u8string>()));
                        }
                    }
                    ++colNum;
                }
            }
        }
        else if (type == u8"sd")
        {
            StatusData statusData;
            statusData.Read(datPath);
            for (auto& datum : statusData.data)
            {
                if (!datum.description.empty())
                {
                    result.push_back(xybase::string::escape(datum.description));
                }
            }
        }
        else if (type == u8"fp")
        {
            FixedPhrase fixedPhrase;
            fixedPhrase.Read(datPath);
            for (auto& category : fixedPhrase.categories)
            {
                result.push_back(category.categoryName);
                result.push_back(category.categoryPron);
                for (auto&& entry : category.entries)
                {
                    if (!entry.text.empty())
                        result.push_back(entry.text);
                    if (!entry.pron.empty())
                        result.push_back(entry.pron);
                }
            }
        }
        else if (type == u8"iab" || type == u8"iwb" || type == u8"iub" || type == u8"inb" || type == u8"ipb" || type == u8"isb" || type == u8"icb" || type == u8"iib")
        {
            std::set<int> targetCells = ParseCellIndices(cellIndicesStr);
            bool translateAllCells = targetCells.empty();

            ItemData itemData;
            itemData.Read(datPath, GetItemSpecType(type));

            for (auto& datum : itemData.data)
            {
                int cellIndex = 1;
                for (auto& cell : datum.row())
                {
                    if (cell.GetType() == 0)
                    {
                        bool shouldTranslate = translateAllCells || targetCells.count(cellIndex) > 0;
                        if (shouldTranslate)
                        {
                            result.push_back(xybase::string::escape(cell.Get<std::u8string>()));
                        }
                    }
                    ++cellIndex;
                }
            }
        }
        else if (type == u8"mbd")
        {
            MonBridge monBridge;
            monBridge.Read(datPath);
            for (auto& datum : monBridge.data)
            {
                if (!datum.displayName.empty())
                {
                    result.push_back(xybase::string::escape(datum.displayName));
                }
            }
        }
        else if (type == u8"erq")
        {
            RecordsOfEminence roe;
            roe.ReadQuest(datPath);
            std::set<int> targetCells = ParseCellIndices(cellIndicesStr);

            for (auto& datum : roe.questData)
            {
                int cellIndex = 1;
                for (auto& cell : datum.row())
                {
                    if (cell.GetType() == 0)
                    {
                        bool shouldTranslate = targetCells.empty() || targetCells.count(cellIndex) > 0;
                        if (shouldTranslate)
                        {
                            result.push_back(xybase::string::escape(cell.Get<std::u8string>()));
                        }
                    }
                    ++cellIndex;
                }
            }
        }
        else if (type == u8"erc")
        {
            RecordsOfEminence roe;
            roe.ReadCategory(datPath);
            for (auto& datum : roe.categoryData)
            {
                try
                {
                    std::u8string catName = datum.categoryName();
                    if (!catName.empty())
                    {
                        result.push_back(xybase::string::escape(catName));
                    }
                }
                catch (...) {}
            }
        }

        return result;
    }
}
