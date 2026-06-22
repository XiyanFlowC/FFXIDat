#include "EventTextOut.h"
#include "Logger.h"
#include <iostream>
#include <fstream>
#include <format>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <vector>
#include <cstring>
#include <sstream>
#include "../FFXIDatAdv/EventDump/ZoneConfig.h"
#include "../FFXIDatAdv/EventDump/EventBinaryDat.h"
#include "../FFXIDatAdv/EventDump/EntityDat.h"
#include "../FFXIDatAdv/EventDump/BytecodeAnalyzer.h"
#include "../FFXIDatAdv/EventDump/EventLinker.h"
#include "../FFXIDatAdv/EventDump/GamePathResolver.h"
#include "../FFXIDat/EventStringBase.h"
#include <xystring.h>
#include "EventDefs.h"

std::string EventTextOut::GetEvsbPathForLang(const ZoneFiles& zf, const std::string& lang)
{
	if (lang == "en" && !zf.evsb_en_path.empty())
		return zf.evsb_en_path;
	if (lang == "de" && !zf.evsb_de_path.empty())
		return zf.evsb_de_path;
	if (lang == "fr" && !zf.evsb_fr_path.empty())
		return zf.evsb_fr_path;
	return zf.evsb_path;
}

EventTextOut::EventTextOut(const std::filesystem::path& outputDir, bool useRefFiles)
	: outputDir_(outputDir), useRefFiles_(useRefFiles)
{
	refCsvPath_ = outputDir_ / "event" / "ref.csv";
}

std::filesystem::path EventTextOut::CommonDir() const
{
	return outputDir_ / "event" / "common";
}

std::filesystem::path EventTextOut::ZoneDir(const std::string& zoneName) const
{
	// zoneName may contain / (e.g. "ev/Bastok Markets") for natural nesting
	return outputDir_ / "event" / "zone" / zoneName;
}

std::string EventTextOut::SafeFilename(const std::string& name) const
{
	if (name.empty()) return "_";
	std::string safe;
	safe.reserve(name.size());
	for (char c : name)
	{
		if (static_cast<unsigned char>(c) < 0x20 ||
			c == '\\' || c == '/' || c == ':' || c == '*' ||
			c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
			safe += '_';
		else
			safe += c;
	}
	return safe;
}

// --- Phase 1 scan result ---
struct ZoneScan
{
	std::string zone_name;
	std::vector<ActorBlock> actors;
	std::unordered_map<uint32_t, EntityEntry> entity_map;
	std::vector<std::u8string> zone_strings;
};

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
	catch (...)
	{
		Logger::Instance().Warning("Failed to load evsb: " + path);
	}
	return result;
}

// --- Phase 2: Common actor verification ---
struct ActorCandidate
{
	std::string actor_name;
	const ActorBlock* block;
	const ZoneScan* scan;
};

static std::unordered_map<std::string, std::vector<ActorCandidate>> GroupCandidates(
	const std::vector<ZoneScan>& scans)
{
	std::unordered_map<std::string, std::vector<ActorCandidate>> groups;
	for (const auto& scan : scans)
	{
		for (const auto& block : scan.actors)
		{
			std::string name;
			auto it = scan.entity_map.find(block.actor_number);
			if (it != scan.entity_map.end() && !it->second.name.empty())
				name = it->second.name;
			else if (block.actor_number == 0x7FFFFFF0)
				name = "Zone Events";
			else if (block.actor_number == 0x7FFFFFFF)
				name = "Zone/Player Events";
			else
				name = "_" + std::to_string(block.actor_number);

			if (name.empty()) continue;
			groups[name].push_back({name, &block, &scan});
		}
	}
	return groups;
}

static bool BytecodeMatches(const ActorBlock& a, const ActorBlock& b)
{
	if (a.events.size() != b.events.size()) return false;
	for (size_t i = 0; i < a.events.size(); ++i)
	{
		const auto& ea = a.events[i];
		const auto& eb = b.events[i];
		if (ea.bytecode.size() != eb.bytecode.size()) return false;
		if (memcmp(ea.bytecode.data(), eb.bytecode.data(), ea.bytecode.size()) != 0) return false;
	}
	return true;
}

// --- Text comparison for common dedup ---
struct EventTexts
{
	uint32_t event_id;
	uint16_t array_index;
	std::vector<std::u8string> texts; // resolved evsb strings
};

