#pragma once

#include <string>
#include <map>
#include <vector>
#include <filesystem>

// Structure to hold language information
struct LanguageInfo
{
	std::wstring code;        // e.g., "en", "zh"
	std::wstring name;        // e.g., "English", "ÖÐÎÄ"
	std::wstring filePath;    // Full path to the .ini file
};

class Localization
{
public:
	static Localization& Instance();

	// Load localization strings from INI file
	bool Load(const std::wstring& filePath);

	// Scan local directory and discover available languages
	std::vector<LanguageInfo> ScanAvailableLanguages(const std::filesystem::path& localDir);

	// Load language by code from local directory
	bool LoadLanguage(const std::wstring& languageCode, const std::filesystem::path& localDir);

	// Get localized string by key
	std::wstring GetString(const std::wstring& key, const std::wstring& defaultValue = L"") const;

	// Set current language
	void SetLanguage(const std::wstring& lang);

	// Get current language
	const std::wstring& GetLanguage() const { return m_currentLanguage; }

	// Get available languages (cached from last scan)
	const std::vector<LanguageInfo>& GetAvailableLanguages() const { return m_availableLanguages; }

private:
	Localization() : m_currentLanguage(L"en") {}
	~Localization() = default;
	Localization(const Localization&) = delete;
	Localization& operator=(const Localization&) = delete;

	std::wstring m_currentLanguage;
	std::map<std::wstring, std::wstring> m_strings;
	std::vector<LanguageInfo> m_availableLanguages;
};

// Convenience macro for getting localized strings
#define LOC(key) Localization::Instance().GetString(key, key)
