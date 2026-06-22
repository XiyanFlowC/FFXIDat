#include "ApplicationAdv.h"
#include "Config.h"
#include "Logger.h"
#include "TranslationDatabase.h"
#include "ProcessorFactory.h"
#include "FinalTextProcessor.h"
#include "ChsToSJis.h"
#include "CsvFile.h"
#include "../FFXIDatProcessor/codepage.h"
#include "EventFileProcessor.h"
#include "BackupManager.h"
#include "SetupWizard.h"
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <xystring.h>
#include <EventStringBase.h>
#include "EventDump/EventBinaryDat.h"
#include "EventDump/EntityDat.h"
#include "EventDump/BytecodeAnalyzer.h"
#include "EventDump/GamePathResolver.h"
#include "EventTextOut.h"
#include "EventDefs.h"
#include "ProcessorUtils.h"
#include "SpecialProcessor.h"
#include "../FFXIDat/ItemData.h"
#include "../FFXIDat/DMsg.h"
#include "../FFXIDat/FixedPhrase.h"
#include "../FFXIDat/RecordsOfEminence.h"

ApplicationAdv& ApplicationAdv::Instance()
{
	static ApplicationAdv instance;
	return instance;
}

static std::filesystem::path GetExeDir()
{
	wchar_t buf[MAX_PATH];
	if (GetModuleFileNameW(nullptr, buf, MAX_PATH) == 0)
		return std::filesystem::current_path();
	return std::filesystem::path(buf).parent_path();
}

bool ApplicationAdv::Initialize()
{
	exeDir_ = GetExeDir();

	Logger::Instance().Initialize(exeDir_ / "log.txt");
	Logger::Instance().Info("FFXITransAdv start");

	if (!Config::Instance().Initialize())
	{
		Logger::Instance().Error("Config init failed");
		return false;
	}

	auto configIni = exeDir_ / "config.ini";
	if (std::filesystem::exists(configIni))
		Config::Instance().LoadFromFile(configIni);

	auto cp932 = exeDir_ / "cp932.csv";
	if (std::filesystem::exists(cp932))
	{
		try { CodeCvt::GetInstance().Init(cp932.wstring()); }
		catch (const std::exception& e)
		{
			Logger::Instance().Error(std::string("cp932.csv: ") + e.what());
		}
	}

	auto chs2sjis = exeDir_ / "chs2sjis.csv";
	if (std::filesystem::exists(chs2sjis))
	{
		try { ChsToSJis::Instance().Init(chs2sjis); }
		catch (const std::exception& e)
		{
			Logger::Instance().Error(std::string("chs2sjis.csv: ") + e.what());
		}
	}

	TranslationDatabase::Instance().InitializeMismatchLog(exeDir_ / "text_mismatch.txt");
	FinalTextProcessor::ResetValidationSummary();
	return true;
}

static bool LoadGlobalTranslations()
{
	auto& db = TranslationDatabase::Instance();
	for (int i = 0;; ++i)
	{
		int loaded = db.LoadText(i);
		if (loaded < 0) return false;
		if (loaded == 0) break;
	}
	int srcCount = db.LoadSourceData();
	if (srcCount < 0) return false;
	if (srcCount > 0)
		Logger::Instance().Info("Source data: " + std::to_string(srcCount) + " entries");
	return true;
}

static int TryLoadCommentTranslation(const std::u8string& comment)
{
	auto& db = TranslationDatabase::Instance();
	const auto& progRoot = Config::Instance().GetProgRoot();

	auto srcPath = progRoot / L"text" / L"src" / (xybase::string::to_wstring(comment) + L".txt");
	auto tgtPath = progRoot / L"text" / L"tgt" / (xybase::string::to_wstring(comment) + L".txt");

	if (!std::filesystem::exists(srcPath) || !std::filesystem::exists(tgtPath))
		return 0;

	int loaded = db.LoadTextPair(srcPath, tgtPath);
	if (loaded > 0)
		std::cout << " [" << loaded << "tx]";
	return loaded;
}

static std::unordered_set<std::string> BuildPairedEvsbSet();

