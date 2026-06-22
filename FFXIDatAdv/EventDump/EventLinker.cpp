#include "EventLinker.h"

EventLinker::EventLinker()
{
}

ZoneData EventLinker::LinkZone(
	const std::string& zone_name,
	uint32_t zone_id,
	const std::vector<ActorBlock>& actors,
	const std::unordered_map<uint32_t, EntityEntry>& entity_map,
	const std::vector<std::u8string>& zone_strings)
{
	ZoneData zone;
	zone.zone_id = zone_id;
	zone.zone_name = zone_name;

	for (const auto& block : actors)
	{
		ResolvedActor actor;
		actor.actor_number = block.actor_number;
		actor.category = ActorCategory::private_;
		actor.imed_data = block.imed_data;

		// Resolve actor name from entity map
		auto it = entity_map.find(block.actor_number);
		if (it != entity_map.end() && !it->second.name.empty())
			actor.actor_name = it->second.name;
		else if (block.actor_number == 0x7FFFFFF0)
			actor.actor_name = "Zone Events";
		else if (block.actor_number == 0x7FFFFFFF)
			actor.actor_name = "Zone/Player Events";
		else
			actor.actor_name = "";

		for (const auto& evt : block.events)
		{
			ResolvedEvent resolved;
			resolved.event_id = evt.event_id;
			resolved.array_index = evt.array_index;
			resolved.byte_offset = evt.byte_offset;
			resolved.byte_size = evt.byte_size;
			resolved.bytecode = evt.bytecode;

			ExtractDialoguesFromEvent(evt, block.actor_number,
				block.imed_data, entity_map, zone_strings, resolved.dialogues);

			actor.events.push_back(resolved);
		}

		zone.actors.push_back(actor);
	}

	return zone;
}

void EventLinker::ExtractDialoguesFromEvent(
	const EventEntry& evt,
	uint32_t actor_number,
	const std::vector<uint32_t>& imed_data,
	const std::unordered_map<uint32_t, EntityEntry>& entity_map,
	const std::vector<std::u8string>& zone_strings,
	std::vector<DialogueLine>& out_dialogues)
{
	analyzer_.ExtractDialogues(
		evt.bytecode,
		actor_number,
		imed_data,
		entity_map,
		zone_strings,
		out_dialogues,
		evt.byte_offset);
}
