#include "DataDumper.h"
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <map>
#include <set>

DataDumper::DataDumper(const std::filesystem::path& outputDir)
	: dir_(outputDir)
{
}

DataDumper::~DataDumper()
{
	Flush();
}

void DataDumper::AddZone(std::unique_ptr<EventAnalyzer> analyzer)
{
	zones_.push_back(std::move(analyzer));
}

void DataDumper::AddOrphanFile(const std::string& comment, const std::vector<std::u8string>& strings)
{
	orphanFiles_.push_back({comment, strings});
}

static std::string SanitizeFilename(const std::string& s)
{
	std::string r;
	for (char c : s)
	{
		if (c == '<' || c == '>' || c == ':' || c == '"' || c == '/' ||
		    c == '\\' || c == '|' || c == '?' || c == '*')
			r += '_';
		else
			r += c;
	}
	if (r.empty()) r = "";
	return r;
}

static std::string GetActorDir(const AnalyzedActor& actor, const std::vector<AnalyzedActor>& allActors)
{
	int count = 0;
	for (const auto& a : allActors)
		if (a.actor_name == actor.actor_name) ++count;
	std::string base = SanitizeFilename(actor.actor_name);
	if (count <= 1)
		return base;
	return base + "_" + std::to_string(actor.actor_number);
}

static std::map<uint16_t, std::set<uint16_t>> CountArrayIndices(const AnalyzedActor& actor)
{
	std::map<uint16_t, std::set<uint16_t>> counts;
	for (const auto& evt : actor.events)
		counts[evt.event_id].insert(evt.array_index);
	return counts;
}

bool HasArrayAmbiguity(uint16_t eid, const std::map<uint16_t, std::set<uint16_t>>& counts)
{
	auto it = counts.find(eid);
	return it != counts.end() && it->second.size() > 1;
}

std::string DataDumper::MakeTextPath(const std::string& zoneName, const std::string& actorName,
	uint32_t an, uint16_t eid, uint16_t aidx, bool hasAmbiguity,
	const std::string& actorDir) const
{
	(void)an;
	auto fname = std::to_string(aidx);
	if (hasAmbiguity && !actorName.empty())
		fname += "." + std::to_string(eid);
	return "event/text/" + zoneName + "/" + actorDir + "/" + fname + ".txt";
}

