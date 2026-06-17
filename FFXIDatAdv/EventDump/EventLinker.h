#pragma once

#include "Models.h"
#include "BytecodeAnalyzer.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

struct EvsbCache
{
	std::vector<std::string> zone_names;
	std::vector<std::vector<std::u8string>> strings;
};

class EventLinker
{
public:
	EventLinker();

	ZoneData LinkZone(
		const std::string& zone_name,
		uint32_t zone_id,
		const std::vector<ActorBlock>& actors,
		const std::unordered_map<uint32_t, EntityEntry>& entity_map,
		const std::vector<std::u8string>& zone_strings);

	void ExtractDialoguesFromEvent(
		const EventEntry& evt,
		uint32_t actor_number,
		const std::vector<uint32_t>& imed_data,
		const std::unordered_map<uint32_t, EntityEntry>& entity_map,
		const std::vector<std::u8string>& zone_strings,
		std::vector<DialogueLine>& out_dialogues);

private:
	BytecodeAnalyzer analyzer_;
};
