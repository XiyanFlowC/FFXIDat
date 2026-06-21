#include "ZoneConfig.h"
#include <fstream>
#include <sstream>
#include <algorithm>

std::vector<ZoneDef> ZoneConfig::Load(const std::string& csvPath)
{
	std::ifstream file(csvPath);
	if (!file.is_open())
		return {};

	std::stringstream ss;
	ss << file.rdbuf();
	return LoadFromString(ss.str());
}

std::vector<ZoneDef> ZoneConfig::LoadFromString(const std::string& content)
{
	std::vector<ZoneDef> result;
	std::istringstream stream(content);
	std::string line;

	while (std::getline(stream, line))
	{
		if (line.empty() || line[0] == '#')
			continue;

		// Format: path,type,lang,zone_name
		std::vector<std::string> fields;
		std::istringstream lineStream(line);
		std::string field;

		while (std::getline(lineStream, field, ','))
			fields.push_back(field);

		if (fields.size() < 4)
			continue;

		const std::string& path = fields[0];
		const std::string& type = fields[1];
		const std::string& lang = fields.size() > 2 ? fields[2] : "";
		const std::string& zoneName = fields[3];
		const std::string& cellIndices = fields.size() > 4 ? fields[4] : "";

		if (path.empty() || type.empty() || zoneName.empty())
			continue;

		// Find existing ZoneDef or create new
		auto it = std::find_if(result.begin(), result.end(),
			[&](const ZoneDef& z) { return z.zone_name == zoneName; });

		if (it == result.end())
		{
			ZoneDef def;
			def.zone_name = zoneName;
			def.path = path;
			def.type = type;
			def.lang = lang;
			def.cell_indices = cellIndices;
			result.push_back(def);
			it = result.end() - 1;
		}

		if (type == "evev")
			it->evev_path = path;
		else if (type == "evac")
			it->evac_path = path;
		else if (type == "evsb")
		{
			if (lang == "na" || lang.empty())
				it->evsb_path = path;
			else if (lang == "ja")
				it->evsb_jp_path = path;
		}

		// Always update raw fields for the latest entry
		it->path = path;
		it->type = type;
		if (!lang.empty()) it->lang = lang;
		if (!cellIndices.empty()) it->cell_indices = cellIndices;
	}

	return result;
}

std::unordered_map<std::string, ZoneDef> ZoneConfig::GroupByZone(const std::vector<ZoneDef>& defs)
{
	std::unordered_map<std::string, ZoneDef> map;
	for (const auto& def : defs)
		map[def.zone_name] = def;
	return map;
}
