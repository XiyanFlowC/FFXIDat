#include "Application.h"
#include "Config.h"
#include "TranslationDatabase.h"
#include "BackupManager.h"
#include "ProcessorFactory.h"
#include "ChsToSJis.h"
#include "Logger.h"
#include "FinalTextProcessor.h"
#include "SetupWizard.h"
#include "../FFXIDatProcessor/codepage.h"
#include "SpecialProcessor.h"
#include <Windows.h>
#include <CommCtrl.h>
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
#include <xystring.h>

#pragma comment(lib, "Comctl32.lib")

int YesNoPrompt(const std::wstring& prompt);

namespace
{
	namespace fs = std::filesystem;
	constexpr wchar_t kAppTitle[] = L"FFXI汉化插入工具";
	constexpr wchar_t kProgressWindowClassName[] = L"FFXITransProgressWindow";

	void PumpWindowMessages()
	{
		MSG msg{};
		while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}

	void ShowMessageBox(const std::wstring& message, UINT flags, const wchar_t* title = kAppTitle)
	{
		Logger::Instance().Info(std::string("Displaying message box. title=") + Logger::ToUtf8(std::wstring(title)) + ", flags=" + std::to_string(flags));
		MessageBoxW(nullptr, message.c_str(), title, flags | MB_SETFOREGROUND | MB_TOPMOST);
	}

	void ShowErrorMessage(const std::wstring& message)
	{
		ShowMessageBox(message, MB_OK | MB_ICONERROR);
	}

	void ShowInfoMessage(const std::wstring& message)
	{
		ShowMessageBox(message, MB_OK | MB_ICONINFORMATION);
	}

	bool ConfirmInSituMode(bool triggeredByCommandLine, bool triggeredByConfig)
	{
		std::wstring sourceText = L"交互选择";
		if (triggeredByCommandLine)
		{
			sourceText = L"命令行参数 insitu";
		}
		else if (triggeredByConfig)
		{
			sourceText = L"config.ini 的 in_situ=true";
		}

		const std::wstring firstWarning =
			L"【危险：InSitu 模式】\n\n"
			L"当前将直接覆盖游戏目录中的 DAT 文件，而不是输出到 output 目录。\n"
			L"只有在你明确知道自己在做什么时才应继续。\n\n"
			L"触发来源：" + sourceText + L"\n\n"
			L"强烈建议先使用 output 模式验证结果，再考虑 InSitu。\n\n"
			L"是否继续进入 InSitu 模式？";

		if (YesNoPrompt(firstWarning) != 'Y')
		{
			Logger::Instance().Warning("User cancelled InSitu mode at first safety confirmation.");
			ShowInfoMessage(L"已取消 InSitu 模式，本次不会覆盖游戏原始文件。");
			return false;
		}

		const std::wstring secondWarning =
			L"继续前请逐项确认：\n\n"
			L"1. 游戏和 PlayOnline 已完全关闭。\n"
			L"2. backup 目录只能存放程序自动创建的原始 DAT 备份，绝对不要手动把修改过的 DAT 复制进去。\n"
			L"3. 只要游戏更新过，旧 backup 就全部失效；必须先删除整个 backup 目录，再重新运行，让程序从新版本原始文件重新建备份。\n"
			L"4. 你最好已经另外备份了整个游戏目录。\n\n"
			L"只有以上全部满足时才应继续。是否确认覆盖原始游戏文件？";

		if (YesNoPrompt(secondWarning) != 'Y')
		{
			Logger::Instance().Warning("User cancelled InSitu mode at second safety confirmation.");
			ShowInfoMessage(L"已取消 InSitu 模式，本次不会覆盖游戏原始文件。\n\n如需继续，建议先用 output 模式测试。");
			return false;
		}

		Logger::Instance().Warning("User explicitly confirmed dangerous InSitu mode after double warning.");
		return true;
	}

