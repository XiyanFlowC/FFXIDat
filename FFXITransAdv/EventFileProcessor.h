#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include "../FFXITrans/FileProcessor.h"
#include "../FFXITrans/ProcessorUtils.h"
#include "EventDefs.h"

class ZoneRegistry
{
public:
	static ZoneRegistry& Instance();

	void LoadFromDefsCsv(const std::filesystem::path& csvPath);
	void LoadFromZoneEventsCsv(const std::filesystem::path& csvPath);

	const ZoneFiles* FindByZoneName(const std::string& zoneName) const;
	const ZoneFiles* FindByEvevPath(const std::string& evevPath) const;

	const std::unordered_map<std::string, ZoneFiles>& AllZones() const { return zones_; }

private:
	ZoneRegistry() = default;
	std::unordered_map<std::string, ZoneFiles> zones_;

	static std::string CanonicalPath(const std::string& path);
};

// FileProcessor for evev type: handles three-file event processing
class EventFileProcessor : public FileProcessor
{
public:
	std::u8string GetSupportedType() const override { return u8"evev"; }

	bool Process(
		const FileProcessDef& fileDef,
		const std::filesystem::path& datPath,
		const std::filesystem::path& outPath,
		const std::map<std::u8string, FileProcessDef>& jpDefsByComment) override;
};
