#include "EventProcessor.h"
#include "../FinalTextProcessor.h"
#include "../TranslationDatabase.h"
#include "../Config.h"
#include "../Logger.h"
#include "../ProcessorUtils.h"
#include "../../FFXIDat/ZoneEventImage.h"
#include "../../FFXIDat/ZoneActor.h"
#include <EventStringBase.h>
#include <xystring.h>
#include <algorithm>
#include <fstream>
#include <set>
#include <map>
#include <sstream>

namespace fs = std::filesystem;

static std::string SafeName(const std::string& s)
{
	std::string r;
	for (char c : s)
	{
		if (c < 0x20 || c == '\\' || c == '/' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
			r += '_';
		else
			r += c;
	}
	return r;
}

static std::string BuildDatPath(const std::string& romPath)
{
	auto root = Config::Instance().GetGameRoot();

	if (romPath.ends_with(".DAT"))
		return (root / romPath).string();
	else
		return (root / (romPath + ".DAT")).string();
}

static std::vector<std::string> ReadTextLines(const fs::path& filePath)
{
	std::vector<std::string> lines;
	std::ifstream f(filePath);
	if (!f.is_open())
		return lines;
	std::string line;
	while (std::getline(f, line))
		lines.push_back(line);
	return lines;
}

struct ZonePaths
{
	std::string evev_path;
	std::string evac_path;
	std::string evsb_ja_path;
	std::string evsb_en_path;
};

class DefsCache
{
public:
	static DefsCache& Get()
	{
		static DefsCache instance;
		return instance;
	}

	const ZonePaths* Find(const std::string& zoneName) const
	{
		auto it = zones_.find(zoneName);
		return it != zones_.end() ? &it->second : nullptr;
	}

	void Load(const fs::path& csvPath)
	{
		if (loaded_) return;
		loaded_ = true;

		std::ifstream f(csvPath);
		if (!f.is_open()) return;

		std::string line;
		while (std::getline(f, line))
		{
			if (line.empty() || line[0] == '#') continue;

			std::vector<std::string> cols;
			std::stringstream ss(line);
			std::string col;
			while (std::getline(ss, col, ','))
				cols.push_back(col);

			if (cols.size() < 4) continue;
			auto& path = cols[0];
			auto& type = cols[1];
			auto& lang = cols[2];
			auto& name = cols[3];

			auto& zp = zones_[name];
			if (type == "evev") zp.evev_path = path;
			else if (type == "evac") zp.evac_path = path;
			else if (type == "evsb" && (lang == "ja" || lang == "jp"))
				zp.evsb_ja_path = path;
			else if (type == "evsb" && (lang == "en" || lang == "na"))
				zp.evsb_en_path = path;
		}
	}

private:
	bool loaded_ = false;
	std::unordered_map<std::string, ZonePaths> zones_;
};

bool EventProcessor::Process(
	const FileProcessDef& fileDef,
	const fs::path& datPath,
	const fs::path& outPath,
	const std::map<std::u8string, FileProcessDef>& jpDefsByComment)
{
	auto& db = TranslationDatabase::Instance();
	auto& cfg = Config::Instance();

	EventStringBase evsb(datPath);
	evsb.Read();

	FinalTextProcessor finalTextProcessor(fileDef.comment, fileDef.type);

	std::string commentStr = xybase::string::to_string(fileDef.comment);
	std::string zoneName;
	{
		auto slashPos = commentStr.rfind('/');
		zoneName = (slashPos != std::string::npos) ? commentStr.substr(slashPos + 1) : commentStr;
	}

	db.LoadLocalScope(
		cfg.GetProgRoot() / L"text" / L"src" / (fileDef.comment + std::u8string(u8".txt")),
		cfg.GetProgRoot() / L"text" / L"tgt" / (fileDef.comment + std::u8string(u8".txt")));

	auto& defs = DefsCache::Get();
	auto progRoot = cfg.GetProgRoot();
	defs.Load(progRoot / L"defs.csv");

	auto* zone = defs.Find(zoneName);
	if (!zone || zone->evev_path.empty())
	{
		std::vector<std::u8string> referenceTexts;
		bool useJaReference = TryGetJapaneseReference(fileDef, jpDefsByComment, referenceTexts);
		size_t textIdx = 0;
		for (auto& s : evsb)
		{
			std::u8string translated;
			if (useJaReference && textIdx < referenceTexts.size())
			{
				std::u8string refTranslated;
				if (db.TryGetTranslationFromReference(s, referenceTexts[textIdx], refTranslated)
					&& ProcessorUtils::TryAdaptInsCategoryForEnglish(s, refTranslated))
					translated = refTranslated;
				else
					translated = db.GetTranslation(s);
			}
			else
			{
				translated = db.GetTranslation(s);
			}
			s = finalTextProcessor.Process(translated, s, static_cast<int64_t>(textIdx + 1), 1);
			++textIdx;
		}
		evsb.path = outPath;
		evsb.Write();
		db.ClearLocalScope();
		return true;
	}

	std::string evevPath = BuildDatPath(zone->evev_path);

	ZoneEventImage evev;
	if (!evev.Load(evevPath))
	{
		Logger::Instance().Error("EventProcessor: failed to load evev: " + evevPath);
		db.ClearLocalScope();
		return false;
	}

	std::unordered_map<uint32_t, std::string> actorNameMap;
	if (!zone->evac_path.empty())
	{
		std::string evacPath = BuildDatPath(zone->evac_path);
		ZoneActor evac;
		if (evac.Load(evacPath))
			actorNameMap = evac.GetIdToNameMap();
	}

	std::map<size_t, std::string> patches;
	fs::path srcEventBase = progRoot / L"text" / L"src" / L"event";
	fs::path tgtEventBase = progRoot / L"text" / L"tgt" / L"event";

	for (const auto& actor : evev.GetActors())
	{
		auto it = actorNameMap.find(actor.actor_id);
		std::string actorName = (it != actorNameMap.end()) ? it->second : std::to_string(actor.actor_id);
		std::string actorDir = SafeName(actorName) + "_" + std::to_string(actor.actor_id);

		std::vector<uint32_t> sortedConstants = actor.constants;
		std::sort(sortedConstants.begin(), sortedConstants.end());

		std::vector<std::pair<uint32_t, std::u8string>> validIndices;
		for (uint32_t idx : sortedConstants)
		{
			if (idx < evsb.Size())
				validIndices.push_back({ idx, evsb[idx] });
		}

		for (const auto& evt : actor.events)
		{
			std::string evtFile = std::to_string(evt.event_index) + ".txt";

			auto findSrcPath = [&](const fs::path& base) -> fs::path {
				fs::path p1 = base / zoneName / actorDir / evtFile;
				if (fs::exists(p1)) return p1;
				fs::path p2 = base / zoneName / actorName / evtFile;
				if (fs::exists(p2)) return p2;
				fs::path p3 = base / "common" / actorDir / evtFile;
				if (fs::exists(p3)) return p3;
				fs::path p4 = base / "common" / actorName / evtFile;
				if (fs::exists(p4)) return p4;
				return {};
				};

			fs::path srcPath = findSrcPath(srcEventBase);

			if (srcPath.empty())
				continue;

			fs::path tgtPath = tgtEventBase / srcPath.lexically_relative(srcEventBase);

			auto srcLines = ReadTextLines(srcPath);
			auto tgtLines = ReadTextLines(tgtPath);

			if (srcLines.empty() || tgtLines.empty())
				continue;
			if (srcLines.size() != tgtLines.size())
				continue;

			for (size_t i = 0; i < srcLines.size(); ++i)
			{
				const std::string& srcLine = srcLines[i];
				for (const auto& [idx, text] : validIndices)
				{
					if (xybase::string::to_string(text) == srcLine)
					{
						patches[idx] = tgtLines[i];
						break;
					}
				}
			}
		}
	}

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
		s = finalTextProcessor.Process(result, s, static_cast<int64_t>(i + 1), 1);
	}

	evsb.path = outPath;
	evsb.Write();

	db.ClearLocalScope();
	return true;
}