	LRESULT CALLBACK ProgressWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_CLOSE:
			return 0;
		default:
			return DefWindowProcW(hwnd, message, wParam, lParam);
		}
	}

	class ProgressDialog
	{
	public:
		explicit ProgressDialog(const wchar_t* title)
		{
			INITCOMMONCONTROLSEX icc{};
			icc.dwSize = sizeof(icc);
			icc.dwICC = ICC_PROGRESS_CLASS;
			InitCommonControlsEx(&icc);

			EnsureWindowClassRegistered();

			const HINSTANCE instance = GetModuleHandleW(nullptr);
			window_ = CreateWindowExW(
				WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
				kProgressWindowClassName,
				title,
				WS_CAPTION | WS_SYSMENU,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				420,
				150,
				nullptr,
				nullptr,
				instance,
				nullptr);

			if (!window_)
				return;

			const HFONT font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
			statusLabel_ = CreateWindowExW(
				0,
				L"STATIC",
				L"准备中...",
				WS_CHILD | WS_VISIBLE,
				20,
				20,
				360,
				36,
				window_,
				nullptr,
				instance,
				nullptr);
			progressBar_ = CreateWindowExW(
				0,
				PROGRESS_CLASSW,
				nullptr,
				WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
				20,
				70,
				360,
				24,
				window_,
				nullptr,
				instance,
				nullptr);

			if (statusLabel_ && font)
				SendMessageW(statusLabel_, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);

			if (progressBar_)
			{
				SendMessageW(progressBar_, PBM_SETRANGE32, 0, 100);
				SendMessageW(progressBar_, PBM_SETPOS, 0, 0);
			}

			CenterWindow();
			ShowWindow(window_, SW_SHOW);
			UpdateWindow(window_);
			PumpWindowMessages();
		}

		~ProgressDialog()
		{
			if (window_)
			{
				DestroyWindow(window_);
				window_ = nullptr;
			}
			PumpWindowMessages();
		}

		void Update(int current, int total, const std::wstring& statusText)
		{
			if (!window_ || !statusLabel_ || !progressBar_)
				return;

			const int safeTotal = total > 0 ? total : 1;
			int safeCurrent = current;
			if (safeCurrent < 0)
				safeCurrent = 0;
			if (safeCurrent > safeTotal)
				safeCurrent = safeTotal;

			SetWindowTextW(statusLabel_, statusText.c_str());
			SendMessageW(progressBar_, PBM_SETRANGE32, 0, safeTotal);
			SendMessageW(progressBar_, PBM_SETPOS, safeCurrent, 0);
			RedrawWindow(window_, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
			PumpWindowMessages();
		}

	private:
		void EnsureWindowClassRegistered()
		{
			const HINSTANCE instance = GetModuleHandleW(nullptr);
			WNDCLASSEXW existing{};
			if (GetClassInfoExW(instance, kProgressWindowClassName, &existing))
				return;

			WNDCLASSEXW windowClass{};
			windowClass.cbSize = sizeof(windowClass);
			windowClass.lpfnWndProc = ProgressWindowProc;
			windowClass.hInstance = instance;
			windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
			windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
			windowClass.lpszClassName = kProgressWindowClassName;
			RegisterClassExW(&windowClass);
		}

		void CenterWindow() const
		{
			if (!window_)
				return;

			RECT rect{};
			if (!GetWindowRect(window_, &rect))
				return;

			RECT workArea{};
			SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);

			const int width = rect.right - rect.left;
			const int height = rect.bottom - rect.top;
			const int x = workArea.left + ((workArea.right - workArea.left) - width) / 2;
			const int y = workArea.top + ((workArea.bottom - workArea.top) - height) / 2;

			SetWindowPos(window_, HWND_TOPMOST, x, y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
		}

		HWND window_ = nullptr;
		HWND statusLabel_ = nullptr;
		HWND progressBar_ = nullptr;
	};

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
		Logger::Instance().Info("Preparing EventStringBase export. comment='" + Logger::ToUtf8(fileDef.comment)
			+ "', input=" + Logger::ToUtf8(inputPath) + ", output=" + Logger::ToUtf8(outputPath));

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
		Logger::Instance().Info("Preparing item CSV export. comment='" + Logger::ToUtf8(fileDef.comment)
			+ "', input=" + Logger::ToUtf8(inputPath) + ", output=" + Logger::ToUtf8(outputPath));
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
		Logger::Instance().Info("Preparing quest DMsg CSV export. comment='" + Logger::ToUtf8(fileDef.comment)
			+ "', input=" + Logger::ToUtf8(inputPath) + ", output=" + Logger::ToUtf8(outputPath));
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

			if ((fileDef.comment == u8"sys/mis/ad" || fileDef.comment == u8"sys/mis/rov") && !title.empty() && !title.starts_with(u8"__"))
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
		Logger::Instance().Info("Preparing ROE quest CSV export. comment='" + Logger::ToUtf8(fileDef.comment)
			+ "', input=" + Logger::ToUtf8(inputPath) + ", output=" + Logger::ToUtf8(outputPath));
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
		Logger::Instance().Info("Preparing ROE category CSV export. comment='" + Logger::ToUtf8(fileDef.comment)
			+ "', input=" + Logger::ToUtf8(inputPath) + ", output=" + Logger::ToUtf8(outputPath));
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
		Logger::Instance().Info("Preparing fixed phrase CSV export. comment='" + Logger::ToUtf8(fileDef.comment)
			+ "', input=" + Logger::ToUtf8(inputPath) + ", output=" + Logger::ToUtf8(outputPath));
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
	const int result = MessageBoxW(
		nullptr,
		prompt.c_str(),
		kAppTitle,
		MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 | MB_SETFOREGROUND | MB_TOPMOST);
	Logger::Instance().Info(std::string("User prompt result=") + (result == IDYES ? "Yes" : "No")
		+ ", prompt=" + Logger::ToUtf8(prompt));
	return result == IDYES ? 'Y' : 'N';
}

