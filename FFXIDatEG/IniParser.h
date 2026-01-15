#pragma once

#include <string>
#include <map>
#include <fstream>

/// <summary>
/// A simple INI file parser that supports:
/// - Sections [SectionName]
/// - Key-value pairs (Key=Value)
/// - Comments (lines starting with ; or #)
/// - Line continuations (backslash at end of line)
/// - Unicode (wstring) support
/// </summary>
class IniParser
{
public:
	using Section = std::map<std::wstring, std::wstring>;
	using Data = std::map<std::wstring, Section>;

	IniParser() = default;
	~IniParser() = default;

	/// <summary>
	/// Load and parse an INI file
	/// </summary>
	/// <param name="filePath">Path to the INI file</param>
	/// <returns>True if loaded successfully, false otherwise</returns>
	bool Load(const std::wstring& filePath);

	/// <summary>
	/// Save data to an INI file
	/// </summary>
	/// <param name="filePath">Path to the INI file</param>
	/// <returns>True if saved successfully, false otherwise</returns>
	bool Save(const std::wstring& filePath) const;

	/// <summary>
	/// Get a string value from the INI data
	/// </summary>
	/// <param name="section">Section name</param>
	/// <param name="key">Key name</param>
	/// <param name="defaultValue">Default value if not found</param>
	/// <returns>The value or default value</returns>
	std::wstring GetString(const std::wstring& section, const std::wstring& key, const std::wstring& defaultValue = L"") const;

	/// <summary>
	/// Set a string value in the INI data
	/// </summary>
	/// <param name="section">Section name</param>
	/// <param name="key">Key name</param>
	/// <param name="value">Value to set</param>
	void SetString(const std::wstring& section, const std::wstring& key, const std::wstring& value);

	/// <summary>
	/// Check if a section exists
	/// </summary>
	bool HasSection(const std::wstring& section) const;

	/// <summary>
	/// Check if a key exists in a section
	/// </summary>
	bool HasKey(const std::wstring& section, const std::wstring& key) const;

	/// <summary>
	/// Get all data
	/// </summary>
	const Data& GetData() const { return m_data; }

	/// <summary>
	/// Clear all data
	/// </summary>
	void Clear() { m_data.clear(); }

private:
	Data m_data;

	/// <summary>
	/// Trim leading and trailing whitespace from a string
	/// </summary>
	static std::wstring Trim(const std::wstring& str, const std::wstring& whitespace = L" \t\r\n");

	/// <summary>
	/// Trim only trailing whitespace
	/// </summary>
	static std::wstring TrimTrailing(const std::wstring& str, const std::wstring& whitespace = L" \t\r\n");

	/// <summary>
	/// Trim only leading whitespace
	/// </summary>
	static std::wstring TrimLeading(const std::wstring& str, const std::wstring& whitespace = L" \t");
};
