#include "Application.h"
#include "Config.h"
#include "TranslationDatabase.h"
#include "BackupManager.h"
#include "ProcessorFactory.h"
#include "ChsToSJis.h"
#include "SetupWizard.h"
#include "../FFXIDatProcessor/codepage.h"
#include <Windows.h>
#include <EventStringBase.h>
#include <ItemData.h>
#include <FixedPhrase.h>
#include <MonBridge.h>
#include <RecordsOfEminence.h>
#include <DMsg.h>
#include <CsvFile.h>
#include <iostream>
#include <fstream>
#include <set>
#include <conio.h>
#include <xystring.h>

namespace
{
	namespace fs = std::filesystem;

	fs::path GetProgramRootFromModulePath()
	{
		wchar_t modulePath[MAX_PATH] = {};
		if (GetModuleFileNameW(nullptr, modulePath, MAX_PATH) == 0)
			return {};
		return fs::path(modulePath).parent_path();
	}

	std::u8string BuildDatRelativePath(const std::u8string& relativePath)
	{
		return relativePath.ends_with(u8".DAT") ? relativePath : relativePath + u8".DAT";
	}

	fs::path BuildDatPath(const FileProcessDef& fileDef)
	{
		return Config::Instance().GetGameRoot() / BuildDatRelativePath(fileDef.path);
	}

	fs::path BuildPrepareOutputPath(const fs::path& outputDir, const std::u8string& comment, const wchar_t* extension)
	{
		return outputDir / (xybase::string::to_wstring(comment) + extension);
	}

	void EnsureParentDirectory(const fs::path& path)
	{
		const auto parentPath = path.parent_path();
		if (!parentPath.empty() && !fs::exists(parentPath))
		{
			fs::create_directories(parentPath);
		}
	}

	void WriteUtf8Lines(const fs::path& outputPath, const std::vector<std::u8string>& lines)
	{
		EnsureParentDirectory(outputPath);

		std::ofstream out(outputPath, std::ios::out | std::ios::binary);
		if (!out.is_open())
		{
			throw std::runtime_error("failed to create output file");
		}

		out.write("\xEF\xBB\xBF", 3);
		for (const auto& line : lines)
		{
			out.write(reinterpret_cast<const char*>(line.c_str()), static_cast<std::streamsize>(line.size()));
			out << "\n";
		}
	}

	bool IsEventPrepareComment(const std::u8string& comment)
	{
		return comment.starts_with(u8"ev/") || comment.starts_with(u8"evx/") || comment.starts_with(u8"gev/");
	}

	bool IsItemPrepareComment(const std::u8string& comment)
	{
		return comment.starts_with(u8"itm/");
	}

	ItemSpecType GetPrepareItemSpecType(const std::u8string& type)
	{
		if (type == u8"iab") return ItemSpecType::ARMOUR;
		if (type == u8"iwb") return ItemSpecType::WEAPON;
		if (type == u8"iub") return ItemSpecType::USABLE;
		if (type == u8"ipb") return ItemSpecType::PUPPET;
		if (type == u8"isb") return ItemSpecType::SLIP;
		if (type == u8"icb") return ItemSpecType::CURRENCY;
		if (type == u8"iib") return ItemSpecType::INSTINCT;
		return ItemSpecType::NORMAL;
	}

	void ExportEventStringBase(const FileProcessDef& fileDef, std::set<std::u8string>& processedStrings, const fs::path& outputDir)
	{
		const auto inputPath = BuildDatPath(fileDef);
		const auto outputPath = BuildPrepareOutputPath(outputDir, fileDef.comment, L".txt");

		std::wcout << L"正在处理 " << inputPath << L" -> " << outputPath << std::endl;

		EventStringBase eventStringBase(inputPath);
		eventStringBase.Read();

		std::vector<std::u8string> extractedStrings;
		for (const auto& str : eventStringBase)
		{
			if (processedStrings.insert(str).second)
			{
				extractedStrings.push_back(str);
			}
		}

		if (!extractedStrings.empty())
		{
			WriteUtf8Lines(outputPath, extractedStrings);
			std::wcout << L"提取了 " << extractedStrings.size() << L" 条文本。\n";
		}
		else
		{
			std::wcout << L"没有新的文本需要提取。\n";
		}
	}