void Application::ShowUsage()
{
	std::wcout << L"FFXI汉化插入工具 Ver." VERSION " by Hyururu\n"
		L"用法：FFXITrans [insitu]\n"
		L"  insitu：危险，直接覆盖游戏目录中的 DAT 文件；仅在确认备份可靠且已理解游戏更新影响时使用\n"
		L"  prepare：输出要准备的游戏数据文件（翻译用）\n"
		L"  无参数则进入交互模式\n";
}

bool Application::InitializeCodePages()
{
	try
	{
		const auto& progRoot = Config::Instance().GetProgRoot();
		CodeCvt::GetInstance().Init(progRoot / L"cp932.csv");
		Logger::Instance().Info("Loaded cp932.csv successfully from " + Logger::ToUtf8(progRoot / L"cp932.csv"));
	}
	catch (std::exception& ex)
	{
		std::wcerr << ex.what() << std::endl;
		std::wcerr << L"处理代码页cp932.csv失败了。" << std::endl;
		Logger::Instance().Error(std::string("Failed to load cp932.csv: ") + ex.what());
		ShowErrorMessage(std::wstring(L"处理代码页 cp932.csv 失败。\n\n") + xybase::string::sys_mbs_to_wcs(ex.what()));
		return false;
	}

	try
	{
		const auto& progRoot = Config::Instance().GetProgRoot();
		ChsToSJis::Instance().Init(progRoot / L"chs2sjis.csv");
		Logger::Instance().Info("Loaded chs2sjis.csv successfully from " + Logger::ToUtf8(progRoot / L"chs2sjis.csv")
			+ ", replacements=" + std::to_string(ChsToSJis::Instance().GetReplacementCount()));
	}
	catch (std::exception& ex)
	{
		std::wcerr << ex.what() << std::endl;
		std::wcerr << L"处理简体汉字转换逻辑chs2sjis.csv失败了。" << std::endl;
		Logger::Instance().Error(std::string("Failed to load chs2sjis.csv: ") + ex.what());
		ShowErrorMessage(std::wstring(L"处理 chs2sjis.csv 失败。\n\n") + xybase::string::sys_mbs_to_wcs(ex.what()));
		return false;
	}

	return true;
}

