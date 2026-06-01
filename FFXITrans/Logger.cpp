#include "Logger.h"

#include <Windows.h>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>

Logger& Logger::Instance()
{
	static Logger instance;
	return instance;
}

void Logger::Initialize(const std::filesystem::path& logPath)
{
	std::lock_guard lock(mutex);

	if (logFile.is_open())
		return;

	logFile.open(logPath, std::ios::out | std::ios::trunc | std::ios::binary);
	if (!logFile.is_open())
		return;

	logFile << BuildPrefix("INFO") << "Logger initialized at " << ToUtf8(logPath) << "\n";
	logFile.flush();
}

void Logger::Close()
{
	std::lock_guard lock(mutex);
	if (!logFile.is_open())
		return;

	logFile << BuildPrefix("INFO") << "Logger closed\n";
	logFile.flush();
	logFile.close();
}

void Logger::Info(const std::string& message)
{
	Log("INFO", message);
}

void Logger::Warning(const std::string& message)
{
	Log("WARN", message);
}

void Logger::Error(const std::string& message)
{
	Log("ERROR", message);
}

void Logger::Info(const std::wstring& message)
{
	Log("INFO", message);
}

void Logger::Warning(const std::wstring& message)
{
	Log("WARN", message);
}

void Logger::Error(const std::wstring& message)
{
	Log("ERROR", message);
}

void Logger::Log(const std::string& level, const std::string& message)
{
	std::lock_guard lock(mutex);
	if (!logFile.is_open())
		return;

	logFile << BuildPrefix(level) << message << '\n';
	logFile.flush();
}

void Logger::Log(const std::string& level, const std::wstring& message)
{
	Log(level, ToUtf8(message));
}

std::string Logger::ToUtf8(const std::wstring& text)
{
	if (text.empty())
		return {};

	const int size = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
	if (size <= 0)
		return {};

	std::string result(static_cast<size_t>(size), '\0');
	WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), result.data(), size, nullptr, nullptr);
	return result;
}

std::string Logger::ToUtf8(const std::filesystem::path& path)
{
	return ToUtf8(path.wstring());
}

std::string Logger::ToUtf8(const std::u8string& text)
{
	return std::string(reinterpret_cast<const char*>(text.c_str()), text.size());
}

std::string Logger::BuildPrefix(const std::string& level) const
{
	const auto now = std::chrono::system_clock::now();
	const auto time = std::chrono::system_clock::to_time_t(now);
	std::tm localTime{};
	localtime_s(&localTime, &time);

	std::ostringstream stream;
	stream << '[' << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S") << "] [" << level << "] ";
	return stream.str();
}