static void ShowInfoMessage(const std::wstring& msg)
{
	MessageBoxW(nullptr, msg.c_str(), L"FFXI翻译工具", MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TOPMOST);
}

int YesNoPrompt(const std::wstring& prompt)
{
	int result = MessageBoxW(nullptr, prompt.c_str(), L"FFXI翻译工具",
		MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 | MB_SETFOREGROUND | MB_TOPMOST);
	return result == IDYES ? 'Y' : 'N';
}

int ApplicationAdv::Run(bool interactive)
{
	if (!Initialize()) return 1;

	if (!LoadGlobalTranslations()) return 1;

	SetupWizard::RunIfConfigMissing(exeDir_);

	const bool configForcesInSitu = Config::Instance().IsInSituMode();
	bool overwrite = configForcesInSitu;
	if (!overwrite)
	{
		if (!Config::Instance().IsInSituNoPrompt())
		{
			auto r = MessageBoxW(nullptr,
				L"是否原位修改游戏文件？\n\n"
				L"选择\"否\"将输出到 output 目录，推荐先测试。",
				L"FFXI翻译工具",
				MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 | MB_SETFOREGROUND | MB_TOPMOST);
			overwrite = (r == IDYES);
		}
	}

	if (overwrite)
	{
		{
			auto r = MessageBoxW(nullptr,
				L"【危险！InSitu 模式】\n\n"
				L"当前会直接覆盖游戏目录中的 DAT 文件。\n"
				L"只有在您完全知道自己在做什么时，才应该使用。\n\n"
				L"强烈建议先使用 output 模式验证，再考虑 InSitu。\n\n"
				L"是否确认进入 InSitu 模式？",
				L"FFXI翻译工具",
				MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2 | MB_SETFOREGROUND | MB_TOPMOST);
			if (r != IDYES)
			{
				ShowInfoMessage(L"已取消 InSitu 模式，这次不会覆盖游戏原始文件。");
				TranslationDatabase::Instance().CloseMismatchLog();
				Logger::Instance().Info("User cancelled InSitu mode.");
				Logger::Instance().Close();
				return 0;
			}
		}

		{
			auto r = MessageBoxW(nullptr,
				L"请再次确认：\n\n"
				L"1. 游戏和 PlayOnline 已完全关闭。\n"
				L"2. backup 目录只能存放由程序自动管理的原始 DAT 数据。\n"
				L"3. 只要游戏更新过，backup 即全部失效——请删除 backup 并重新运行。\n"
				L"4. 确认您已经有数据备份以免意外。\n\n"
				L"是否确认覆盖原始游戏文件？",
				L"FFXI翻译工具",
				MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2 | MB_SETFOREGROUND | MB_TOPMOST);
			if (r != IDYES)
			{
				ShowInfoMessage(L"已取消 InSitu 模式，这次不会覆盖游戏原始文件。\n\n您可以继续使用 output 模式测试。");
				TranslationDatabase::Instance().CloseMismatchLog();
				Logger::Instance().Info("User cancelled InSitu mode at second confirmation.");
				Logger::Instance().Close();
				return 0;
			}
		}

		Config::Instance().SetInSituMode(true);
		std::wcout << L"正在原位修改游戏文件，文件修改前请确认/恢复备份。" << std::endl;
		std::wcout << L"注意：如果游戏已经更新，务必删除 backup 目录后重新运行。" << std::endl;
	}
	else
	{
		Config::Instance().SetInSituMode(false);
	}
	Logger::Instance().Info(std::string("Output mode: ") + (overwrite ? "in-place" : "output directory"));

	BackupManager::Instance().PromptAndRestore();

	// Read defs.csv
	auto defsPath = exeDir_ / "defs.csv";
	if (!std::filesystem::exists(defsPath))
	{
		Logger::Instance().Error("defs.csv not found at " + Logger::ToUtf8(defsPath));
		return 1;
	}

	CsvFile defs(defsPath, std::ios::in | std::ios::binary);
	std::vector<FileProcessDef> fileDefs;

	while (!defs.IsEof())
	{
		FileProcessDef fd;
		fd.path = defs.NextCell();
		fd.type = defs.NextCell();
		fd.lang = LangCode::Normalize(defs.NextCell());
		fd.comment = defs.NextCell();
		if (!defs.IsEol())
			fd.cellIndicesStr = defs.NextCell();
		defs.NextLine();
		if (fd.path.empty() || fd.type.empty() || fd.comment.empty())
			continue;
		fileDefs.push_back(fd);
	}

	const auto& gameRoot = Config::Instance().GetGameRoot();
	bool isEn = Config::Instance().IsEnglishMode();
	overwrite = Config::Instance().IsInSituMode();
	const auto& outRoot = overwrite ? gameRoot : Config::Instance().GetOutRoot();
	auto pairedEvsb = BuildPairedEvsbSet();
	auto specialProc = std::make_unique<SpecialProcessor>();

	int total = 0, processed = 0, errors = 0;
	for (const auto& def : fileDefs)
	{
		bool isTarget = isEn ? (def.lang == LangCode::EN_U8) : (def.lang == LangCode::JA_U8 || def.lang == LangCode::EN_U8);
		if (isTarget) total++;
	}

	for (const auto& def : fileDefs)
	{
		bool isTarget = isEn ? (def.lang == LangCode::EN_U8) : (def.lang == LangCode::JA_U8 || def.lang == LangCode::EN_U8);
		if (!isTarget) continue;

		processed++;
		std::cout << "[" << processed << "/" << total << "] " << xybase::string::to_string(def.comment) << std::flush;

		TryLoadCommentTranslation(def.comment);

		auto datPath = gameRoot / (def.path + u8".DAT");
		auto outPath = overwrite ? datPath : outRoot / (def.path + u8".DAT");

		if (!std::filesystem::exists(datPath))
		{
			std::cout << " [SKIP]" << std::endl;
			Logger::Instance().Warning("DAT not found: " + Logger::ToUtf8(datPath));
			continue;
		}

		std::filesystem::create_directories(outPath.parent_path());

		// Skip evev and evac — they are metadata files, not translation targets
		if (def.type == u8"evev" || def.type == u8"evac")
		{
			std::cout << " [SKIP]" << std::endl;
			continue;
		}

		// Build jpDefs (lazily) for Japanese reference lookup
		static std::map<std::u8string, FileProcessDef> jpDefsByComment = [&fileDefs]() {
			std::map<std::u8string, FileProcessDef> jp;
			for (const auto& fd : fileDefs)
				if (fd.lang == u8"ja")
					jp[fd.comment] = fd;
			return jp;
			}();

		// Pre-process: try EjrefToleranceProcessor first
		static auto ejrefProc = ProcessorFactory::Instance().GetEjrefToleranceProcessor();
		if (ejrefProc)
		{
			try {
				if (ejrefProc->Process(def, datPath, outPath, jpDefsByComment))
				{
					std::cout << " OK" << std::endl;
					continue;
				}
			}
			catch (...) {}
		}

		if (specialProc)
		{
			try {
				if (specialProc->Process(def, datPath, outPath, jpDefsByComment))
				{
					std::cout << " OK" << std::endl;
					continue;
				}
			}
			catch (...) {}
		}


		// For evsb: paired → EventFileProcessor (patches + TranslationDB), orphan → EvsbProcessor
		if (def.type == u8"evsb")
		{
			/*static bool pairedBuilt = false;
			static std::unordered_set<std::string> pairedEvsb;
			if (!pairedBuilt) { pairedEvsb = BuildPairedEvsbSet(); pairedBuilt = true; }*/

			std::string canonPath = xybase::string::to_string(def.path);
			for (auto& c : canonPath) c = toupper((unsigned char)c);
			if (canonPath.size() > 4 && canonPath.substr(canonPath.size() - 4) == ".DAT")
				canonPath = canonPath.substr(0, canonPath.size() - 4);

			bool paired = pairedEvsb.count(canonPath) > 0;

			if (paired)
			{
				EventFileProcessor efp;
				if (efp.Process(def, datPath, outPath, jpDefsByComment))
				{
					std::cout << " OK" << std::endl;
					continue;
				}
			}
		}

		auto processor = ProcessorFactory::Instance().GetProcessor(def.type);
		if (!processor)
		{
			continue;
		}

		try
		{
			processor->Process(def, datPath, outPath, jpDefsByComment);
			std::cout << " OK" << std::endl;
		}
		catch (const std::exception& e)
		{
			std::cout << " ERR" << std::endl;
			Logger::Instance().Error(std::string("  ") + e.what());
			Logger::Instance().Error(std::string("Process ") + Logger::ToUtf8(def.comment) + ": " + e.what());
			errors++;
		}
	}

	TranslationDatabase::Instance().CloseMismatchLog();
	Logger::Instance().Info("Done. processed=" + std::to_string(processed) + " errors=" + std::to_string(errors));
	Logger::Instance().Close();

	std::cout << "\n[DONE] " << processed << " files, " << errors << " errors"
		<< ", mismatches: " << TranslationDatabase::Instance().GetMismatchCount() << std::endl;

	ShowInfoMessage(
		L"处理完毕。\n\n"
		L"共处理 " + std::to_wstring(processed) + L" 个文件，" +
		std::to_wstring(errors) + L" 个错误。\n"
		L"文本失配数量：" + std::to_wstring(TranslationDatabase::Instance().GetMismatchCount()) +
		L"\n失配文本已保存到 text_mismatch.txt 中。");

	return errors;
}

