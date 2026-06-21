#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <format>
#include <memory>
#include <sstream>
#include <clocale>
#include <cstring>
#include <Windows.h>

#include "EventDump/Models.h"
#include "EventDump/ZoneConfig.h"
#include "EventDump/EventBinaryDat.h"
#include "EventDump/EntityDat.h"
#include "EventDump/EventLinker.h"
#include "EventDump/EventWriter.h"
#include "EventDump/DataDumper.h"
#include "EventDump/GamePathResolver.h"
#include "EventDump/EventAnalyzer.h"

#include <EventStringBase.h>
#include <EventString.h>
#include <xystring.h>

// System data export helpers
#include "../FFXIDat/ItemData.h"
#include "../FFXIDat/DMsg.h"
#include "../FFXIDat/FixedPhrase.h"
#include "../FFXIDat/RecordsOfEminence.h"
#include "../FFXIDat/XiString.h"
#include "../FFXIDat/StatusData.h"
#include "../FFXIDat/MonBridge.h"
#include <CsvFile.h>

static void ExportItemCsv(const std::string& datPath, const std::string& type, const std::filesystem::path& outPath)
{
	ItemSpecType spec = ItemSpecType::NORMAL;
	if (type == "iab") spec = ItemSpecType::ARMOUR;
	else if (type == "iwb") spec = ItemSpecType::WEAPON;
	else if (type == "iub") spec = ItemSpecType::USABLE;
	else if (type == "ipb") spec = ItemSpecType::PUPPET;
	else if (type == "isb") spec = ItemSpecType::SLIP;
	else if (type == "icb") spec = ItemSpecType::CURRENCY;
	else if (type == "iib") spec = ItemSpecType::INSTINCT;

	ItemData itemData;
	itemData.Read(xybase::string::sys_mbs_to_wcs(datPath), spec);
	CsvFile csv(outPath, std::ios::out | std::ios::binary);
	csv.NewCell(u8"ID"); csv.NewCell(u8"Name"); csv.NewCell(u8"Description"); csv.NewLine();
	for (const auto& datum : itemData.data)
	{
		try {
			if (datum.name() == u8".") continue;
			csv.NewCell(xybase::string::itos<char8_t>(datum.id));
			csv.NewCell(datum.name());
			csv.NewCell(datum.description());
			csv.NewLine();
		} catch (...) {}
	}
}

static void ExportFixedPhraseCsv(const std::string& datPath, const std::filesystem::path& outPath)
{
	FixedPhrase fp;
	fp.Read(xybase::string::sys_mbs_to_wcs(datPath));
	fp.ToCsv(outPath.wstring());
}

static void ExportRoeQuestCsv(const std::string& datPath, const std::filesystem::path& outPath)
{
	RecordsOfEminence roe;
	roe.ReadQuest(xybase::string::sys_mbs_to_wcs(datPath));
	CsvFile csv(outPath, std::ios::out | std::ios::binary);
	csv.NewCell(u8"ID"); csv.NewCell(u8"QuestName"); csv.NewCell(u8"Description"); csv.NewCell(u8"Note"); csv.NewLine();
	for (const auto& e : roe.questData)
	{
		csv.NewCell(xybase::string::itos<char8_t>(e.id));
		try { csv.NewCell(e.questName()); } catch (...) { csv.NewCell(u8""); }
		try { csv.NewCell(e.description()); } catch (...) { csv.NewCell(u8""); }
		try { csv.NewCell(e.note()); } catch (...) { csv.NewCell(u8""); }
		csv.NewLine();
	}
}

static void ExportRoeCategoryCsv(const std::string& datPath, const std::filesystem::path& outPath)
{
	RecordsOfEminence roe;
	roe.ReadCategory(xybase::string::sys_mbs_to_wcs(datPath));
	CsvFile csv(outPath, std::ios::out | std::ios::binary);
	csv.NewCell(u8"ID"); csv.NewCell(u8"CategoryName"); csv.NewLine();
	for (const auto& e : roe.categoryData)
	{
		csv.NewCell(xybase::string::itos<char8_t>(e.id));
		try { csv.NewCell(e.categoryName()); } catch (...) { csv.NewCell(u8""); }
		csv.NewLine();
	}
}

static void ExportQuestDMsgCsv(const std::string& datPath, const std::filesystem::path& outPath)
{
	DMsg dmsg(xybase::string::sys_mbs_to_wcs(datPath));
	dmsg.Read();
	CsvFile csv(outPath, std::ios::out | std::ios::binary);
	csv.NewCell(u8"ID"); csv.NewCell(u8"Name"); csv.NewCell(u8"Desc"); csv.NewLine();
	for (const auto& row : dmsg)
	{
		const auto& cells = row.GetCellsConst();
		if (cells.size() < 3 || cells[0].GetType() != 1) continue;
		int id = cells[0].Get<int>();
		csv.NewCell(xybase::string::itos<char8_t>(id));
		csv.NewCell(cells[1].GetType() == 0 ? cells[1].Get<std::u8string>() : u8"");
		csv.NewCell(cells[2].GetType() == 0 ? cells[2].Get<std::u8string>() : u8"");
		csv.NewLine();
	}
}

static void ExportRegularDMsgCsv(const std::string& datPath, const std::filesystem::path& outPath)
{
	DMsg dmsg(xybase::string::sys_mbs_to_wcs(datPath));
	dmsg.Read();
	dmsg.ToCsv(outPath.wstring());
}

static void ExportXiStringTxt(const std::string& datPath, const std::filesystem::path& outPath)
{
	XiString xis(xybase::string::sys_mbs_to_wcs(datPath));
	xis.Read();
	std::ofstream out(outPath);
	for (auto& s : xis)
		out << xybase::string::to_string(xybase::string::escape(xybase::string::to_utf8(xis.Decode(s.str)))) << "\n";
}

static void ExportStatusDataTxt(const std::string& datPath, const std::filesystem::path& outPath)
{
	StatusData sd;
	sd.Read(xybase::string::sys_mbs_to_wcs(datPath));
	std::ofstream out(outPath);
	for (const auto& d : sd.data)
		if (!d.description.empty())
			out << xybase::string::to_string(xybase::string::escape(d.description)) << "\n";
}

static void ExportMonBridgeTxt(const std::string& datPath, const std::filesystem::path& outPath)
{
	MonBridge mb;
	mb.Read(xybase::string::sys_mbs_to_wcs(datPath));
	std::ofstream out(outPath);
	for (const auto& d : mb.data)
		if (!d.displayName.empty())
			out << xybase::string::to_string(xybase::string::escape(d.displayName)) << "\n";
}
#include "../FFXIDatProcessor/codepage.h"

