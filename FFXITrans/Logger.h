#pragma once

#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>

class Logger
{
public:
	static Logger& Instance();

	void Initialize(const std::filesystem::path& logPath);
	void Close();

	void Info(const std::string& message);
	void Warning(const std::string& message);
	void Error(const std::string& message);

	void Info(const std::wstring& message);
	void Warning(const std::wstring& message);
	void Error(const std::wstring& message);

	void Log(const std::string& level, const std::string& message);
	void Log(const std::string& level, const std::wstring& message);

	static std::string ToUtf8(const std::wstring& text);
	static std::string ToUtf8(const std::filesystem::path& path);
	static std::string ToUtf8(const std::u8string& text);

private:
	Logger() = default;
	Logger(const Logger&) = delete;
	Logger& operator=(const Logger&) = delete;

	std::string BuildPrefix(const std::string& level) const;

	std::mutex mutex;
	std::ofstream logFile;
};