// --- Write UTF-8 BOM lines to file ---
static void WriteUtf8Lines(const std::filesystem::path& path, const std::vector<std::u8string>& lines)
{
	std::filesystem::create_directories(path.parent_path());
	std::ofstream out(path, std::ios::out | std::ios::binary);
	if (!out.is_open()) return;
	out.write("\xEF\xBB\xBF", 3);
	for (const auto& line : lines)
	{
		out.write(reinterpret_cast<const char*>(line.data()), line.size());
		out << "\n";
	}
}

// --- Build a set of evsb paths that have paired evev files ---
static std::unordered_set<std::string> BuildPairedEvsbSet()
{
	std::unordered_set<std::string> paired;
	for (const auto& [name, zf] : ZoneRegistry::Instance().AllZones())
	{
		if (zf.evev_path.empty()) continue;
		if (!zf.evsb_path.empty())
		{
			std::string r = zf.evsb_path;
			for (auto& c : r) c = toupper((unsigned char)c);
			if (r.size() > 4 && r.substr(r.size() - 4) == ".DAT") r = r.substr(0, r.size() - 4);
			paired.insert(r);
		}
	}
	return paired;
}

// --- Phase 1: Extract text from all evev files ---
// Returns all unique text lines exported (for dedup)
static std::unordered_set<std::u8string> ExtractEvevText(const std::vector<FileProcessDef>& evDefs, const std::string& lang)
{
	(void)evDefs;
	Logger::Instance().Info("Phase 1: Evev extraction start");
	std::cout << "Phase 1: Processing evev files..." << std::endl;

	// Run event text extraction via EventTextOut → text/src_{lang}/event/
	auto langW = std::wstring(lang.begin(), lang.end());
	auto evevOutDir = Config::Instance().GetProgRoot() / L"text" / (L"src_" + langW);
	EventTextOut extractor(evevOutDir);
	extractor.SetEvsbLang(lang);
	auto zones = ZoneRegistry::Instance().AllZones();
	extractor.RunAllZones(zones);

	// Collect all unique text lines from event data
	std::unordered_set<std::u8string> allLines;
	for (const auto& [name, zf] : zones)
	{
		if (zf.evev_path.empty()) continue;
		std::string evevPath = GamePathResolver::ResolvePath(zf.evev_path);
		std::string evacPath = zf.evac_path.empty() ? "" : GamePathResolver::ResolvePath(zf.evac_path);
		auto evsbRaw = EventTextOut::GetEvsbPathForLang(zf, lang);
		std::string evsbPath = evsbRaw.empty() ? "" : GamePathResolver::ResolvePath(evsbRaw);

		auto actors = EventBinaryDat::Parse(evevPath);
		if (actors.empty()) continue;

		std::unordered_map<uint32_t, EntityEntry> entityMap;
		if (!evacPath.empty())
		{
			for (auto& e : EntityDat::Parse(evacPath))
				entityMap[e.entity_id] = std::move(e);
		}

		std::vector<std::u8string> zoneStrings;
		if (!evsbPath.empty())
		{
			EventStringBase esb(xybase::string::sys_mbs_to_wcs(evsbPath));
			esb.Read();
			for (size_t i = 0; i < esb.Size(); ++i)
				zoneStrings.push_back(esb[i]);
		}

		BytecodeAnalyzer analyzer;
		for (const auto& block : actors)
		{
			for (const auto& evt : block.events)
			{
				std::vector<DialogueLine> dls;
				analyzer.ExtractDialogues(evt.bytecode, block.actor_number,
					block.imed_data, entityMap, zoneStrings, dls, evt.byte_offset);

				for (const auto& dl : dls)
				{
					// Only dedup the text content (not speaker)
					if (!dl.text.empty())
						allLines.insert(dl.text);
				}
			}
		}
	}

	Logger::Instance().Info("Phase 1 done. Unique event lines: " + std::to_string(allLines.size()));
	std::cout << "Phase 1 done. " << allLines.size() << " unique event lines extracted." << std::endl;
	return allLines;
}

