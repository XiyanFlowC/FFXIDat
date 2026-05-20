#include "Config.h"
#include "Config.h"
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <regex>
#include <algorithm>
#include <cwctype>
#include <xystring.h>

namespace
{
    bool IsTruthyValue(const std::wstring& value)
    {
        return value == L"1" || value == L"true" || value == L"yes" || value == L"on";
    }

    bool IsFalsyValue(const std::wstring& value)
    {
        return value == L"0" || value == L"false" || value == L"no" || value == L"off" || value == L"native";
    }

    std::wstring ToLowerAscii(std::wstring value)
    {
        std::transform(value.begin(), value.end(), value.begin(), [](wchar_t ch)
            {
                return static_cast<wchar_t>(::towlower(ch));
            });
        return value;
    }

    bool TryParseBabelMode(const std::wstring& rawValue, bool& currentOriginalEnabled, bool& alternateOriginalEnabled)
    {
        const std::wstring value = ToLowerAscii(rawValue);

        if (IsFalsyValue(value))
        {
            currentOriginalEnabled = false;
            alternateOriginalEnabled = false;
            return true;
        }

        if (value == L"bilingual" || value == L"1" || value == L"true" || value == L"on")
        {
            currentOriginalEnabled = true;
            alternateOriginalEnabled = false;
            return true;
        }

        if (value == L"exotic")
        {
            currentOriginalEnabled = false;
            alternateOriginalEnabled = true;
            return true;
        }

        if (value == L"yes" || value == L"tower" || value == L"trilingual")
        {
            currentOriginalEnabled = true;
            alternateOriginalEnabled = true;
            return true;
        }

        return false;
    }
}

Config& Config::Instance()
{
    static Config instance;
    return instance;
}

bool Config::Initialize()
{
    // Get program root
    wchar_t module_path[MAX_PATH];
    if (GetModuleFileNameW(NULL, module_path, MAX_PATH))
    {
        std::filesystem::path exePath = module_path;
        progRoot = exePath.parent_path();
    }

    // Check if config.ini has both game_path and english_mode
    fs::path configPath = progRoot / "config.ini";
    bool hasGamePath = false;
    bool hasEnglishMode = false;
    bool skipRegistry = CheckConfigFileHasRequiredSettings(configPath, hasGamePath, hasEnglishMode);

    if (skipRegistry)
    {
        std::wcout << L"检测到 config.ini 中已配置游戏路径和语言模式，跳过注册表读取。" << std::endl;
    }
    else
    {
        // Try to initialize from registry
        if (InitializeFromRegistry() != 0)
        {
            return false;
        }

        std::wcout << L"路径初始化完毕。\n游戏路径：" << gameRoot << L"\n程序数据路径：" << progRoot << std::endl;
        if (englishMode)
            std::wcout << L"注意：检测到 PlayOnlineEU 注册表项，程序将仅处理英文文件。" << std::endl;
    }

    return true;
}

int Config::InitializeFromRegistry()
{
    HKEY hKey;
    const wchar_t* subKey1 = L"SOFTWARE\\WOW6432Node\\PlayOnline\\InstallFolder";
    const wchar_t* subKey2 = L"SOFTWARE\\WOW6432Node\\PlayOnlineEU\\InstallFolder";
    const wchar_t* subKey3 = L"SOFTWARE\\WOW6432Node\\PlayOnlineUS\\InstallFolder";
    const wchar_t* valueName = L"0001";
    wchar_t valueData[MAX_PATH];
    DWORD bufferSize = sizeof(valueData);
    DWORD valueType;

    // Try primary key first (priority for JP)
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, subKey1, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        if (RegQueryValueExW(hKey, valueName, nullptr, &valueType, reinterpret_cast<LPBYTE>(valueData), &bufferSize) == ERROR_SUCCESS)
        {
            if (valueType == REG_SZ)
            {
                gameRoot = valueData;
                englishMode = false;
            }
            else
            {
                std::wcerr << L"未能正确读取注册表信息，请检查游戏安装信息。\n";
                RegCloseKey(hKey);
                return -1;
            }
        }
        else
        {
            std::wcerr << L"发现了POL的安装信息，但是没有找到游戏的。\n";
            RegCloseKey(hKey);
            return -1;
        }
        RegCloseKey(hKey);
    }
    else if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, subKey2, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        if (RegQueryValueExW(hKey, valueName, nullptr, &valueType, reinterpret_cast<LPBYTE>(valueData), &bufferSize) == ERROR_SUCCESS)
        {
            if (valueType == REG_SZ)
            {
                gameRoot = valueData;
                englishMode = true;
            }
            else
            {
                std::wcerr << L"未能正确读取注册表信息，请检查游戏安装信息（EU）。\n";
                RegCloseKey(hKey);
                return -1;
            }
        }
        else
        {
            std::wcerr << L"发现了POL(EU)的安装信息，但是没有找到游戏的。\n";
            RegCloseKey(hKey);
            return -1;
        }
        RegCloseKey(hKey);
    }
    else if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, subKey3, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        if (RegQueryValueExW(hKey, valueName, nullptr, &valueType, reinterpret_cast<LPBYTE>(valueData), &bufferSize) == ERROR_SUCCESS)
        {
            if (valueType == REG_SZ)
            {
                gameRoot = valueData;
                englishMode = true;
            }
            else
            {
                std::wcerr << L"未能正确读取注册表信息，请检查游戏安装信息（US）。\n";
                RegCloseKey(hKey);
                return -1;
            }
        }
        else
        {
            std::wcerr << L"发现了POL(US)的安装信息，但是没有找到游戏的。\n";
            RegCloseKey(hKey);
            return -1;
        }
        RegCloseKey(hKey);
    }
    else
    {
        std::wcerr << L"您的计算机中并未安装FFXI或POL，程序无法继续。\n";
        return -1;
    }

    return 0;
}

