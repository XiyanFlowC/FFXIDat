#include "EventTextIn.h"
#include "Logger.h"
#include <iostream>
#include <fstream>
#include <format>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include "../FFXIDatAdv/EventDump/ZoneConfig.h"
#include "../FFXIDatAdv/EventDump/EventBinaryDat.h"
#include "../FFXIDatAdv/EventDump/EntityDat.h"
#include "../FFXIDatAdv/EventDump/BytecodeAnalyzer.h"
#include "../FFXIDatAdv/EventDump/GamePathResolver.h"
#include "../FFXIDat/EventStringBase.h"
#include <xystring.h>

EventTextIn::EventTextIn(const std::filesystem::path& textsDir)
	: textsDir_(textsDir)
{
}

// --- Read all lines from a TXT file, resolving @ref references ---
static std::vector<std::u8string> ReadTxtLines(const std::filesystem::path& path)
{
	std::vector<std::u8string> lines;
	std::ifstream file(path);
	if (!file.is_open()) return lines;

	// Check if first line is an @ref reference
	std::string firstLine;
	if (std::getline(file, firstLine))
	{
		if (firstLine.size() > 5 && firstLine.substr(0, 5) == "@ref ")
		{
			// Resolve reference: read the canonical file instead
			auto canonPath = path.parent_path() / firstLine.substr(5);
			file.close();
			return ReadTxtLines(canonPath);
		}
		lines.push_back({firstLine.begin(), firstLine.end()});
	}

	std::string line;
	while (std::getline(file, line))
		lines.push_back({line.begin(), line.end()});
	return lines;
}

