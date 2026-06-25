#pragma once
#include "Models.h"
#include <cstdint>
#include <string>
#include <vector>
#include <set>
#include <unordered_map>

struct AnalyzedEvent
{
	uint16_t event_id;
	uint16_t array_index;
	std::vector<std::u8string> textLines;       // deduped dialogue lines (txt content)
	std::vector<std::string> speakers;         // speaker name per raw dialogue entry
	std::vector<uint32_t> lineIndices;         // line index into textLines per raw dialogue
	std::vector<uint32_t> evsbRefs;            // sorted, unique evsb indices referenced
	uint64_t textHash = 0;
};

struct AnalyzedActor
{
	std::string actor_name;
	uint32_t actor_number;
	std::vector<uint32_t> imed_data;
	std::vector<AnalyzedEvent> events;
};

class EventAnalyzer
{
public:
	void Load(
		const std::string& zoneName,
		const std::string& evevPath,
		const std::string& evacPath,
		const std::string& evsbPath);

	const std::vector<AnalyzedActor>& GetActors() const { return actors_; }
	const std::vector<std::u8string>& GetEvsbStrings() const { return evsbStrings_; }
	const std::string& GetName() const { return zoneName_; }
	const std::set<uint32_t>& GetReferencedIndices() const { return referencedIndices_; }

private:
	std::string zoneName_;
	std::vector<AnalyzedActor> actors_;
	std::vector<std::u8string> evsbStrings_;
	std::set<uint32_t> referencedIndices_;
};
