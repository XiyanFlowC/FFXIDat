#pragma once

#include "Models.h"
#include <string>
#include <vector>
#include <unordered_map>

class ZoneConfig
{
public:
	static std::vector<ZoneDef> Load(const std::string& csvPath);

	static std::vector<ZoneDef> LoadFromString(const std::string& content);

	static std::unordered_map<std::string, ZoneDef> GroupByZone(const std::vector<ZoneDef>& defs);
};