	void ExportItemCsv(const FileProcessDef& fileDef, const fs::path& outputDir)
	{
		const auto inputPath = BuildDatPath(fileDef);
		const auto outputPath = BuildPrepareOutputPath(outputDir, fileDef.comment, L".csv");
		EnsureParentDirectory(outputPath);

		std::wcout << L"正在导出 CSV " << inputPath << L" -> " << outputPath << std::endl;

		ItemData itemData;
		itemData.Read(inputPath, GetPrepareItemSpecType(fileDef.type));

		CsvFile output(outputPath, std::ios::out | std::ios::binary);
		output.NewCell(u8"ID");
		output.NewCell(u8"Name");
		output.NewCell(u8"Description");
		output.NewLine();

		for (const auto& datum : itemData.data)
		{
			if (datum.name() == u8".")
				continue;

			output.NewCell(xybase::string::itos<char8_t>(datum.id));
			output.NewCell(datum.name());
			output.NewCell(datum.description());
			output.NewLine();
		}

		std::wcout << L"CSV 导出完成：" << outputPath << std::endl;
	}

	void ExportQuestDMsgCsv(const FileProcessDef& fileDef, const fs::path& outputDir)
	{
		const auto inputPath = BuildDatPath(fileDef);
		const auto outputPath = BuildPrepareOutputPath(outputDir, fileDef.comment, L".csv");
		EnsureParentDirectory(outputPath);

		std::wcout << L"正在导出 CSV " << inputPath << L" -> " << outputPath << std::endl;

		DMsg data(inputPath);
		data.Read();

		CsvFile output(outputPath, std::ios::out | std::ios::binary);
		output.NewCell(u8"ID");
		output.NewCell(u8"Name");
		output.NewCell(u8"Desc");
		output.NewLine();

		for (const auto& row : data)
		{
			const auto& cells = row.GetCellsConst();
			if (cells.size() < 3 || cells[0].GetType() != 1)
				continue;

			int id = cells[0].Get<int>();
			std::u8string title = cells[1].GetType() == 0 ? cells[1].Get<std::u8string>() : std::u8string{};
			std::u8string description = cells[2].GetType() == 0 ? cells[2].Get<std::u8string>() : std::u8string{};

			if (fileDef.comment == u8"sys/mis/ad" && !title.empty() && !title.starts_with(u8"__"))
			{
				id = -id;
			}

			output.NewCell(xybase::string::itos<char8_t>(id));
			output.NewCell(title);
			output.NewCell(description);
			output.NewLine();
		}

		std::wcout << L"CSV 导出完成：" << outputPath << std::endl;
	}

	void ExportRoeQuestCsv(const FileProcessDef& fileDef, const fs::path& outputDir)
	{
		const auto inputPath = BuildDatPath(fileDef);
		const auto outputPath = BuildPrepareOutputPath(outputDir, fileDef.comment, L".csv");
		EnsureParentDirectory(outputPath);

		std::wcout << L"正在导出 CSV " << inputPath << L" -> " << outputPath << std::endl;

		RecordsOfEminence data;
		data.ReadQuest(inputPath);

		CsvFile output(outputPath, std::ios::out | std::ios::binary);
		output.NewCell(u8"ID");
		output.NewCell(u8"QuestName");
		output.NewCell(u8"Description");
		output.NewCell(u8"Note");
		output.NewLine();

		for (const auto& entry : data.questData)
		{
			output.NewCell(xybase::string::itos<char8_t>(entry.id));
			output.NewCell(entry.questName());
			output.NewCell(entry.description());
			output.NewCell(entry.note());
			output.NewLine();
		}

		std::wcout << L"CSV 导出完成：" << outputPath << std::endl;
	}

	void ExportRoeCategoryCsv(const FileProcessDef& fileDef, const fs::path& outputDir)
	{
		const auto inputPath = BuildDatPath(fileDef);
		const auto outputPath = BuildPrepareOutputPath(outputDir, fileDef.comment, L".csv");
		EnsureParentDirectory(outputPath);

		std::wcout << L"正在导出 CSV " << inputPath << L" -> " << outputPath << std::endl;

		RecordsOfEminence data;
		data.ReadCategory(inputPath);

		CsvFile output(outputPath, std::ios::out | std::ios::binary);
		output.NewCell(u8"ID");
		output.NewCell(u8"CategoryName");
		output.NewLine();

		for (const auto& entry : data.categoryData)
		{
			output.NewCell(xybase::string::itos<char8_t>(entry.id));
			output.NewCell(entry.categoryName());
			output.NewLine();
		}

		std::wcout << L"CSV 导出完成：" << outputPath << std::endl;
	}

