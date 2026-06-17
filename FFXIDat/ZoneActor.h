#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

struct ActorEntry
{
	uint32_t actor_id;
	std::string name;
};

class ZoneActor
{
public:
	bool Load(const std::string& path);

	const std::vector<ActorEntry>& GetEntries() const { return entries_; }
	std::unordered_map<uint32_t, std::string> GetIdToNameMap() const;

	bool SetName(uint32_t actor_id, const std::string& name);
	bool Write(const std::string& path) const;

private:
	std::vector<ActorEntry> entries_;

	static constexpr uint32_t NAME_BYTES = 28;
	static constexpr uint32_t RECORD_SIZE = 32;
};