bool Application::LoadTranslations()
{
	auto& db = TranslationDatabase::Instance();
	Logger::Instance().Info("Loading translation databases.");

	// Load numbered text files
	for (int i = 0;; ++i)
	{
		int loaded = db.LoadText(i);
		if (loaded < 0)
		{
			std::wcerr << L"加载文本文件失败。" << std::endl;
			Logger::Instance().Error("Failed to load numbered text translation database.");
			ShowErrorMessage(L"加载文本文件失败。");
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
		Logger::Instance().Error("Failed to load text/src -> text/tgt translation database.");
		ShowErrorMessage(L"加载 text\\src / text\\tgt 结构失败。");
		return false;
	}

	if (sourceCount > 0)
	{
		std::wcout << L"从 text\\src / text\\tgt 额外覆盖了 " << sourceCount << L" 条文本数据。" << std::endl;
	}

	std::wcout << L"共读取了 " << std::to_wstring(db.GetTranslationCount()) << L" 条文本数据。" << std::endl;
	Logger::Instance().Info("Translation database loaded. totalEntries=" + std::to_string(db.GetTranslationCount())
		+ ", sourceDataEntries=" + std::to_string(sourceCount));
	return true;
}

bool Application::Initialize()
{
	setlocale(LC_ALL, "");
	Logger::Instance().Initialize(GetProgramRootFromModulePath() / "log.txt");
	Logger::Instance().Info(std::string("Application initialize started. version=") + VERSION);

	std::wcout << L"FFXI汉化插入工具 Ver." VERSION " by Hyururu" << std::endl;

	// Initialize configuration
	if (!Config::Instance().Initialize())
	{
		Logger::Instance().Error("Config initialization failed.");
		ShowErrorMessage(L"初始化配置失败。请检查 config.ini 或游戏安装路径设置。");
		return false;
	}

	// Load config file if exists
	const auto& progRoot = Config::Instance().GetProgRoot();
	if (std::filesystem::exists(progRoot / "config.ini"))
	{
		Config::Instance().LoadFromFile(progRoot / "config.ini");
	}
	else
	{
		Logger::Instance().Warning("config.ini does not exist. Using registry/default values only.");
		Logger::Instance().Info("Final config state: " + Config::Instance().DescribeStateForLog());
	}

	// Initialize code pages
	if (!InitializeCodePages())
	{
		return false;
	}

	// Initialize mismatch log
	TranslationDatabase::Instance().InitializeMismatchLog(progRoot / "text_mismatch.txt");
   FinalTextProcessor::ResetValidationSummary();
	Logger::Instance().Info("Application initialize completed.");

	return true;
}

std::vector<FileProcessDef> Application::LoadFileDefinitions(bool respectExcludes)
{
	const auto& progRoot = Config::Instance().GetProgRoot();
	Logger::Instance().Info("Loading defs.csv from " + Logger::ToUtf8(progRoot / "defs.csv")
		+ ", respectExcludes=" + std::string(respectExcludes ? "true" : "false"));
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
				Logger::Instance().Initialize(progRoot / "log.txt");
				Logger::Instance().Info("Command line mode: prepare");
				if (!Config::Instance().Initialize())
				{
					Logger::Instance().Error("Config initialization failed in prepare mode.");
					ShowErrorMessage(L"初始化配置失败。请检查 config.ini 或游戏安装路径设置。");
					Logger::Instance().Close();
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
					Logger::Instance().Error("Prepare mode failed with exit code " + std::to_string(ret));
					Logger::Instance().Close();
					return ret;
				}
				std::wcout << L"prepare 执行成功。" << std::endl;
				Logger::Instance().Info("Prepare mode completed successfully.");
				ShowInfoMessage(L"prepare 执行成功。");
				Logger::Instance().Close();
				return 0;
			}
			else if (cmd == "insitu")
			{
				inSitu = true;
				Config::Instance().SetInSituMode(true);
				Logger::Instance().Initialize(progRoot / "log.txt");
				Logger::Instance().Info("Command line mode: insitu");
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
			Logger::Instance().Close();
			return -1;
		}

		// Load translations
		if (!LoadTranslations())
		{
			Logger::Instance().Close();
			return -4;
		}

		// Determine output mode
		const bool configForcesInSitu = Config::Instance().IsInSituMode();
		bool overwrite = inSitu || configForcesInSitu;
		if (!overwrite)
		{
			if (!Config::Instance().IsInSituNoPrompt())
				overwrite = (YesNoPrompt(L"要在原位修改游戏文件吗？\n\n选择“否”将输出到 output 目录（推荐）。") == 'Y');
		}

		if (overwrite)
		{
			if (!ConfirmInSituMode(inSitu, configForcesInSitu && !inSitu))
			{
				TranslationDatabase::Instance().CloseMismatchLog();
				Logger::Instance().Info("Application run cancelled before entering InSitu mode.");
				Logger::Instance().Close();
				return 0;
			}

			Config::Instance().SetInSituMode(true); // Update config for processors
			std::wcout << L"将在原位修改游戏文件。文件修改前将验证/创建备份。" << std::endl;
			std::wcout << L"注意：如果游戏已经更新，必须先删除整个 backup 目录后再重新运行。" << std::endl;
		}
		else
		{
			Config::Instance().SetInSituMode(false);
		}
		Logger::Instance().Info(std::string("Output mode selected: ") + (overwrite ? "in-place" : "output directory"));

		// Handle backup
		BackupManager::Instance().PromptAndRestore();

		std::wcout << L"开始处理文件，请勿关闭程序。" << std::endl;

		// Process translations
		int ret = ProcessTranslations();

		// Close mismatch log
		TranslationDatabase::Instance().CloseMismatchLog();
		Logger::Instance().Info("Application run finished with exit code " + std::to_string(ret));
		Logger::Instance().Close();

		std::wcout << L"处理完毕。" << std::endl;
		std::wcout << L"共有 " << std::to_wstring(TranslationDatabase::Instance().GetMismatchCount())
			<< L" 条文本失配。失配文本已经保存到 text_mismatch.txt 中。" << std::endl;
		if (FinalTextProcessor::GetSkippedValidationCount() > 0)
		{
			std::wcerr << L"警告：有 " << std::to_wstring(FinalTextProcessor::GetSkippedValidationCount())
				<< L" 条文本因控制序列校验失败而回退为原文。详情见 log.txt。" << std::endl;
		}

        std::wstring finalMessage =
			L"处理完毕。\n\n共有 "
			+ std::to_wstring(TranslationDatabase::Instance().GetMismatchCount())
            + L" 条文本失配。失配文本已经保存到 text_mismatch.txt 中。";
		if (FinalTextProcessor::GetSkippedValidationCount() > 0)
		{
			finalMessage += L"\n\n警告：有 "
				+ std::to_wstring(FinalTextProcessor::GetSkippedValidationCount())
				+ L" 条文本因控制序列校验失败而回退为原文。详情见 log.txt。";
		}

		ShowInfoMessage(finalMessage);
		return ret;
	}
	catch (std::exception& ex)
	{
		TranslationDatabase::Instance().CloseMismatchLog();
		std::wcerr << L"发生了意外错误。" << std::endl;
		std::wcerr << xybase::string::sys_mbs_to_wcs(ex.what()) << std::endl;
		Logger::Instance().Error(std::string("Unhandled exception: ") + ex.what());
		ShowErrorMessage(std::wstring(L"发生了意外错误。\n\n") + xybase::string::sys_mbs_to_wcs(ex.what()));
		Logger::Instance().Close();
		return -1;
	}
}

