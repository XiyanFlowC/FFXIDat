#include <iostream>
#include <string>
#include <filesystem>
#include <format>
#include <clocale>
#include <Windows.h>

#include "ApplicationAdv.h"
#include "../FFXIDatAdv/EventDump/GamePathResolver.h"
#include "EventTextOut.h"
#include "EventTextIn.h"
#include "EventFileProcessor.h"
#include "EventDefs.h"
#include "Logger.h"
#include "Config.h"
#include "../FFXIDatProcessor/codepage.h"

static std::filesystem::path GetExeDir()
{
	wchar_t buf[MAX_PATH];
	if (GetModuleFileNameW(nullptr, buf, MAX_PATH) == 0)
		return std::filesystem::current_path();
	return std::filesystem::path(buf).parent_path();
}

static std::filesystem::path FindDefsCsv(const std::filesystem::path& exeDir)
{
	// Always read defs.csv from the executable directory
	return exeDir / "defs.csv";
}

static void ListZones()
{
	const auto& zones = ZoneRegistry::Instance().AllZones();
	std::cout << "Available zones (" << zones.size() << "):" << std::endl;
	int i = 0;
	for (const auto& [name, zf] : zones)
		std::cout << std::format("  {:4d}. {} (evev: {})", ++i, name, zf.evev_path) << std::endl;
}

static void InitCodeCvt()
{
	auto exeDir = GetExeDir();
	auto cp932 = exeDir / "cp932.csv";
	if (std::filesystem::exists(cp932))
	{
		try { CodeCvt::GetInstance().Init(cp932.wstring()); }
		catch (const std::exception& e)
		{ Logger::Instance().Error(std::string("cp932.csv: ") + e.what()); }
	}
}

static std::filesystem::path GetOutputDir()
{
	return Config::Instance().GetProgRoot() / L"text" / L"src_";
}

static void RunExtract(const std::string& target, const std::string& lang)
{
	GamePathResolver::Init();
	InitCodeCvt();
	const auto& zones = ZoneRegistry::Instance().AllZones();
	if (zones.empty()) { Logger::Instance().Error("No zone data"); return; }
	auto langW = std::wstring(lang.begin(), lang.end());
	auto outDir = Config::Instance().GetProgRoot() / L"text" / (L"src_" + langW);
	EventTextOut extractor(outDir);
	extractor.SetEvsbLang(lang);
	if (target.empty()) extractor.RunAllZones(zones);
	else {
		auto* zf = ZoneRegistry::Instance().FindByZoneName(target);
		if (!zf) { Logger::Instance().Error("Zone not found: " + target); return; }
		extractor.RunZone(target, *zf);
	}
}

static void RunApply(const std::string& target)
{
	GamePathResolver::Init();
	InitCodeCvt();
	const auto& zones = ZoneRegistry::Instance().AllZones();
	if (zones.empty()) { Logger::Instance().Error("No zone data"); return; }
	EventTextIn writer(GetOutputDir());
	if (target.empty()) writer.RunAllZones(); else writer.RunZone(target);
}

static void PrintHelp()
{
	std::cout << "FFXITransAdv - FFXI dialog tool" << std::endl;
	std::cout << std::endl;
	std::cout << "Subcommands:" << std::endl;
	std::cout << "  (no args)    Run full processing (FFXITrans compatible)" << std::endl;
	std::cout << "  extract [z]  Extract event dialog to TXT" << std::endl;
	std::cout << "  apply [z]    Write translated TXT back to DAT" << std::endl;
	std::cout << "  prepare      Prepare source text for translation" << std::endl;
	std::cout << "  list         List event zones" << std::endl;
	std::cout << "  help         Show this help" << std::endl;
}

int main(int argc, char** argv)
{
	setlocale(LC_ALL, "");

	auto exeDir = GetExeDir();
	auto defsPath = FindDefsCsv(exeDir);
	if (!defsPath.empty())
		ZoneRegistry::Instance().LoadFromDefsCsv(defsPath);

	if (argc >= 2)
	{
		std::string cmd = argv[1];

		if (cmd == "help" || cmd == "--help" || cmd == "-h") { PrintHelp(); return 0; }

		if (cmd == "list" || cmd == "--list-zones" || cmd == "ls")
		{
			ListZones();
			return 0;
		}

		if (cmd == "extract" || cmd == "--text-out")
		{
			std::string target, lang = "ja";
			for (int i = 2; i < argc; ++i)
			{
				std::string a = argv[i];
				if (a == "--lang" && i + 1 < argc) { lang = argv[++i]; if (lang == "jp") lang = "ja"; }
				else target = a;
			}
			RunExtract(target, lang);
			return 0;
		}

		if (cmd == "apply" || cmd == "--text-in")
		{
			RunApply(argc > 2 ? argv[2] : "");
			return 0;
		}

		if (cmd == "prepare")
		{
			std::string lang = "ja";
			for (int i = 2; i < argc; ++i)
			{
				std::string a = argv[i];
				if (a == "--lang" && i + 1 < argc) { lang = argv[++i]; if (lang == "jp") lang = "ja"; }
				else lang = a;
			}
			return ApplicationAdv::Instance().PrepareSourceData(lang);
		}

		Logger::Instance().Error("Unknown command: " + cmd);
		PrintHelp();
		return 1;
	}

	return ApplicationAdv::Instance().Run(true);
}