static void PrintHelp(const char* prog)
{
	std::cout << "FFXIDatAdv - FFXI 事件对话导出工具" << std::endl;
	std::cout << std::endl;
	std::cout << "用法:" << std::endl;
	std::cout << "  " << prog << "                          处理全部区域（自动验证公共 Actor）" << std::endl;
	std::cout << "  " << prog << " <zone_id>                 按区域 ID 处理" << std::endl;
	std::cout << "  " << prog << " \"<zone_name>\"            按区域名称处理" << std::endl;
	std::cout << "  " << prog << " --list-zones              列出可用区域" << std::endl;
	std::cout << "  " << prog << " --ffxi-path <path>        指定 FFXI 安装路径" << std::endl;
	std::cout << "  " << prog << " --out <dir>               输出目录 (默认: ./event/)" << std::endl;
	std::cout << "  " << prog << " --pretty                  美化 JSON 输出" << std::endl;
	std::cout << "  " << prog << " --lang <na|jp|all>        语言: na=英语, jp=日语, all=双语 (需要 --split-text)" << std::endl;
	std::cout << "  " << prog << " --dump-opcodes            在 JSON 中包含 opcode 反汇编" << std::endl;
	std::cout << "  " << prog << " --split-text              文本分离：文本与指令分别存储" << std::endl;
	std::cout << "  " << prog << " --help                    显示此帮助" << std::endl;
}

static std::unordered_map<std::string, ZoneDef> LoadZoneConfig(const std::string& csvPath)
{
	auto defs = ZoneConfig::Load(csvPath);
	auto map = ZoneConfig::GroupByZone(defs);
	// Keep only zones with evev data
	for (auto it = map.begin(); it != map.end();)
	{
		if (it->second.evev_path.empty())
			it = map.erase(it);
		else
			++it;
	}
	return map;
}

// --- Phase 1: Scan -------------------------------------
struct ZoneScan
{
	uint32_t zone_id;
	std::string zone_name;
	std::vector<ActorBlock> actors;
	std::unordered_map<uint32_t, EntityEntry> entity_map;
	std::vector<std::u8string> zone_strings;     // NA
	std::vector<std::u8string> zone_strings_jp;  // JP (only for --lang all)
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
		std::cerr << "[WARN] Failed to load evsb: " << path << std::endl;
	}
	return result;
}

// --- Phase 2: Verify common actors ---------------------
struct ActorCandidate
{
	std::string actor_name;
	uint32_t zone_id;
	std::string zone_name;
	const ActorBlock* block;
};

static std::unordered_map<std::string, std::vector<ActorCandidate>> GroupCandidates(
	const std::vector<ZoneScan>& scans)
{
	std::unordered_map<std::string, std::vector<ActorCandidate>> groups;
	for (const auto& scan : scans)
	{
		for (const auto& block : scan.actors)
		{
			// Resolve name
			std::string name;
			auto it = scan.entity_map.find(block.actor_number);
			if (it != scan.entity_map.end() && !it->second.name.empty())
				name = it->second.name;
			else if (block.actor_number == 0x7FFFFFF0)
				name = "Zone Events";
			else if (block.actor_number == 0x7FFFFFFF)
				name = "Zone/Player Events";
			else
				continue; // unnamed, skip

			if (name.empty())
				continue;

			groups[name].push_back({name, scan.zone_id, scan.zone_name, &block});
		}
	}
	return groups;
}

static bool BytecodeMatches(const ActorBlock& a, const ActorBlock& b)
{
	if (a.events.size() != b.events.size())
		return false;
	for (size_t i = 0; i < a.events.size(); ++i)
	{
		const auto& ea = a.events[i];
		const auto& eb = b.events[i];
		if (ea.bytecode.size() != eb.bytecode.size())
			return false;
		if (memcmp(ea.bytecode.data(), eb.bytecode.data(), ea.bytecode.size()) != 0)
			return false;
	}
	return true;
}

struct VerifyResult
{
	std::string actor_name;
	bool is_common;
	std::vector<std::string> zone_names;
	const ActorBlock* reference_block;
};

static std::vector<VerifyResult> VerifyCandidates(
	const std::unordered_map<std::string, std::vector<ActorCandidate>>& groups)
{
	std::vector<VerifyResult> results;
	for (const auto& [name, candidates] : groups)
	{
		VerifyResult vr;
		vr.actor_name = name;
		vr.is_common = false;
		vr.reference_block = nullptr;

		// Collect unique zones
		std::unordered_set<std::string> unique_zones;
		for (const auto& c : candidates)
			unique_zones.insert(c.zone_name);

		// Only consider common promotion if appears in ≥2 different zones
		if (unique_zones.size() < 2)
		{
			for (const auto& c : candidates)
				vr.zone_names.push_back(c.zone_name);
			results.push_back(vr);
			continue;
		}

		// Compare bytecode across all instances
		const auto& first = *candidates[0].block;
		vr.reference_block = &first;
		bool all_match = true;
		for (const auto& c : candidates)
		{
			if (std::find(vr.zone_names.begin(), vr.zone_names.end(), c.zone_name) == vr.zone_names.end())
				vr.zone_names.push_back(c.zone_name);
			if (!BytecodeMatches(first, *c.block))
			{
				all_match = false;
				std::cerr << "[VERIFY] WARNING: Actor \"" << name
					<< "\" differs between zone " << candidates[0].zone_name
					<< " and zone " << c.zone_name << std::endl;
			}
		}

		vr.is_common = all_match;
		if (vr.is_common)
		{
			std::cout << "[VERIFY] OK: Actor \"" << name << "\" ("
				<< unique_zones.size() << " zones) bytecode match." << std::endl;
		}
		results.push_back(vr);
	}
	return results;
}