int Application::ProcessTranslations()
{
	auto fileDefs = LoadFileDefinitions();
	std::map<std::u8string, FileProcessDef> jpDefsByComment;
	Logger::Instance().Info("ProcessTranslations started.");

	// Build JP definition mapping
	for (const auto& def : fileDefs)
	{
		if (def.lang == u8"ja")
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
			if (def.lang == u8"ja") totalFiles++;
		}
	}

	ProgressDialog progressDialog(L"FFXI汉化插入工具 - 翻译进度");
	progressDialog.Update(0, totalFiles, L"正在准备处理列表...");

	if (overwrite)
	{
		std::wcout << L"正在检查并创建备份，请稍候..." << std::endl;
		progressDialog.Update(0, totalFiles, L"正在检查并创建备份，请稍候...");
		for (const auto& fileDef : fileDefs)
		{
			if (Config::Instance().IsEnglishMode())
			{
				if (fileDef.lang != u8"en")
					continue;
			}
			else
			{
				if (fileDef.lang != u8"ja")
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
			if (fileDef.lang != u8"ja")
				continue;
		}

		fileCounter++;

		// Display progress
		wchar_t progress[128];
		swprintf_s(progress, L"[%d/%d] %d%%", fileCounter, totalFiles, fileCounter * 100 / totalFiles);
		const auto commentText = xybase::string::to_wstring(fileDef.comment);
		std::wcout << L"\r处理中：" << progress << L" "
			<< commentText << L"          ";
		progressDialog.Update(fileCounter, totalFiles, std::wstring(progress) + L" " + commentText);

		// Prepare paths
		std::filesystem::path datPath = gameRoot / (fileDef.path + u8".DAT");
		std::filesystem::path outputPath = overwrite ? datPath : outRoot / (fileDef.path + u8".DAT");
		Logger::Instance().Info("Processing file comment='" + Logger::ToUtf8(fileDef.comment)
			+ "', type='" + Logger::ToUtf8(fileDef.type)
			+ "', lang='" + Logger::ToUtf8(fileDef.lang)
			+ "', input=" + Logger::ToUtf8(datPath)
			+ ", output=" + Logger::ToUtf8(outputPath));

		if (!std::filesystem::exists(datPath))
		{
			Logger::Instance().Warning("Input DAT file does not exist: " + Logger::ToUtf8(datPath));
			continue;
		}

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
				if (processed)
					Logger::Instance().Info("Completed file via ejref tolerance processor. comment='" + Logger::ToUtf8(fileDef.comment) + "'.");
			}
			catch (const std::exception& ex)
			{
				// Continue to regular processor if ejref fails
				Logger::Instance().Error("Ejref tolerance processor failed for comment='" + Logger::ToUtf8(fileDef.comment)
					+ "': " + ex.what());
				if (Config::Instance().IsVerbose())
				{
					std::wcerr << L"\nEjref处理器失败，回退到常规处理器："
						<< xybase::string::sys_mbs_to_wcs(ex.what()) << std::endl;
				}
			}
		}

		if (!processed)
		{
			auto specProc = SpecialProcessor();
			processed = specProc.Process(fileDef, datPath, outputPath, jpDefsByComment);
			if (processed)
				Logger::Instance().Info("Completed file via special processor. comment='" + Logger::ToUtf8(fileDef.comment) + "'.");
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
					Logger::Instance().Info("Completed file via regular processor. comment='" + Logger::ToUtf8(fileDef.comment) + "'.");
				}
				catch (const std::exception& ex)
				{
					std::wcerr << L"\n处理失败：" << xybase::string::to_wstring(fileDef.path)
						<< L" - " << xybase::string::sys_mbs_to_wcs(ex.what()) << std::endl;
					Logger::Instance().Error("Regular processor failed for comment='" + Logger::ToUtf8(fileDef.comment)
						+ "', type='" + Logger::ToUtf8(fileDef.type) + "': " + ex.what());
				}
			}
			else
			{
				Logger::Instance().Warning("No processor found for comment='" + Logger::ToUtf8(fileDef.comment)
					+ "', type='" + Logger::ToUtf8(fileDef.type) + "'.");
				if (Config::Instance().IsVerbose())
				{
					std::wcerr << L"\n未找到处理器：" << xybase::string::to_wstring(fileDef.type)
						<< L" [" << xybase::string::to_wstring(fileDef.comment) << L"]" << std::endl;
				}
			}
		}
	}

	progressDialog.Update(totalFiles, totalFiles, L"处理完毕。");
	std::wcout << std::endl;
	Logger::Instance().Info("ProcessTranslations finished.");
	return 0;
}