void DataDumper::Flush()
{
	// 1. Detect common actors (same name across zones) -- bytecode_hash removed
	std::map<std::string, std::vector<std::pair<std::string, AnalyzedActor*>>> commonCandidates;
	for (const auto& zone : zones_)
	{
		for (auto& actor : const_cast<std::vector<AnalyzedActor>&>(zone->GetActors()))
		{
			if (actor.actor_name == "") continue;
			commonCandidates[actor.actor_name].push_back({zone->GetName(), &actor});
		}
	}

	std::unordered_set<AnalyzedActor*> commonActors;
	for (const auto& [key, list] : commonCandidates)
	{
		if (list.size() >= 2)
		{
			std::set<std::string> zoneNames;
			for (const auto& [zn, _] : list) zoneNames.insert(zn);
			if (zoneNames.size() >= 2)
				for (const auto& [_, ptr] : list)
					commonActors.insert(ptr);
		}
	}

	// 2. Detect text-common per-event: compare textHash for same (name@hash, event_id) across zones
	std::unordered_map<AnalyzedActor*, std::set<uint16_t>> textCommonEvents;
	for (const auto& [key, list] : commonCandidates)
	{
		if (list.size() < 2) continue;
		std::set<std::string> zn;
		for (const auto& [z, _] : list) zn.insert(z);
		if (zn.size() < 2) continue;

		std::set<uint16_t> allEids;
		for (const auto& [_, ptr] : list)
			for (const auto& evt : ptr->events)
				if (!evt.textLines.empty())
					allEids.insert(evt.event_id);

		for (uint16_t eid : allEids)
		{
			uint64_t commonHash = 0;
			bool firstFound = false;
			bool allMatch = true;
			for (size_t zi = 0; zi < list.size(); ++zi)
			{
				auto* actor = list[zi].second;
				const AnalyzedEvent* match = nullptr;
				for (const auto& evt : actor->events)
					if (evt.event_id == eid && !evt.textLines.empty())
					{ match = &evt; break; }
				if (!match) continue;
				if (!firstFound)
				{ commonHash = match->textHash; firstFound = true; }
				else if (match->textHash != commonHash)
				{ allMatch = false; break; }
			}
			if (firstFound && allMatch)
				for (const auto& [_, ptr] : list)
					textCommonEvents[ptr].insert(eid);
		}
	}

	// 3. Write output
	std::unordered_map<AnalyzedActor*, std::unordered_map<uint16_t, std::string>> actualPaths;
	for (const auto& zone : zones_)
	{
		for (const auto& actor : zone->GetActors())
		{
			bool isCommon = commonActors.count(const_cast<AnalyzedActor*>(&actor)) > 0;
			std::string zoneName = zone->GetName();
			std::string jsonZone = (isCommon ? "common" : zoneName);
			auto& tcSet = textCommonEvents[const_cast<AnalyzedActor*>(&actor)];
			std::string jsonDir = isCommon
				? SanitizeFilename(actor.actor_name)
				: GetActorDir(actor, zone->GetActors());
			auto arrayCounts = CountArrayIndices(actor);
			auto* ap = &actualPaths[const_cast<AnalyzedActor*>(&actor)];

			for (const auto& evt : actor.events)
			{
				if (evt.textLines.empty()) continue;

				bool evtTextCommon = tcSet.count(evt.event_id) > 0;
				std::string textZone = evtTextCommon ? "common" : zoneName;
				std::string textDirName = evtTextCommon
					? SanitizeFilename(actor.actor_name)
					: GetActorDir(actor, zone->GetActors());
				auto fname = std::to_string(evt.array_index);

				auto relPath = MakeTextPath(textZone, actor.actor_name, actor.actor_number,
					evt.event_id, evt.array_index, false, textDirName);

				auto cit = contentCache_.find(evt.textHash);
				if (cit != contentCache_.end())
				{
					(*ap)[evt.event_id] = cit->second;
				}
				else
				{
					auto textDir = dir_ / "event" / "text" / textZone / textDirName;
					std::filesystem::create_directories(textDir);
					auto txtPath = textDir / (fname + ".txt");
					if (std::filesystem::exists(txtPath))
					{
						std::ifstream in(txtPath, std::ios::binary);
						std::string existing((std::istreambuf_iterator<char>(in)),
							std::istreambuf_iterator<char>());
						std::string newContent;
						for (const auto& line : evt.textLines)
							newContent += std::string(line.begin(), line.end()) + "\n";
						if (existing == newContent)
						{
							contentCache_[evt.textHash] = relPath;
							(*ap)[evt.event_id] = relPath;
							continue;
						}
						bool resolved = false;
						std::string fallbackZones[] = {textZone, zoneName};
						std::string fallbackDirs[] = {textDirName, GetActorDir(actor, zone->GetActors())};
						for (int fi = 0; fi < 2 && !resolved; ++fi)
						{
							if (fi == 0 && textZone == zoneName) continue;
							auto fbDir = dir_ / "event" / "text" / fallbackZones[fi] / fallbackDirs[fi];
							std::filesystem::create_directories(fbDir);
							auto fbPath = fbDir / (fname + ".txt");
							if (!std::filesystem::exists(fbPath))
							{
								textZone = fallbackZones[fi];
								textDirName = fallbackDirs[fi];
								textDir = fbDir;
								txtPath = fbPath;
								relPath = MakeTextPath(textZone, actor.actor_name, actor.actor_number,
									evt.event_id, evt.array_index, false, textDirName);
								resolved = true;
							}
						}
						if (!resolved)
						{
							std::string ufname = std::to_string(evt.array_index) + "." + std::to_string(evt.event_id) + ".txt";
							txtPath = textDir / ufname;
							relPath = MakeTextPath(textZone, actor.actor_name, actor.actor_number,
								evt.event_id, evt.array_index, true, textDirName);
						}
					}
					if (!std::filesystem::exists(txtPath))
					{
						std::ofstream out(txtPath);
						for (const auto& line : evt.textLines)
							out << std::string(line.begin(), line.end()) << "\n";
					}
					contentCache_[evt.textHash] = relPath;
					(*ap)[evt.event_id] = relPath;
				}
			}

			auto eventDir = dir_ / "event" / jsonZone;
			std::filesystem::create_directories(eventDir);
			auto jsonPath = eventDir / (jsonDir + ".json");
			if (!std::filesystem::exists(jsonPath))
				WriteActorJson(jsonPath, actor, zoneName, arrayCounts, isCommon, &tcSet, ap);
		}
	}

	// 4. ref.csv
	{
		auto refPath = dir_ / "event" / "ref.csv";
		std::ofstream out(refPath);
		out << "zone,actor,actor_number,event_id,array_index,text_path,string_count\n";
		for (const auto& zone : zones_)
		{
			for (const auto& actor : zone->GetActors())
			{
				auto* ap = &actualPaths[const_cast<AnalyzedActor*>(&actor)];
				for (const auto& evt : actor.events)
				{
					if (evt.textLines.empty()) continue;
					auto ait = ap->find(evt.event_id);
					std::string tp = (ait != ap->end()) ? ait->second : "";
					if (tp.empty()) continue;
					out << zone->GetName() << ","
						<< actor.actor_name << ","
						<< actor.actor_number << ","
						<< evt.event_id << ","
						<< evt.array_index << ","
						<< tp << ","
						<< evt.textLines.size() << "\n";
				}
			}
		}
	}

	// 5. evsb_msgs.txt - evsb strings not referenced by any dialogue (within-zone orphan, cross-zone dedup)
	{
		std::set<std::u8string> orphanTexts;
		for (const auto& zone : zones_)
		{
			const auto& refs = zone->GetReferencedIndices();
			const auto& strings = zone->GetEvsbStrings();
			for (uint32_t i = 0; i < (uint32_t)strings.size(); ++i)
			{
				if (refs.count(i) == 0 && !strings[i].empty())
					orphanTexts.insert(strings[i]);
			}
		}

		if (!orphanTexts.empty())
		{
			auto path = dir_ / "evsb_msgs.txt";
			std::ofstream out(path);
			for (const auto& u : orphanTexts)
				out << std::string(u.begin(), u.end()) << "\n";
		}
	}

	// 6. Whole-file orphan evsb (zones with no evev)
	for (const auto& [name, strings] : orphanFiles_)
	{
		auto path = dir_ / (name + ".txt");
		std::filesystem::create_directories(path.parent_path());
		std::ofstream out(path);
		for (const auto& u : strings)
			if (!u.empty())
				out << std::string(u.begin(), u.end()) << "\n";
	}

	std::cout << "[db] Done. " << zones_.size() << " zones to " << dir_ << std::endl;
}

