#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>

struct ActorBlock;
struct EntityEntry;
struct ZoneFiles;

struct EventTextOutResult
{
	std::string zone_name;
	int total_actors = 0;
	int total_events = 0;
	int total_lines = 0;
	int skipped_no_text = 0;
};

class EventTextOut
{
public:
	EventTextOut(const std::filesystem::path& outputDir, bool useRefFiles = false);

	EventTextOutResult RunAllZones(const std::unordered_map<std::string, ZoneFiles>& zones);
	EventTextOutResult RunZone(const std::string& zoneName, const ZoneFiles& zone);

	EventTextOutResult RunAllZones();
	EventTextOutResult RunZone(const std::string& zoneName);

	void SetEvsbLang(const std::string& lang) { evsbLang_ = lang; }

	static std::string GetEvsbPathForLang(const ZoneFiles& zf, const std::string& lang);

	static std::string ActorBareName(const ActorBlock& block,
		const std::unordered_map<uint32_t, EntityEntry>& entity_map);

private:
	std::filesystem::path outputDir_;
	bool useRefFiles_ = false;
	std::string evsbLang_ = "ja";
	std::filesystem::path refCsvPath_;
	std::vector<std::pair<std::string, std::string>> refEntries_;

	std::filesystem::path CommonDir() const;
	std::filesystem::path ZoneDir(const std::string& zoneName) const;
	std::string SafeFilename(const std::string& name) const;

	static size_t TextSetHash(const std::vector<std::string>& texts);
	void WriteRefCsv();
	void WriteTxtFile(const std::filesystem::path& path,
		const std::vector<std::string>& texts,
		std::unordered_map<size_t, std::string>& textCache);
	void WriteTxtFileSimple(const std::filesystem::path& path, const std::vector<std::string>& texts);
};
