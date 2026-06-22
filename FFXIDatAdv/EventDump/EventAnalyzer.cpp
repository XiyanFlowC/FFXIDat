#include "EventAnalyzer.h"
#include "EventBinaryDat.h"
#include "EntityDat.h"
#include "EventLinker.h"
#include "GamePathResolver.h"
#include <EventStringBase.h>
#include <xystring.h>
#include <algorithm>
#include <iostream>
#include <map>

static std::vector<std::u8string> LoadEvsbStrings(const std::string& path)
{
	std::vector<std::u8string> result;
	try
	{
		EventStringBase esb(xybase::string::sys_mbs_to_wcs(path));
		esb.Read();
		result.reserve(esb.Size());
		for (size_t i = 0; i < esb.Size(); ++i)
			result.push_back(esb[i]);
	}
	catch (...) {}
	return result;
}

void EventAnalyzer::Load(
	const std::string& zoneName,
	const std::string& evevPath,
	const std::string& evacPath,
	const std::string& evsbPath)
{
	zoneName_ = zoneName;
	evsbStrings_ = LoadEvsbStrings(evsbPath);
	if (evsbStrings_.empty()) return;

	auto actors = EventBinaryDat::Parse(evevPath);

	std::unordered_map<uint32_t, EntityEntry> entityMap;
	if (!evacPath.empty())
	{
		auto entries = EntityDat::Parse(evacPath);
		for (const auto& e : entries)
			entityMap[e.entity_id] = e;
	}

	EventLinker linker;

	for (const auto& block : actors)
	{
		std::string actorName;
		if (block.actor_number == 0x7FFFFFF0)
			actorName = "Zone Events";
		else if (block.actor_number == 0x7FFFFFFF)
			actorName = "Zone/Player Events";
		else
		{
			auto it = entityMap.find(block.actor_number);
			if (it != entityMap.end() && !it->second.name.empty())
				actorName = it->second.name;
			else
				actorName = "";
		}
		if (actorName.empty()) continue;

		AnalyzedActor aa;
		aa.actor_name = actorName;
		aa.actor_number = block.actor_number;
		aa.imed_data = block.imed_data;

		{
			const auto& data = block.event_data;
			uint64_t h = 0;
			for (size_t i = 0; i < data.size(); ++i)
				h = h * 131 + data[i] + (h >> 31);
			char buf[32];
			snprintf(buf, sizeof(buf), "%016llX", (unsigned long long)h);
			aa.bytecode_hash = buf;
		}

		for (const auto& evt : block.events)
		{
			std::vector<DialogueLine> dialogues;
			linker.ExtractDialoguesFromEvent(evt, block.actor_number,
				block.imed_data, entityMap, evsbStrings_, dialogues);

			if (dialogues.empty()) continue;

			std::map<uint32_t, size_t> idxToLine;
			std::vector<std::u8string> textLines;
			std::vector<std::string> speakers;
			std::vector<uint32_t> lineIndices;
			std::set<uint32_t> evsbRefs;

			for (const auto& dl : dialogues)
			{
				uint32_t idx = dl.evsb_index;
				evsbRefs.insert(idx);
				referencedIndices_.insert(idx);

				auto lit = idxToLine.find(idx);
				if (lit != idxToLine.end())
				{
					speakers.push_back(dl.speaker);
					lineIndices.push_back((uint32_t)lit->second);
				}
				else
				{
					size_t lineNum = textLines.size();
					idxToLine[idx] = lineNum;
					textLines.push_back(dl.text);
					speakers.push_back(dl.speaker);
					lineIndices.push_back((uint32_t)lineNum);
				}
			}

			AnalyzedEvent ae;
			ae.event_id = evt.event_id;
			ae.array_index = evt.array_index;
			ae.textLines = std::move(textLines);
			ae.speakers = std::move(speakers);
			ae.lineIndices = std::move(lineIndices);
			ae.evsbRefs.assign(evsbRefs.begin(), evsbRefs.end());

			uint64_t h = 0;
			for (const auto& line : ae.textLines)
				for (auto c : line)
					h = h * 131 + (unsigned char)c + (h >> 31);
			ae.textHash = h;

			aa.events.push_back(std::move(ae));
		}

		if (!aa.events.empty())
			actors_.push_back(std::move(aa));
	}
}
