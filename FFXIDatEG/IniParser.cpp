#include "IniParser.h"
#include <algorithm>

bool IniParser::Load(const std::wstring& filePath)
{
	m_data.clear();

	std::wifstream file(filePath);
	if (!file.is_open())
		return false;

	std::wstring currentSection;
	std::wstring line;

	while (std::getline(file, line))
	{
		// Trim trailing whitespace
		line = TrimTrailing(line);

		// Trim leading whitespace
		line = TrimLeading(line);

		// Skip empty lines and comments
		if (line.empty() || line[0] == L';' || line[0] == L'#')
			continue;

		// Check for section header
		if (line[0] == L'[' && line.length() > 1)
		{
			size_t endBracket = line.find(L']');
			if (endBracket != std::wstring::npos)
			{
				currentSection = line.substr(1, endBracket - 1);
				currentSection = Trim(currentSection);
				continue;
			}
		}

		// Parse key=value
		size_t pos = line.find(L'=');
		if (pos != std::wstring::npos)
		{
			std::wstring key = line.substr(0, pos);
			std::wstring value = line.substr(pos + 1);

			// Trim key and value
			key = Trim(key, L" \t");
			value = TrimLeading(value);

			m_data[currentSection][key] = value;
		}
	}

	return true;
}

bool IniParser::Save(const std::wstring& filePath) const
{
	std::wofstream file(filePath);
	if (!file.is_open())
		return false;

	for (const auto& section : m_data)
	{
		// Write section header (skip empty section name for global keys)
		if (!section.first.empty())
		{
			file << L"[" << section.first << L"]\n";
		}

		// Write key-value pairs
		for (const auto& kv : section.second)
		{
			file << kv.first << L"=" << kv.second << L"\n";
		}

		file << L"\n";
	}

	return true;
}

std::wstring IniParser::GetString(const std::wstring& section, const std::wstring& key, const std::wstring& defaultValue) const
{
	auto secIt = m_data.find(section);
	if (secIt == m_data.end())
		return defaultValue;

	auto keyIt = secIt->second.find(key);
	if (keyIt == secIt->second.end())
		return defaultValue;

	return keyIt->second;
}

void IniParser::SetString(const std::wstring& section, const std::wstring& key, const std::wstring& value)
{
	m_data[section][key] = value;
}

bool IniParser::HasSection(const std::wstring& section) const
{
	return m_data.find(section) != m_data.end();
}

bool IniParser::HasKey(const std::wstring& section, const std::wstring& key) const
{
	auto secIt = m_data.find(section);
	if (secIt == m_data.end())
		return false;

	return secIt->second.find(key) != secIt->second.end();
}

std::wstring IniParser::Trim(const std::wstring& str, const std::wstring& whitespace)
{
	if (str.empty())
		return str;

	size_t start = str.find_first_not_of(whitespace);
	if (start == std::wstring::npos)
		return L"";

	size_t end = str.find_last_not_of(whitespace);
	return str.substr(start, end - start + 1);
}

std::wstring IniParser::TrimTrailing(const std::wstring& str, const std::wstring& whitespace)
{
	if (str.empty())
		return str;

	size_t end = str.find_last_not_of(whitespace);
	if (end == std::wstring::npos)
		return L"";

	return str.substr(0, end + 1);
}

std::wstring IniParser::TrimLeading(const std::wstring& str, const std::wstring& whitespace)
{
	if (str.empty())
		return str;

	size_t start = str.find_first_not_of(whitespace);
	if (start == std::wstring::npos)
		return L"";

	return str.substr(start);
}
