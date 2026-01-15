#include "Localization.h"
#include "IniParser.h"
#include <fstream>
#include <algorithm>
#include "../xybase/xystring.h"

Localization& Localization::Instance()
{
	static Localization instance;
	return instance;
}

bool Localization::Load(const std::wstring& filePath)
{
	m_strings.clear();

	// Read UTF-8 encoded file
	std::ifstream file(filePath, std::ios::binary);
	if (!file.is_open())
		return false;

	// Use IniParser to parse the file
	IniParser parser;
	
	std::string line;
	std::wstring continuedLine;
	bool isContinuation = false;
	
	while (std::getline(file, line))
	{
		// Convert UTF-8 string to wstring
		std::u8string u8line(reinterpret_cast<const char8_t*>(line.c_str()), line.size());
		std::wstring wline = xybase::string::to_wstring(u8line);

		// Trim trailing whitespace
		wline.erase(wline.find_last_not_of(L" \t\r\n") + 1);

		// Check for line continuation (backslash at end)
		bool hasBackslash = false;
		if (!wline.empty() && wline.back() == L'\\')
		{
			hasBackslash = true;
			wline.pop_back();  // Remove the backslash
			// Trim trailing whitespace after removing backslash
			wline.erase(wline.find_last_not_of(L" \t\r\n") + 1);
		}

		// Append to continued line
		if (isContinuation)
		{
			continuedLine += wline;
		}
		else
		{
			continuedLine = wline;
		}

		// If line ends with backslash, continue to next line
		if (hasBackslash)
		{
			isContinuation = true;
			continue;
		}

		// Process the complete line
		isContinuation = false;
		wline = continuedLine;

		// Trim leading whitespace
		wline.erase(0, wline.find_first_not_of(L" \t"));

		// Skip empty lines, comments, and section headers
		if (wline.empty() || wline[0] == L';' || wline[0] == L'#' || wline[0] == L'[')
			continue;

		// Parse key=value
		size_t pos = wline.find(L'=');
		if (pos != std::wstring::npos)
		{
			std::wstring key = wline.substr(0, pos);
			std::wstring value = wline.substr(pos + 1);

			// Trim key and value
			key.erase(key.find_last_not_of(L" \t") + 1);
			value.erase(0, value.find_first_not_of(L" \t"));

			m_strings[key] = xybase::string::unescape(value);
		}
	}

	return true;
}

std::vector<LanguageInfo> Localization::ScanAvailableLanguages(const std::filesystem::path& localDir)
{
	std::vector<LanguageInfo> languages;
	m_availableLanguages.clear();

	// Check if local directory exists
	if (!std::filesystem::exists(localDir) || !std::filesystem::is_directory(localDir))
	{
		return languages;
	}

	// Scan for .ini files in local directory
	for (const auto& entry : std::filesystem::directory_iterator(localDir))
	{
		if (entry.is_regular_file() && entry.path().extension() == L".ini")
		{
			// Try to load language metadata
			std::ifstream file(entry.path(), std::ios::binary);
			if (!file.is_open())
				continue;

			std::wstring languageCode;
			std::wstring languageName;

			std::string line;
			std::wstring continuedLine;
			bool isContinuation = false;
			
			while (std::getline(file, line))
			{
				// Convert UTF-8 string to wstring
				std::u8string u8line(reinterpret_cast<const char8_t*>(line.c_str()), line.size());
				std::wstring wline = xybase::string::to_wstring(u8line);

				// Trim trailing whitespace
				wline.erase(wline.find_last_not_of(L" \t\r\n") + 1);

				// Check for line continuation (backslash at end)
				bool hasBackslash = false;
				if (!wline.empty() && wline.back() == L'\\')
				{
					hasBackslash = true;
					wline.pop_back();  // Remove the backslash
					// Trim trailing whitespace after removing backslash
					wline.erase(wline.find_last_not_of(L" \t\r\n") + 1);
				}

				// Append to continued line
				if (isContinuation)
				{
					continuedLine += wline;
				}
				else
				{
					continuedLine = wline;
				}

				// If line ends with backslash, continue to next line
				if (hasBackslash)
				{
					isContinuation = true;
					continue;
				}

				// Process the complete line
				isContinuation = false;
				wline = continuedLine;

				// Trim leading whitespace
				wline.erase(0, wline.find_first_not_of(L" \t"));

				// Skip empty lines and comments
				if (wline.empty() || wline[0] == L';' || wline[0] == L'#')
					continue;

				// Parse key=value
				size_t pos = wline.find(L'=');
				if (pos != std::wstring::npos)
				{
					std::wstring key = wline.substr(0, pos);
					std::wstring value = wline.substr(pos + 1);

					// Trim key and value
					key.erase(key.find_last_not_of(L" \t") + 1);
					value.erase(0, value.find_first_not_of(L" \t"));

					if (key == L"language_code")
					{
						languageCode = value;
					}
					else if (key == L"language_name")
					{
						languageName = value;
					}

					// If we have both, we can stop reading
					if (!languageCode.empty() && !languageName.empty())
						break;
				}
			}

			// Add to list if we found both required fields
			if (!languageCode.empty() && !languageName.empty())
			{
				LanguageInfo info;
				info.code = languageCode;
				info.name = languageName;
				info.filePath = entry.path().wstring();
				languages.push_back(info);
			}
		}
	}

	// Sort by language code for consistency
	std::sort(languages.begin(), languages.end(),
		[](const LanguageInfo& a, const LanguageInfo& b) {
			return a.code < b.code;
		});

	// Cache the results
	m_availableLanguages = languages;

	return languages;
}

bool Localization::LoadLanguage(const std::wstring& languageCode, const std::filesystem::path& localDir)
{
	// Construct expected file path
	std::filesystem::path langFile = localDir / (languageCode + L".ini");

	// Try to load the file
	if (std::filesystem::exists(langFile))
	{
		if (Load(langFile.wstring()))
		{
			SetLanguage(languageCode);
			return true;
		}
	}

	return false;
}

std::wstring Localization::GetString(const std::wstring& key, const std::wstring& defaultValue) const
{
	auto it = m_strings.find(key);
	if (it == m_strings.end())
		return defaultValue;

	return it->second;
}

void Localization::SetLanguage(const std::wstring& lang)
{
	m_currentLanguage = lang;
}