// --- Phase 2: Export orphan evsb files, filtered by event dedup set ---
static void ExportOrphanEvsb(
	const std::vector<FileProcessDef>& evsbDefs,
	const std::unordered_set<std::u8string>& eventLines,
	const std::filesystem::path& outputDir)
{
	Logger::Instance().Info("Phase 2: Orphan evsb export start");
	std::cout << "Phase 2: Processing orphan evsb files..." << std::endl;

	auto pairedEvsb = BuildPairedEvsbSet();

	// Also track lines already exported by evev as u8string set for ExportEventStringBase
	std::set<std::u8string> dedupSet;
	for (const auto& line : eventLines)
		dedupSet.insert(line);

	int exported = 0;
	for (const auto& def : evsbDefs)
	{
		// Check if this evsb has a paired evev (canonical path match)
		std::string pathStr = xybase::string::to_string(def.path);
		for (auto& c : pathStr) c = toupper((unsigned char)c);
		if (pathStr.size() > 4 && pathStr.substr(pathStr.size() - 4) == ".DAT")
			pathStr = pathStr.substr(0, pathStr.size() - 4);
		if (pairedEvsb.count(pathStr))
		{
			// Has paired evev — skip (text already exported via event extraction)
			continue;
		}

		// Orphan evsb: export strings, deduped
		const auto& progRoot = Config::Instance().GetGameRoot();
		auto datPath = progRoot / (def.path + u8".DAT");
		auto outPath = outputDir / (xybase::string::to_wstring(def.comment) + L".txt");

		if (!std::filesystem::exists(datPath))
		{
			Logger::Instance().Warning("DAT not found: " + Logger::ToUtf8(datPath));
			continue;
		}

		try
		{
			EventStringBase esb(datPath);
			esb.Read();

			std::vector<std::u8string> extracted;
			for (const auto& str : esb)
			{
				if (dedupSet.insert(str).second)
					extracted.push_back(str);
			}

			if (!extracted.empty())
			{
				WriteUtf8Lines(outPath, extracted);
				exported += static_cast<int>(extracted.size());
				std::cout << "  " << xybase::string::to_string(def.comment) << ": "
					<< extracted.size() << " lines" << std::endl;
			}
		}
		catch (const std::exception& e)
		{
			Logger::Instance().Error("Orphan evsb export failed for " + Logger::ToUtf8(def.comment) + ": " + e.what());
		}
	}

	Logger::Instance().Info("Phase 2 done. Orphan evsb lines exported: " + std::to_string(exported));
	std::cout << "Phase 2 done." << std::endl;
}