static std::vector<EventTexts> ExtractEventTexts(const ActorBlock& block,
	const std::unordered_map<uint32_t, EntityEntry>& entity_map,
	const std::vector<std::u8string>& zone_strings)
{
	BytecodeAnalyzer analyzer;
	std::vector<EventTexts> result;
	for (const auto& evt : block.events)
	{
		std::vector<DialogueLine> dls;
		analyzer.ExtractDialogues(evt.bytecode, block.actor_number,
			block.imed_data, entity_map, zone_strings, dls, evt.byte_offset);
		EventTexts et;
		et.event_id = evt.event_id;
		et.array_index = evt.array_index;
		for (const auto& dl : dls)
			et.texts.push_back(dl.text);
		result.push_back(std::move(et));
	}
	return result;
}

static bool LinesMatch(const std::vector<std::u8string>& a, const std::vector<std::u8string>& b)
{
	if (a.size() != b.size()) return false;
	for (size_t i = 0; i < a.size(); ++i)
	{
		// If either side is an unresolved reference (e.g. "7751*"), treat as match
		auto isUnresolved = [](const std::u8string& s) {
			return !s.empty() && s.back() == '*'
				&& s.find_first_not_of(u8"0123456789*") == std::string::npos;
		};
		if (isUnresolved(a[i]) || isUnresolved(b[i]))
			continue;
		if (a[i] != b[i]) return false;
	}
	return true;
}

static bool TextMatches(const std::vector<EventTexts>& a, const std::vector<EventTexts>& b)
{
	std::unordered_map<uint16_t, std::vector<std::u8string>> m_a, m_b;
	for (const auto& e : a) m_a[e.event_id] = e.texts;
	for (const auto& e : b) m_b[e.event_id] = e.texts;

	if (m_a.empty() || m_b.empty()) return false;

	int matched = 0;
	for (const auto& [eid, textsA] : m_a)
	{
		auto it = m_b.find(eid);
		if (it == m_b.end()) continue;
		if (!LinesMatch(textsA, it->second)) return false;
		matched++;
	}

	return matched > 0;
}

// --- Compute a stable hash for a text set ---
size_t EventTextOut::TextSetHash(const std::vector<std::u8string>& texts)
{
	size_t h = 0;
	for (const auto& t : texts)
		h ^= std::hash<std::u8string>{}(t) + 0x9e3779b9 + (h << 6) + (h >> 2);
	return h;
}

void EventTextOut::WriteRefCsv()
{
	if (refEntries_.empty()) return;
	std::ofstream csv(refCsvPath_);
	if (!csv.is_open()) return;
	for (const auto& [canon, ref] : refEntries_)
		csv << Logger::ToUtf8(canon) << "," << Logger::ToUtf8(ref) << "\n";
}

// --- Write a TXT file with global dedup ---
// If the same text set was already written elsewhere, write a reference instead.
void EventTextOut::WriteTxtFile(const std::filesystem::path& path,
	const std::vector<std::u8string>& texts,
	std::unordered_map<size_t, std::u8string>& textCache)
{
	if (texts.empty()) return;

	// Check if identical text was already written
	size_t h = TextSetHash(texts);
	auto it = textCache.find(h);
	if (it != textCache.end())
	{
		// Skip self-reference (same file content already written)
		auto canon = it->second;
		auto pStr = path.lexically_normal().u8string();
		// Extract relative key for comparison if stored as relative
		auto pos = pStr.find(u8"event");
		if (pos != std::u8string::npos)
			pStr = pStr.substr(pos);
		// Normalize separators for comparison
		for (auto& c : canon) if (c == '\\') c = '/';
		for (auto& c : pStr) if (c == '\\') c = '/';
		if (canon == pStr)
			return; // already written, skip

		if (useRefFiles_)
		{
			// Create directory only when actually writing reference file, to avoid creating empty dirs for skipped files
			std::filesystem::create_directories(path.parent_path());
			std::ofstream file(path);
			if (file.is_open())
				file << "@ref " << Logger::ToUtf8(canon) << "\n";
		}
		else
		{
			refEntries_.emplace_back(canon, pStr);
		}
		return;
	}

	// Create directory only when actually writing reference file, to avoid creating empty dirs for skipped files
	std::filesystem::create_directories(path.parent_path());

	// First occurrence: write normally and cache
	std::ofstream file(path);
	if (!file.is_open())
	{
		Logger::Instance().Error("Cannot write: " + Logger::ToUtf8(path));
		return;
	}
	for (const auto& t : texts)
		file << Logger::ToUtf8(t) << "\n";
	// Store path relative to event output base for portable references
	// Normalize to forward slashes
	auto pStr = path.lexically_normal().u8string();
	// Find "event/" boundary (after output root)
	auto pos = pStr.find(u8"event\\");
	if (pos == std::u8string::npos) pos = pStr.find(u8"event/");
	if (pos != std::u8string::npos)
	{
		auto rel = pStr.substr(pos);
		for (auto& c : rel) if (c == '\\') c = '/';
		textCache[h] = rel;
	}
	else
		textCache[h] = path.filename().u8string();
}

