#pragma once

#include "Models.h"
#include <string>
#include <vector>
#include <unordered_map>

struct ActorRef
{
	uint32_t zone_id;
	const ResolvedActor* actor;
};

struct VerificationResult
{
	std::string actor_name;
	bool all_match;
	std::vector<uint32_t> zone_ids;
	std::vector<ResolvedEvent> events;
};

class CommonVerifier
{
public:
	// Collect events from all zones for each actor_name
	std::unordered_map<std::string, std::vector<ActorRef>> GroupByName(
		const std::vector<ZoneData>& allZones);

	// Verify that bytecode is identical across all instances
	VerificationResult Verify(
		const std::string& actorName,
		const std::vector<ActorRef>& instances);

	// Run full verification across all zones
	std::vector<VerificationResult> RunVerification(const std::vector<ZoneData>& allZones);
};
