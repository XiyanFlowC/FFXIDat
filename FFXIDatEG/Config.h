#pragma once

#include <string>
#include <map>
#include <fstream>
#include <filesystem>

class Config
{
public:
	static Config& Instance();

	// Load configuration from INI file
	bool Load(const std::wstring& filePath);
	
	// Save configuration to INI file
	bool Save(const std::wstring& filePath);
	
	// Get string value
	std::wstring GetString(const std::wstring& section, const std::wstring& key, const std::wstring& defaultValue = L"");
	
	// Get integer value
	int GetInt(const std::wstring& section, const std::wstring& key, int defaultValue = 0);
	
	// Set string value
	void SetString(const std::wstring& section, const std::wstring& key, const std::wstring& value);
	
	// Set integer value
	void SetInt(const std::wstring& section, const std::wstring& key, int value);

	// Convenience methods for common settings
	std::wstring GetUILanguage() const;
	void SetUILanguage(const std::wstring& language);

private:
	Config() = default;
	~Config() = default;
	Config(const Config&) = delete;
	Config& operator=(const Config&) = delete;

	std::map<std::wstring, std::map<std::wstring, std::wstring>> m_data;
	std::wstring m_filePath;
};