// --- Phase 3: Export other types (xis, dmsg, item, fp, roe, ...) ---
static void ExportOtherTypes(
	const std::vector<FileProcessDef>& allDefs,
	const std::filesystem::path& outputDir)
{
	Logger::Instance().Info("Phase 3: Other types export start");
	std::cout << "Phase 3: Processing xis/dmsg/item/fp/roe/..." << std::endl;

	const auto& gameRoot = Config::Instance().GetGameRoot();
	std::set<std::u8string> processedStrings;

	auto getItemSpecType = [](const std::u8string& type) -> ItemSpecType {
		if (type == u8"iab") return ItemSpecType::ARMOUR;
		if (type == u8"iwb") return ItemSpecType::WEAPON;
		if (type == u8"iub") return ItemSpecType::USABLE;
		if (type == u8"ipb") return ItemSpecType::PUPPET;
		if (type == u8"isb") return ItemSpecType::SLIP;
		if (type == u8"icb") return ItemSpecType::CURRENCY;
		if (type == u8"iib") return ItemSpecType::INSTINCT;
		return ItemSpecType::NORMAL;
	};

	auto isQuestDMsg = [](const std::u8string& comment) -> bool {
		return comment.starts_with(u8"sys/mis/") || comment.starts_with(u8"sys/qst/");
	};

	int count = 0;
	for (const auto& def : allDefs)
	{
		// Skip evev/evac (metadata) and evsb (handled in Phase 1/2)
		if (def.type == u8"evev" || def.type == u8"evac" || def.type == u8"evsb")
			continue;

		auto datPath = gameRoot / (def.path + u8".DAT");
		if (!std::filesystem::exists(datPath))
		{
			Logger::Instance().Warning("DAT not found for " + Logger::ToUtf8(def.comment));
			continue;
		}

		auto outPath = outputDir / (xybase::string::to_wstring(def.comment));

		try
		{
			bool isItem = def.type == u8"iab" || def.type == u8"iwb" || def.type == u8"iub"
				|| def.type == u8"inb" || def.type == u8"ipb" || def.type == u8"isb"
				|| def.type == u8"icb" || def.type == u8"iib";

			if (isItem)
			{
				outPath += L".csv";
				std::filesystem::create_directories(outPath.parent_path());

				ItemData itemData;
				itemData.Read(datPath.wstring(), getItemSpecType(def.type));

				CsvFile csv(outPath, std::ios::out | std::ios::binary);
				csv.NewCell(u8"ID");
				csv.NewCell(u8"Name");
				csv.NewCell(u8"Description");
				csv.NewLine();

				int rows = 0;
				for (const auto& datum : itemData.data)
				{
					try {
						if (datum.name() == u8".")
							continue;
						csv.NewCell(xybase::string::itos<char8_t>(datum.id));
						csv.NewCell(datum.name());
						csv.NewCell(datum.description());
						csv.NewLine();
						++rows;
					} catch (...) {}
				}
				std::cout << "  " << xybase::string::to_string(def.comment) << ": "
					<< rows << " items" << std::endl;
				++count;
			}
			else if (def.type == u8"dmsg" && isQuestDMsg(def.comment))
			{
				outPath += L".csv";
				std::filesystem::create_directories(outPath.parent_path());

				DMsg data(datPath);
				data.Read();

				CsvFile csv(outPath, std::ios::out | std::ios::binary);
				csv.NewCell(u8"ID");
				csv.NewCell(u8"Name");
				csv.NewCell(u8"Desc");
				csv.NewLine();

				int rows = 0;
				for (const auto& row : data)
				{
					const auto& cells = row.GetCellsConst();
					if (cells.size() < 3 || cells[0].GetType() != 1)
						continue;

					int id = cells[0].Get<int>();
					std::u8string title = cells[1].GetType() == 0 ? cells[1].Get<std::u8string>() : std::u8string{};
					std::u8string desc = cells[2].GetType() == 0 ? cells[2].Get<std::u8string>() : std::u8string{};

					if ((def.comment == u8"sys/mis/ad" || def.comment == u8"sys/mis/rov")
						&& !title.empty() && !title.starts_with(u8"__"))
					{
						id = -id;
					}

					auto id8 = std::to_string(id);
					csv.NewCell(std::u8string(id8.begin(), id8.end()));
					csv.NewCell(title);
					csv.NewCell(desc);
					csv.NewLine();
					++rows;
				}
				std::cout << "  " << xybase::string::to_string(def.comment) << ": "
					<< rows << " entries" << std::endl;
				++count;
			}
			else if (def.type == u8"dmsg")
			{
				// Non-quest DMsg (sys/key_item, sys/region, etc.) — TXT via CollectStrings
				outPath += L".txt";
				std::filesystem::create_directories(outPath.parent_path());

				auto strings = ProcessorUtils::CollectStrings(datPath, def.type, def.cellIndicesStr);
				std::vector<std::u8string> extracted;
				for (const auto& s : strings)
				{
					if (processedStrings.insert(s).second)
						extracted.push_back(s);
				}
				if (!extracted.empty())
				{
					WriteUtf8Lines(outPath, extracted);
					std::cout << "  " << xybase::string::to_string(def.comment) << ": "
						<< extracted.size() << " lines" << std::endl;
					++count;
				}
			}
			else if (def.type == u8"erq")
			{
				outPath += L".csv";
				std::filesystem::create_directories(outPath.parent_path());

				RecordsOfEminence roe;
				roe.ReadQuest(datPath.wstring());

				CsvFile csv(outPath, std::ios::out | std::ios::binary);
				csv.NewCell(u8"ID");
				csv.NewCell(u8"QuestName");
				csv.NewCell(u8"Description");
				csv.NewCell(u8"Note");
				csv.NewLine();

				for (const auto& entry : roe.questData)
				{
					csv.NewCell(xybase::string::itos<char8_t>(entry.id));
					try { csv.NewCell(entry.questName()); } catch (...) { csv.NewCell(u8""); }
					try { csv.NewCell(entry.description()); } catch (...) { csv.NewCell(u8""); }
					try { csv.NewCell(entry.note()); } catch (...) { csv.NewCell(u8""); }
					csv.NewLine();
				}
				std::cout << "  " << xybase::string::to_string(def.comment) << ": "
					<< roe.questData.size() << " entries" << std::endl;
				++count;
			}
			else if (def.type == u8"erc")
			{
				outPath += L".csv";
				std::filesystem::create_directories(outPath.parent_path());

				RecordsOfEminence roe;
				roe.ReadCategory(datPath.wstring());

				CsvFile csv(outPath, std::ios::out | std::ios::binary);
				csv.NewCell(u8"ID");
				csv.NewCell(u8"CategoryName");
				csv.NewLine();

				for (const auto& entry : roe.categoryData)
				{
					csv.NewCell(xybase::string::itos<char8_t>(entry.id));
					try {
						std::u8string name = entry.categoryName();
						csv.NewCell(name);
					} catch (...) {
						csv.NewCell(u8"");
					}
					csv.NewLine();
				}
				std::cout << "  " << xybase::string::to_string(def.comment) << ": "
					<< roe.categoryData.size() << " entries" << std::endl;
				++count;
			}
			else if (def.type == u8"fp")
			{
				outPath += L".csv";
				std::filesystem::create_directories(outPath.parent_path());

				FixedPhrase fp;
				fp.Read(datPath.wstring());
				fp.ToCsv(outPath.wstring());

				int total = 0;
				for (const auto& cat : fp.categories)
					total += 1 + static_cast<int>(cat.entries.size());
				std::cout << "  " << xybase::string::to_string(def.comment) << ": "
					<< total << " entries" << std::endl;
				++count;
			}
			else if (def.type == u8"xis")
			{
				outPath += L".txt";
				std::filesystem::create_directories(outPath.parent_path());

				auto strings = ProcessorUtils::CollectStrings(datPath, def.type, def.cellIndicesStr);
				std::vector<std::u8string> extracted;
				for (const auto& s : strings)
				{
					if (processedStrings.insert(s).second)
						extracted.push_back(s);
				}
				if (!extracted.empty())
				{
					WriteUtf8Lines(outPath, extracted);
					std::cout << "  " << xybase::string::to_string(def.comment) << ": "
						<< extracted.size() << " lines" << std::endl;
					++count;
				}
			}
			else
			{
				// sd, mbd, regular dmsg → TXT via CollectStrings
				outPath += L".txt";
				std::filesystem::create_directories(outPath.parent_path());

				auto strings = ProcessorUtils::CollectStrings(datPath, def.type, def.cellIndicesStr);
				std::vector<std::u8string> extracted;
				for (const auto& s : strings)
				{
					if (processedStrings.insert(s).second)
						extracted.push_back(s);
				}
				if (!extracted.empty())
				{
					WriteUtf8Lines(outPath, extracted);
					std::cout << "  " << xybase::string::to_string(def.comment) << ": "
						<< extracted.size() << " lines" << std::endl;
					++count;
				}
			}
		}
		catch (const std::exception& e)
		{
			Logger::Instance().Warning("ExportOtherTypes failed for " + Logger::ToUtf8(def.comment) + ": " + e.what());
		}
	}

	Logger::Instance().Info("Phase 3 done. Exported " + std::to_string(count) + " files.");
	std::cout << "Phase 3 done. Exported " << count << " files." << std::endl;
}