int Application::PrepareSourceData()
{
	const auto& progRoot = Config::Instance().GetProgRoot();
	Logger::Instance().Info("PrepareSourceData started.");

	try
	{
		try
		{
			CodeCvt::GetInstance().Init(progRoot / L"cp932.csv");
			Logger::Instance().Info("Loaded cp932.csv successfully for prepare mode from " + Logger::ToUtf8(progRoot / L"cp932.csv"));
		}
		catch (std::exception& ex)
		{
			std::wcerr << ex.what() << std::endl;
			std::wcerr << L"处理代码页cp932.csv失败了。" << std::endl;
			ShowErrorMessage(std::wstring(L"prepare 阶段初始化 cp932.csv 失败。\n\n") + xybase::string::sys_mbs_to_wcs(ex.what()));
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
			if (fileDef.lang != u8"ja")
				continue;

			const auto datPath = BuildDatPath(fileDef);
			if (!fs::exists(datPath))
			{
				std::wcout << L"警告：游戏文件不存在，跳过 " << datPath << std::endl;
				Logger::Instance().Warning("Prepare mode skipped missing DAT file: " + Logger::ToUtf8(datPath));
				continue;
			}

			remainingDefs.push_back(fileDef);
		}

		std::wcout << L"已加载 " << remainingDefs.size() << L" 个 JA 文件定义。" << std::endl;

		const int totalFiles = static_cast<int>(remainingDefs.size());
		int processedCount = 0;
		ProgressDialog progressDialog(L"FFXI汉化插入工具 - Prepare 进度");
		progressDialog.Update(0, totalFiles, L"正在准备源数据...");
		auto updatePrepareProgress = [&](const FileProcessDef& fileDef)
			{
				++processedCount;
				progressDialog.Update(
					processedCount,
					totalFiles,
					L"正在准备：" + xybase::string::to_wstring(fileDef.comment));
			};

		std::set<std::u8string> processedStrings;
		bool foundDbgScene = false;

		for (auto it = remainingDefs.begin(); it != remainingDefs.end();)
		{
			if (it->comment == u8"ev/dbg_scene")
			{
				foundDbgScene = true;
				updatePrepareProgress(*it);
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
				updatePrepareProgress(*it);
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
				updatePrepareProgress(*it);
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
			updatePrepareProgress(fileDef);
			Logger::Instance().Info("Preparing source data for comment='" + Logger::ToUtf8(fileDef.comment)
				+ "', type='" + Logger::ToUtf8(fileDef.type) + "'.");
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

		progressDialog.Update(totalFiles, totalFiles, L"源数据准备完成。");
		std::wcout << L"源数据准备完成。" << std::endl;
		return 0;
	}
	catch (const std::exception& ex)
	{
		std::wcerr << L"准备源数据失败：" << xybase::string::sys_mbs_to_wcs(ex.what()) << std::endl;
		ShowErrorMessage(std::wstring(L"准备源数据失败。\n\n") + xybase::string::sys_mbs_to_wcs(ex.what()));
		return -1;
	}
}