bool Config::CheckConfigFileHasRequiredSettings(const fs::path& configPath, bool& hasGamePath, bool& hasEnglishMode)
{
    hasGamePath = false;
    hasEnglishMode = false;

    if (!fs::exists(configPath))
        return false;

    std::wifstream configFile(configPath);
    std::wstring line;

    while (std::getline(configFile, line))
    {
        // Trim and skip comments/empty lines
        line = std::regex_replace(line, std::wregex(L"^\\s+|\\s+$"), L"");
        if (line.empty() || line[0] == L';' || line[0] == L'#')
            continue;

        auto delimiterPos = line.find(L'=');
        if (delimiterPos == std::wstring::npos)
            continue;

        std::wstring key = line.substr(0, delimiterPos);
        std::wstring value = line.substr(delimiterPos + 1);
        key = std::regex_replace(key, std::wregex(L"^\\s+|\\s+$"), L"");
        value = std::regex_replace(value, std::wregex(L"^\\s+|\\s+$"), L"");

        if (key == L"game_path" && !value.empty())
        {
            hasGamePath = true;
        }
        else if (key == L"english_mode")
        {
            hasEnglishMode = true;
        }

        // Early exit if both found
        if (hasGamePath && hasEnglishMode)
            break;
    }

    configFile.close();
    return hasGamePath && hasEnglishMode;
}