void DataDumper::WriteActorJson(const std::filesystem::path& path, const AnalyzedActor& actor,
	const std::string& zoneName,
	const std::map<uint16_t, std::set<uint16_t>>& arrayCounts,
	bool isCommon,
	const std::set<uint16_t>* tcEvents,
	const std::unordered_map<uint16_t, std::string>* actualPaths)
{
	std::ofstream out(path);
	out << "{\n";
	out << "  \"actor\": " << EscapeJson(actor.actor_name) << ",\n";
	out << "  \"actor_number\": " << actor.actor_number << ",\n";

	std::vector<std::string> allSpeakers;
	std::unordered_map<std::string, int> speakerMap;
	for (const auto& evt : actor.events)
		for (const auto& s : evt.speakers)
			if (speakerMap.find(s) == speakerMap.end())
			{
				speakerMap[s] = (int)allSpeakers.size();
				allSpeakers.push_back(s);
			}

	if (!allSpeakers.empty())
	{
		out << "  \"speakers\": [";
		for (size_t i = 0; i < allSpeakers.size(); ++i)
		{
			if (i) out << ", ";
			out << EscapeJson(allSpeakers[i]);
		}
		out << "],\n";
	}

	out << "  \"events\": [\n";
	bool first = true;
	for (const auto& evt : actor.events)
	{
		if (evt.textLines.empty()) continue;
		if (!first) out << ",\n";
		first = false;

		std::string tp;
		if (actualPaths)
		{
			auto ait = actualPaths->find(evt.event_id);
			tp = (ait != actualPaths->end()) ? ait->second : "";
		}

		out << "    {\n";
		out << "      \"event_id\": " << evt.event_id << ",\n";
		out << "      \"text\": " << EscapeJson(tp) << ",\n";
		/*out << "      \"evsb_refs\": [";
		for (size_t ri = 0; ri < evt.evsbRefs.size(); ++ri)
		{
			if (ri) out << ", ";
			out << evt.evsbRefs[ri];
		}
		out << "],\n";*/ // meaningless
		out << "      \"dialogues\": [\n";
		for (size_t i = 0; i < evt.speakers.size(); ++i)
		{
			if (i) out << ",\n";
			out << "        {\"speaker\": " << speakerMap[evt.speakers[i]]
				<< ", \"line\": " << evt.lineIndices[i] << "}";
		}
		out << "\n      ]\n";
		out << "    }";
	}
	out << "\n  ]\n";
	out << "}\n";
}

std::string DataDumper::EscapeJson(const std::string& s) const
{
	std::string r = "\"";
	for (char c : s)
	{
		switch (c)
		{
		case '"': r += "\\\""; break;
		case '\\': r += "\\\\"; break;
		case '\n': r += "\\n"; break;
		case '\r': r += "\\r"; break;
		case '\t': r += "\\t"; break;
		default: r += c;
		}
	}
	r += "\"";
	return r;
}