	void ExportFixedPhraseCsv(const FileProcessDef& fileDef, const fs::path& outputDir)
	{
		const auto inputPath = BuildDatPath(fileDef);
		const auto outputPath = BuildPrepareOutputPath(outputDir, fileDef.comment, L".csv");
		EnsureParentDirectory(outputPath);

		std::wcout << L"正在导出 CSV " << inputPath << L" -> " << outputPath << std::endl;

		FixedPhrase data;
		data.Read(inputPath);
		data.ToCsv(outputPath);

		std::wcout << L"CSV 导出完成：" << outputPath << std::endl;
	}

	bool TryCollectPrepareTextStrings(const FileProcessDef& fileDef, std::vector<std::u8string>& extractedStrings)
	{
		const auto inputPath = BuildDatPath(fileDef);

		if (fileDef.type == u8"mb")
		{
			MonBridge data;
			data.Read(inputPath);
			for (const auto& entry : data.data)
			{
				if (!entry.displayName.empty())
				{
					extractedStrings.push_back(xybase::string::escape(entry.displayName));
				}
			}
			return true;
		}

		if (fileDef.type == u8"sd" || fileDef.type == u8"dmsg" || fileDef.type == u8"xis"
			|| fileDef.type == u8"mbd")
		{
			extractedStrings = ProcessorUtils::CollectStrings(inputPath, fileDef.type, fileDef.cellIndicesStr);
			return true;
		}

		return false;
	}
}

Application& Application::Instance()
{
	static Application instance;
	return instance;
}

// Use extern function from FFXITrans.cpp
int YesNoPrompt(const std::wstring& prompt)
{
	std::wcout << prompt << L" (Y/N): ";
	while (true)
	{
		int key = _getch();
		if (key == 'Y' || key == 'y')
			return 'Y';
		else if (key == 'N' || key == 'n')
			return 'N';
	}
}

void Application::ShowUsage()
{
	std::wcout << L"FFXI汉化插入工具 Ver." VERSION " by Hyururu\n"
		L"用法：FFXITrans [insitu]\n"
		L"  insitu：直接在游戏目录修改文件，否则输出到output目录\n"
		L"  prepare：输出要准备的游戏数据文件（翻译用）\n"
		L"  无参数则进入交互模式\n";
}

bool Application::InitializeCodePages()
{
	try
	{
		const auto& progRoot = Config::Instance().GetProgRoot();
		CodeCvt::GetInstance().Init(progRoot / L"cp932.csv");
	}
	catch (std::exception& ex)
	{
		std::wcerr << ex.what() << std::endl;
		std::wcerr << L"处理代码页cp932.csv失败了。" << std::endl;
		return false;
	}

	try
	{
		const auto& progRoot = Config::Instance().GetProgRoot();
		ChsToSJis::Instance().Init(progRoot / L"chs2sjis.csv");
	}
	catch (std::exception& ex)
	{
		std::wcerr << ex.what() << std::endl;
		std::wcerr << L"处理简体汉字转换逻辑chs2sjis.csv失败了。" << std::endl;
		return false;
	}

	return true;
}

bool Application::LoadTranslations()
{
	auto& db = TranslationDatabase::Instance();

	// Load numbered text files
	for (int i = 0;; ++i)
	{
		int loaded = db.LoadText(i);
		if (loaded < 0)
		{
			std::wcerr << L"加载文本文件失败。" << std::endl;
			return false;
		}
		if (loaded == 0)
			break;
	}

	// Load source data
	int sourceCount = db.LoadSourceData();
	if (sourceCount < 0)
	{
		std::wcerr << L"加载 text\\src / text\\tgt 结构失败。" << std::endl;
		return false;
	}

	if (sourceCount > 0)
	{
		std::wcout << L"从 text\\src / text\\tgt 额外覆盖了 " << sourceCount << L" 条文本数据。" << std::endl;
	}

	std::wcout << L"共读取了 " << std::to_wstring(db.GetTranslationCount()) << L" 条文本数据。" << std::endl;
	return true;
}

