#pragma once

#include "Models.h"
#include <string>
#include <vector>

class EventBinaryDat
{
public:
	static std::vector<ActorBlock> Parse(const std::string& path);

	static std::vector<ActorBlock> ParseBytes(const std::vector<uint8_t>& data);
};
