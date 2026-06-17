#pragma once

#include <string>
#include <filesystem>
#include <set>

class ApplicationAdv
{
public:
	static ApplicationAdv& Instance();

	int Run(bool interactive);
	int PrepareSourceData(const std::string& lang = "ja");

private:
	ApplicationAdv() = default;

	bool Initialize();
	int ProcessAllFiles();
	int ProcessOneFile(const std::string& comment, const std::string& type, const std::string& path, const std::string& lang);

	std::filesystem::path exeDir_;
};