bool Application::Initialize()
{
	setlocale(LC_ALL, "");

	std::wcout << L"FFXI汉化插入工具 Ver." VERSION " by Hyururu" << std::endl;

	// Initialize configuration
	if (!Config::Instance().Initialize())
	{
		return false;
	}

	// Load config file if exists
	const auto& progRoot = Config::Instance().GetProgRoot();
	if (std::filesystem::exists(progRoot / "config.ini"))
	{
		Config::Instance().LoadFromFile(progRoot / "config.ini");
	}

	// Initialize code pages
	if (!InitializeCodePages())
	{
		return false;
	}

	// Initialize mismatch log
	TranslationDatabase::Instance().InitializeMismatchLog(progRoot / "text_mismatch.txt");

	return true;
}

std::vector<FileProcessDef> Application::LoadFileDefinitions(bool respectExcludes)
{
	const auto& progRoot = Config::Instance().GetProgRoot();
	CsvFile def(progRoot / "defs.csv", std::ios::in | std::ios::binary);
	std::vector<FileProcessDef> fileDefs;

	int excludedCount = 0;

	while (!def.IsEof())
	{
		FileProcessDef fileDef;
		fileDef.path = def.NextCell();
		fileDef.type = def.NextCell();
		fileDef.lang = def.NextCell();
		fileDef.comment = def.NextCell();
		if (!def.IsEol())
		{
			fileDef.cellIndicesStr = def.NextCell();
		}
		def.NextLine();

		if (fileDef.path.empty() || fileDef.type.empty() || fileDef.lang.empty() || fileDef.comment.empty())
			continue;

		// Check if this definition should be excluded
	 if (respectExcludes && Config::Instance().IsExcluded(fileDef.comment))
		{
			if (Config::Instance().IsVerbose())
			{
				std::wcout << L"跳过已排除项：" << xybase::string::to_wstring(fileDef.comment) << std::endl;
			}
			excludedCount++;
			continue;
		}

		fileDefs.push_back(fileDef);
	}

  if (respectExcludes && excludedCount > 0)
	{
		std::wcout << L"根据配置已排除 " << excludedCount << L" 项文件定义。" << std::endl;
	}

	return fileDefs;
}

int Application::Run(int argc, char** argv)
{
 setlocale(LC_ALL, "");

	try
	{
		const auto progRoot = GetProgramRootFromModulePath();
		if (!progRoot.empty() && SetupWizard::RunIfConfigMissing(progRoot))
		{
			return 0;
		}

		// Parse command line
		bool inSitu = false;
		if (argc > 1)
		{
			if (argc != 2)
			{
				std::wcerr << L"参数错误。\n";
				return -1;
			}

			std::string cmd{ argv[1] };
			if (cmd == "prepare")
			{
				if (!Config::Instance().Initialize())
				{
					system("pause");
					return -1;
				}

				const auto& progRoot = Config::Instance().GetProgRoot();
				if (std::filesystem::exists(progRoot / "config.ini"))
				{
					Config::Instance().LoadFromFile(progRoot / "config.ini");
				}

				int ret = PrepareSourceData();
				if (ret != 0)
				{
					std::wcerr << L"prepare 执行失败。" << std::endl;
					system("pause");
					return ret;
				}
				std::wcout << L"prepare 执行成功。" << std::endl;
				return 0;
			}
			else if (cmd == "insitu")
			{
				inSitu = true;
				Config::Instance().SetInSituMode(true);
			}
			else
			{
				ShowUsage();
				return 0;
			}
		}

		// Initialize application
		if (!Initialize())
		{
			system("pause");
			return -1;
		}

		// Load translations
		if (!LoadTranslations())
		{
			system("pause");
			return -4;
		}

		// Handle backup
		BackupManager::Instance().PromptAndRestore();

		// Determine output mode
		bool overwrite;
		if (inSitu)
		{
			overwrite = true;
		}
		else
		{
			overwrite = Config::Instance().IsInSituNoPrompt() ? false : (YesNoPrompt(L"要在原位修改游戏文件吗？") == 'Y');
		}

		if (overwrite)
		{
			Config::Instance().SetInSituMode(true); // Update config for processors
			std::wcout << L"将在原位修改游戏文件。文件修改前将被备份。" << std::endl;
		}

		std::wcout << L"开始处理文件，请勿关闭程序。" << std::endl;

		// Process translations
		int ret = ProcessTranslations();

		// Close mismatch log
		TranslationDatabase::Instance().CloseMismatchLog();

		std::wcout << L"处理完毕。" << std::endl;
		std::wcout << L"共有 " << std::to_wstring(TranslationDatabase::Instance().GetMismatchCount())
			<< L" 条文本失配。失配文本已经保存到 text_mismatch.txt 中。" << std::endl;

		system("pause");
		return ret;
	}
	catch (std::exception& ex)
	{
		TranslationDatabase::Instance().CloseMismatchLog();
		std::wcerr << L"发生了意外错误。" << std::endl;
		std::wcerr << xybase::string::sys_mbs_to_wcs(ex.what()) << std::endl;
		system("pause");
		return -1;
	}
}

