#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

enum class ActorCategory
{
	private_,
	common,
	special
};

struct EntityEntry
{
	uint32_t entity_id;
	std::string name;
};

struct EventEntry
{
	uint16_t event_id;
	uint16_t array_index;
	uint32_t byte_offset;
	uint32_t byte_size;
	std::vector<uint8_t> bytecode;
};

struct ActorBlock
{
	uint32_t actor_number;
	std::vector<uint32_t> imed_data;
	std::vector<uint8_t> event_data; // full event bytecode image
	std::vector<EventEntry> events;
};

struct ZoneRawData
{
	uint32_t zone_id;
	std::string zone_name;
	std::vector<ActorBlock> actors;
	std::unordered_map<uint32_t, EntityEntry> entity_map;
	std::vector<std::u8string> zone_strings;
};

struct RefInfo
{
	enum Type { evsb_index, workaddr };
	Type type;
	uint32_t target;
	std::string work_type; // "WorkLocal", "Work_Zone", etc.
};

struct DialogueLine
{
	std::string speaker;
	std::string text;
	uint32_t message_id = 0;
	uint32_t evsb_index = 0;
	std::vector<RefInfo> refs;
};

	struct ResolvedEvent
{
	uint16_t event_id;
	uint16_t array_index;
	uint32_t byte_offset;
	uint32_t byte_size;
	std::vector<uint8_t> bytecode;
	std::vector<DialogueLine> dialogues;
	std::string text_ref; // relative path to text file (when --split-text)
	std::vector<std::string> opcodes; // optional disassembly lines
};

struct ResolvedActor
{
	uint32_t actor_number;
	std::string actor_name;
	ActorCategory category;
	std::vector<uint32_t> imed_data;
	std::vector<ResolvedEvent> events;
};

struct ZoneData
{
	uint32_t zone_id;
	std::string zone_name;
	std::vector<ResolvedActor> actors;
};

struct ZoneDef
{
	std::string zone_name;
	std::string evev_path;
	std::string evac_path;
	std::string evsb_path;    // NA (English) strings
	std::string evsb_jp_path; // JP (Japanese) strings
	std::string path;         // raw path from CSV
	std::string type;         // raw type from CSV
	std::string lang;         // raw lang from CSV
	std::string cell_indices; // raw cell indices from CSV

	bool IsValid() const
	{
		return !zone_name.empty() && !evev_path.empty();
	}

	std::string GetEvsbPath(bool preferJp = false) const
	{
		if (preferJp)
		{
			if (!evsb_jp_path.empty())
				return evsb_jp_path;
			return evsb_path;
		}
		if (!evsb_path.empty())
			return evsb_path;
		return evsb_jp_path;
	}
};

struct CommonActorData
{
	std::string actor_name;
	bool verified;
	std::vector<std::string> zone_names;
	std::vector<ResolvedEvent> events;
};

struct ZoneIndexEntry
{
	uint32_t actor_number;
	std::string actor_name;
	ActorCategory category;
	std::string local_ref;
	std::string common_ref;
};

struct ZoneIndex
{
	uint32_t zone_id;
	std::string zone_name;
	std::vector<ZoneIndexEntry> entries;
};