// Overload without dedup (for backward compat)
void EventTextOut::WriteTxtFileSimple(const std::filesystem::path& path, const std::vector<std::u8string>& texts)
{
	std::unordered_map<size_t, std::u8string> dummy;
	WriteTxtFile(path, texts, dummy);
}

// --- Build event key for filename: {aidx}.txt ---
static std::string EventFileName(uint16_t event_id, uint16_t array_index)
{
	(void)event_id;
	return std::to_string(array_index) + ".txt";
}

// --- Replace duplicate texts with same content: if two events of same
//      event_id have identical texts, only write one file ---
struct EventFileKey
{
	uint16_t event_id;
	uint16_t array_index;
	std::vector<std::string> texts;

	bool IsDuplicateOf(const EventFileKey& other) const
	{
		return event_id == other.event_id && texts == other.texts;
	}
};

// --- Resolve bare actor name (without entity_id suffix) ---
// Unnamed actors always get bare name "_". Entity_id is appended later
// when disambiguation is needed to avoid conflicts.
std::string EventTextOut::ActorBareName(const ActorBlock& block,
	const std::unordered_map<uint32_t, EntityEntry>& entity_map)
{
	auto it = entity_map.find(block.actor_number);
	if (it != entity_map.end() && !it->second.name.empty())
		return it->second.name;
	if (block.actor_number == 0x7FFFFFF0)
		return "Zone Events";
	if (block.actor_number == 0x7FFFFFFF)
		return "Zone/Player Events";
	return "_";
}

// --- Public interface ---

EventTextOutResult EventTextOut::RunAllZones()
{
	// Legacy: load from zone_events.csv
	auto defs = ZoneConfig::Load("../FFXIDatAdv/data/zone_events.csv");
	auto config = ZoneConfig::GroupByZone(defs);

	// Convert to ZoneFiles format and process
	std::unordered_map<std::string, ZoneFiles> zones;
	for (const auto& [name, def] : config)
	{
		ZoneFiles zf;
		zf.zone_name = name;
		zf.evev_path = def.evev_path;
		zf.evac_path = def.evac_path;
		zf.evsb_path = def.evsb_jp_path.empty() ? def.evsb_path : def.evsb_jp_path;
		zf.evsb_en_path = def.evsb_path.empty() ? def.evsb_jp_path : def.evsb_path;
		zones[name] = std::move(zf);
	}

	return RunAllZones(zones);
}

