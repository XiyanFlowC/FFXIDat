#include "Config.h"
#include "IniParser.h"
#include <sstream>
#include <algorithm>

Config& Config::Instance()
{
	static Config instance;
	return instance;
}

bool Config::Load(const std::wstring& filePath)
{
	m_filePath = filePath;
	
	IniParser parser;
	if (!parser.Load(filePath))
		return false;
	
	m_data = parser.GetData();
	return true;
}

bool Config::Save(const std::wstring& filePath)
{
	IniParser parser;
	// Copy data to parser
	for (const auto& section : m_data)
	{
		for (const auto& kv : section.second)
		{
			parser.SetString(section.first, kv.first, kv.second);
		}
	}
	
	return parser.Save(filePath);
}

std::wstring Config::GetString(const std::wstring& section, const std::wstring& key, const std::wstring& defaultValue)
{
	auto secIt = m_data.find(section);
	if (secIt == m_data.end())
		return defaultValue;

	auto keyIt = secIt->second.find(key);
	if (keyIt == secIt->second.end())
		return defaultValue;

	return keyIt->second;
}

int Config::GetInt(const std::wstring& section, const std::wstring& key, int defaultValue)
{
	std::wstring strValue = GetString(section, key);
	if (strValue.empty())
		return defaultValue;

	try
	{
		return std::stoi(strValue);
	}
	catch (...)
	{
		return defaultValue;
	}
}

void Config::SetString(const std::wstring& section, const std::wstring& key, const std::wstring& value)
{
	m_data[section][key] = value;
}

void Config::SetInt(const std::wstring& section, const std::wstring& key, int value)
{
	m_data[section][key] = std::to_wstring(value);
}

std::wstring Config::GetUILanguage() const
{
	auto secIt = m_data.find(L"General");
	if (secIt == m_data.end())
		return L"en";  // Default to English

	auto keyIt = secIt->second.find(L"UILanguage");
	if (keyIt == secIt->second.end())
		return L"en";  // Default to English

	return keyIt->second;
}

void Config::SetUILanguage(const std::wstring& language)
{
	m_data[L"General"][L"UILanguage"] = language;
	
	// Auto-save configuration
	if (!m_filePath.empty())
	{
		Save(m_filePath);
	}
}

std::wstring Config::GetFontName() const
{
	auto secIt = m_data.find(L"General");
	if (secIt == m_data.end())
		return L"MS Gothic";  // Default font for Japanese/Chinese text

	auto keyIt = secIt->second.find(L"FontName");
	if (keyIt == secIt->second.end())
		return L"MS Gothic";  // Default font

	return keyIt->second;
}

void Config::SetFontName(const std::wstring& fontName)
{
	m_data[L"General"][L"FontName"] = fontName;
	
	// Auto-save configuration
	if (!m_filePath.empty())
	{
		Save(m_filePath);
	}
}
