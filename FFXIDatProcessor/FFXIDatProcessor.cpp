// FFXIDatProcessor.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <clocale>
#include <format>
#include <exception>
#include <set>
#include <unordered_map>

#include "DMsg.h"

#include "xystring.h"
#include "DataManager.h"
#include "FFXIDatProcessor.h"
#include "codepage.h"
#include "XiString.h"
#include "../FFXIDat/FixedPhrase.h"
#include "../FFXIDat/RecordsOfEminence.h"
#include "../FFXIDat/MonBridge.h"
#include "../FFXIDat/ItemData.h"
#include "../FFXIDat/BlockFile.h"

#include "liteopt.h"

#include "SQLiteDataSource.h"

#include "EventStringBase.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#undef GetMessage
void CsvToEventStringData(const char *src, const char *out);
void CsvToFixedPhrase(const char *src, const char *out = nullptr);
void ExportItemData(void);

int cfg_block = 0, cfg_xor = 0;

int help(const char *para)
{
	std::wcout << L"FFXI 数据文件处理器" << std::endl
		<< std::endl;
	for (int i = 0; i < 32; ++i)
	{
		if (!LOPT_FLG_CHK(_reged_opt[i].flg, LOPT_FLG_DESC_VLD)) continue;
		if (LOPT_FLG_CHK(_reged_opt[i].flg, LOPT_FLG_CH_VLD))
			std::wcout << std::format(L"-{}  ", (wchar_t)_reged_opt[i].ch_opt);
		else
			std::wcout << L"    ";
		if (LOPT_FLG_CHK(_reged_opt[i].flg, LOPT_FLG_STR_VLD))
			std::wcout << std::format(L"--{:16} ", xybase::string::to_wstring(_reged_opt[i].long_opt));
		else
			std::wcout << L"                  ";

		if (LOPT_FLG_CHK(_reged_opt[i].flg, LOPT_FLG_VAL_NEED))
			std::wcout << L"* ";
		else
			std::wcout << L"  ";

		auto ch = (wchar_t *)_reged_opt[i].desc;
		int cur = 0;
		int lfc = 24;
		while (*ch)
		{
			if (*ch == L'\n')
			{
				cur = 0;
				if (*++ch)
					std::wcout << L"\n                             ";
				else
					std::wcout << std::endl;
				continue;
			}
			std::wcout << *ch++;
			if (cur++ > lfc)
			{
				cur = 0;
				if (*ch)
					std::wcout << L"\n                             ";
				else
					std::wcout << std::endl;
			}
		}
		if (cur) std::wcout << L"\n";
	}
	std::wcout << L"标记了 * 的项目，表示该开关需要后随一个参数。" << std::endl;
	return 0;
}

const char *cfg_type = nullptr, *cfg_lang = nullptr, *cfg_path = nullptr;