// --- Process zones from ZoneRegistry data ---
EventTextOutResult EventTextOut::RunAllZones(const std::unordered_map<std::string, ZoneFiles>& zones)
{
	EventTextOutResult total;

	if (zones.empty())
	{
		Logger::Instance().Error("No zone configuration found.");
		return total;
	}

	// === Phase 1: Scan all zones ===
	std::cout << "=== Phase 1: Scanning zones ===" << std::endl;
	std::vector<ZoneScan> scans;
	uint32_t processed = 0;

	for (const auto& [zoneName, zf] : zones)
	{
		++processed;
		if (zf.evev_path.empty())
		{
			Logger::Instance().Warning("Skip " + zoneName + ": no evev path");
			continue;
		}

		std::cout << "[" << processed << "/" << zones.size() << "] Scanning: " << zoneName << std::endl;

		std::string evevPath = GamePathResolver::ResolvePath(zf.evev_path);
		std::string evacPath = zf.evac_path.empty() ? "" : GamePathResolver::ResolvePath(zf.evac_path);
		std::string evsbPath = GamePathResolver::ResolvePath(EventTextOut::GetEvsbPathForLang(zf, evsbLang_));

		auto actors = EventBinaryDat::Parse(evevPath);
		if (actors.empty())
		{
			continue;
		}

		std::unordered_map<uint32_t, EntityEntry> entityMap;
		if (!evacPath.empty())
		{
			auto entities = EntityDat::Parse(evacPath);
			for (auto& e : entities)
				entityMap[e.entity_id] = std::move(e);
		}

		std::vector<std::u8string> zoneStrings;
		if (!evsbPath.empty())
			zoneStrings = LoadEvsbStrings(evsbPath);
		if (zoneStrings.empty()) {
			std::cout << "Empty zone string!!! evsb=" << evsbPath << std::endl;
		}

		ZoneScan scan;
		scan.zone_name = zoneName;
		scan.actors = std::move(actors);
		scan.entity_map = std::move(entityMap);
		scan.zone_strings = std::move(zoneStrings);
		scans.push_back(std::move(scan));
	}

	// === Phase 2: Verify common actors (text-only based) ===
	std::cout << "\n=== Phase 2: Verifying common actors ===" << std::endl;
	auto groups = GroupCandidates(scans);

	struct VerifyResult
	{
		std::string actor_name;
		bool is_text_common;
		std::vector<std::string> zone_names;
	};

	std::vector<VerifyResult> verifyResults;

	for (const auto& [name, candidates] : groups)
	{
		VerifyResult vr;
		vr.actor_name = name;
		vr.is_text_common = false;

		std::unordered_set<std::string> unique_zones;
		for (const auto& c : candidates)
			unique_zones.insert(c.scan->zone_name);
		for (const auto& c : candidates)
			vr.zone_names.push_back(c.scan->zone_name);

		if (unique_zones.size() < 2)
		{
			verifyResults.push_back(vr);
			continue;
		}

		// Compare resolved text across all zones (regardless of bytecode)
		std::vector<std::vector<EventTexts>> all_texts;
		for (const auto& c : candidates)
		{
			auto texts = ExtractEventTexts(*c.block, c.scan->entity_map, c.scan->zone_strings);
			all_texts.push_back(std::move(texts));
		}

		bool all_text_match = true;
		for (size_t i = 1; i < all_texts.size(); ++i)
		{
			if (!TextMatches(all_texts[0], all_texts[i]))
			{
				all_text_match = false;
				break;
			}
		}

		if (all_text_match)
		{
			vr.is_text_common = true;
			std::cout << "[VERIFY] OK: Actor \"" << name << "\" ("
				<< unique_zones.size() << " zones) text match." << std::endl;
		}

		verifyResults.push_back(vr);
	}

	std::unordered_map<std::string, const VerifyResult*> resultLookup;
	for (const auto& vr : verifyResults)
		resultLookup[vr.actor_name] = &vr;

	// --- Build actor folder disambiguation map ---
	// For each zone, collect {actor_name → [(entity_id, {event_ids}), ...]}.
	// If a name maps to multiple entity_ids with overlapping event_ids,
	// disambiguate by appending "_{entity_id}". Otherwise, omit the suffix.
	struct ActorGroup {
		uint32_t entity_id;
		std::set<uint16_t> event_ids;
		std::string safe_name; // bare folder-safe name
	};
	std::unordered_map<std::string,  // zone name
		std::unordered_map<std::string,  // bare actor name
			std::vector<ActorGroup>>> zoneActorMap;
	for (const auto& scan : scans)
	{
		auto& zm = zoneActorMap[scan.zone_name];
		for (const auto& block : scan.actors)
		{
			auto bareName = ActorBareName(block, scan.entity_map);
			auto& grp = zm[bareName];
			bool found = false;
			for (auto& g : grp)
			{
				if (g.entity_id == block.actor_number) { found = true; break; }
			}
			if (!found)
			{
				ActorGroup ag;
				ag.entity_id = block.actor_number;
				ag.safe_name = SafeFilename(bareName);
				grp.push_back(std::move(ag));
			}
		}
		// Fill event_ids for each entity
		for (const auto& block : scan.actors)
		{
			auto bareName = ActorBareName(block, scan.entity_map);
			auto& grp = zm[bareName];
			for (auto& g : grp)
			{
				if (g.entity_id == block.actor_number)
				{
					for (const auto& evt : block.events)
						g.event_ids.insert(evt.event_id);
					break;
				}
			}
		}
	}

	// Build a per-zone lookup: (zone_name, entity_id) → actor_folder_name
	// Also build a lookup for which bareNames need disambiguation for common actors
	auto needsDisamb = [](const std::vector<ActorGroup>& grp) {
		if (grp.size() <= 1) return false;
		// Check if event_ids across all groups overlap
		std::set<uint16_t> allEvents;
		for (const auto& g : grp)
		{
			for (auto eid : g.event_ids)
				if (!allEvents.insert(eid).second) return true; // overlap
		}
		return false;
	};

	// === Phase 3: Write TXT files ===
	std::cout << "\n=== Phase 3: Writing TXT ===" << std::endl;

	// Global text dedup: text_hash → canonical file path
	std::unordered_map<size_t, std::u8string> globalTextCache;

	for (const auto& scan : scans)
	{
		EventTextOutResult zoneResult;
		zoneResult.zone_name = scan.zone_name;

		for (const auto& block : scan.actors)
		{
			auto bareName = ActorBareName(block, scan.entity_map);
			auto safeBare = SafeFilename(bareName);

			// Decide folder name:
			// - if disambiguation needed, append _{entity_id}
			// - otherwise, just the bare name
			const auto& grp = zoneActorMap[scan.zone_name][bareName];
			std::string actorFolder = safeBare;
			if (needsDisamb(grp))
				actorFolder = safeBare + "_" + std::to_string(block.actor_number);

			auto rit = resultLookup.find(bareName);
			bool isCommon = (rit != resultLookup.end() && rit->second->is_text_common);

			struct EventFile
			{
				uint16_t event_id;
				uint16_t array_index;
				std::vector<std::u8string> texts;
			};

			std::vector<EventFile> events;
			for (const auto& evt : block.events)
			{
				BytecodeAnalyzer analyzer;
				std::vector<DialogueLine> dls;
				analyzer.ExtractDialogues(evt.bytecode, block.actor_number,
					block.imed_data, scan.entity_map, scan.zone_strings,
					dls, evt.byte_offset);

				if (dls.empty())
				{
					zoneResult.skipped_no_text++;
					continue;
				}

				EventFile ef;
				ef.event_id = evt.event_id;
				ef.array_index = evt.array_index;
				for (const auto& dl : dls)
					ef.texts.push_back(dl.text);
				events.push_back(std::move(ef));
			}

			if (events.empty()) continue;

			// Determine output directory:
			// Common: /common/{bareActorName}/
			// Zone-specific: /zone/{zoneName}/{actorFolder}/
			auto actorDir = isCommon
				? (CommonDir() / safeBare)
				: (ZoneDir(scan.zone_name) / actorFolder);

			// Group events by event_id to decide filename disambiguation
			std::unordered_map<uint16_t, std::vector<EventFile>> byEventId;
			for (auto& ef : events)
				byEventId[ef.event_id].push_back(std::move(ef));

			for (auto& [eid, evts] : byEventId)
			{
				int counter = 0;
				for (auto& evtFile : evts)
				{
				bool multi = byEventId[eid].size() > 1;
				std::string fname = std::to_string(evtFile.array_index) + ".txt";
					auto outPath = actorDir / fname;

					WriteTxtFile(outPath, evtFile.texts, globalTextCache);
					zoneResult.total_events++;
					zoneResult.total_lines += static_cast<int>(evtFile.texts.size());
					counter++;
				}
			}

			zoneResult.total_actors++;
		}

		std::cout << "  " << scan.zone_name << ": "
			<< zoneResult.total_actors << " actors, "
			<< zoneResult.total_events << " events, "
			<< zoneResult.total_lines << " lines"
			<< (zoneResult.skipped_no_text ? std::format(" ({} no-text)", zoneResult.skipped_no_text) : "")
			<< std::endl;

		total.total_actors += zoneResult.total_actors;
		total.total_events += zoneResult.total_events;
		total.total_lines += zoneResult.total_lines;
		total.skipped_no_text += zoneResult.skipped_no_text;
	}

	WriteRefCsv();
	if (!refEntries_.empty())
		std::cout << "  ref.csv written with " << refEntries_.size() << " reference mappings" << std::endl;

	std::cout << "\n[DONE] TXT written to: " << outputDir_ / "event" << std::endl;
	return total;
}