int Application::ProcessTranslations()
{
	auto fileDefs = LoadFileDefinitions();
	std::map<std::u8string, FileProcessDef> jpDefsByComment;

	// Build JP definition mapping
	for (const auto& def : fileDefs)
	{
		if (def.lang == u8"jp")
			jpDefsByComment[def.comment] = def;
	}

	// Determine output mode
	bool overwrite = Config::Instance().IsInSituMode();
	const auto& gameRoot = Config::Instance().GetGameRoot();
	const auto& outRoot = overwrite ? gameRoot : Config::Instance().GetOutRoot();

	int fileCounter = 0;
	int totalFiles = 0;

	// Count files to process
	for (const auto& def : fileDefs)
	{
		if (Config::Instance().IsEnglishMode())
		{
			if (def.lang == u8"en") totalFiles++;
		}
		else
		{
			if (def.lang == u8"jp") totalFiles++;
		}
	}

	if (overwrite)
	{
		std::wcout << L"正在检查并创建备份，请稍候..." << std::endl;
		for (const auto& fileDef : fileDefs)
		{
			if (Config::Instance().IsEnglishMode())
			{
				if (fileDef.lang != u8"en")
					continue;
			}
			else
			{
				if (fileDef.lang != u8"jp")
					continue;
			}

			const std::filesystem::path datPath = gameRoot / (fileDef.path + u8".DAT");
			if (!std::filesystem::exists(datPath))
				continue;

			BackupManager::Instance().BackupGameFile(fileDef.path + u8".DAT");
		}
	}

	for (const auto& fileDef : fileDefs)
	{
		// Filter by language
		if (Config::Instance().IsEnglishMode())
		{
			if (fileDef.lang != u8"en")
				continue;
		}
		else
		{
			if (fileDef.lang != u8"jp")
				continue;
		}

		fileCounter++;

		// Display progress
		wchar_t progress[128];
		swprintf_s(progress, L"[%d/%d] %d%%", fileCounter, totalFiles, fileCounter * 100 / totalFiles);
		std::wcout << L"\r处理中：" << progress << L" "
			<< xybase::string::to_wstring(fileDef.comment) << L"          ";

		// Prepare paths
		std::filesystem::path datPath = gameRoot / (fileDef.path + u8".DAT");
		std::filesystem::path outputPath = overwrite ? datPath : outRoot / (fileDef.path + u8".DAT");

		if (!std::filesystem::exists(datPath))
			continue;

		// Create output directory if needed
		if (!std::filesystem::exists(outputPath.parent_path()))
		{
			std::filesystem::create_directories(outputPath.parent_path());
		}

		// Try ejref_tolerance special processor first
		bool processed = false;
		auto ejrefProcessor = ProcessorFactory::Instance().GetEjrefToleranceProcessor();
		if (ejrefProcessor)
		{
			try
			{
				processed = ejrefProcessor->Process(fileDef, datPath, outputPath, jpDefsByComment);
			}
			catch (const std::exception& ex)
			{
				// Continue to regular processor if ejref fails
				if (Config::Instance().IsVerbose())
				{
					std::wcerr << L"\nEjref处理器失败，回退到常规处理器：" 
						<< xybase::string::sys_mbs_to_wcs(ex.what()) << std::endl;
				}
			}
		}

		// If not processed by ejref, use regular processor
		if (!processed)
		{
			auto processor = ProcessorFactory::Instance().GetProcessor(fileDef.type);
			if (processor)
			{
				try
				{
					processor->Process(fileDef, datPath, outputPath, jpDefsByComment);
				}
				catch (const std::exception& ex)
				{
					std::wcerr << L"\n处理失败：" << xybase::string::to_wstring(fileDef.path)
						<< L" - " << xybase::string::sys_mbs_to_wcs(ex.what()) << std::endl;
				}
			}
			else
			{
				if (Config::Instance().IsVerbose())
				{
					std::wcerr << L"\n未找到处理器：" << xybase::string::to_wstring(fileDef.type)
						<< L" [" << xybase::string::to_wstring(fileDef.comment) << L"]" << std::endl;
				}
			}
		}
	}

	std::wcout << std::endl;
	return 0;
}