int main(int argc, const char **argv)
{
	// setlocale(LC_ALL, "ja_JP");
	setlocale(LC_ALL, "");

	PathUtil::Init();

	try
	{
		CodeCvt::GetInstance().Init(PathUtil::progRootPath + L"/cp932.csv");
	}
	catch (std::exception &ex)
	{
		std::wcerr << ex.what() << std::endl;
		std::wcerr << L"处理代码页cp932.csv失败了。" << std::endl;
		return -2;
	}

	lopt_regopt("sql-init", 0, LOPT_FLG_VAL_NEED, [](const char *str)->int {
		std::filesystem::remove(PathUtil::progRootPath + L"/text.db");
		SQLiteDataSource ds;
		ds.Initialise();
		CsvFile def(str, std::ios_base::in | std::ios_base::binary);
		ds.InitialiseFileDefinition(def);
		return 0;
		}, L"用指定的定义初始化SQLite数据库。（已经存在的数据库会被删除）");
	lopt_regopt("sql-cond-type", 0, LOPT_FLG_VAL_NEED, [](const char *str)->int {
		cfg_type = str;
		return 0;
		}, L"指定操作对象的类型。");
	lopt_regopt("sql-cond-lang", 0, LOPT_FLG_VAL_NEED, [](const char *str)->int {
		cfg_lang = str;
		return 0;
		}, L"指定操作对象的语言。");
	lopt_regopt("sql-cond-path", 0, LOPT_FLG_VAL_NEED, [](const char *str)->int {
		cfg_path = str;
		return 0;
		}, L"指定操作对象的路径。");
	lopt_regopt("sql-file-update", 0, LOPT_FLG_VAL_NEED, [](const char *str)->int {
		// std::filesystem::remove(PathUtil::progRootPath + L"/text.db");
		SQLiteDataSource ds;
		CsvFile def(str, std::ios_base::in | std::ios_base::binary);
		ds.InitialiseFileDefinition(def);
		return 0;
		}, L"用指定的定义更新SQLite数据库。");
	lopt_regopt("sql-purge", 0, 0, [](const char *str)->int {
		SQLiteDataSource ds;
		ds.Purge();
		return 0;
		}, L"清除SQLite数据库中的无引用文本。");
	lopt_regopt("sql-trans-dump", 0, 0, [](const char *str)->int {
		SQLiteDataSource ds;
		ds.DumpTranslationData();
		return 0;
		}, L"导出SQLite数据库中的原文和翻译数据到文本文件。");
	lopt_regopt("sql-trans-dump-empty", 0, 0, [](const char *str)->int {
		SQLiteDataSource ds;
		ds.ExportNoTranslation();
		return 0;
		}, L"导出SQLite数据库中没有翻译的数据到文本文件。");
	lopt_regopt("sql-trans-import", 0, 0, [](const char *str)->int {
		SQLiteDataSource ds;
		try
		{
			ds.ImportTranslation();
		}
		catch (SQLException &ex)
		{
			std::wcout << xybase::string::to_wstring(ex.what()) << L"\nSQL 错误。\n";
		}
		catch (std::exception &ex)
		{
			std::wcout << xybase::string::to_wstring(ex.what()) << L"\n发生了错误。\n";
		}
		catch (...)
		{
			std::wcout << L"发生了未知错误。\n";
		}
		return 0;
		}, L"导入文本文件中的翻译数据到SQLite数据库中。");
	lopt_regopt("sql-dat-trans", 'T', 0, [](const char *str)->int {
		SQLiteDataSource ds;
		ds.TransAndOut();
		return 0;
		}, L"按SQLite中的定义和翻译数据，试图翻译游戏Dat并输出。");
	lopt_regopt("sql-dat-read", 'q', 0, [](const char *str)->int {
		SQLiteDataSource ds;
		if (cfg_type && strcmp(cfg_type, "item") == 0)
		{
			const char* types[] = {"ieb", "inb", "iub", "iwb", "iab", "isb", "ipb", "icb", "iib"};
			for (const auto& type : types)
			{
				ds.DatToDatabase(cfg_lang, type, cfg_path);
			}
		}
		ds.DatToDatabase(cfg_lang, cfg_type, cfg_path);
		return 0;
		}, L"根据SQLite数据库中定义从游戏安装目录抽取文本，并存入。");
	lopt_regopt("do-xor", 'x', 0, [](const char *str)->int {cfg_xor = 1; return 0; }, L"要求DMsg进行Xor保护。");
	lopt_regopt("block", 'b', 0, [](const char *str)->int {cfg_block = 1; return 0; }, L"要求DMsg以块形式保存。。");
	lopt_regopt("dmsg-to-csv", 'm', LOPT_FLG_VAL_NEED, [](const char *str) -> int {
		std::string path(str);
		DMsg f(str);
		try {
			f.Read();
			f.ToCsv(path.replace(path.find(".DAT"), 4, ".csv"));
		}
		catch (std::exception &ex) {
			std::wcerr << L"发生错误：" << ex.what() << std::endl;
		}
		return 0;
		}, L"转换一个D_Msg文件到CSV。");
	lopt_regopt("csv-to-dmsg", 'M', LOPT_FLG_VAL_NEED, [](const char *str) -> int {CsvToDMsg(str); return 0; }, L"转换一个CSV文件到D_Msg。");
	lopt_regopt("xis-to-csv", 's', LOPT_FLG_VAL_NEED, [](const char *str) -> int {
		std::string path(str);
		XiString f(str);
		try {
			f.Read();
			f.ToCsv(path.replace(path.find(".DAT"), 4, ".csv"));
		}
		catch (std::exception &ex) {
			std::wcerr << L"发生错误：" << ex.what() << std::endl;
		}
		return 0;
		}, L"转换一个XiString文件到CSV。");
	lopt_regopt("csv-to-xis", 'S', LOPT_FLG_VAL_NEED, [](const char *str) -> int {CsvToXiString(str); return 0; }, L"转换一个CSV文件到XiString。");
	lopt_regopt("fp-to-csv", 'p', LOPT_FLG_VAL_NEED, [](const char *str) -> int {
		std::string path(str);
		FixedPhrase f;
		try {
			f.Read(xybase::string::to_wstring(str));
			f.ToCsv(xybase::string::to_wstring(path.replace(path.find(".DAT"), 4, ".csv")));
		}
		catch (std::exception &ex) {
			std::wcerr << L"发生错误：" << ex.what() << std::endl;
		}
		return 0;
		}, L"转换一个FixedPhrase文件到CSV。");
	lopt_regopt("csv-to-fp", 'P', LOPT_FLG_VAL_NEED, [](const char *str) -> int {CsvToFixedPhrase(str); return 0; }, L"转换一个CSV文件到FixedPhrase。");
	lopt_regopt("roe-quest-to-csv", 0, LOPT_FLG_VAL_NEED, [](const char *str) -> int {
		std::string path(str);
		RecordsOfEminence roe;
		try {
			roe.ReadQuest(str);
			roe.QuestToICsv(path.replace(path.find(".DAT"), 4, ".quest.csv").c_str());
		}
		catch (std::exception &ex) {
			std::wcerr << L"发生错误：" << ex.what() << std::endl;
		}
		return 0;
		}, L"转换一个Records of Eminence Quest文件到CSV。");
	lopt_regopt("roe-category-to-csv", 0, LOPT_FLG_VAL_NEED, [](const char *str) -> int {
		std::string path(str);
		RecordsOfEminence roe;
		try {
			roe.ReadCategory(str);
			roe.CategoryToICsv(path.replace(path.find(".DAT"), 4, ".category.csv").c_str());
		}
		catch (std::exception &ex) {
			std::wcerr << L"发生错误：" << ex.what() << std::endl;
		}
		return 0;
		}, L"转换一个Records of Eminence Category文件到CSV。");
	lopt_regopt("mb-to-csv", 0, LOPT_FLG_VAL_NEED, [](const char *str) -> int {
		std::string path(str);
		MonBridge mb;
		try {
			mb.Read(str);
			mb.ToICsv(xybase::string::to_wstring(path.replace(path.find(".DAT"), 4, ".mb.csv")));
		}
		catch (std::exception &ex) {
			std::wcerr << L"发生错误：" << ex.what() << std::endl;
		}
		return 0;
		}, L"转换一个MonBridge文件到CSV。");
	lopt_regopt("install-path", 'I', LOPT_FLG_VAL_NEED, [](const char *str) -> int {
		PathUtil::gameRootPath = xybase::string::sys_mbs_to_wcs(str);
		return 0;
		}, L"指定游戏安装目录。应为第一个开关。");
	lopt_regopt("scan-extract", 'X', 0, [](const char *str)-> int {
		ExtractSysText();
		return 0;
		}, L"扫描并导出游戏目录。");
	lopt_regopt("export-item-data", 0, 0, [](const char *str)-> int {
		ExportItemData();
		return 0;
		}, L"导出游戏中的物品数据。");
	lopt_regopt("help", '?', 0, help, L"显示本信息。");
	if (argc == 1) help(nullptr);

	if (argv[1][0] != '-')
	{
		// 智能多文件处理
		for (int i = 1; i < argc; ++i)
		{
			std::string path(argv[i]);
			if (path.ends_with(".dmsg.csv"))
			{
				CsvToDMsg(argv[i], path.substr(0, path.size() - 9).c_str());
			}
			if (path.ends_with(".xis.csv"))
			{
				CsvToXiString(argv[i], path.substr(0, path.size() - 8).c_str());
			}
			if (path.ends_with(".evsb.csv"))
			{
				CsvToEventStringData(argv[i], path.substr(0, path.size() - 9).c_str());
			}
			if (path.ends_with(".fp.csv"))
			{
				CsvToFixedPhrase(argv[i], path.substr(0, path.size() - 7).c_str());
			}
			if (path.ends_with(".DAT"))
			{
				static char m[8];
				std::ifstream eye(path, std::ios::binary);
				auto size = std::filesystem::file_size(path);
				eye.read(m, 8);
				int flag = *((int32_t *)m);

				if (strcmp(m, "d_msg") == 0)
				{
					std::wcout << "xistr p=" << path.c_str() << std::endl;
					try
					{
						DMsg f(path);
						f.Read();
						f.ToCsv(path + ".dmsg.csv");
					}
					catch (std::exception &ex)
					{
						std::wcout << "Failed: " << ex.what() << std::endl;
					}
				}
				else if (strcmp(m, "XISTRING") == 0)
				{
					std::wcout << "xistr p=" << path.c_str() << std::endl;
					try
					{
						XiString s(path);
						s.Read();
						s.ToCsv(path + ".xis.csv");
					}
					catch (std::exception &ex)
					{
						std::wcout << "Failed: " << ex.what() << std::endl;
					}
				}
				else if (memcmp(m, "\x02\x01\x01", 4) == 0 || memcmp(m, "\x02\x02\x01", 4) == 0 ||
					memcmp(m, "\x02\x03\x01", 4) == 0 || memcmp(m, "\x02\x04\x02\x01", 4) == 0)
				{
					std::wcout << "fp p=" << path.c_str() << std::endl;
					try
					{
						FixedPhrase fp;
						std::wstring wpath = xybase::string::to_wstring(path);
						fp.Read(wpath);
						fp.ToCsv(wpath + L".fp.csv");
					}
					catch (std::exception& ex)
					{
						std::wcout << "Failed: " << ex.what() << std::endl;
					}
				}
				else if ((flag & 0xFFFFFF) == size - 4)
				{
					{
						std::wcout << "evsb p=" << path.c_str() << std::endl;
						try
						{
							EventStringBase esb(path);
							esb.Read();
							esb.ToCsv(path + ".evsb.csv");
						}
						catch (std::exception &ex)
						{
							std::wcout << "Failed: " << ex.what() << std::endl;
						}
					}
				}
			}
		}
		return 0;
	}

	try
	{
		int ret = lopt_parse(argc, argv);
		if (ret)
		{
			if (ret < 0)
			{
				std::wcerr << std::format(L"{}: 语法有误或是不存在的开关。\n", xybase::string::to_wstring(argv[-ret]));
			}
			exit(ret);
		}
	}
	catch (std::exception &ex)
	{
		std::wcout << ex.what() << std::endl;
	}

	lopt_finalize();

	/*DMsg t(DataManager::GetOutPathConf(1, 165, 64));
	t.FromCsv(DataManager::GetOutPathConf(1, 165, 64) + ".dmsg.csv");
	t.Write();*/

	// ExtractSysText();

	/*DMsg test(DataManager::GetPath(1, 0, 14));
	test.Read();
	test.ToCsv("test.csv");*/
	// read when launch:
	// 165 85 Area Name
	// 175 31 Equip Slot
	// 165 87 Job Name
	// 165 53/54 dmsg 附近 XISTRING 也有 系统文本很多
	// 175 32 
	//
	// 
	// 
	// 168 24 定型文辞书
	//
  /*  DMsg daijinaMono(DataManager::GetPath(1, 175, 34));
	daijinaMono.Read();
	daijinaMono.ToCsv("dm.csv");*/
}

