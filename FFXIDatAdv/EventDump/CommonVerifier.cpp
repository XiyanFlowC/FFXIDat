#include "CommonVerifier.h"
#include <algorithm>
#include <iostream>
#include <cstring>

std::unordered_map<std::string, std::vector<ActorRef>> CommonVerifier::GroupByName(
	const std::vector<ZoneData>& allZones)
{
	std::unordered_map<std::string, std::vector<ActorRef>> groups;

	for (const auto& zone : allZones)
	{
		for (const auto& actor : zone.actors)
		{
			if (actor.actor_name.empty() || actor.actor_name == "_")
				continue;

			// Skip special actors
			if (actor.actor_number == 0x7FFFFFF0 ||
				actor.actor_number == 0x7FFFFFFF)
				continue;

		groups[actor.actor_name].push_back({zone.zone_id, &actor});
	}
	}

	return groups;
}

VerificationResult CommonVerifier::Verify(
	const std::string& actorName,
	const std::vector<ActorRef>& instances)
{
	VerificationResult result;
	result.actor_name = actorName;

	for (const auto& inst : instances)
		result.zone_ids.push_back(inst.zone_id);

	if (instances.size() < 2)
	{
		result.all_match = true;
		return result;
	}

	result.all_match = true;

	// Use the first instance as baseline
	const auto& base = *instances[0].actor;

	// Compare each event by array_index
	size_t maxEvents = base.events.size();
	for (const auto& inst : instances)
	{
		if (inst.actor->events.size() != maxEvents)
		{
			result.all_match = false;
			break;
		}
	}

	if (result.all_match)
	{
		for (size_t idx = 0; idx < maxEvents; ++idx)
		{
			const auto& baseEvent = base.events[idx];
			const auto& baseBytes = baseEvent.bytecode;

			for (size_t i = 1; i < instances.size(); ++i)
			{
				const auto& cmpEvent = instances[i].actor->events[idx];
				const auto& cmpBytes = cmpEvent.bytecode;

				if (baseBytes.size() != cmpBytes.size() ||
					memcmp(baseBytes.data(), cmpBytes.data(), baseBytes.size()) != 0)
				{
					result.all_match = false;

					std::cerr << "[VERIFY] WARNING: Actor \"" << actorName
						<< "\" event[" << idx << "] (id=" << baseEvent.event_id
						<< ") differs between instances."
						<< " Base size=" << baseBytes.size()
						<< " vs other size=" << cmpBytes.size()
						<< std::endl;
					break;
				}
			}

			if (!result.all_match)
				break;
		}
	}

	if (result.all_match)
	{
		result.events = base.events;
		std::cout << "[VERIFY] OK: Actor \"" << actorName << "\" (" << instances.size()
			<< " zones) bytecode match." << std::endl;
	}

	return result;
}

std::vector<VerificationResult> CommonVerifier::RunVerification(
	const std::vector<ZoneData>& allZones)
{
	auto groups = GroupByName(allZones);
	std::vector<VerificationResult> results;

	for (const auto& [name, instances] : groups)
	{
		auto res = Verify(name, instances);
		results.push_back(res);
	}

	return results;
}