int Application::PrepareSourceData()
{
  const auto& progRoot = Config::Instance().GetProgRoot();

	try
	{
		try
		{
			CodeCvt::GetInstance().Init(progRoot / L"cp932.csv");
		}
		catch (std::exception& ex)
		{
			std::wcerr << ex.what() << std::endl;
			std::wcerr << L"处理代码页cp932.csv失败了。" << std::endl;
			return -2;
		}

		std::wcout << L"开始准备源数据..." << std::endl;

		const fs::path outputDir = progRoot / L"text" / L"src_";
		if (!fs::exists(outputDir))
		{
			fs::create_directories(outputDir);
			std::wcout << L"已创建输出目录：" << outputDir << std::endl;
		}

		auto fileDefs = LoadFileDefinitions(false);
		std::vector<FileProcessDef> remainingDefs;
		for (const auto& fileDef : fileDefs)
		{
			if (fileDef.lang != u8"jp")
				continue;

			const auto datPath = BuildDatPath(fileDef);
			if (!fs::exists(datPath))
			{
				std::wcout << L"警告：游戏文件不存在，跳过 " << datPath << std::endl;
				continue;
			}

			remainingDefs.push_back(fileDef);
		}

		std::wcout << L"已加载 " << remainingDefs.size() << L" 个 JP 文件定义。" << std::endl;

		std::set<std::u8string> processedStrings;
		bool foundDbgScene = false;

		for (auto it = remainingDefs.begin(); it != remainingDefs.end();)
		{
			if (it->comment == u8"ev/dbg_scene")
			{
				foundDbgScene = true;
				ExportEventStringBase(*it, processedStrings, outputDir);
				it = remainingDefs.erase(it);
			}
			else
			{
				++it;
			}
		}

		if (!foundDbgScene)
		{
			std::wcout << L"未找到 ev/dbg_scene，跳过预处理。" << std::endl;
		}

		for (auto it = remainingDefs.begin(); it != remainingDefs.end();)
		{
			if (IsEventPrepareComment(it->comment))
			{
				ExportEventStringBase(*it, processedStrings, outputDir);
				it = remainingDefs.erase(it);
			}
			else
			{
				++it;
			}
		}

		for (auto it = remainingDefs.begin(); it != remainingDefs.end();)
		{
			if (IsItemPrepareComment(it->comment))
			{
				ExportItemCsv(*it, outputDir);
				it = remainingDefs.erase(it);
			}
			else
			{
				++it;
			}
		}

		for (const auto& fileDef : remainingDefs)
		{
			const auto inputPath = BuildDatPath(fileDef);

			if (fileDef.type == u8"fp")
			{
				ExportFixedPhraseCsv(fileDef, outputDir);
				continue;
			}

			if (fileDef.type == u8"erq")
			{
				ExportRoeQuestCsv(fileDef, outputDir);
				continue;
			}

			if (fileDef.type == u8"erc")
			{
				ExportRoeCategoryCsv(fileDef, outputDir);
				continue;
			}

			if (fileDef.type == u8"dmsg" && ProcessorUtils::IsQuestDMsg(fileDef.comment))
			{
				ExportQuestDMsgCsv(fileDef, outputDir);
				continue;
			}

			const auto outputPath = BuildPrepareOutputPath(outputDir, fileDef.comment, L".txt");
			std::wcout << L"正在处理 " << inputPath << L" -> " << outputPath << std::endl;

			std::vector<std::u8string> extractedStrings;
			if (!TryCollectPrepareTextStrings(fileDef, extractedStrings))
			{
				std::wcout << L"未支持的 prepare 类型，已跳过：" << xybase::string::to_wstring(fileDef.type)
					<< L" [" << xybase::string::to_wstring(fileDef.comment) << L"]" << std::endl;
				continue;
			}

			if (!extractedStrings.empty())
			{
				WriteUtf8Lines(outputPath, extractedStrings);
				std::wcout << L"提取了 " << extractedStrings.size() << L" 条文本。" << std::endl;
			}
			else
			{
				std::wcout << L"没有提取到可导出的文本。" << std::endl;
			}
		}

		std::wcout << L"源数据准备完成。" << std::endl;
		return 0;
	}
	catch (const std::exception& ex)
	{
		std::wcerr << L"准备源数据失败：" << xybase::string::sys_mbs_to_wcs(ex.what()) << std::endl;
		return -1;
	}
}