void CsvToDMsg(const char *src, const char *out)
{
	std::string csvPath = src;
	if (!csvPath.ends_with(".csv"))
	{
		std::wcerr << L"指定的文件不是CSV。" << std::endl;
		return;
	}
	std::string datPath = csvPath;
	DMsg f(out ? out : datPath.replace(datPath.find(".csv"), 4, ".DAT"));
	try {
		f.mode = cfg_block ? DMsg::Mode::Block : DMsg::Mode::Variable;
		f.obs = cfg_xor == 1;
		f.FromCsv(csvPath);
		f.Write();
	}
	catch (xybase::RuntimeException &ex)
	{
		std::wcerr << L"发生异常：" << ex.GetMessage() << std::endl;
	}
	catch (std::exception &ex) {
		std::wcerr << L"发生错误：" << ex.what() << std::endl;
	}
}

void CsvToEventStringData(const char *src, const char *out)
{
	std::string csvPath = src;
	if (!csvPath.ends_with(".csv"))
	{
		std::wcerr << L"指定的文件不是CSV。" << std::endl;
		return;
	}
	std::string datPath = csvPath;
	EventStringBase f(out ? out : datPath.replace(datPath.find(".csv"), 4, ".DAT"));
	try {
		f.FromCsv(csvPath);
		f.Write();
	}
	catch (xybase::RuntimeException &ex)
	{
		std::wcerr << L"发生异常：" << ex.GetMessage() << std::endl;
	}
	catch (std::exception &ex) {
		std::wcerr << L"发生错误：" << ex.what() << std::endl;
	}
}

void CsvToXiString(const char *src, const char *out)
{
	std::string csvPath = src;
	if (!csvPath.ends_with(".csv"))
	{
		std::wcerr << L"指定的文件不是CSV。" << std::endl;
		return;
	}
	std::string datPath = csvPath;
	XiString f(out ? out : datPath.replace(datPath.find(".csv"), 4, ".DAT"));
	try {
		f.FromCsv(csvPath);
		f.Write();
	}
	catch (xybase::RuntimeException &ex)
	{
		std::wcerr << L"发生异常：" << ex.GetMessage() << std::endl;
	}
	catch (std::exception &ex) {
		std::wcerr << L"发生错误：" << ex.what() << std::endl;
	}
}

void CsvToFixedPhrase(const char *src, const char *out)
{
	std::string csvPath = src;
	if (!csvPath.ends_with(".csv"))
	{
		std::wcerr << L"指定的文件不是CSV。" << std::endl;
		return;
	}
	std::string datPath = csvPath;
	FixedPhrase f;
	try {
		f.FromCsv(xybase::string::to_wstring(csvPath));
		f.Write(xybase::string::to_wstring(out ? out : datPath.replace(datPath.find(".csv"), 4, ".DAT")));
	}
	catch (xybase::RuntimeException &ex)
	{
		std::wcerr << L"发生异常：" << ex.GetMessage() << std::endl;
	}
	catch (std::exception &ex) {
		std::wcerr << L"发生错误：" << ex.what() << std::endl;
	}
}