bool Config::LoadFromFile(const fs::path& configPath)
{
    if (!fs::exists(configPath))
        return false;

    std::wcout << L"读取配置文件中..." << std::endl;
    std::wifstream configFile(configPath);
    std::wstring line;
    bool babelConfigured = false;
    bool legacyBilingualConfigured = false;

    while (std::getline(configFile, line))
    {
        // Trim and skip comments/empty lines
        line = std::regex_replace(line, std::wregex(L"^\\s+|\\s+$"), L"");
        if (line.empty() || line[0] == L';' || line[0] == L'#')
            continue;

        auto delimiterPos = line.find(L'=');
        if (delimiterPos == std::wstring::npos)
            continue;

        std::wstring key = line.substr(0, delimiterPos);
        std::wstring value = line.substr(delimiterPos + 1);
        key = std::regex_replace(key, std::wregex(L"^\\s+|\\s+$"), L"");
        value = std::regex_replace(value, std::wregex(L"^\\s+|\\s+$"), L"");

        if (key == L"game_path")
        {
            gameRoot = value;
            std::wcout << L"使用配置文件中的游戏路径：" << gameRoot << std::endl;
        }
        else if (key == L"in_situ")
        {
            inSitu = (value == L"1" || value == L"true" || value == L"yes");
            inSituNoprompt = true;
        }
        else if (key == L"english_mode")
        {
            englishMode = (value == L"1" || value == L"true" || value == L"yes");
            std::wcout << L"使用配置文件中的语言模式：" << (englishMode ? L"英文" : L"日文") << std::endl;
        }
        else if (key == L"output_path")
        {
            if (!inSitu)
            {
                outRoot = value;
                std::wcout << L"使用配置文件中的输出路径：" << outRoot << std::endl;
            }
        }
        else if (key == L"no_mismatch_log")
        {
            noMismatchLog = (value == L"1" || value == L"true" || value == L"yes");
        }
        else if (key == L"en_as_ja")
        {
            enAsJa = (value == L"1" || value == L"true" || value == L"yes");
        }
        else if (key == L"ejref_tolerance")
        {
            ejrefTolerance = (value == L"1" || value == L"true" || value == L"yes");
        }
        else if (key == L"verbose")
        {
            verbose = (value == L"1" || value == L"true" || value == L"yes");
        }
        else if (key == L"noname")
        {
            noname = IsTruthyValue(value);
        }
        else if (key == L"babel")
        {
            bool currentOriginalEnabled = babelCurrentOriginal;
            bool alternateOriginalEnabled = babelAlternateOriginal;
            if (TryParseBabelMode(value, currentOriginalEnabled, alternateOriginalEnabled))
            {
                babelCurrentOriginal = currentOriginalEnabled;
                babelAlternateOriginal = alternateOriginalEnabled;
                babelConfigured = true;
            }
            else
            {
                std::wcerr << L"未识别的 babel 配置值：" << value << L"，将保持当前设置。" << std::endl;
            }
        }
        else if (key == L"bilingual")
        {
            legacyBilingualConfigured = true;
            if (!babelConfigured)
            {
                babelCurrentOriginal = IsTruthyValue(value);
                babelAlternateOriginal = false;
            }
        }
        else if (key == L"excludes")
        {
            // Parse comma-separated excludes list
            excludes.clear();
            std::wstring valueStr = value;
            size_t pos = 0;
            while ((pos = valueStr.find(L',')) != std::wstring::npos)
            {
                std::wstring token = valueStr.substr(0, pos);
                token = std::regex_replace(token, std::wregex(L"^\\s+|\\s+$"), L"");
                if (!token.empty())
                {
                    excludes.insert(xybase::string::to_utf8(token));
                }
                valueStr.erase(0, pos + 1);
            }
            // Add the last token
            valueStr = std::regex_replace(valueStr, std::wregex(L"^\\s+|\\s+$"), L"");
            if (!valueStr.empty())
            {
                excludes.insert(xybase::string::to_utf8(valueStr));
            }

            if (!excludes.empty() && verbose)
            {
                std::wcout << L"已配置排除项（共 " << excludes.size() << L" 项）：";
                for (const auto& ex : excludes)
                {
                    std::wcout << L" " << xybase::string::to_wstring(ex);
                }
                std::wcout << std::endl;
            }
        }
        else if (key == L"sys_job_workaround")
        {
            if (value == L"off" || value == L"0" || value == L"false" || value == L"no")
            {
                samuraiJobTransNot = false;
                monkJobAbbreviated = false;
                samuraiJobSpecial = false;
			}
			if (value == L"samurai_trans_not")
				samuraiJobTransNot = true, samuraiJobSpecial = false;
            if (value == L"monk_abbreviated")
				monkJobAbbreviated = true;
			if (value == L"samurai_special")
				samuraiJobSpecial = true, samuraiJobTransNot = false;
        }
    }

    configFile.close();
    if (legacyBilingualConfigured)
    {
        std::wcout << L"注意：bilingual 配置项已废弃，请改用 babel。" << std::endl;
    }
    return true;
}

bool Config::IsExcluded(const std::u8string& comment) const
{
    // First check for exact match (faster)
    if (excludes.find(comment) != excludes.end())
        return true;

    // Check for wildcard matches
    for (const auto& pattern : excludes)
    {
        if (MatchWildcard(comment, pattern))
            return true;
    }

    return false;
}

bool Config::MatchWildcard(const std::u8string& text, const std::u8string& pattern) const
{
    // If pattern has no wildcards, it's just a regular string comparison
    if (pattern.find(u8'*') == std::u8string::npos && pattern.find(u8'?') == std::u8string::npos)
        return text == pattern;

    size_t textPos = 0;
    size_t patternPos = 0;
    size_t textLen = text.length();
    size_t patternLen = pattern.length();
    size_t starPos = std::u8string::npos;
    size_t textBacktrack = 0;

    while (textPos < textLen)
    {
        if (patternPos < patternLen && (pattern[patternPos] == u8'?' || pattern[patternPos] == text[textPos]))
        {
            // Match single character or '?'
            ++textPos;
            ++patternPos;
        }
        else if (patternPos < patternLen && pattern[patternPos] == u8'*')
        {
            // Remember position of '*' for backtracking
            starPos = patternPos;
            textBacktrack = textPos;
            ++patternPos;
        }
        else if (starPos != std::u8string::npos)
        {
            // Backtrack to last '*' and try matching from next position
            patternPos = starPos + 1;
            ++textBacktrack;
            textPos = textBacktrack;
        }
        else
        {
            // No match
            return false;
        }
    }

    // Skip trailing '*' in pattern
    while (patternPos < patternLen && pattern[patternPos] == u8'*')
        ++patternPos;

    // Match successful if we've consumed the entire pattern
    return patternPos == patternLen;
}
