#include "EventFileProcessor.h"
#include "EventTextOut.h"
#include "EventTextIn.h"
#include "Logger.h"
#include "FinalTextProcessor.h"
#include "TranslationDatabase.h"
#include "Config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <set>
#include "EventDump/ZoneConfig.h"
#include "EventDump/EventBinaryDat.h"
#include "EventDump/EntityDat.h"
#include "EventDump/BytecodeAnalyzer.h"
#include "EventDump/GamePathResolver.h"
#include <EventStringBase.h>
#include <xystring.h>

// --- ZoneRegistry ---

ZoneRegistry& ZoneRegistry::Instance()
{
	static ZoneRegistry instance;
	return instance;
}

std::string ZoneRegistry::CanonicalPath(const std::string& path)
{
	std::string r = path;
	for (auto& c : r)
		if (c == '\\') c = '/';
	if (r.size() > 4 && r.substr(r.size() - 4) == ".DAT")
		r = r.substr(0, r.size() - 4);
	while (!r.empty() && (r[0] == ' ' || r[0] == '\t')) r = r.substr(1);
	while (!r.empty() && (r.back() == ' ' || r.back() == '\t')) r.pop_back();
	return r;
}

void ZoneRegistry::LoadFromDefsCsv(const std::filesystem::path& csvPath)
{
	zones_.clear();

	std::ifstream file(csvPath);
	if (!file.is_open())
	{
		Logger::Instance().Error("Cannot open defs.csv: " + Logger::ToUtf8(csvPath));
		return;
	}

	std::string line;
	while (std::getline(file, line))
	{
		while (!line.empty() && (line[0] == ' ' || line[0] == '\t')) line = line.substr(1);
		while (!line.empty() && (line.back() == ' ' || line.back() == '\t')) line.pop_back();
		if (line.empty() || line[0] == '#') continue;

		std::vector<std::string> cols;
		std::stringstream ss(line);
		std::string col;
		while (std::getline(ss, col, ','))
			cols.push_back(col);

		if (cols.size() < 3) continue;

		std::string path = CanonicalPath(cols[0]);
		std::string type = cols[1];
		std::string lang = cols.size() > 2 ? LangCode::Normalize(cols[2]) : "";
		std::string comment = cols.size() > 3 ? cols[3] : "";

		if (path.empty() || comment.empty()) continue;

		if (type != "evev" && type != "evac" && type != "evsb")
			continue;

		std::string zoneName = comment;
		if (zoneName.empty()) continue;

		auto& zf = zones_[zoneName];
		zf.zone_name = zoneName;

		if (type == "evev")
			zf.evev_path = path;
		else if (type == "evac")
			zf.evac_path = path;
		else if (type == "evsb" && (lang == LangCode::JA || lang.empty()))
			zf.evsb_path = path;
		else if (type == "evsb" && lang == LangCode::EN)
			zf.evsb_en_path = path;
	}

	for (auto it = zones_.begin(); it != zones_.end();)
	{
		if (it->second.evev_path.empty())
			it = zones_.erase(it);
		else
			++it;
	}
}

void ZoneRegistry::LoadFromZoneEventsCsv(const std::filesystem::path& csvPath)
{
	zones_.clear();

	auto defs = ZoneConfig::Load(csvPath.string());
	auto config = ZoneConfig::GroupByZone(defs);

	for (const auto& [name, def] : config)
	{
		ZoneFiles zf;
		zf.zone_name = name;
		zf.evev_path = CanonicalPath(def.evev_path);
		zf.evac_path = CanonicalPath(def.evac_path);
		zf.evsb_path = CanonicalPath(!def.evsb_jp_path.empty() ? def.evsb_jp_path : def.evsb_path);
		zf.evsb_en_path = CanonicalPath(!def.evsb_path.empty() ? def.evsb_path : def.evsb_jp_path);
		zones_[name] = std::move(zf);
	}
}

const ZoneFiles* ZoneRegistry::FindByZoneName(const std::string& zoneName) const
{
	auto it = zones_.find(zoneName);
	return it != zones_.end() ? &it->second : nullptr;
}

const ZoneFiles* ZoneRegistry::FindByEvevPath(const std::string& evevPath) const
{
	std::string canon = CanonicalPath(evevPath);
	for (const auto& [name, zf] : zones_)
	{
		if (CanonicalPath(zf.evev_path) == canon)
			return &zf;
	}
	return nullptr;
}

// --- Helpers ---

static std::string SafePathSegment(const std::string& s)
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

// Format: {aidx}.txt (array_index primary)
static std::string EventFileName(uint16_t event_id, uint16_t array_index)
{
	(void)event_id;
	return std::to_string(array_index) + ".txt";
}

// Same format, kept for backward compatibility
static std::string EventFileNameCommon(uint16_t event_id, uint16_t array_index)
{
	(void)event_id;
	return std::to_string(array_index) + ".txt";
}