void ExtractSysText()
{
	std::filesystem::path bfLogPath = std::filesystem::path(PathUtil::progRootPath) / L"bflogs.csv";
	std::filesystem::path typeLogPath = std::filesystem::path(PathUtil::progRootPath) / L"types.csv";
	std::ofstream bflog(bfLogPath, std::ios::out | std::ios::binary);
	std::ofstream typelog(typeLogPath, std::ios::out | std::ios::binary);

	auto csvEscape = [](const std::string &s) -> std::string
	{
		std::string out;
		out.reserve(s.size() + 2);
		out.push_back('"');
		for (char ch : s)
		{
			if (ch == '"') out += "\"\"";
			else out.push_back(ch);
		}
		out.push_back('"');
		return out;
	};

	auto checkByteAt = [](const std::filesystem::path &p, size_t offset, uint8_t expected) -> bool
	{
		std::ifstream eye(p, std::ios::in | std::ios::binary);
		if (!eye.is_open()) return false;
		eye.seekg((std::streamoff)offset, std::ios::beg);
		char b = 0;
		eye.read(&b, 1);
		return eye.gcount() == 1 && (uint8_t)b == expected;
	};

	auto hasPeriodicFFMarker = [](const std::filesystem::path &p, size_t fileSize, size_t recordSize, size_t minCount = 1) -> bool
	{
		if (recordSize == 0 || fileSize < recordSize) return false;
		size_t count = fileSize / recordSize;
		if (count < minCount) return false;

		std::ifstream eye(p, std::ios::in | std::ios::binary);
		if (!eye.is_open()) return false;

		for (size_t i = 1; i <= count; ++i)
		{
			size_t markerPos = i * recordSize - 1;
			eye.seekg((std::streamoff)markerPos, std::ios::beg);
			char marker = 0;
			eye.read(&marker, 1);
			if (eye.gcount() != 1 || (uint8_t)marker != 0xFF)
			{
				return false;
			}
		}
		return true;
	};

	auto joinTypes = [](const std::set<std::string> &types) -> std::string
	{
		if (types.empty()) return "unknown";
		std::string joined;
		for (const auto &t : types)
		{
			if (!joined.empty()) joined += "|";
			joined += t;
		}
		return joined;
	};

	std::unordered_map<int, std::vector<uint8_t>> vtableCache;
	std::unordered_map<int, std::vector<uint16_t>> ftableCache;

	auto readVTable = [&](int romNumber) -> const std::vector<uint8_t>&
	{
		auto it = vtableCache.find(romNumber);
		if (it != vtableCache.end()) return it->second;

		std::filesystem::path vtablePath;
		if (romNumber == 1)
			vtablePath = std::filesystem::path(PathUtil::gameRootPath) / L"VTABLE.DAT";
		else
			vtablePath = std::filesystem::path(PathUtil::gameRootPath) / (L"ROM" + std::to_wstring(romNumber)) / (L"VTABLE" + std::to_wstring(romNumber) + L".DAT");

		auto &tbl = vtableCache[romNumber];
		if (std::filesystem::exists(vtablePath))
		{
			std::ifstream eye(vtablePath, std::ios::in | std::ios::binary);
			if (eye.is_open())
			{
				eye.seekg(0, std::ios::end);
				size_t sz = (size_t)eye.tellg();
				eye.seekg(0, std::ios::beg);
				tbl.resize(sz);
				eye.read((char *)tbl.data(), sz);
			}
		}
		return tbl;
	};

	auto readFTable = [&](int romNumber) -> const std::vector<uint16_t>&
	{
		auto it = ftableCache.find(romNumber);
		if (it != ftableCache.end()) return it->second;

		std::filesystem::path ftablePath;
		if (romNumber == 1)
			ftablePath = std::filesystem::path(PathUtil::gameRootPath) / L"FTABLE.DAT";
		else
			ftablePath = std::filesystem::path(PathUtil::gameRootPath) / (L"ROM" + std::to_wstring(romNumber)) / (L"FTABLE" + std::to_wstring(romNumber) + L".DAT");

		auto &tbl = ftableCache[romNumber];
		if (std::filesystem::exists(ftablePath))
		{
			std::ifstream eye(ftablePath, std::ios::in | std::ios::binary);
			if (eye.is_open())
			{
				eye.seekg(0, std::ios::end);
				size_t sz = (size_t)eye.tellg();
				eye.seekg(0, std::ios::beg);
				tbl.resize(sz / sizeof(uint16_t));
				eye.read((char *)tbl.data(), tbl.size() * sizeof(uint16_t));
			}
		}
		return tbl;
	};

	auto getGlobalFileId = [&](int romNumber, int localFileId) -> int
	{
		const auto &vtable = readVTable(romNumber);
		const auto &ftable = readFTable(romNumber);
		for (size_t globalId = 0; globalId < ftable.size(); ++globalId)
		{
			if (ftable[globalId] == (uint16_t)localFileId)
			{
				if (globalId < vtable.size() && vtable[globalId] == romNumber)
					return (int)globalId;
			}
		}
		return (romNumber << 24) | (localFileId & 0xFFFFFF);
	};

	auto logBlockFile = [&](const std::filesystem::path &p, const BlockFile &bf)
	{
		bflog << csvEscape(p.string());
		int blockCount = 0;
		bflog << "," << csvEscape(bf.type);
		for (const auto *block : bf.blocks)
		{
			if (blockCount >= 32) break;
			char rawName[5] = { 0 };
			memcpy(rawName, block->blockHeader.name, 4);
			std::string cleanName = (rawName);
			bflog << "," << csvEscape(cleanName + "-" + std::to_string((uint32_t)block->blockHeader.type));
			++blockCount;
		}
		bflog << "\n";
	};

	typelog << "path,global_id,types\n";

	for (int rom = 1; rom < 12; ++rom)
	{
		int cmax = rom == 1 ? 385 : 30;
		for (int c = 0; c < cmax; ++c)
		{
			for (int n = 0; n < 128; ++n)
			{
				std::filesystem::path p = PathUtil::GetPath(rom, c, n);
				if (!std::filesystem::exists(p)) continue;

				auto size = std::filesystem::file_size(p);
				if (size < 4) continue;

				int localFileId = c * 128 + n;
				int globalId = getGlobalFileId(rom, localFileId);
				std::set<std::string> detectedTypes;

				const bool isROM = (rom == 1);
				const bool isFolder96Plus = (c >= 96);
				const bool isROERange = isROM && c == 307 && n >= 15 && n <= 26;
				const bool isMonBridgeRange = isROM && c == 288 && n >= 66 && n <= 69;
				const bool allowDMsgXiFp = isROM && isFolder96Plus;
				const bool allowItemData = isROM;
				const bool allowROE = isROERange;
				const bool allowMonBridge = isMonBridgeRange;

				char m[8] = { 0 };
				int flag = 0;
				{
					std::ifstream eye(p, std::ios::in | std::ios::binary);
					eye.read((char *)&flag, 4);
					eye.seekg(0, std::ios::beg);
					eye.read(m, 8);
				}

				// 1) BlockFile
				if (/*m[4] == 1 && m[5] == 1 && */!m[6] && !m[7] && m[0] && m[1] && m[2] && m[3])
				{
					try
					{
						BlockFile bf(p);
						bf.Read();
						std::wcout << L"blockfile p=" << p << std::endl;
						logBlockFile(p, bf);
						detectedTypes.insert("block");
					}
					catch (...) {}
				}

				// 2) EventStringBase
				if ((flag & 0xFFFFFF) == size - 4)
				{
					std::wcout << L"evsb p=" << p << std::endl;
					try
					{
						EventStringBase esb(p);
						esb.Read();
						esb.ToCsv(PathUtil::GetOutPathConf(rom, c, n) + L".evsb.csv");
						detectedTypes.insert("evsb");
					}
					catch (std::exception &ex)
					{
						std::wcout << L"Failed: " << ex.what() << std::endl;
					}
				}

				// 3) DMsg
				if (allowDMsgXiFp && strcmp(m, "d_msg") == 0)
				{
					std::wcout << L"dmsg p=" << p << std::endl;
					try
					{
						DMsg f(p);
						f.Read();
						f.ToCsv(PathUtil::GetOutPathConf(rom, c, n) + L".dmsg.csv");
						detectedTypes.insert("dmsg");
					}
					catch (std::exception &ex)
					{
						std::wcout << L"Failed: " << ex.what() << std::endl;
					}
				}

				// 4) XiString
				if (allowDMsgXiFp && strcmp(m, "XISTRING") == 0)
				{
					std::wcout << L"xistr p=" << p << std::endl;
					try
					{
						XiString s(p);
						s.Read();
						s.ToCsv(PathUtil::GetOutPathConf(rom, c, n) + L".xis.csv");
						detectedTypes.insert("xis");
					}
					catch (std::exception &ex)
					{
						std::wcout << L"Failed: " << ex.what() << std::endl;
					}
				}

				// 5) FixedPhrase
				if (allowDMsgXiFp && (memcmp(m, "\x02\x01\x01", 4) == 0 || memcmp(m, "\x02\x02\x01", 4) == 0 || memcmp(m, "\x02\x03\x01", 4) == 0 || memcmp(m, "\x02\x04\x01", 4) == 0))
				{
					std::wcout << L"fp p=" << p << std::endl;
					try
					{
						FixedPhrase fp;
						fp.Read(xybase::string::to_wstring(p));
						fp.ToCsv(PathUtil::GetOutPathConf(rom, c, n) + L".fp.csv");
						detectedTypes.insert("fp");
					}
					catch (std::exception &ex)
					{
						std::wcout << L"Failed: " << ex.what() << std::endl;
					}
				}

				// 6) ROR5 + periodic 0xFF heuristic group: ItemData / ROE / MonBridge
				bool itemPeriodic = hasPeriodicFFMarker(p, (size_t)size, sizeof(ItemEntry));
				bool itemCurrencyLike = ((size_t)size == 0xC000) && checkByteAt(p, sizeof(ItemEntry) - 1, 0xFF);
				if (allowItemData && (itemPeriodic || itemCurrencyLike))
				{
					const std::pair<ItemSpecType, const wchar_t *> itemSpecs[] = {
						{ ItemSpecType::NORMAL, L"normal" },
						{ ItemSpecType::USABLE, L"usable" },
						{ ItemSpecType::WEAPON, L"weapon" },
						{ ItemSpecType::ARMOUR, L"armour" },
						{ ItemSpecType::PUPPET, L"puppet" },
						{ ItemSpecType::SLIP, L"slip" },
						{ ItemSpecType::CURRENCY, L"currency" },
					};
					for (const auto &[spec, name] : itemSpecs)
					{
						try
						{
							ItemData item;
							item.Read(xybase::string::to_wstring(p), spec);
							if (!item.data.size() || !item.data.begin()->cellCount())
								continue;
							if (!item.data.empty())
							{
								std::wcout << L"item(" << name << L") p=" << p << std::endl;
								item.ToICsv(PathUtil::GetOutPathConf(rom, c, n) + L".item." + std::wstring(name) + L".csv");
								detectedTypes.insert("item." + xybase::string::to_string(std::wstring(name)));
							}
						}
						catch (...) {}
					}
				}

				bool roeQuestPeriodic = hasPeriodicFFMarker(p, (size_t)size, sizeof(RoeQuestEntry));
				if (allowROE && roeQuestPeriodic)
				{
					try
					{
						RecordsOfEminence roe;
						roe.ReadQuest(p.wstring());
						if (!roe.questData.empty())
						{
							std::wcout << L"roe.quest p=" << p << std::endl;
							roe.QuestToICsv(xybase::string::sys_wcs_to_mbs(PathUtil::GetOutPathConf(rom, c, n) + L".roe.quest.csv").c_str());
							detectedTypes.insert("roe.quest");
						}
					}
					catch (...) {}
				}

				bool roeCategoryPeriodic = hasPeriodicFFMarker(p, (size_t)size, sizeof(RoeCategoryEntry));
				if (allowROE && roeCategoryPeriodic)
				{
					try
					{
						RecordsOfEminence roe;
						roe.ReadCategory(p.wstring());
						if (!roe.categoryData.empty())
						{
							std::wcout << L"roe.category p=" << p << std::endl;
							roe.CategoryToICsv(xybase::string::sys_wcs_to_mbs(PathUtil::GetOutPathConf(rom, c, n) + L".roe.category.csv").c_str());
							detectedTypes.insert("roe.category");
						}
					}
					catch (...) {}
				}

				bool mbPeriodic = hasPeriodicFFMarker(p, (size_t)size, sizeof(MBRecord));
				if (allowMonBridge && mbPeriodic)
				{
					try
					{
						MonBridge mb;
						mb.Read(p.wstring());
						if (!mb.data.empty())
						{
							std::wcout << L"mb p=" << p << std::endl;
							mb.ToICsv(PathUtil::GetOutPathConf(rom, c, n) + L".mb.csv");
							detectedTypes.insert("mb");
						}
					}
					catch (...) {}
				}

				if (!detectedTypes.empty())
					typelog << csvEscape(p.string()) << "," << globalId << "," << csvEscape(joinTypes(detectedTypes)) << "\n";
			}
		}
	}
}