EventTextOutResult EventTextOut::RunZone(const std::string& zoneName)
{
	// Legacy: load from zone_events.csv
	auto defs = ZoneConfig::Load("../FFXIDatAdv/data/zone_events.csv");
	auto config = ZoneConfig::GroupByZone(defs);

	// Find the zone
	bool found = false;
	for (const auto& [name, def] : config)
	{
		if (name == zoneName)
		{
			found = true;
			std::unordered_map<std::string, ZoneDef> single;
			single[name] = def;
			// For a single zone, common dedup is irrelevant
			// Directly extract and write per-zone
			auto actors = EventBinaryDat::Parse(GamePathResolver::ResolvePath(def.evev_path));
			if (actors.empty())
			{
				Logger::Instance().Error("No actors found for " + zoneName);
				return {};
			}

			std::unordered_map<uint32_t, EntityEntry> entityMap;
			if (!def.evac_path.empty())
			{
				auto entities = EntityDat::Parse(GamePathResolver::ResolvePath(def.evac_path));
				for (auto& e : entities)
					entityMap[e.entity_id] = std::move(e);
			}

			std::vector<std::u8string> zoneStrings;
			auto evsbPath = def.GetEvsbPath(evsbLang_ == "ja");
			if (!evsbPath.empty())
			{
				auto resolved = GamePathResolver::ResolvePath(evsbPath);
				if (!resolved.empty())
					zoneStrings = LoadEvsbStrings(resolved);
			}

			EventTextOutResult result;
			result.zone_name = zoneName;

			// Build disambiguation map for this zone
			std::unordered_map<std::string, std::vector<std::pair<uint32_t, std::set<uint16_t>>>> nameMap;
			for (const auto& block : actors)
			{
				auto bare = ActorBareName(block, entityMap);
				bool found = false;
				for (auto& [eid, _] : nameMap[bare])
					if (eid == block.actor_number) { found = true; break; }
				if (!found)
				{
					nameMap[bare].push_back({block.actor_number, {}});
				}
			}
			for (const auto& block : actors)
			{
				auto bare = ActorBareName(block, entityMap);
				for (auto& [eid, evset] : nameMap[bare])
					if (eid == block.actor_number)
						for (const auto& evt : block.events)
							evset.insert(evt.event_id);
			}
			auto needsDisambLegacy = [](const std::vector<std::pair<uint32_t, std::set<uint16_t>>>& grp) {
				if (grp.size() <= 1) return false;
				std::set<uint16_t> all;
				for (const auto& [_, es] : grp)
					for (auto e : es)
						if (!all.insert(e).second) return true;
				return false;
			};
			// Build lookup: entity_id → folder name
			std::unordered_map<uint32_t, std::string> folderLookup;
			for (const auto& [bare, grp] : nameMap)
			{
				auto safeBare = SafeFilename(bare);
				bool disamb = needsDisambLegacy(grp);
				for (const auto& [eid, _] : grp)
					folderLookup[eid] = disamb ? (safeBare + "_" + std::to_string(eid)) : safeBare;
			}

			BytecodeAnalyzer analyzer;
			std::unordered_map<size_t, std::u8string> localTextCache;

			for (const auto& block : actors)
			{
				auto actorDir = ZoneDir(zoneName) / folderLookup[block.actor_number];

				// Pre-pass: detect duplicate event_ids within this actor
				std::unordered_map<uint16_t, int> eventIdCount;
				for (const auto& evt : block.events)
					eventIdCount[evt.event_id]++;

				for (const auto& evt : block.events)
				{
					std::vector<DialogueLine> dls;
					analyzer.ExtractDialogues(evt.bytecode, block.actor_number,
						block.imed_data, entityMap, zoneStrings, dls, evt.byte_offset);

					if (dls.empty())
					{
						result.skipped_no_text++;
						continue;
					}

					std::vector<std::u8string> texts;
					for (const auto& dl : dls)
						texts.push_back(dl.text);

					bool multi = eventIdCount[evt.event_id] > 1;
					std::string fname = std::to_string(evt.event_id)
						+ (multi ? "." + std::to_string(evt.array_index) : "") + ".txt";
					auto outPath = actorDir / fname;
					WriteTxtFile(outPath, texts, localTextCache);
					result.total_events++;
					result.total_lines += static_cast<int>(texts.size());
				}
				result.total_actors++;
			}

			std::cout << "[DONE] " << zoneName << ": "
				<< result.total_actors << " actors, "
				<< result.total_events << " events, "
				<< result.total_lines << " lines" << std::endl;
			return result;
		}
	}

	if (!found)
		Logger::Instance().Error("Zone not found: " + zoneName);
	return {};
}