// Load ref.csv into a map: ref_path → canonical_path
static std::unordered_map<std::string, std::string> LoadRefCsv(const std::filesystem::path& path)
{
	std::unordered_map<std::string, std::string> refs;
	std::ifstream f(path);
	if (!f.is_open()) return refs;
	std::string line;
	while (std::getline(f, line))
	{
		auto comma = line.find(',');
		if (comma == std::string::npos) continue;
		auto canon = line.substr(0, comma);
		auto ref = line.substr(comma + 1);
		// Normalize separators
		for (auto& c : canon) if (c == '\\') c = '/';
		for (auto& c : ref) if (c == '\\') c = '/';
		refs[ref] = canon;
	}
	return refs;
}

// Try reading a TXT file, following ref.csv chain
static std::vector<std::string> ReadTxtWithRef(
	const std::filesystem::path& filePath,
	const std::unordered_map<std::string, std::string>& refs)
{
	// Direct read
	{
		std::ifstream f(filePath);
		if (f.is_open())
		{
			std::vector<std::string> lines;
			std::string line;
			while (std::getline(f, line))
				lines.push_back(line);
			// Check for @ref in first line (backward compat)
			if (!lines.empty() && lines[0].size() > 5 &&
				lines[0].substr(0, 5) == "@ref ")
			{
				auto canonPath = filePath.parent_path() / lines[0].substr(5);
				return ReadTxtWithRef(canonPath, refs);
			}
			return lines;
		}
	}

	// Try ref.csv resolution
	auto pStr = filePath.lexically_normal().string();
	for (auto& c : pStr) if (c == '\\') c = '/';
	auto it = refs.find(pStr);
	if (it != refs.end())
	{
		// Find "event/" in canonical path
		auto pos = it->second.find("event/");
		if (pos != std::string::npos)
		{
			auto base = filePath.parent_path();
			// Walk up to event/ directory
			while (base.filename() != "event" && base.has_parent_path())
				base = base.parent_path();
			if (base.filename() == "event")
				return ReadTxtWithRef(base / it->second.substr(pos + 6), refs);
		}
	}

	return {}; // not found
}

// Build list of candidate TXT paths for a given event
static std::vector<std::filesystem::path> CandidatePaths(
	const std::filesystem::path& textsDir,
	const std::string& zoneName,
	const std::string& actorDirWithId,
	const std::string& bareDir,
	uint16_t event_id, uint16_t array_index)
{
	std::vector<std::filesystem::path> candidates;
	auto eventFile = EventFileName(event_id, array_index);
	auto eventFileLegacy = EventFileNameCommon(event_id, array_index);

	auto base = textsDir / "event" / "zone" / zoneName;
	// 1 actorDirWithId / {eid}.{aidx}.txt
	candidates.push_back(base / actorDirWithId / eventFile);
	// 2 actorDirWithId / {eid}.txt
	candidates.push_back(base / actorDirWithId / eventFileLegacy);
	// 3 bareDir / {eid}.{aidx}.txt
	candidates.push_back(base / bareDir / eventFile);
	// 4 bareDir / {eid}.txt
	candidates.push_back(base / bareDir / eventFileLegacy);

	// common
	auto common = textsDir / "event" / "common" / bareDir;
	// 5 common / {eid}.{aidx}.txt
	candidates.push_back(common / eventFile);
	// 6 common / {eid}.txt
	candidates.push_back(common / eventFileLegacy);

	return candidates;
}

// --- EventFileProcessor ---

