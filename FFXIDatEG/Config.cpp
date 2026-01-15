#include "Config.h"
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
	m_data.clear();

	std::wifstream file(filePath);
	if (!file.is_open())
		return false;

	std::wstring currentSection;
	std::wstring line;

	while (std::getline(file, line))
	{
		// Trim whitespace
		line.erase(0, line.find_first_not_of(L" \t\r\n"));
		line.erase(line.find_last_not_of(L" \t\r\n") + 1);

		// Skip empty lines and comments
		if (line.empty() || line[0] == L';' || line[0] == L'#')
			continue;

		// Check for section header
		if (line[0] == L'[' && line[line.length() - 1] == L']')
		{
			currentSection = line.substr(1, line.length() - 2);
			continue;
		}

		// Parse key=value
		size_t pos = line.find(L'=');
		if (pos != std::wstring::npos)
		{
			std::wstring key = line.substr(0, pos);
			std::wstring value = line.substr(pos + 1);

			// Trim key and value
			key.erase(key.find_last_not_of(L" \t") + 1);
			value.erase(0, value.find_first_not_of(L" \t"));

			m_data[currentSection][key] = value;
		}
	}

	return true;
}

bool Config::Save(const std::wstring& filePath)
{
	std::wofstream file(filePath);
	if (!file.is_open())
		return false;

	for (const auto& section : m_data)
	{
		if (!section.first.empty())
		{
			file << L"[" << section.first << L"]\n";
		}

		for (const auto& kv : section.second)
		{
			file << kv.first << L"=" << kv.second << L"\n";
		}

		file << L"\n";
	}

	return true;
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