// --- Phase 3: Export ------------------------------------
static std::string ResolveActorName(const ActorBlock& block,
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

// --- Main ----------------------------------------------

// Dialogue dedup: dialogue fingerprint -> (actor_name, canonical_event_id, text_ref_path)
struct DialogueKey
{
	size_t hash;
	bool operator==(const DialogueKey& o) const { return hash == o.hash; }
};
struct DialogueDedupEntry
{
	std::string actor_name;
	uint32_t canonical_event_id;
	std::string text_ref;
};
using DialogueDedupMap = std::unordered_map<size_t, DialogueDedupEntry>;

static size_t HashDialogues(const std::vector<DialogueLine>& dls)
{
	size_t h = 0;
	for (const auto& dl : dls)
	{
		h ^= std::hash<std::string>{}(dl.speaker) + 0x9e3779b9 + (h << 6) + (h >> 2);
		h ^= std::hash<std::string>{}(dl.text) + 0x9e3779b9 + (h << 6) + (h >> 2);
	}
	return h;
}

static int RunAllZones(
	const std::unordered_map<std::string, ZoneDef>& config,
	const std::string& outputDir,
	bool pretty,
	const std::string& lang = "na",
	bool splitText = false,
	bool dumpOpcodes = false)
{
	std::filesystem::path outPath(outputDir);
	EventWriter writer(outPath, pretty);
	EventLinker linker;

	std::vector<ZoneScan> scans;
	std::vector<ZoneIndex> zoneIndices;
	std::vector<CommonActorData> commonActors;

	// === Phase 1: Scan all zones (raw data only) ===
	std::cout << "=== Phase 1: Scanning zones ===" << std::endl;
	uint32_t processedCount = 0;
	for (const auto& [zoneName, def] : config)
	{
		++processedCount;
		if (!def.IsValid())
		{
			std::cerr << "[SKIP] " << zoneName << ": incomplete definition" << std::endl;
			continue;
		}

		std::cout << "[" << processedCount << "/" << config.size() << "] Scanning: "
			<< zoneName << std::endl;

		std::string evevPath = GamePathResolver::ResolvePath(def.evev_path);
		std::string evacPath = def.evac_path.empty() ? "" :
			GamePathResolver::ResolvePath(def.evac_path);
		// Determine which evsb file(s) to load
		std::string evsbNa, evsbJp;
		evsbNa = def.GetEvsbPath(false).empty() ? "" :
			GamePathResolver::ResolvePath(def.GetEvsbPath(false));
		evsbJp = def.GetEvsbPath(true).empty() ? "" :
			GamePathResolver::ResolvePath(def.GetEvsbPath(true));

		auto actors = EventBinaryDat::Parse(evevPath);
		if (actors.empty())
		{
			std::cerr << "[WARN] " << zoneName << ": no actors found" << std::endl;
			continue;
		}

		std::unordered_map<uint32_t, EntityEntry> entityMap;
		if (!evacPath.empty())
		{
			auto entities = EntityDat::Parse(evacPath);
			for (auto& e : entities)
				entityMap[e.entity_id] = std::move(e);
		}

		// Load strings: zone_strings is primary, zone_strings_jp is secondary (--lang all)
		std::vector<std::u8string> zoneStrings, zoneStringsJp;
		if (lang == "ja")
		{
			if (!evsbJp.empty()) zoneStrings = LoadEvsbStrings(evsbJp);
		}
		else
		{
			if (!evsbNa.empty()) zoneStrings = LoadEvsbStrings(evsbNa);
			if (lang == "all" && !evsbJp.empty())
				zoneStringsJp = LoadEvsbStrings(evsbJp);
		}

		ZoneScan scan;
		scan.zone_id = static_cast<uint32_t>(processedCount);
		scan.zone_name = zoneName;
		scan.actors = std::move(actors);
		scan.entity_map = std::move(entityMap);
		scan.zone_strings = std::move(zoneStrings);
		scan.zone_strings_jp = std::move(zoneStringsJp);
		scans.push_back(std::move(scan));
	}

	// === Phase 2: Verify common actors (raw bytecode compare) ===
	std::cout << "\n=== Phase 2: Verifying common actors ===" << std::endl;
	auto groups = GroupCandidates(scans);
	auto verifyResults = VerifyCandidates(groups);

	// Build lookup: actor_name → is_common? + reference block
	std::unordered_map<std::string, const VerifyResult*> resultLookup;
	for (const auto& vr : verifyResults)
		resultLookup[vr.actor_name] = &vr;

	std::unordered_map<std::string, ResolvedActor> commonActorCache; // name → resolved

	// Dialogue dedup maps (only for --split-text)
	DialogueDedupMap dedupMap;
	DialogueDedupMap dedupMapJp; // JP dedup (--lang all)

	// Helper: write text file + set text_ref
	auto handleSplitText = [&](ResolvedEvent& re, const EventEntry& evt,
		const ActorBlock& block, const ZoneScan& scan,
		const std::string& actorName, bool isCommon, DialogueDedupMap& dmap,
		const std::string& effLang) {
		if (re.dialogues.empty()) return;
		size_t h = HashDialogues(re.dialogues);
		auto dit = dmap.find(h);
		if (dit != dmap.end())
		{
			re.text_ref = dit->second.text_ref;
		}
		else
		{
			std::string tr = isCommon
				? writer.MakeTextRefCommon(actorName, evt.event_id)
				: writer.MakeTextRefZone(scan.zone_name, actorName, evt.event_id);
			writer.WriteTextFile(tr, effLang, actorName, evt.event_id, re.dialogues);
			dmap[h] = {actorName, evt.event_id, tr};
			re.text_ref = tr;
		}
	};

	BytecodeAnalyzer disasmAnalyzer; // for --dump-opcodes

	// === Phase 3: Link and export ===
	std::cout << "\n=== Phase 3: Exporting ===" << std::endl;
	for (const auto& scan : scans)
	{
		ZoneIndex zi;
		zi.zone_id = scan.zone_id;
		zi.zone_name = scan.zone_name;

		// === Collect private actors for bytecode-dedup ===
		struct PrivateActorInfo {
			const ActorBlock* block;
			std::string name;
			ResolvedActor resolved;
			size_t bytecodeHash;
		};
		std::vector<PrivateActorInfo> pendingPrivate;
		// Helper: hash all events' bytecode for dedup
		auto hashActorBytecode = [](const ActorBlock& blk) -> size_t {
			size_t h = 0;
			for (const auto& evt : blk.events) {
				for (uint8_t b : evt.bytecode)
					h = h * 31 + b;
			}
			return h;
		};

		for (const auto& block : scan.actors)
		{
			std::string actorName = ResolveActorName(block, scan.entity_map);
			if (actorName == "_")
				continue;

			auto it = resultLookup.find(actorName);
			bool isCommon = (it != resultLookup.end() && it->second->is_common);

			if (isCommon)
			{
				// Common actor: analyze once, cache result (existing logic unchanged)
				if (commonActorCache.find(actorName) == commonActorCache.end())
				{
					ResolvedActor ra;
					ra.actor_number = block.actor_number;
					ra.actor_name = actorName;
					ra.category = ActorCategory::common;
					ra.imed_data = block.imed_data;

					for (const auto& evt : block.events)
					{
						ResolvedEvent re;
						re.event_id = evt.event_id;
						re.array_index = evt.array_index;
						re.byte_offset = evt.byte_offset;
						re.byte_size = evt.byte_size;
						re.bytecode = evt.bytecode;
						linker.ExtractDialoguesFromEvent(evt, block.actor_number,
							block.imed_data, scan.entity_map, scan.zone_strings, re.dialogues);

						if (dumpOpcodes)
						{
							auto lines = disasmAnalyzer.Disassemble(
								evt.bytecode, block.actor_number, block.imed_data,
								scan.entity_map, scan.zone_strings, evt.byte_offset);
							re.opcodes = std::move(lines);
						}

						if (splitText)
						{
							std::string effLang = (lang == "all") ? "na" : lang;
							handleSplitText(re, evt, block, scan, actorName, true, dedupMap, effLang);
							if (lang == "all" && !scan.zone_strings_jp.empty())
							{
								std::vector<DialogueLine> jpDlgs;
								linker.ExtractDialoguesFromEvent(evt, block.actor_number,
									block.imed_data, scan.entity_map, scan.zone_strings_jp, jpDlgs);
								if (!jpDlgs.empty())
								{
									ResolvedEvent jpRe;
									jpRe.dialogues = std::move(jpDlgs);
									handleSplitText(jpRe, evt, block, scan, actorName, true, dedupMapJp, "ja");
								}
							}
						}

						ra.events.push_back(std::move(re));
					}
					commonActorCache[actorName] = ra;

					CommonActorData cad;
					cad.actor_name = actorName;
					cad.verified = true;
					cad.zone_names = it->second->zone_names;
					for (const auto& e : ra.events)
					{
						ResolvedEvent re;
						re.event_id = e.event_id;
						re.array_index = e.array_index;
						re.byte_offset = e.byte_offset;
						re.byte_size = e.byte_size;
						re.bytecode = e.bytecode;
						re.dialogues = e.dialogues;
						re.text_ref = e.text_ref;
						cad.events.push_back(std::move(re));
					}
					commonActors.push_back(cad);
					writer.WriteCommonActorFile(cad);
				}

				ZoneIndexEntry entry;
				entry.actor_number = block.actor_number;
				entry.actor_name = actorName;
				entry.category = ActorCategory::common;
				zi.entries.push_back(entry);
			}
			else
			{
				// Private actor: resolve dialogues, defer file write for dedup
				PrivateActorInfo info;
				info.block = &block;
				info.name = actorName;
				info.bytecodeHash = hashActorBytecode(block);
				info.resolved.actor_number = block.actor_number;
				info.resolved.actor_name = actorName;
				info.resolved.category = ActorCategory::private_;
				info.resolved.imed_data = block.imed_data;

				for (const auto& evt : block.events)
				{
					ResolvedEvent re;
					re.event_id = evt.event_id;
					re.array_index = evt.array_index;
					re.byte_offset = evt.byte_offset;
					re.byte_size = evt.byte_size;
					re.bytecode = evt.bytecode;
					linker.ExtractDialoguesFromEvent(evt, block.actor_number,
						block.imed_data, scan.entity_map, scan.zone_strings, re.dialogues);

					if (dumpOpcodes)
					{
						auto lines = disasmAnalyzer.Disassemble(
							evt.bytecode, block.actor_number, block.imed_data,
							scan.entity_map, scan.zone_strings, evt.byte_offset);
						re.opcodes = std::move(lines);
					}

					if (splitText)
					{
						std::string effLang = (lang == "all") ? "na" : lang;
						handleSplitText(re, evt, block, scan, actorName, false, dedupMap, effLang);
						if (lang == "all" && !scan.zone_strings_jp.empty())
						{
							std::vector<DialogueLine> jpDlgs;
							linker.ExtractDialoguesFromEvent(evt, block.actor_number,
								block.imed_data, scan.entity_map, scan.zone_strings_jp, jpDlgs);
							if (!jpDlgs.empty())
							{
								ResolvedEvent jpRe;
								jpRe.dialogues = std::move(jpDlgs);
								handleSplitText(jpRe, evt, block, scan, actorName, false, dedupMapJp, "ja");
							}
						}
					}

					info.resolved.events.push_back(std::move(re));
				}
				pendingPrivate.push_back(std::move(info));
			}
		}

		// === Dedup and write private actors ===
		// SafeFilename for disambiguated filenames (mirrors EventWriter.cpp static fn)
		auto safeName = [](const std::string& s) {
			if (s.empty()) return std::string("_");
			std::string r; r.reserve(s.size());
			for (char c : s) {
				if (static_cast<unsigned char>(c) < 0x20 || c == '\\' || c == '/' || c == ':' ||
					c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
					r += '_';
				else r += c;
			}
			return r;
		};

		std::unordered_map<std::string, std::string> filenameCache; // "name:hash" → filename
		std::unordered_set<std::string> writtenFiles;
		std::unordered_map<std::string, uint32_t> nameCount;
		for (auto& info : pendingPrivate) nameCount[info.name]++;

		for (auto& info : pendingPrivate)
		{
			std::string key = info.name + ":" + std::to_string(info.bytecodeHash);
			auto fit = filenameCache.find(key);
			if (fit == filenameCache.end())
			{
				std::string fname;
				if (nameCount[info.name] == 1)
					fname = writer.MakeActorFilename(info.name, info.block->actor_number);
				else
					fname = safeName(info.name) + "_" + std::to_string(info.block->actor_number) + ".json";
				filenameCache[key] = fname;
				fit = filenameCache.find(key);
				if (writtenFiles.insert(fname).second)
					writer.WriteActorFile(info.resolved, scan.zone_name, fname, splitText);
			}

			ZoneIndexEntry entry;
			entry.actor_number = info.block->actor_number;
			entry.actor_name = info.name;
			entry.local_ref = fit->second; // explicit ref for dedup filename
			entry.category = ActorCategory::private_;
			zi.entries.push_back(entry);
		}

		writer.WriteZoneIndex(zi, scan.zone_name);
		zoneIndices.push_back(zi);
	}

	// Write indexes
	if (!commonActors.empty())
		writer.WriteCommonIndex(commonActors);
	writer.WriteMasterIndex(zoneIndices, commonActors);

	std::cout << "\n=======================================" << std::endl;
	std::cout << "Common actors found: " << commonActors.size() << std::endl;
	std::cout << "[DONE] Output written to: " << outputDir << std::endl;
	return 0;
}

// --- Dump Event JSON (--dump-event-json) ---
static void WriteTextJsonFile(const std::filesystem::path& path, const std::vector<std::string>& lines)
{
	std::filesystem::create_directories(path.parent_path());
	std::ofstream out(path, std::ios::binary);
	out << "[";
	for (size_t i = 0; i < lines.size(); ++i)
	{
		if (i) out << ", ";
		out << "\"";
		for (char c : lines[i])
		{
			switch (c)
			{
			case '"': out << "\\\""; break;
			case '\\': out << "\\\\"; break;
			case '\n': out << "\\n"; break;
			case '\r': out << "\\r"; break;
			case '\t': out << "\\t"; break;
			default: out << c;
			}
		}
		out << "\"";
	}
	out << "]\n";
}

static std::string EscapeJsonStr(const std::string& s)
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

static std::string Sanitize(const std::string& s)
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
	if (r.empty()) r = "_";
	return r;
}

static std::string GetActorDirName(const AnalyzedActor& actor, const std::vector<AnalyzedActor>& all)
{
	int count = 0;
	for (const auto& a : all)
		if (a.actor_name == actor.actor_name) ++count;
	std::string base = Sanitize(actor.actor_name);
	if (count <= 1) return base;
	return base + "_" + std::to_string(actor.actor_number);
}

static std::string MakeTxtJsonPath(const std::string& zone,
	const std::string& actorDir, uint16_t aidx)
{
	return zone + "/" + actorDir + "/" + std::to_string(aidx) + ".json";
}

static int DumpEventJson(
	const std::unordered_map<std::string, ZoneDef>& config,
	const std::string& csvPath,
	const std::string& outDir)
{
	std::cout << "=== Dump Event JSON ===" << std::endl;
	std::filesystem::path root(outDir);
	std::filesystem::create_directories(root);

	// First pass: run EventAnalyzer for ja and en for each zone
	struct ZoneResult
	{
		std::string name;
		std::vector<AnalyzedActor> actorsJa;
		std::vector<AnalyzedActor> actorsEn;
	};

	std::vector<ZoneResult> zones;

	for (const auto& [zoneName, def] : config)
	{
		if (!def.IsValid()) continue;
		std::cout << "  " << zoneName << std::flush;

		auto evevPath = GamePathResolver::ResolvePath(def.evev_path);
		if (evevPath.empty()) { std::cout << " [no evev]" << std::endl; continue; }

		auto evsbJaPath = GamePathResolver::ResolvePath(def.GetEvsbPath(true));
		auto evsbEnPath = GamePathResolver::ResolvePath(def.GetEvsbPath(false));
		if (evsbJaPath.empty() && evsbEnPath.empty()) { std::cout << " [no evsb]" << std::endl; continue; }

		ZoneResult zr;
		zr.name = zoneName;

		if (!evsbJaPath.empty())
		{
			auto evacPath = GamePathResolver::ResolvePath(def.evac_path);
			EventAnalyzer a;
			a.Load(zoneName, evevPath, evacPath, evsbJaPath);
			zr.actorsJa = a.GetActors();
		}

		if (!evsbEnPath.empty())
		{
			auto evacPath = GamePathResolver::ResolvePath(def.evac_path);
			EventAnalyzer a;
			a.Load(zoneName, evevPath, evacPath, evsbEnPath);
			zr.actorsEn = a.GetActors();
		}

		if (zr.actorsJa.empty() && zr.actorsEn.empty())
		{ std::cout << " [no actors]" << std::endl; continue; }

		std::cout << " (" << (zr.actorsJa.empty() ? zr.actorsEn.size() : zr.actorsJa.size()) << " actors)" << std::endl;
		zones.push_back(std::move(zr));
	}

	// Detect common actors (same name + bytecode_hash across >=2 zones)
	std::map<std::string, std::vector<std::pair<size_t, AnalyzedActor*>>> commonCandidates;
	for (size_t zi = 0; zi < zones.size(); ++zi)
	{
		for (auto& aa : zones[zi].actorsJa)
		{
			if (aa.actor_name == "_") continue;
			std::string key = aa.actor_name + "@" + aa.bytecode_hash;
			commonCandidates[key].push_back({zi, &aa});
		}
	}

	struct CommonInfo
	{
		std::set<uint32_t> actorNumbers;
		std::set<std::string> zoneNames;
	};
	std::unordered_map<AnalyzedActor*, CommonInfo> commonActors;
	for (const auto& [key, list] : commonCandidates)
	{
		if (list.size() >= 2)
		{
			std::set<std::string> znames;
			for (const auto& [zi, _] : list) znames.insert(zones[zi].name);
			if (znames.size() >= 2)
			{
				CommonInfo info;
				for (const auto& [zi, ptr] : list)
				{
					info.actorNumbers.insert(ptr->actor_number);
					info.zoneNames.insert(zones[zi].name);
				}
				for (const auto& [_, ptr] : list)
					commonActors[ptr] = info;
			}
		}
	}

	// Write text JSON files and collect paths
	struct EventPathInfo
	{
		std::string txt;
	};

	// Path lookup: [zoneIdx][actorIdx][event_id] -> EventPathInfo
	std::vector<std::unordered_map<AnalyzedActor*, std::unordered_map<uint16_t, EventPathInfo>>> allPaths(zones.size());
	std::unordered_map<uint64_t, EventPathInfo> contentDedupJa;
	std::unordered_map<uint64_t, EventPathInfo> contentDedupEn;
	std::unordered_set<AnalyzedActor*> writtenCommon;

	for (size_t zi = 0; zi < zones.size(); ++zi)
	{
		auto& zone = zones[zi];
		auto& pathMap = allPaths[zi];
		size_t numActors = (std::max)(zone.actorsJa.size(), zone.actorsEn.size());

		for (size_t ai = 0; ai < numActors; ++ai)
		{
			bool hasJa = ai < zone.actorsJa.size();
			bool hasEn = ai < zone.actorsEn.size();
			auto& aJa = hasJa ? zone.actorsJa[ai] : zone.actorsEn[ai];
			auto& aEn = hasEn ? zone.actorsEn[ai] : zone.actorsJa[ai];
			bool isCommon = commonActors.count(&aJa) > 0;
			std::string jsonZone = isCommon ? "common" : zone.name;
			std::string actorDir = isCommon ? Sanitize(aJa.actor_name)
				: GetActorDirName(aJa, zone.actorsJa);

			// Write actor JSON (only once for common actors)
			auto evDir = root / "event" / jsonZone;
			std::filesystem::create_directories(evDir);
			auto actorJsonPath = evDir / (actorDir + ".json");

			// Build speakers list
			std::vector<std::string> allSpeakers;
			std::unordered_map<std::string, int> speakerMap;
			for (const auto& evt : aJa.events)
				for (const auto& s : evt.speakers)
					if (speakerMap.find(s) == speakerMap.end())
					{
						speakerMap[s] = (int)allSpeakers.size();
						allSpeakers.push_back(s);
					}

			std::ofstream jout(actorJsonPath, std::ios::binary);
				jout << "{\n";
				jout << "  \"actor\": " << EscapeJsonStr(aJa.actor_name) << ",\n";
				if (isCommon)
				{
					auto cit = commonActors.find(&aJa);
					jout << "  \"actor_numbers\": [";
					const auto& ci = cit->second;
					bool firstNum = true;
					for (uint32_t n : ci.actorNumbers)
					{
						if (!firstNum) jout << ", ";
						firstNum = false;
						jout << n;
					}
					jout << "],\n";
				}
				else
				{
					jout << "  \"actor_number\": " << aJa.actor_number << ",\n";
				}
			if (!allSpeakers.empty())
			{
				jout << "  \"speakers\": [";
				for (size_t si = 0; si < allSpeakers.size(); ++si)
				{
					if (si) jout << ", ";
					jout << EscapeJsonStr(allSpeakers[si]);
				}
				jout << "],\n";
			}
			jout << "  \"events\": [\n";

			bool firstEvent = true;
			size_t numEvents = (std::max)(aJa.events.size(), aEn.events.size());
			for (size_t ei = 0; ei < numEvents; ++ei)
			{
				bool hasEvtJa = ei < aJa.events.size();
				bool hasEvtEn = ei < aEn.events.size();

				if (hasEvtJa && aJa.events[ei].textLines.empty()) continue;
				if (!hasEvtJa && (!hasEvtEn || aEn.events[ei].textLines.empty())) continue;

				const auto& evtJa = hasEvtJa ? aJa.events[ei] : aEn.events[ei];
				const auto& evtEn = hasEvtEn ? aEn.events[ei] : aJa.events[ei];
				uint16_t eid = evtJa.event_id;

				auto txtRel = MakeTxtJsonPath(jsonZone, actorDir, evtJa.array_index);

				auto fullJa = root / "event" / "txt" / "ja" / txtRel;
				auto fullEn = root / "event" / "txt" / "en" / txtRel;

				std::string reportPath = txtRel;

				// Content dedup for ja — if same hash written elsewhere, reuse that path in JSON
				auto ditJa = contentDedupJa.find(evtJa.textHash);
				if (ditJa != contentDedupJa.end())
					reportPath = ditJa->second.txt;
				else if (!std::filesystem::exists(fullJa) && hasEvtJa)
				{
					WriteTextJsonFile(fullJa, evtJa.textLines);
					contentDedupJa[evtJa.textHash] = {txtRel};
				}

				// Content dedup for en
				auto ditEn = contentDedupEn.find(evtEn.textHash);
				if (ditEn == contentDedupEn.end() && !std::filesystem::exists(fullEn) && hasEvtEn && !evtEn.textLines.empty())
				{
					WriteTextJsonFile(fullEn, evtEn.textLines);
					contentDedupEn[evtEn.textHash] = {txtRel};
				}

				pathMap[&aJa][eid] = {reportPath};

				if (!firstEvent) jout << ",\n";
				firstEvent = false;

				jout << "    {\n";
				jout << "      \"event_id\": " << eid << ",\n";
				jout << "      \"array_index\": " << evtJa.array_index << ",\n";
				jout << "      \"txt\": " << EscapeJsonStr(reportPath) << ",\n";
				/*jout << "      \"evsb_refs\": [";
				for (size_t ri = 0; ri < evtJa.evsbRefs.size(); ++ri)
				{
					if (ri) jout << ", ";
					jout << evtJa.evsbRefs[ri];
				}
				jout << "],\n";*/ // meaningless
				jout << "      \"dialogues\": [\n";
				for (size_t di = 0; di < evtJa.speakers.size(); ++di)
				{
					if (di) jout << ",\n";
					jout << "        {\"speaker\": " << speakerMap[evtJa.speakers[di]]
						<< ", \"line\": " << evtJa.lineIndices[di] << "}";
				}
				jout << "\n      ]\n";
				jout << "    }";
			}
			jout << "\n  ]\n}\n";
		}
	}

	// ref.csv
	{
		auto refPath = root / "event" / "ref.csv";
		std::ofstream out(refPath, std::ios::binary);
		out << "zone,actor,actor_number,event_id,array_index,txt\n";
		for (size_t zi = 0; zi < zones.size(); ++zi)
		{
			auto& zone = zones[zi];
			auto& pathMap = allPaths[zi];
			size_t na = (std::max)(zone.actorsJa.size(), zone.actorsEn.size());
			for (size_t ai = 0; ai < na; ++ai)
			{
				bool hasJa = ai < zone.actorsJa.size();
				auto& a = hasJa ? zone.actorsJa[ai] : zone.actorsEn[ai];
				auto it = pathMap.find(&a);
				if (it == pathMap.end()) continue;
				for (const auto& evt : a.events)
				{
					if (evt.textLines.empty()) continue;
					auto pit = it->second.find(evt.event_id);
					if (pit == it->second.end()) continue;
					out << zone.name << ","
						<< a.actor_name << ","
						<< a.actor_number << ","
						<< evt.event_id << ","
						<< evt.array_index << ","
						<< pit->second.txt << "\n";
				}
			}
		}
	}

	// evsb_msgs.txt — orphan strings from both languages
	{
		std::set<std::u8string> orphanTexts;
		for (const auto& zr : zones)
		{
			// Use the first actor's referenced indices as representative
			// Actually, run a fresh analyzer just for orphan detection
			// For simplicity, collect from ja pass (same structure)
		}

		// Actually, we don't have the referenced indices stored per-zone anymore.
		// The zones_ vector doesn't carry EventAnalyzer instances, just actors.
		// Skip orphan detection for JSON dump — it's clean event data.
		// User can use --dump-db for full evsb_msgs.txt.
	}

	std::cout << "[JSON] Done. " << zones.size() << " zones to " << outDir << std::endl;
	return 0;
}

// --- Dump Database (--dump-db) ---
static int DumpDatabase(
	const std::unordered_map<std::string, ZoneDef>& config,
	const std::string& csvPath,
	const std::string& outDir,
	bool fullExport,
	bool preferJp)
{
	std::cout << "=== Dump DB ===" << std::endl;
	std::filesystem::path dumpDir(outDir);
	DataDumper dumper(dumpDir);

	for (const auto& [zoneName, def] : config)
	{
		if (!def.IsValid()) continue;

		std::cout << "  " << zoneName << std::endl;

		auto evevPath = GamePathResolver::ResolvePath(def.evev_path);
		if (evevPath.empty()) continue;

		auto evacPath = GamePathResolver::ResolvePath(def.evac_path);
		auto evsbPath = GamePathResolver::ResolvePath(def.GetEvsbPath(preferJp));
		if (evsbPath.empty()) continue;

		auto analyzer = std::make_unique<EventAnalyzer>();
		analyzer->Load(zoneName, evevPath, evacPath, evsbPath);
		if (analyzer->GetActors().empty()) continue;

		dumper.AddZone(std::move(analyzer));
	}

	// Process orphan evsb files (no evev) — only when doing full export
	if (fullExport)
	{
		auto allDefs = ZoneConfig::Load(csvPath);
		auto allZones = ZoneConfig::GroupByZone(allDefs);
		for (const auto& [name, def] : allZones)
		{
			if (config.find(name) != config.end()) continue;
			auto evsbPath = def.GetEvsbPath(true);
			if (evsbPath.empty()) continue;
			auto resolvedPath = GamePathResolver::ResolvePath(evsbPath);
			if (resolvedPath.empty()) continue;
			auto strings = LoadEvsbStrings(resolvedPath);
			if (!strings.empty())
				dumper.AddOrphanFile(name, strings);
		}
	}

	// Process system data (items, dmsg, xis, fp, roe, sd, mbd) — only when doing full export
	if (fullExport)
	{
		std::ifstream csvStream(csvPath);
		std::string line;
		std::set<std::string> seen;
		while (std::getline(csvStream, line))
		{
			if (line.empty() || line[0] == '#') continue;
			std::vector<std::string> f;
			std::istringstream ls(line);
			for (std::string v; std::getline(ls, v, ','); ) f.push_back(v);
			if (f.size() < 4) continue;
			auto& p = f[0]; auto& t = f[1]; auto& l = f[2]; auto& c = f[3];
			auto ci = f.size() > 4 ? f[4] : "";

			if (t == "evev" || t == "evac" || t == "evsb") continue;
			if (!l.empty() && l != "ja") continue;
			if (!seen.insert(c + "@" + t).second) continue;

			auto dp = GamePathResolver::ResolvePath(p);
			if (dp.empty()) continue;

			try
			{
				namespace fs = std::filesystem;
				fs::path base(dumpDir);
				bool iab = t == "iab"||t=="iwb"||t=="iub"||t=="inb"||t=="ipb"||t=="isb"||t=="icb"||t=="iib";
				if (iab) { auto o=base/(c+".csv"); fs::create_directories(o.parent_path()); ExportItemCsv(dp,t,o); }
				else if (t=="fp") { auto o=base/(c+".csv"); fs::create_directories(o.parent_path()); ExportFixedPhraseCsv(dp,o); }
				else if (t=="erq") { auto o=base/(c+".csv"); fs::create_directories(o.parent_path()); ExportRoeQuestCsv(dp,o); }
				else if (t=="erc") { auto o=base/(c+".csv"); fs::create_directories(o.parent_path()); ExportRoeCategoryCsv(dp,o); }
				else if (t=="dmsg"&&(c.starts_with("sys/mis/")||c.starts_with("sys/qst/"))) { auto o=base/(c+".csv"); fs::create_directories(o.parent_path()); ExportQuestDMsgCsv(dp,o); }
				else if (t=="dmsg") { auto o=base/(c+".csv"); fs::create_directories(o.parent_path()); ExportRegularDMsgCsv(dp,o); }
				else if (t=="xis") { auto o=base/(c+".txt"); fs::create_directories(o.parent_path()); ExportXiStringTxt(dp,o); }
				else if (t=="sd") { auto o=base/(c+".txt"); fs::create_directories(o.parent_path()); ExportStatusDataTxt(dp,o); }
				else if (t=="mbd") { auto o=base/(c+".txt"); fs::create_directories(o.parent_path()); ExportMonBridgeTxt(dp,o); }
			}
			catch (std::exception& e) { std::cerr << "[WARN] " << t << " " << c << ": " << e.what() << "\n"; }
		}
	}

	dumper.Flush();
	std::cout << "[DB] Written to: " << outDir << std::endl;
	return 0;
}

static int RunSingleZone(
	const std::string& zoneNameOrId,
	const std::unordered_map<std::string, ZoneDef>& config,
	const std::string& outputDir,
	bool pretty,
	const std::string& lang = "na",
	bool splitText = false,
	bool dumpOpcodes = false)
{
	for (const auto& [name, def] : config)
	{
		if (name == zoneNameOrId)
		{
			std::unordered_map<std::string, ZoneDef> single;
			single[name] = def;
			return RunAllZones(single, outputDir, pretty, lang, splitText, dumpOpcodes);
		}
	}
	std::cout << "[ERROR] Zone not found: " << zoneNameOrId << std::endl;
	return 1;
}

static int ListZones(const std::unordered_map<std::string, ZoneDef>& config)
{
	std::cout << "Available zones (" << config.size() << "):" << std::endl;
	int i = 0;
	for (const auto& [name, def] : config)
	{
		++i;
		std::cout << std::format("  {:4d}. {} (evev: {}, evac: {}, evsb: {}, evsb_jp: {})",
			i, name, def.evev_path,
			def.evac_path.empty() ? "-" : def.evac_path,
			def.evsb_path.empty() ? "-" : def.evsb_path,
			def.evsb_jp_path.empty() ? "-" : def.evsb_jp_path) << std::endl;
	}
	return 0;
}

static int DumpZoneOpcodes(
	const std::string& zoneNameOrId,
	const std::unordered_map<std::string, ZoneDef>& config,
	const std::string& outputDir,
	bool preferJp)
{
	// Find the zone definition
	for (const auto& [name, def] : config)
	{
		if (name == zoneNameOrId)
		{
			std::string evevPath = GamePathResolver::ResolvePath(def.evev_path);
			std::string evacPath = def.evac_path.empty() ? "" :
				GamePathResolver::ResolvePath(def.evac_path);
			std::string evsbPath = def.GetEvsbPath(preferJp).empty() ? "" :
				GamePathResolver::ResolvePath(def.GetEvsbPath(preferJp));

			auto actors = EventBinaryDat::Parse(evevPath);
			if (actors.empty())
			{
				std::cerr << "[ERROR] No actors found for " << name << std::endl;
				return 1;
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

			BytecodeAnalyzer analyzer;
			std::filesystem::path outPath(outputDir);
			std::filesystem::create_directories(outPath);

			for (const auto& block : actors)
			{
				std::string actorName;
				auto it = entityMap.find(block.actor_number);
				if (it != entityMap.end() && !it->second.name.empty())
					actorName = it->second.name;
				else
					actorName = std::to_string(block.actor_number);

				std::string filename = actorName + ".opcodes.txt";
				std::ofstream file(outPath / filename);
				if (!file.is_open())
				{
					std::cerr << "[ERROR] Cannot write " << filename << std::endl;
					continue;
				}

				file << "; Zone: " << name << std::endl;
				file << "; Actor: " << actorName << " (0x" << std::hex << block.actor_number << std::dec << ")" << std::endl;
				file << "; imed_data: [";
				for (size_t i = 0; i < block.imed_data.size(); ++i)
				{
					if (i > 0) file << ", ";
					file << block.imed_data[i];
				}
				file << "]" << std::endl;
				file << "; Events: " << block.events.size() << std::endl;
				file << std::endl;

				for (const auto& evt : block.events)
				{
					file << "--- Event index=" << evt.array_index
						<< " id=" << evt.event_id
						<< " offset=" << evt.byte_offset
						<< " size=" << evt.byte_size << " ---" << std::endl;

					auto lines = analyzer.Disassemble(
						evt.bytecode, block.actor_number, block.imed_data,
						entityMap, zoneStrings, evt.byte_offset);

					for (const auto& line : lines)
						file << line << std::endl;

					file << std::endl;
				}

				std::cout << "[DUMP] " << filename << " (" << block.events.size() << " events)" << std::endl;
			}
			return 0;
		}
	}

	// Try by config index
	int idx = 0;
	for (const auto& [name, def] : config)
	{
		++idx;
		if (std::to_string(idx) == zoneNameOrId)
		{
			std::unordered_map<std::string, ZoneDef> single;
			single[name] = def;
			return DumpZoneOpcodes(name, single, outputDir, preferJp);
		}
	}

	std::cout << "[ERROR] Zone not found: " << zoneNameOrId << std::endl;
	return 1;
}

int main(int argc, char** argv)
{
	setlocale(LC_ALL, "");
	GamePathResolver::Init();

	// Initialize CP932 codepage
	{
		std::error_code ec;
		std::filesystem::path cp932Csv;
		auto exeDir = std::filesystem::current_path();
		auto candidates = {
			exeDir / "cp932.csv",
			exeDir / ".." / ".." / "data" / "cp932.csv",
			std::filesystem::path("data") / "cp932.csv",
			std::filesystem::path("..") / "data" / "cp932.csv",
			std::filesystem::path("D:\\Projects\\FFXIDat\\data\\cp932.csv"),
		};
		for (const auto& c : candidates)
		{
			if (std::filesystem::exists(c, ec))
			{
				cp932Csv = c;
				break;
			}
		}
		if (!cp932Csv.empty())
		{
			try { CodeCvt::GetInstance().Init(cp932Csv.wstring()); }
			catch (const std::exception& e)
			{
				std::cerr << "[WARN] Failed to load cp932.csv: " << e.what() << std::endl;
			}
		}
		else
			std::cerr << "[WARN] cp932.csv not found, SJIS text may be garbled." << std::endl;
	}

	std::string outputDir = "event";
	bool pretty = false;
	bool listOnly = false;
	std::string lang = "na"; // "na", "jp", or "all"
	bool dumpOpcodes = false;
	bool splitText = false;
	bool dumpDb = false;
	bool dumpEventJson = false;
	std::string singleTarget;

	for (int i = 1; i < argc; ++i)
	{
		std::string arg = argv[i];
		if (arg == "--help" || arg == "-h")
		{
			PrintHelp(argv[0]);
			return 0;
		}
		else if (arg == "--ffxi-path" && i + 1 < argc)
		{
			GamePathResolver::Init();
			++i;
		}
		else if (arg == "--out" && i + 1 < argc)
			outputDir = argv[++i];
		else if (arg == "--pretty")
			{ pretty = true; continue; }
		else if (arg == "--dump-opcodes")
			{ dumpOpcodes = true; continue; }
		else if (arg == "--dump-db")
			{ dumpDb = true; continue; }
		else if (arg == "--dump-event-json")
			{ dumpEventJson = true; continue; }
		else if (arg == "--split-text")
			splitText = true;
		else if (arg == "--list-zones")
			{ listOnly = true; continue; }
		else if (arg == "--lang" && i + 1 < argc)
		{
			lang = argv[++i];
			if (lang == "ja") lang = "ja";
		}
		else
			singleTarget = arg;
	}

	// --- Locate defs.csv ---
	std::string csvPath;
	{
		wchar_t modulePath[MAX_PATH];
		GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
		auto exeDir = std::filesystem::path(modulePath).parent_path();
		auto p = exeDir / "defs.csv";
		if (std::filesystem::exists(p))
			csvPath = p.string();
		else
		{
			// Dev layout fallback
			for (auto& rel : {
				std::filesystem::path("data/defs.csv"),
				std::filesystem::path("../FFXIDatAdv/data/defs.csv"),
			})
			{
				if (std::filesystem::exists(rel))
				{
					csvPath = rel.string();
					break;
				}
			}
		}
	}

	if (csvPath.empty())
	{
		std::cerr << "[ERROR] defs.csv not found." << std::endl;
		return 1;
	}

	auto config = LoadZoneConfig(csvPath);
	if (config.empty())
	{
		std::cerr << "[ERROR] No zone configuration found." << std::endl;
		return 1;
	}

	if (listOnly)
		return ListZones(config);

	if (dumpOpcodes && !singleTarget.empty())
	{
		return DumpZoneOpcodes(singleTarget, config, outputDir, lang == "ja");
	}

	if (dumpDb)
	{
		if (!singleTarget.empty())
		{
			auto it = config.find(singleTarget);
			if (it == config.end())
			{
				std::cerr << "[ERROR] Zone not found: " << singleTarget << std::endl;
				return 1;
			}
			std::unordered_map<std::string, ZoneDef> single;
			single[singleTarget] = it->second;
			return DumpDatabase(single, csvPath, outputDir, false, lang == "ja");
		}
		return DumpDatabase(config, csvPath, outputDir, true, lang == "ja");
	}

	if (dumpEventJson)
	{
		return DumpEventJson(config, csvPath, outputDir);
	}

	if (!singleTarget.empty())
	{
		return RunSingleZone(singleTarget, config, outputDir, pretty, lang, splitText, dumpOpcodes);
	}

	if (lang == "all" && !splitText)
	{
		std::cerr << "[ERROR] --lang all requires --split-text" << std::endl;
		return 1;
	}

	return RunAllZones(config, outputDir, pretty, lang, splitText, dumpOpcodes);
}
