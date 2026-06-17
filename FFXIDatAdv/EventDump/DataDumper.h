#pragma once
#include "EventAnalyzer.h"
#include <string>
#include <vector>
#include <filesystem>
#include <unordered_map>
#include <map>
#include <set>

class DataDumper
{
public:
	DataDumper(const std::filesystem::path& outputDir);
	~DataDumper();

	void AddZone(std::unique_ptr<EventAnalyzer> analyzer);
	void AddOrphanFile(const std::string& comment, const std::vector<std::u8string>& strings);
	void Flush();

private:
	std::filesystem::path dir_;
	std::vector<std::unique_ptr<EventAnalyzer>> zones_;
	std::vector<std::pair<std::string, std::vector<std::u8string>>> orphanFiles_;
	std::unordered_map<uint64_t, std::string> contentCache_;

	std::string MakeTextPath(const std::string& zoneName, const std::string& actor, uint32_t an,
		uint16_t eid, uint16_t aidx, bool hasArrayAmbiguity,
		const std::string& actorDir) const;
	void WriteActorJson(const std::filesystem::path& path, const AnalyzedActor& actor,
		const std::string& zoneName,
		const std::map<uint16_t, std::set<uint16_t>>& arrayCounts,
		bool isCommon,
		const std::set<uint16_t>* tcEvents,
		const std::unordered_map<uint16_t, std::string>* actualPaths);
	std::string EscapeJson(const std::string& s) const;
};
