#pragma once

#include <string>
#include <filesystem>

struct EventTextInResult
{
	std::string zone_name;
	int total_events = 0;
	int replaced = 0;
	int verified_ok = 0;
	int fallback_lines = 0;
	int errors = 0;
};

class EventTextIn
{
public:
	EventTextIn(const std::filesystem::path& textsDir);

	EventTextInResult RunAllZones();
	EventTextInResult RunZone(const std::string& zoneName);

private:
	std::filesystem::path textsDir_;

	std::filesystem::path CommonDir() const;
	std::filesystem::path ZoneDir(const std::string& zoneName) const;
};