bool EventFileProcessor::Process(
	const FileProcessDef& fileDef,
	const std::filesystem::path& datPath,
	const std::filesystem::path& outPath,
	const std::map<std::u8string, FileProcessDef>& jpDefsByComment)
{
	(void)jpDefsByComment;

	std::string comment = xybase::string::to_string(fileDef.comment);
	auto* zone = ZoneRegistry::Instance().FindByZoneName(comment);
	if (!zone)
	{
		Logger::Instance().Error("EventFileProcessor: zone not found: " + comment);
		return false;
	}

	// Load original evsb
	EventStringBase evsb(datPath);
	evsb.Read();
	if (evsb.Size() == 0)
	{
		Logger::Instance().Warning("EventFileProcessor: empty evsb for " + comment);
		return false;
	}

	// Parse evev
	std::string evevPath = GamePathResolver::ResolvePath(zone->evev_path);
	auto actors = EventBinaryDat::Parse(evevPath);
	if (actors.empty())
	{
		Logger::Instance().Error("EventFileProcessor: empty evev for " + comment);
		return false;
	}

	std::string evacPath = zone->evac_path.empty() ? "" : GamePathResolver::ResolvePath(zone->evac_path);
	std::unordered_map<uint32_t, EntityEntry> entityMap;
	if (!evacPath.empty())
	{
		for (auto& e : EntityDat::Parse(evacPath))
			entityMap[e.entity_id] = std::move(e);
	}

	// Use the tgt text directory for ref.csv
	auto textsDir = Config::Instance().GetProgRoot() / L"text" / L"tgt";
	auto refs = LoadRefCsv(textsDir / "event" / "ref.csv");

	auto srcDir = Config::Instance().GetProgRoot() / L"text" / L"src" / L"event";
	bool hasSrcDir = std::filesystem::exists(srcDir);
	bool srcValidation = Config::Instance().IsSrcValidation();

	auto& db = TranslationDatabase::Instance();
	FinalTextProcessor finalTextProcessor(fileDef.comment, fileDef.type);
	BytecodeAnalyzer analyzer;

	// Build patches: evsb index → translated text
	std::map<size_t, std::string> patches;
	size_t patchedEvents = 0;
	size_t skippedEvents = 0;

	for (const auto& actor : actors)
	{
		// Build actor directory names
		auto bareName = EventTextOut::ActorBareName(actor, entityMap);
		auto safeBare = SafePathSegment(bareName);
		auto dirWithId = safeBare + "_" + std::to_string(actor.actor_number);

		for (const auto& evt : actor.events)
		{
			auto candidates = CandidatePaths(
				textsDir, comment, dirWithId, safeBare,
				evt.event_id, evt.array_index);

			// Find first existing candidate
			std::filesystem::path txtPath;
			for (const auto& c : candidates)
			{
				auto lines = ReadTxtWithRef(c, refs);
				if (!lines.empty())
				{
					txtPath = c;
					break;
				}
			}

			if (txtPath.empty())
			{
				skippedEvents++;
				continue; // no translation patch for this event
			}

			// Re-read to get lines (ReadTxtWithRef was already called above for probing)
			auto dstLines = ReadTxtWithRef(txtPath, refs);

			// Extract original dialogue lines from bytecode
			std::vector<std::u8string> zoneStrings;
			zoneStrings.reserve(evsb.Size());
			for (size_t si = 0; si < evsb.Size(); ++si)
				zoneStrings.push_back(evsb[si]);

			std::vector<DialogueLine> dls;
			analyzer.ExtractDialogues(evt.bytecode, actor.actor_number,
				actor.imed_data, entityMap, zoneStrings, dls, evt.byte_offset);

			if (dls.empty())
			{
				skippedEvents++;
				continue;
			}

			// SRC validation or line count check
			bool applyPatches = false;

			if (hasSrcDir && srcValidation)
			{
				// Build SRC path (mirrors TXT structure under text/src/event/)
				auto srcPath = srcDir / txtPath.lexically_relative(textsDir);
				auto srcLines = ReadTxtWithRef(srcPath, refs);

				if (dls.size() == srcLines.size() && dls.size() == dstLines.size())
				{
					// SRC content check
					bool allMatch = true;
					for (size_t i = 0; i < dls.size(); ++i)
					{
						if (srcLines[i] != dls[i].text)
						{
							Logger::Instance().Warning(
								"EventFileProcessor SRC mismatch: " + comment
								+ " event=" + std::to_string(evt.event_id)
								+ "." + std::to_string(evt.array_index)
								+ " line=" + std::to_string(i)
								+ " — falling back to TransDB for this event");
							allMatch = false;
							break;
						}
					}
					applyPatches = allMatch;
				}
				else
				{
					Logger::Instance().Warning(
						"EventFileProcessor line count mismatch (SRC): " + comment
						+ " event=" + std::to_string(evt.event_id)
						+ "." + std::to_string(evt.array_index)
						+ " dls=" + std::to_string(dls.size())
						+ " src=" + std::to_string(srcLines.size())
						+ " dst=" + std::to_string(dstLines.size())
						+ " — falling back to TransDB");
				}
			}
			else
			{
				// Without SRC validation: just check line count
				if (dls.size() == dstLines.size())
					applyPatches = true;
				else
				{
					Logger::Instance().Warning(
						"EventFileProcessor line count mismatch: " + comment
						+ " event=" + std::to_string(evt.event_id)
						+ "." + std::to_string(evt.array_index)
						+ " dls=" + std::to_string(dls.size())
						+ " dst=" + std::to_string(dstLines.size())
						+ " — falling back to TransDB");
				}
			}

			if (applyPatches)
			{
				for (size_t i = 0; i < dls.size(); ++i)
					patches[dls[i].message_id] = dstLines[i];
				patchedEvents++;
			}
			else
			{
				skippedEvents++;
			}
		}
	}

	// Apply patches + TranslationDB fill + FinalTextProcessor
	for (size_t i = 0; i < evsb.Size(); ++i)
	{
		auto& s = evsb[i];

		std::u8string result;
		auto patchIt = patches.find(i);
		if (patchIt != patches.end())
		{
			result = std::u8string(
				reinterpret_cast<const char8_t*>(patchIt->second.data()),
				patchIt->second.size());
		}
		else
		{
			result = db.GetTranslation(s);
		}

		s = finalTextProcessor.Process(result, s,
			static_cast<int64_t>(i + 1), 1);
	}

	evsb.path = outPath;
	evsb.Write();

	Logger::Instance().Info(
		"EventFileProcessor: " + comment
		+ " strings=" + std::to_string(evsb.Size())
		+ " patched_events=" + std::to_string(patchedEvents)
		+ " skipped_events=" + std::to_string(skippedEvents));
	return true;
}