#include "IdResolver.h"

void ExportItemData(void)
{
	IdResolver ir;
	ir.Initialise();

	// Create output directory
	std::filesystem::path exportDir = std::filesystem::path(PathUtil::progRootPath) / L"item_export";
	std::filesystem::create_directories(exportDir);
	std::filesystem::path imageDir = exportDir / L"images";
	std::filesystem::create_directories(imageDir);

	std::wcout << L"Exporting item data..." << std::endl;

	// Read Japanese item data
	ItemData janorm, jausable, jaweapon, jaarmour, japuppet, jaslip, jacurrency, janorm2, jaarmour2, jains;
	MonBridge jamb;

	std::wcout << L"Reading Japanese item data..." << std::endl;
	janorm.Read(ir.GetDatFilePath(4), ItemSpecType::NORMAL);
	jausable.Read(ir.GetDatFilePath(5), ItemSpecType::USABLE);	
	jaweapon.Read(ir.GetDatFilePath(6), ItemSpecType::WEAPON);
	jaarmour.Read(ir.GetDatFilePath(7), ItemSpecType::ARMOUR);
	japuppet.Read(ir.GetDatFilePath(8), ItemSpecType::PUPPET);
	jacurrency.Read(ir.GetDatFilePath(9), ItemSpecType::CURRENCY);
	jaslip.Read(ir.GetDatFilePath(55547), ItemSpecType::SLIP);
	jaarmour2.Read(ir.GetDatFilePath(55548), ItemSpecType::ARMOUR);
	jamb.Read(ir.GetDatFilePath(55549));
	jains.Read(ir.GetDatFilePath(55550), ItemSpecType::INSTINCT);
	janorm2.Read(ir.GetDatFilePath(55551), ItemSpecType::NORMAL);

	// Read English item data
	ItemData ennorm, enusable, enweapon, enarmour, enpuppet, enslip, encurrency, ennorm2, enarmour2, enins;
	MonBridge enmb;

	std::wcout << L"Reading English item data..." << std::endl;
	ennorm.Read(ir.GetDatFilePath(73), ItemSpecType::NORMAL);
	enusable.Read(ir.GetDatFilePath(74), ItemSpecType::USABLE);
	enweapon.Read(ir.GetDatFilePath(75), ItemSpecType::WEAPON);
	enarmour.Read(ir.GetDatFilePath(76), ItemSpecType::ARMOUR);
	enpuppet.Read(ir.GetDatFilePath(77), ItemSpecType::PUPPET);
	encurrency.Read(ir.GetDatFilePath(91), ItemSpecType::CURRENCY);
	enslip.Read(ir.GetDatFilePath(55667), ItemSpecType::SLIP);
	enarmour2.Read(ir.GetDatFilePath(55668), ItemSpecType::ARMOUR);
	enmb.Read(ir.GetDatFilePath(55669));
	enins.Read(ir.GetDatFilePath(55670), ItemSpecType::INSTINCT);
	ennorm2.Read(ir.GetDatFilePath(55671), ItemSpecType::NORMAL);

	// Helper lambda to export a specific item type
	auto exportItemType = [&](const ItemData& jaData, const ItemData& enData, const wchar_t* typeName, ItemSpecType specType) {
		std::wcout << L"Exporting " << typeName << L" items..." << std::endl;

		// Create map of English names by ID
		std::unordered_map<uint32_t, std::tuple<std::u8string, std::u8string, std::u8string, std::u8string>> enNames;
		for (const auto& datum : enData.data) {
			try {
				std::u8string name = datum.name();
				std::u8string sg, pl, desc;
				try { sg = datum.name_sg(); } catch (...) {}
				try { pl = datum.name_pl(); } catch (...) {}
				try { desc = datum.description(); }
				catch (...) {}	
				enNames[datum.id] = std::make_tuple(name, sg, pl, desc);
			} catch (...) {}
		}

		// Open CSV file
		std::filesystem::path csvPath = exportDir / (std::wstring(typeName) + L".csv");
		CsvFile csv(csvPath, std::ios::out | std::ios::binary);

		// Write header
		csv.NewCell(u8"ID");
		csv.NewCell(u8"Name_JA");
		csv.NewCell(u8"Name_EN");
		csv.NewCell(u8"Name_EN_Singular");
		csv.NewCell(u8"Name_EN_Plural");
		csv.NewCell(u8"Description_JA");
		csv.NewCell(u8"Description_EN");
		csv.NewCell(u8"StackSize");
		csv.NewCell(u8"ItemType");
		csv.NewCell(u8"ResourceID");

		// ValidTargets flags
		csv.NewCell(u8"Target_Self");
		csv.NewCell(u8"Target_Player");
		csv.NewCell(u8"Target_Party");
		csv.NewCell(u8"Target_Alliance");
		csv.NewCell(u8"Target_NPC");
		csv.NewCell(u8"Target_Enemy");
		csv.NewCell(u8"Target_Ukn0x40");
		csv.NewCell(u8"Target_Corpse");
		csv.NewCell(u8"Target_Ukn0x100");
		csv.NewCell(u8"Target_Ukn0x200");
		csv.NewCell(u8"Target_Ukn0x400");
		csv.NewCell(u8"Target_Ukn0x800");
		csv.NewCell(u8"Target_Ukn0x1000");
		csv.NewCell(u8"Target_Ukn0x2000");
		csv.NewCell(u8"Target_Ukn0x4000");
		csv.NewCell(u8"Target_Ukn0x8000");

		// Item header flags
		csv.NewCell(u8"Flag_WallDecoration");
		csv.NewCell(u8"Flag_GMItem");
		csv.NewCell(u8"Flag_MysteryBox");
		csv.NewCell(u8"Flag_Ukn1");
		csv.NewCell(u8"Flag_Alt");
		csv.NewCell(u8"Flag_Inscribable");
		csv.NewCell(u8"Flag_NotListable");
		csv.NewCell(u8"Flag_Scroll");
		csv.NewCell(u8"Flag_Linkshell");
		csv.NewCell(u8"Flag_Usable");
		csv.NewCell(u8"Flag_NPCTradeable");
		csv.NewCell(u8"Flag_Equipment");
		csv.NewCell(u8"Flag_Unsellable");
		csv.NewCell(u8"Flag_Unmailable");
		csv.NewCell(u8"Flag_Ex");
		csv.NewCell(u8"Flag_Rare");

		// Add spec-specific headers
		if (specType == ItemSpecType::WEAPON) {
			for (auto&& h : { u8"Level", u8"EquipSlots", u8"Races", u8"Jobs", u8"SuperiorLevel", u8"Ukn2", 
				u8"DMG", u8"Delay", u8"DPS", u8"Skill", u8"Ukn12", u8"Ukn7", u8"Ukn9",
				u8"MaxCharges", u8"CastFactor", u8"UseTime", u8"ReuseTime", u8"Ukn20", 
				u8"RelatedItemId", u8"iLvl", u8"Ukn22", u8"Ukn23" }) {
				csv.NewCell(h);
			}
		} else if (specType == ItemSpecType::ARMOUR) {
			for (auto&& h : { u8"Level", u8"EquipSlots", u8"Races", u8"Jobs", u8"SuperiorLevel", u8"ShieldSize", 
				u8"MaxCharges", u8"CastFactor", u8"UseTime", u8"ReuseTime", u8"Ukn1", 
				u8"RelatedItemId", u8"iLvl", u8"Ukn3", u8"Ukn4" }) {
				csv.NewCell(h);
			}
		} else if (specType == ItemSpecType::USABLE) {
			for (auto&& h : { u8"CastFactor", u8"Ukn1", u8"Ukn2", u8"Ukn3" }) {
				csv.NewCell(h);
			}
		} else if (specType == ItemSpecType::NORMAL) {
			for (auto&& h : { u8"Element", u8"Storage", u8"RelatedItemId", u8"Ukn4", u8"Ukn5" }) {
				csv.NewCell(h);
			}
		} else if (specType == ItemSpecType::PUPPET) {
			for (auto&& h : { u8"Slot_Head", u8"Slot_Body", u8"Slot_Attachment", u8"Slot_Raw",
				u8"Fire", u8"Ice", u8"Air", u8"Earth", u8"Thunder", u8"Water", u8"Light", u8"Dark", 
				u8"Ukn" }) {
				csv.NewCell(h);
			}
		} else if (specType == ItemSpecType::SLIP) {
			for (int i = 0; i < 70; ++i) {
				csv.NewCell(xybase::string::to_utf8(std::string("Ukn") + std::to_string(i)));
			}
		} else if (specType == ItemSpecType::CURRENCY) {
			csv.NewCell(u8"Ukn");
		} else if (specType == ItemSpecType::INSTINCT) {
			for (int i = 0; i < 13; ++i) {
				csv.NewCell(xybase::string::to_utf8(std::string("Ukn") + std::to_string(i)));
			}
		}

		csv.NewLine();

		// Write data rows
		for (const auto& datum : jaData.data) {
			auto toU8 = [](auto v) {
				return xybase::string::to_utf8(std::to_string(v));
			};

			// Get Japanese text
			std::u8string jaName, jaDesc;
			try { jaName = datum.name(); } catch (...) {}
			try { jaDesc = datum.description(); } catch (...) {}

			// Get English text
			std::u8string enName, enSg, enPl, enDesc;
			auto enIt = enNames.find(datum.id);
			if (enIt != enNames.end()) {
				enName = std::get<0>(enIt->second);
				enSg = std::get<1>(enIt->second);
				enPl = std::get<2>(enIt->second);
				enDesc = std::get<3>(enIt->second);
			}

			const auto& hdr = datum.flags();

			// Write basic fields
			csv.NewCell(toU8(datum.id));
			csv.NewCell(jaName);
			csv.NewCell(enName);
			csv.NewCell(enSg);
			csv.NewCell(enPl);
			csv.NewCell(jaDesc);
			csv.NewCell(enDesc);
			csv.NewCell(toU8(datum.stack_size()));
			csv.NewCell(toU8(datum.item_type()));
			csv.NewCell(toU8(datum.resource_id()));

			// ValidTargets flags
			uint16_t targets = datum.valid_targets();
			csv.NewCell((targets & 0x01) ? u8"1" : u8"0"); // Self
			csv.NewCell((targets & 0x02) ? u8"1" : u8"0"); // Player
			csv.NewCell((targets & 0x04) ? u8"1" : u8"0"); // Party
			csv.NewCell((targets & 0x08) ? u8"1" : u8"0"); // Alliance
			csv.NewCell((targets & 0x10) ? u8"1" : u8"0"); // NPC
			csv.NewCell((targets & 0x20) ? u8"1" : u8"0"); // Enemy
			csv.NewCell((targets & 0x40) ? u8"1" : u8"0"); // Ukn0x40
			csv.NewCell((targets & 0x80) ? u8"1" : u8"0"); // Corpse
			csv.NewCell((targets & 0x100) ? u8"1" : u8"0"); // Ukn0x100
			csv.NewCell((targets & 0x200) ? u8"1" : u8"0"); // Ukn0x200
			csv.NewCell((targets & 0x400) ? u8"1" : u8"0"); // Ukn0x400
			csv.NewCell((targets & 0x800) ? u8"1" : u8"0"); // Ukn0x800
			csv.NewCell((targets & 0x1000) ? u8"1" : u8"0"); // Ukn0x1000
			csv.NewCell((targets & 0x2000) ? u8"1" : u8"0"); // Ukn0x2000
			csv.NewCell((targets & 0x4000) ? u8"1" : u8"0"); // Ukn0x4000
			csv.NewCell((targets & 0x8000) ? u8"1" : u8"0"); // Ukn0x8000

			// Write flags as 1/0
			csv.NewCell(hdr.is_wall_decoration ? u8"1" : u8"0");
			csv.NewCell(hdr.is_gm_item ? u8"1" : u8"0");
			csv.NewCell(hdr.is_in_mystery_box ? u8"1" : u8"0");
			csv.NewCell(hdr.ukn_flg1 ? u8"1" : u8"0");
			csv.NewCell(hdr.is_alt ? u8"1" : u8"0");
			csv.NewCell(hdr.is_inscribable ? u8"1" : u8"0");
			csv.NewCell(hdr.is_not_listable ? u8"1" : u8"0");
			csv.NewCell(hdr.is_scroll ? u8"1" : u8"0");
			csv.NewCell(hdr.is_linkshell ? u8"1" : u8"0");
			csv.NewCell(hdr.is_usable ? u8"1" : u8"0");
			csv.NewCell(hdr.is_npc_tradeable ? u8"1" : u8"0");
			csv.NewCell(hdr.is_equipment ? u8"1" : u8"0");
			csv.NewCell(hdr.is_unsellable ? u8"1" : u8"0");
			csv.NewCell(hdr.is_unmailable ? u8"1" : u8"0");
			csv.NewCell(hdr.is_ex ? u8"1" : u8"0");
			csv.NewCell(hdr.is_rare ? u8"1" : u8"0");

			// Write spec-specific fields
			if (specType == ItemSpecType::WEAPON) {
				const auto& spec = datum.originalEntry.spec.weapon;
				csv.NewCell(toU8(spec.level));
				csv.NewCell(toU8(*(uint16_t*)&spec.equip_slots));
				csv.NewCell(toU8(*(uint16_t*)&spec.races));
				csv.NewCell(toU8(*(uint32_t*)&spec.jobs));
				csv.NewCell(toU8(spec.slvl));
				csv.NewCell(toU8(spec.ukn2));
				csv.NewCell(toU8(spec.dmg));
				csv.NewCell(toU8(spec.delay));
				csv.NewCell(toU8(spec.dps));
				csv.NewCell(toU8(spec.skill));
				csv.NewCell(toU8(spec.ukn12));
				csv.NewCell(toU8(spec.ukn7));
				csv.NewCell(toU8(spec.ukn9));
				csv.NewCell(toU8(spec.max_charges));
				csv.NewCell(toU8(spec.cast_factor));
				csv.NewCell(toU8(spec.use_time));
				csv.NewCell(toU8(spec.reuse_time));
				csv.NewCell(toU8(spec.ukn20));
				csv.NewCell(toU8(spec.related_item_id));
				csv.NewCell(toU8(spec.ilvl));
				csv.NewCell(toU8(spec.ukn22));
				csv.NewCell(toU8(spec.ukn23));
			} else if (specType == ItemSpecType::ARMOUR) {
				const auto& spec = datum.originalEntry.spec.armour;
				csv.NewCell(toU8(spec.level));
				csv.NewCell(toU8(*(uint16_t*)&spec.equip_slots));
				csv.NewCell(toU8(*(uint16_t*)&spec.equip_races));
				csv.NewCell(toU8(*(uint32_t*)&spec.equip_jobs));
				csv.NewCell(toU8(spec.slvl));
				csv.NewCell(toU8(spec.shield_size));
				csv.NewCell(toU8(spec.max_charges));
				csv.NewCell(toU8(spec.cast_factor));
				csv.NewCell(toU8(spec.use_time));
				csv.NewCell(toU8(spec.reuse_time));
				csv.NewCell(toU8(spec.ukn1));
				csv.NewCell(toU8(spec.related_item_id));
				csv.NewCell(toU8(spec.ilvl));
				csv.NewCell(toU8(spec.ukn3));
				csv.NewCell(toU8(spec.ukn4));
			} else if (specType == ItemSpecType::USABLE) {
				const auto& spec = datum.originalEntry.spec.usable;
				csv.NewCell(toU8(spec.cast_factor));
				csv.NewCell(toU8(spec.ukn1));
				csv.NewCell(toU8(spec.ukn2));
				csv.NewCell(toU8(spec.ukn3));
			} else if (specType == ItemSpecType::NORMAL) {
				const auto& spec = datum.originalEntry.spec.normal;
				csv.NewCell(toU8(spec.element));
				csv.NewCell(toU8(spec.storage));
				csv.NewCell(toU8(spec.related_item_id));
				csv.NewCell(toU8(spec.ukn4));
				csv.NewCell(toU8(spec.ukn5));
			} else if (specType == ItemSpecType::PUPPET) {
				const auto& spec = datum.originalEntry.spec.puppet;
				csv.NewCell(spec.equip_slots.head ? u8"1" : u8"0");
				csv.NewCell(spec.equip_slots.body ? u8"1" : u8"0");
				csv.NewCell(spec.equip_slots.attachment ? u8"1" : u8"0");
				csv.NewCell(toU8(*(uint16_t*)&spec.equip_slots)); // Raw slot value
				csv.NewCell(toU8(spec.fire));
				csv.NewCell(toU8(spec.ice));
				csv.NewCell(toU8(spec.air));
				csv.NewCell(toU8(spec.earth));
				csv.NewCell(toU8(spec.thunder));
				csv.NewCell(toU8(spec.water));
				csv.NewCell(toU8(spec.light));
				csv.NewCell(toU8(spec.dark));
				csv.NewCell(toU8(spec.ukn));
			} else if (specType == ItemSpecType::SLIP) {
				const auto& spec = datum.originalEntry.spec.slip;
				for (int i = 0; i < 70; ++i) {
					csv.NewCell(toU8(spec.ukn[i]));
				}
			} else if (specType == ItemSpecType::CURRENCY) {
				const auto& spec = datum.originalEntry.spec.currency;
				csv.NewCell(toU8(spec.ukn));
			} else if (specType == ItemSpecType::INSTINCT) {
				const auto& spec = datum.originalEntry.spec.instinct;
				for (int i = 0; i < 13; ++i) {
					csv.NewCell(toU8(spec.ukn[i]));
				}
			}

			csv.NewLine();

			// Export image if exists
			if (datum.image.texture) {
				try {
					auto bmp = datum.image.GetBitmap();
					if (bmp) {
						const BITMAPFILEHEADER* fileHeader = reinterpret_cast<const BITMAPFILEHEADER*>(bmp.get());
						const BITMAPINFOHEADER* infoHeader = reinterpret_cast<const BITMAPINFOHEADER*>(bmp.get() + sizeof(BITMAPFILEHEADER));
						const char* pixelData = bmp.get() + fileHeader->bfOffBits;

						int width = infoHeader->biWidth;
						int height = abs(infoHeader->biHeight);
						int channels = infoHeader->biBitCount / 8;

						// Convert to RGBA if needed
						std::vector<unsigned char> rgba;
						if (channels == 4) {
							// Already RGBA, but need to flip vertically and convert BGRA to RGBA
							// Also convert alpha from FFXI range (0-128) to standard range (0-255)
							rgba.resize(width * height * 4);
							for (int y = 0; y < height; ++y) {
								for (int x = 0; x < width; ++x) {
									int srcIdx = ((height - 1 - y) * width + x) * 4;
									int dstIdx = (y * width + x) * 4;
									rgba[dstIdx + 0] = pixelData[srcIdx + 2]; // R
									rgba[dstIdx + 1] = pixelData[srcIdx + 1]; // G
									rgba[dstIdx + 2] = pixelData[srcIdx + 0]; // B
									// Convert alpha: FFXI uses 0-128 (128=opaque), convert to 0-255
									unsigned char ffxiAlpha = pixelData[srcIdx + 3];
									rgba[dstIdx + 3] = (ffxiAlpha <= 128) ? (ffxiAlpha * 255 / 128) : 255;
								}
							}
						} else if (channels == 3) {
							// RGB, convert to RGBA
							rgba.resize(width * height * 4);
							for (int y = 0; y < height; ++y) {
								for (int x = 0; x < width; ++x) {
									int srcIdx = ((height - 1 - y) * width + x) * 3;
									int dstIdx = (y * width + x) * 4;
									rgba[dstIdx + 0] = pixelData[srcIdx + 2]; // R
									rgba[dstIdx + 1] = pixelData[srcIdx + 1]; // G
									rgba[dstIdx + 2] = pixelData[srcIdx + 0]; // B
									rgba[dstIdx + 3] = 255; // A (full opaque for RGB)
								}
							}
						} else if (channels == 1) {
							// Indexed color with palette
							const unsigned char* palette = reinterpret_cast<const unsigned char*>(bmp.get() + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));
							rgba.resize(width * height * 4);
							for (int y = 0; y < height; ++y) {
								for (int x = 0; x < width; ++x) {
									int srcIdx = (height - 1 - y) * width + x;
									int dstIdx = (y * width + x) * 4;
									unsigned char paletteIdx = pixelData[srcIdx];
									rgba[dstIdx + 0] = palette[paletteIdx * 4 + 2]; // R
									rgba[dstIdx + 1] = palette[paletteIdx * 4 + 1]; // G
									rgba[dstIdx + 2] = palette[paletteIdx * 4 + 0]; // B
									// Convert alpha: FFXI uses 0-128 (128=opaque), convert to 0-255
									unsigned char ffxiAlpha = palette[paletteIdx * 4 + 3];
									rgba[dstIdx + 3] = (ffxiAlpha <= 128) ? (ffxiAlpha * 255 / 128) : 255;
								}
							}
						}

						if (!rgba.empty()) {
							std::filesystem::path pngPath = imageDir / (std::to_wstring(datum.id) + L".png");
							stbi_write_png(xybase::string::sys_wcs_to_mbs(pngPath.wstring()).c_str(), width, height, 4, rgba.data(), width * 4);
						}
					}
				} catch (...) {
					// Ignore image export errors
				}
			}
		}

		csv.Close();
	};

	// Export all item types
	exportItemType(janorm, ennorm, L"items_normal", ItemSpecType::NORMAL);
	exportItemType(jausable, enusable, L"items_usable", ItemSpecType::USABLE);
	exportItemType(jaweapon, enweapon, L"items_weapon", ItemSpecType::WEAPON);
	exportItemType(jaarmour, enarmour, L"items_armour", ItemSpecType::ARMOUR);
	exportItemType(japuppet, enpuppet, L"items_puppet", ItemSpecType::PUPPET);
	exportItemType(jaslip, enslip, L"items_slip", ItemSpecType::SLIP);
	exportItemType(jacurrency, encurrency, L"items_currency", ItemSpecType::CURRENCY);
	exportItemType(jains, enins, L"items_instinct", ItemSpecType::INSTINCT);

	// Handle additional armour and normal files
	std::wcout << L"Exporting additional armour items..." << std::endl;
	exportItemType(jaarmour2, enarmour2, L"items_armour2", ItemSpecType::ARMOUR);
	std::wcout << L"Exporting additional normal items..." << std::endl;
	exportItemType(janorm2, ennorm2, L"items_normal2", ItemSpecType::NORMAL);

	std::wcout << L"Item export complete! Files saved to: " << exportDir << std::endl;
}
