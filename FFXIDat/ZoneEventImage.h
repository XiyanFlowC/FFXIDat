#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct EventDescriptor
{
	uint16_t event_id;
	uint16_t event_index;
};

struct ActorBlock
{
	uint32_t actor_id;
	std::vector<uint32_t> constants;
	std::vector<EventDescriptor> events;
};

class ZoneEventImage
{
public:
	bool Load(const std::string& path);

	const std::vector<ActorBlock>& GetActors() const { return actors_; }

private:
	std::vector<ActorBlock> actors_;
};