// --- Prepare mode entry point ---
int ApplicationAdv::PrepareSourceData(const std::string& lang)
{
	if (!Initialize())
	{
		Logger::Instance().Error("PrepareSourceData: initialization failed.");
		return 1;
	}

	GamePathResolver::Init();

	const auto& progRoot = Config::Instance().GetProgRoot();
	auto defsPath = exeDir_ / "defs.csv";
	if (!std::filesystem::exists(defsPath))
	{
		Logger::Instance().Error("defs.csv not found.");
		return 1;
	}

	// Read all defs
	CsvFile defs(defsPath, std::ios::in | std::ios::binary);
	std::vector<FileProcessDef> fileDefs;
	while (!defs.IsEof())
	{
		FileProcessDef fd;
		fd.path = defs.NextCell();
		fd.type = defs.NextCell();
		fd.lang = LangCode::Normalize(defs.NextCell());
		fd.comment = defs.NextCell();
		if (!defs.IsEol()) fd.cellIndicesStr = defs.NextCell();
		defs.NextLine();
		if (fd.path.empty() || fd.type.empty() || fd.comment.empty()) continue;
		fileDefs.push_back(fd);
	}

	// Filter by source language (default: ja)
	std::u8string targetLang = (lang == "en") ? std::u8string(LangCode::EN_U8) : std::u8string(LangCode::JA_U8);
	std::vector<FileProcessDef> jpDefs;
	for (const auto& fd : fileDefs)
	{
		if (fd.lang != targetLang) continue;
		auto datPath = Config::Instance().GetGameRoot() / (fd.path + u8".DAT");
		if (!std::filesystem::exists(datPath)) continue;
		jpDefs.push_back(fd);
	}

	auto langW = std::wstring(targetLang.begin(), targetLang.end());
	auto outputDir = progRoot / L"text" / (L"src_" + langW);
	std::filesystem::create_directories(outputDir);

	std::cout << "Prepare mode started. " << jpDefs.size() << " files to process (" << lang << ")." << std::endl;

	// Separate evev defs
	std::vector<FileProcessDef> evevDefs, evsbDefs, otherDefs;
	for (const auto& fd : jpDefs)
	{
		if (fd.type == u8"evev") evevDefs.push_back(fd);
		else if (fd.type == u8"evsb") evsbDefs.push_back(fd);
		else otherDefs.push_back(fd);
	}

	// Phase 1: Evev → extract event text + build dedup set
	auto eventLines = ExtractEvevText(evevDefs, lang);

	// Phase 2: Orphan evsb → export filtered by event dedup
	ExportOrphanEvsb(evsbDefs, eventLines, outputDir);

	// Phase 3: Other types
	ExportOtherTypes(otherDefs, outputDir);

	std::cout << "\nPrepare done." << std::endl;
	Logger::Instance().Info("PrepareSourceData completed.");
	return 0;
}
