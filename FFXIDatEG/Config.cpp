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
	// Update stored file path if provided
	if (!filePath.empty())
	{
		m_filePath = filePath;
	}
	
	// If no path specified and no stored path, fail
	if (m_filePath.empty())
		return false;
	
	IniParser parser;
	// Copy data to parser
	for (const auto& section : m_data)
	{
		for (const auto& kv : section.second)
		{
			parser.SetString(section.first, kv.first, kv.second);
		}
	}
	
	return parser.Save(m_filePath);
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
	
	// Auto-save configuration if path is available
	// Note: Don't auto-save here to avoid multiple saves when setting multiple values
	// Auto-save should be done by specific setter methods or explicitly called
}

void Config::SetInt(const std::wstring& section, const std::wstring& key, int value)
{
	m_data[section][key] = std::to_wstring(value);
	
	// Auto-save configuration using stored path
	if (!m_filePath.empty())
	{
		Save(L"");  // Empty string means use stored path
	}
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
	
	// Auto-save configuration using stored path
	if (!m_filePath.empty())
	{
		Save(L"");  // Empty string means use stored path
	}
}

std::wstring Config::GetFontName() const
{
	auto secIt = m_data.find(L"General");
	if (secIt == m_data.end())
		return L"Yu Gothic UI";  // Default font for Japanese/Chinese text

	auto keyIt = secIt->second.find(L"FontName");
	if (keyIt == secIt->second.end())
		return L"Yu Gothic UI";  // Default font

	return keyIt->second;
}

void Config::SetFontName(const std::wstring& fontName)
{
	m_data[L"General"][L"FontName"] = fontName;
	
	// Auto-save configuration using stored path
	if (!m_filePath.empty())
	{
		Save(L"");  // Empty string means use stored path
	}
}

bool Config::GetEnableCategoryHierarchy() const
{
	auto secIt = m_data.find(L"General");
	if (secIt == m_data.end())
		return false;  // Default to disabled

	auto keyIt = secIt->second.find(L"EnableCategoryHierarchy");
	if (keyIt == secIt->second.end())
		return false;  // Default to disabled

	// Convert to boolean
	std::wstring value = keyIt->second;
	return value == L"1" || value == L"true";
}

void Config::SetEnableCategoryHierarchy(bool enable)
{
	m_data[L"General"][L"EnableCategoryHierarchy"] = enable ? L"1" : L"0";
	
	// Auto-save configuration using stored path
	if (!m_filePath.empty())
	{
		Save(L"");  // Empty string means use stored path
	}
}