EventTextOutResult EventTextOut::RunZone(const std::string& zoneName, const ZoneFiles& zone)
{
	EventTextOutResult result;
	result.zone_name = zoneName;

	std::string evevPath = GamePathResolver::ResolvePath(zone.evev_path);
	std::string evacPath = zone.evac_path.empty() ? "" : GamePathResolver::ResolvePath(zone.evac_path);
	std::string evsbPath = GamePathResolver::ResolvePath(EventTextOut::GetEvsbPathForLang(zone, evsbLang_));

	auto actors = EventBinaryDat::Parse(evevPath);
	if (actors.empty())
	{
		Logger::Instance().Error("No actors found for " + zoneName);
		return {};
	}

	std::unordered_map<uint32_t, EntityEntry> entityMap;
	if (!evacPath.empty())
	{
		auto entities = EntityDat::Parse(evacPath);
		for (auto& e : entities)
			entityMap[e.entity_id] = std::move(e);
	}

	std::vector<std::u8string> zoneStrings;
	if (!evsbPath.empty())
		zoneStrings = LoadEvsbStrings(evsbPath);

	// Build disambiguation map for this zone
	std::unordered_map<std::string, std::vector<std::pair<uint32_t, std::set<uint16_t>>>> nameMap;
	for (const auto& block : actors)
	{
		auto bare = ActorBareName(block, entityMap);
		bool found = false;
		for (auto& [eid, _] : nameMap[bare])
			if (eid == block.actor_number) { found = true; break; }
		if (!found)
			nameMap[bare].push_back({block.actor_number, {}});
	}
	for (const auto& block : actors)
	{
		auto bare = ActorBareName(block, entityMap);
		for (auto& [eid, evset] : nameMap[bare])
			if (eid == block.actor_number)
				for (const auto& evt : block.events)
					evset.insert(evt.event_id);
	}
	auto needsDisamb = [](const std::vector<std::pair<uint32_t, std::set<uint16_t>>>& grp) {
		if (grp.size() <= 1) return false;
		std::set<uint16_t> all;
		for (const auto& [_, es] : grp)
			for (auto e : es)
				if (!all.insert(e).second) return true;
		return false;
	};
	std::unordered_map<uint32_t, std::string> folderLookup;
	for (const auto& [bare, grp] : nameMap)
	{
		auto safeBare = SafeFilename(bare);
		bool disamb = needsDisamb(grp);
		for (const auto& [eid, _] : grp)
			folderLookup[eid] = disamb ? (safeBare + "_" + std::to_string(eid)) : safeBare;
	}

	BytecodeAnalyzer analyzer;
	std::unordered_map<size_t, std::u8string> localTextCache;

	for (const auto& block : actors)
	{
		auto actorDir = ZoneDir(zoneName) / folderLookup[block.actor_number];

		for (const auto& evt : block.events)
		{
			std::vector<DialogueLine> dls;
			analyzer.ExtractDialogues(evt.bytecode, block.actor_number,
				block.imed_data, entityMap, zoneStrings, dls, evt.byte_offset);

			if (dls.empty())
			{
				result.skipped_no_text++;
				continue;
			}

			std::vector<std::u8string> texts;
			for (const auto& dl : dls)
				texts.push_back(dl.text);

			auto outPath = actorDir / EventFileName(evt.event_id, evt.array_index);
			WriteTxtFile(outPath, texts, localTextCache);
			result.total_events++;
			result.total_lines += static_cast<int>(texts.size());
		}
		result.total_actors++;
	}

	std::cout << "[DONE] " << zoneName << ": "
		<< result.total_actors << " actors, "
		<< result.total_events << " events, "
		<< result.total_lines << " lines" << std::endl;
	return result;
}