// --- Safe filename for path construction ---
static std::string SafeName(const std::string& s)
{
	std::string r;
	r.reserve(s.size());
	for (char c : s)
	{
		if (static_cast<unsigned char>(c) < 0x20 ||
			c == '\\' || c == '/' || c == ':' || c == '*' ||
			c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
			r += '_';
		else
			r += c;
	}
	return r;
}

// --- Build event filename: {aidx}.txt ---
static std::string EventFile(uint16_t event_id, uint16_t array_index)
{
	(void)event_id;
	return std::to_string(array_index) + ".txt";
}

// --- Re-parse bytecode to get (message_id, resolved_text) ---
struct TextRef
{
	uint32_t message_id;
	std::u8string text;
};

static std::vector<TextRef> GetTextRefs(const EventEntry& evt,
	uint32_t actor_number,
	const std::vector<uint32_t>& imed_data,
	const std::unordered_map<uint32_t, EntityEntry>& entity_map,
	const std::vector<std::u8string>& zone_strings)
{
	BytecodeAnalyzer analyzer;
	std::vector<DialogueLine> dls;
	analyzer.ExtractDialogues(evt.bytecode, actor_number,
		imed_data, entity_map, zone_strings, dls, evt.byte_offset);

	std::vector<TextRef> refs;
	for (const auto& dl : dls)
		refs.push_back({dl.message_id, dl.text});
	return refs;
}

// --- Match TXT lines to evsb refs and verify ---
struct PatchInfo
{
	bool verified = false;
	std::vector<uint32_t> ids;      // message_ids to patch
	std::vector<std::u8string> texts; // translated texts
	int errors = 0;
};

static PatchInfo MatchAndVerify(
	const std::vector<std::u8string>& srcTxt,    // original text (from src/)
	const std::vector<std::u8string>& dstTxt,    // translation (from dst/)
	const std::vector<TextRef>& refs)          // current evsb content
{
	PatchInfo pi;

	if (refs.empty()) { pi.errors = 1; return pi; }

	// Extract current evsb texts
	std::vector<std::u8string> current;
	for (const auto& r : refs) current.push_back(r.text);

	// Verify: current evsb == TXT src
	if (current.size() == srcTxt.size())
	{
		bool ok = true;
		for (size_t i = 0; i < current.size(); ++i)
			if (current[i] != srcTxt[i]) { ok = false; break; }

		if (ok)
		{
			pi.verified = true;
			for (size_t i = 0; i < refs.size(); ++i)
			{
				pi.ids.push_back(refs[i].message_id);
				pi.texts.push_back(i < dstTxt.size() && !dstTxt[i].empty() ? dstTxt[i] : refs[i].text);
			}
			return pi;
		}
	}

	// Fallback: per-line match
	for (size_t i = 0; i < srcTxt.size() && i < dstTxt.size(); ++i)
	{
		bool found = false;
		for (const auto& ref : refs)
		{
			if (ref.text == srcTxt[i])
			{
				pi.ids.push_back(ref.message_id);
				pi.texts.push_back(dstTxt[i]);
				found = true;
				break;
			}
		}
		if (!found)
		{
			Logger::Instance().Warning("No match for: " + Logger::ToUtf8(srcTxt[i]));
			pi.errors++;
		}
	}
	return pi;
}

// --- Apply patches to evsb ---
static bool ApplyPatches(const std::string& evsbPath,
	const std::vector<uint32_t>& ids,
	const std::vector<std::u8string>& texts)
{
	if (ids.empty() || ids.size() != texts.size()) return false;
	try
	{
		EventStringBase esb(xybase::string::sys_mbs_to_wcs(evsbPath));
		esb.Read();
		for (size_t i = 0; i < ids.size(); ++i)
		{
			if (ids[i] < esb.Size())
			{
				esb[ids[i]] = texts[i];
			}
		}
		esb.Write();
		return true;
	}
	catch (const std::exception& e)
	{
		Logger::Instance().Error(std::string("Patch failed: ") + e.what());
		return false;
	}
}

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

// --- Process one zone ---
EventTextInResult EventTextIn::RunZone(const std::string& zoneName)
{
	EventTextInResult result;
	result.zone_name = zoneName;

	auto defs = ZoneConfig::Load("../FFXIDatAdv/data/zone_events.csv");
	auto config = ZoneConfig::GroupByZone(defs);
	auto it = config.find(zoneName);
	if (it == config.end())
	{
		Logger::Instance().Error("Zone not found: " + zoneName);
		result.errors++;
		return result;
	}

	const auto& def = it->second;
	std::string evevPath = GamePathResolver::ResolvePath(def.evev_path);
	std::string evacPath = def.evac_path.empty() ? "" : GamePathResolver::ResolvePath(def.evac_path);
	std::string evsbPath = def.GetEvsbPath(false).empty() ? "" : GamePathResolver::ResolvePath(def.GetEvsbPath(false));

	auto actors = EventBinaryDat::Parse(evevPath);
	if (actors.empty())
	{
		Logger::Instance().Error("No actors in " + zoneName);
		result.errors++;
		return result;
	}

	std::unordered_map<uint32_t, EntityEntry> entityMap;
	if (!evacPath.empty())
	{
		for (auto& e : EntityDat::Parse(evacPath))
			entityMap[e.entity_id] = std::move(e);
	}

	std::vector<std::u8string> zoneStrings;
	if (!evsbPath.empty())
		zoneStrings = LoadEvsbStrings(evsbPath);

	// Derive src/dst directories
	auto zoneDir = textsDir_ / "src" / "event" / "zone" / zoneName;
	auto dstBase = textsDir_ / "dst" / "event" / "zone" / zoneName;
	auto commonDir = textsDir_ / "src" / "event" / "common";
	auto commonDst = textsDir_ / "dst" / "event" / "common";

	for (const auto& block : actors)
	{
		// Resolve actor name
		std::string actorName;
		auto en = entityMap.find(block.actor_number);
		if (en != entityMap.end() && !en->second.name.empty())
			actorName = en->second.name;
		else if (block.actor_number == 0x7FFFFFF0)
			actorName = "Zone Events";
		else if (block.actor_number == 0x7FFFFFFF)
			actorName = "Zone/Player Events";
		else
			actorName = "_" + std::to_string(block.actor_number);

		for (const auto& evt : block.events)
		{
			auto fileName = EventFile(evt.event_id, evt.array_index);

			// Try zone-specific src first, then common src
			std::filesystem::path srcPath = zoneDir / SafeName(actorName) / fileName;
			if (!std::filesystem::exists(srcPath))
				srcPath = commonDir / SafeName(actorName) / fileName;
			if (!std::filesystem::exists(srcPath))
				continue; // no src file, nothing to patch

			result.total_events++;

			// Read source (original) text
			auto srcLines = ReadTxtLines(srcPath);

			// Try zone-specific dst first, then common dst
			std::filesystem::path dstPath = dstBase / SafeName(actorName) / fileName;
			if (!std::filesystem::exists(dstPath))
				dstPath = commonDst / SafeName(actorName) / fileName;

			std::vector<std::u8string> dstLines;
			if (std::filesystem::exists(dstPath))
				dstLines = ReadTxtLines(dstPath);
			else
				dstLines = srcLines; // no translation, keep original

			// Re-parse bytecode → get current evsb refs
			auto refs = GetTextRefs(evt, block.actor_number,
				block.imed_data, entityMap, zoneStrings);

			// Match and verify
			auto patch = MatchAndVerify(srcLines, dstLines, refs);
			result.errors += patch.errors;
			if (patch.verified) result.verified_ok++;

			// Apply
			if (!patch.ids.empty())
			{
				if (ApplyPatches(evsbPath, patch.ids, patch.texts))
					result.replaced++;
				else
					result.errors++;
			}
		}
	}

	std::cout << std::format("[DONE] {}: {} events, {} replaced, {} verified, {} errors",
		zoneName, result.total_events, result.replaced,
		result.verified_ok, result.errors) << std::endl;
	return result;
}

EventTextInResult EventTextIn::RunAllZones()
{
	EventTextInResult total;
	auto defs = ZoneConfig::Load("../FFXIDatAdv/data/zone_events.csv");
	auto config = ZoneConfig::GroupByZone(defs);

	for (const auto& [name, def] : config)
	{
		auto r = RunZone(name);
		total.total_events += r.total_events;
		total.replaced += r.replaced;
		total.verified_ok += r.verified_ok;
		total.errors += r.errors;
	}

	std::cout << "\n[DONE] All zones: " << total.total_events << " events, "
		<< total.replaced << " replaced, " << total.verified_ok << " verified, "
		<< total.errors << " errors" << std::endl;
	return total;
}
