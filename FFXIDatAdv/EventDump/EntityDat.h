#pragma once

#include "Models.h"
#include <string>
#include <vector>

class EntityDat
{
public:
	static std::vector<EntityEntry> Parse(const std::string& path);

	static std::vector<EntityEntry> ParseBytes(const std::vector<uint8_t>& data);

	static bool IsValidEntityId(uint32_t entity_id);
};
