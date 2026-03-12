// FFXITrans.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <string>
#include <fstream>
#include <map>
#include <conio.h>
#include <exception>
#include <filesystem>
#include <xystring.h>
#include <vector>
#include <sstream>
#include <set>
#include <unordered_set>
#include <regex>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <CsvFile.h>

#include <XiString.h>
#include <DMsg.h>
#include <EventStringBase.h>
#include <StatusData.h>
#include <ItemData.h>
#include <FixedPhrase.h>
#include <MonBridge.h>
#include <RecordsOfEminence.h>

#include "liteopt.h"

namespace fs = std::filesystem;

fs::path gameRoot, progRoot;
fs::path outRoot = "./output";
bool englishMode = false; // if true, only process English files (PlayOnlineEU)
bool in_situ_noprompt = false;
bool backup_enabled = true;
bool backup_noprompt = false;
bool no_mismatch_log = false;

#include "../FFXIDatProcessor/codepage.h"
#include "ChsToSJis.h"

std::map<std::u8string, std::u8string> textMapping;
std::map<std::u8string, std::u8string> commentToJpPath;
std::unordered_set<std::u8string> mismatchSet;

// 失配文本统计和文件输出
int mismatchCount = 0;
std::ofstream mismatchFile;

std::u8string GetTranslation(const std::u8string &text) {
	std::u8string translation;

	auto itr = textMapping.find(text);
	if (itr == textMapping.end())
	{
		// 统计失配文本数量
		mismatchCount++;
		
		// 将失配文本写入文件
		if (mismatchSet.find(text) == mismatchSet.end()) {
			mismatchSet.insert(text);
			if (mismatchFile.is_open()) {
				mismatchFile.write(reinterpret_cast<const char*>(text.c_str()), text.length());
				mismatchFile << "\n";
			}
		}
		
		return text;
	}

	translation = itr->second;
	translation = ChsToSJis::Instance().ReplaceHanzi(translation);

	return translation;
}

// 解析cell索引字符串，如 "2|3" 返回 {2, 3}
std::set<int> ParseCellIndices(const std::u8string &cellIndicesStr) {
	std::set<int> indices;
	if (cellIndicesStr.empty()) {
		return indices;
	}
	
	std::string str = reinterpret_cast<const char*>(cellIndicesStr.c_str());
	std::stringstream ss(str);
	std::string token;
	
	while (std::getline(ss, token, '|')) {
		try {
			int index = std::stoi(token);
			if (index > 0) { // 确保索引有效（从1开始）
				indices.insert(index);
			}
		}
		catch (const std::exception&) {
			// 忽略无效的索引
		}
	}
	
	return indices;
}

int PathInit()
{
	HKEY hKey;
	const wchar_t *subKey1 = L"SOFTWARE\\WOW6432Node\\PlayOnline\\InstallFolder";
	const wchar_t *subKey2 = L"SOFTWARE\\WOW6432Node\\PlayOnlineEU\\InstallFolder";
	const wchar_t* subKey3 = L"SOFTWARE\\WOW6432Node\\PlayOnlineUS\\InstallFolder";
	const wchar_t *valueName = L"0001";
	wchar_t valueData[MAX_PATH];
	DWORD bufferSize = sizeof(valueData);

	DWORD valueType;
	// Try primary key first (priority for JP)
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, subKey1,0, KEY_READ, &hKey) == ERROR_SUCCESS) {
		if (RegQueryValueExW(hKey, valueName, nullptr, &valueType, reinterpret_cast<LPBYTE>(valueData), &bufferSize) == ERROR_SUCCESS) {
			if (valueType == REG_SZ) {
				gameRoot = valueData;
				englishMode = false;
			}
			else
			{
				std::wcerr << L"未能正确读取注册表信息，请检查游戏安装信息。\n";
				RegCloseKey(hKey);
				return -1;
			}
		}
		else
		{
			std::wcerr << L"发现了POL的安装信息，但是没有找到游戏的。\n";
			RegCloseKey(hKey);
			return -1;
		}
		RegCloseKey(hKey);
	}
	else if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, subKey2,0, KEY_READ, &hKey) == ERROR_SUCCESS) {
		// Fallback: PlayOnlineEU -> only process English
		if (RegQueryValueExW(hKey, valueName, nullptr, &valueType, reinterpret_cast<LPBYTE>(valueData), &bufferSize) == ERROR_SUCCESS) {
			if (valueType == REG_SZ) {
				gameRoot = valueData;
				englishMode = true;
			}
			else
			{
				std::wcerr << L"未能正确读取注册表信息，请检查游戏安装信息（EU）。\n";
				RegCloseKey(hKey);
				return -1;
			}
		}
		else
		{
			std::wcerr << L"发现了POL(EU)的安装信息，但是没有找到游戏的。\n";
			RegCloseKey(hKey);
			return -1;
		}
		RegCloseKey(hKey);
	}
	else if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, subKey3, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
		// Fallback: PlayOnlineEU -> only process English
		if (RegQueryValueExW(hKey, valueName, nullptr, &valueType, reinterpret_cast<LPBYTE>(valueData), &bufferSize) == ERROR_SUCCESS) {
			if (valueType == REG_SZ) {
				gameRoot = valueData;
				englishMode = true;
			}
			else
			{
				std::wcerr << L"未能正确读取注册表信息，请检查游戏安装信息（US）。\n";
				RegCloseKey(hKey);
				return -1;
			}
		}
		else
		{
			std::wcerr << L"发现了POL(EU)的安装信息，但是没有找到游戏的。\n";
			RegCloseKey(hKey);
			return -1;
		}
		RegCloseKey(hKey);
	}
	else
	{
		std::wcerr << L"您的计算机中并未安装FFXI或POL，程序无法继续。\n";
		return -1;
	}

	wchar_t module_path[MAX_PATH];
	if (GetModuleFileNameW(NULL, module_path, MAX_PATH))
	{
		std::filesystem::path exePath = module_path;
		progRoot = exePath.parent_path();
	}
	std::wcout << L"路径初始化完毕。\n游戏路径：" << gameRoot << L"\n程序数据路径：" << progRoot << std::endl;
	if (englishMode) std::wcout << L"注意：检测到 PlayOnlineEU 注册表项，程序将仅处理英文文件。" << std::endl;
	return 0;
}

int LoadText(int seq)
{
	fs::path textPath = progRoot / (std::string("text") + std::to_string(seq) + ".txt");
	fs::path transPath = progRoot / (std::string("text") + std::to_string(seq) + "_translated.txt");
	if (seq == 0) {
		textPath = progRoot / "text.txt";
		transPath = progRoot / "text_translated.txt";
	}
	std::wcout << L"读取：" << textPath << L" -=- " << transPath << std::endl;

	std::ifstream
		oEye(textPath, std::ios::in | std::ios::binary),
		tEye(transPath, std::ios::in | std::ios::binary);
	std::string text;
	std::string trans;

	// 读取开头3字节，检查是否有BOM
	char bom[3] = { 0 };
	oEye.read(bom, 3);
	if (bom[0] == char(0xEF) && bom[1] == char(0xBB) && bom[2] == char(0xBF)) {
		// 有BOM，继续正常读取
	}
	else if (bom[0] == char(0xFF) && bom[1] == char(0xFE)) {
		std::wcerr << L"不支持的编码格式（UTF-16 LE），请转换为UTF-8编码。\n";
		return -1;
	}
	else if (bom[0] == char(0xFE) && bom[1] == char(0xFF)) {
		std::wcerr << L"不支持的编码格式（UTF-16 BE），请转换为UTF-8编码。\n";
		return -1;
	}
	else {
		// 没有BOM，重置文件指针到开头
		oEye.seekg(0);
	}

	tEye.read(bom, 3);
	if (bom[0] == char(0xEF) && bom[1] == char(0xBB) && bom[2] == char(0xBF)) {
		// 有BOM，继续正常读取
	}
	else if (bom[0] == char(0xFF) && bom[1] == char(0xFE)) {
		std::wcerr << L"不支持的编码格式（UTF-16 LE），请转换为UTF-8编码。\n";
		return -1;
	}
	else if (bom[0] == char(0xFE) && bom[1] == char(0xFF)) {
		std::wcerr << L"不支持的编码格式（UTF-16 BE），请转换为UTF-8编码。\n";
		return -1;
	}
	else {
		// 没有BOM，重置文件指针到开头
		tEye.seekg(0);
	}

	int i = 0;
	while (std::getline(oEye, text)) {
		if (!std::getline(tEye, trans))
		{
			std::wcerr << L"翻译文件和原文文件的行数不一致。\n";
			return i;
		}
		// trim trailing \r if exists
		if (!text.empty() && text.back() == '\r') {
			text.pop_back();
		}
		if (!trans.empty() && trans.back() == '\r') {
			trans.pop_back();
		}
		textMapping[(char8_t *)text.c_str()] = (char8_t *)trans.c_str();
		++i;
	}
	//std::wcout << L"读取了" << i << L"条文本翻译数据。\n";
	return i;
}

int YesNoPrompt(const std::wstring &prompt)
{
	std::wcout << prompt << L"（Y-是，N-否）" << std::endl;
	int key = toupper(_getch());
	while (key != 'Y' && key != 'N')
		key = toupper(_getch());

	return key;
}

void BackupGameFile(fs::path path)
{
	fs::path gamePath = gameRoot / path;
	fs::path backPath = progRoot / "backup" / path;
	if (!fs::exists(backPath.parent_path()))
		fs::create_directories(backPath.parent_path());
	fs::copy(gamePath, backPath, fs::copy_options::skip_existing);
}


struct FileDef {
	std::u8string comment;
	std::u8string lang;
	std::u8string path;
	std::u8string type;
};

void ExtractEventStringBase(const FileDef &def, std::set<std::u8string> &processedStrings, const fs::path &outputDir)
{
	fs::path inputPath = gameRoot / def.path;
	fs::path outputPath = outputDir / def.lang / xybase::string::to_wstring(def.comment + u8".txt");
	std::wcout << L"正在处理 " << inputPath << L" -> " << outputPath << std::endl;
	EventStringBase esb(inputPath);
	esb.Read();
	std::vector<std::u8string> extractedStrings;
	for (const auto &str : esb)
	{
		if (processedStrings.find(str) == processedStrings.end())
		{
			extractedStrings.push_back(str);
			processedStrings.insert(str);
		}
	}
	if (!extractedStrings.empty())
	{
		std::ofstream out(outputPath, std::ios::out | std::ios::binary);
		out.write("\xEF\xBB\xBF", 3); // UTF-8 BOM
		for (const auto &str : extractedStrings)
		{
			out.write(reinterpret_cast<const char*>(str.c_str()), str.length());
			out << "\n";
		}
		std::wcout << L"提取了 " << extractedStrings.size() << L" 条文本。\n";
	}
	else
	{
		std::wcout << L"没有新的文本需要提取。\n";
	}
}

void PrepareSourceData()
{
	fs::path outputDir{ L"data/src" };
	fs::path en = outputDir / L"en", ja = outputDir / L"ja";
	//fs::path zh = outputDir / L"zh";

	CsvFile defs(progRoot / L"defs.csv", std::ios::in | std::ios::binary);

	std::list<FileDef> fileDefs;

	while (!defs.IsEof())
	{
		std::u8string path = defs.NextCell();
		std::u8string type = defs.NextCell();
		std::u8string lang = defs.NextCell();
		std::u8string comment = defs.NextCell();
		if (comment.empty() || lang.empty() || path.empty() || type.empty())
			break;
		fileDefs.push_back({ comment, lang, path, type });
		commentToJpPath[comment] = path;
	}

	// 处理 ev/* 和 evx/* 文件
	// 首先处理 ev/dbg_scene，
	std::set<std::u8string> processedStrings; // 避免重复处理同一文本
	auto itr = std::find_if(fileDefs.begin(), fileDefs.end(), [](const FileDef &def) {
		return def.comment == u8"ev/dbg_scene" && def.lang == u8"jp";
		});
	auto dbg_scene_jp = *itr;
	itr = fileDefs.erase(itr);
	// 再找英语
	itr = std::find_if(fileDefs.begin(), fileDefs.end(), [](const FileDef &def) {
		return def.comment == u8"ev/dbg_scene" && def.lang == u8"en";
		});
	auto dbg_scene_en = *itr;
	itr = fileDefs.erase(itr);

	// 这两个文件中含有许多在其他文件中也会出现的文本，先处理它们，提取出文本后再处理其他文件时就能避免重复提取同一文本了。
	ExtractEventStringBase(dbg_scene_jp, processedStrings, outputDir);
	ExtractEventStringBase(dbg_scene_en, processedStrings, outputDir);

	// 现在处理剩下的 ev/* 和 evx/* 文件
	itr = fileDefs.begin();
	while (itr != fileDefs.end())
	{
		if ((itr->comment.starts_with(u8"ev/") || itr->comment.starts_with(u8"evx/")) && itr->lang == u8"jp")
		{
			ExtractEventStringBase(*itr, processedStrings, outputDir);
			itr = fileDefs.erase(itr);
		}
		else
			++itr;
	}

	// 处理 itm/ 类型的文件
	// Item 类型的数据特殊处理，制作为 ID，名字，描述 的匹配
	itr = fileDefs.begin();
	while (itr != fileDefs.end())
	{
		if (itr->path.starts_with(u8"itm/"))
		{
			ItemData id;
			id.Read(gameRoot / itr->path);

			CsvFile output(progRoot / xybase::string::to_wstring(itr->comment + u8".csv"), std::ios::out | std::ios::binary);

			output.NewCell(u8"ID");
			output.NewCell(u8"Name");
			output.NewCell(u8"Description");
			output.NewLine();

			for (auto datum : id.data)
			{
				if (datum.name() == u8".")
					continue; // 跳过无效条目

				output.NewCell(xybase::string::itos<char8_t>(datum.id));
				output.NewCell(datum.name());
				output.NewCell(datum.description());
				output.NewLine();
			}
			itr = fileDefs.erase(itr);
		}
		else
			++itr;
	}

	// 处理剩下的文件
	for (const auto &def : fileDefs)
	{
		fs::path inputPath = gameRoot / def.path;
		fs::path outputPath = outputDir / def.lang / xybase::string::to_wstring(def.comment + u8".txt");
		std::wcout << L"正在处理 " << inputPath << L" -> " << outputPath << std::endl;
		std::vector<std::u8string> extractedStrings;
		if (def.type == u8"sd")
		{
			StatusData data;
			data.Read(inputPath);
			for (const auto &entry : data.data)
			{
				extractedStrings.push_back(entry.description);
			}
		}
		else if (def.type == u8"fp")
		{
			FixedPhrase data;
			data.Read(inputPath);
			data.ToCsv(progRoot / xybase::string::to_wstring(def.comment + u8".csv"));
		}
		else if (def.type == u8"mb")
		{
			MonBridge data;
			data.Read(inputPath);
			for (const auto &entry : data.data)
			{
				extractedStrings.push_back(entry.displayName);
			}
		}
		else if (def.type == u8"erq")
		{
			RecordsOfEminence data;
			data.ReadQuest(inputPath);
			for (const auto &entry : data.questData)
			{
				extractedStrings.push_back(entry.questName());
				extractedStrings.push_back(entry.description());
				extractedStrings.push_back(entry.note());
			}
		}
		else if (def.type == u8"erc")
		{
			RecordsOfEminence data;
			data.ReadCategory(inputPath);
			for (const auto &entry : data.categoryData)
			{
				extractedStrings.push_back(entry.categoryName());
			}
		}
		else if (def.type == u8"dmsg")
		{
			DMsg data(inputPath);
			for (const auto& datum : data)
			{
				for (const auto& datulus : datum)
				{
					if (datulus.GetType() == 0) // string type
					{
						extractedStrings.push_back(datulus.Get<std::u8string>());
					}
				}
			}
		}
		else if (def.type == u8"xis")
		{
			XiString data(inputPath);
			data.Read();
			for (const auto& str : data)
			{
				extractedStrings.push_back(xybase::string::to_utf8(XiString::Decode(str.str)));
			}
		}
		
		if (!extractedStrings.empty())
		{
			std::ofstream out(outputPath, std::ios::out | std::ios::binary);
			out.write("\xEF\xBB\xBF", 3); // UTF-8 BOM
			for (const auto &str : extractedStrings)
			{
				auto escapedStr = xybase::string::escape(str);
				out.write(reinterpret_cast<const char*>(escapedStr.c_str()), escapedStr.length());
				out << "\n";
			}
			std::wcout << L"提取了 " << extractedStrings.size() << L" 条文本。\n";
		}
		
	}
}

int main(int argc, char **argv)
{
	setlocale(LC_ALL, "");
	bool in_situ = false;

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
			if (PathInit())
			{
				system("pause");
				exit(-1);
			}
			PrepareSourceData();
		}
		if (cmd == "insitu")
			in_situ = true;
		else
		{
			std::wcout << L"FFXI汉化插入工具 Ver.0.10-alpha by Hyururu\n"
				L"用法：FFXITrans [insitu]\n"
				L"  insitu：直接在游戏目录修改文件，否则输出到output目录\n"
				L"  prepare：输出要准备的游戏数据文件（翻译用）\n"
				L"  无参数则进入交互模式\n";
			return 0;
		}
	}
	try
	{
		std::wcout << L"FFXI汉化插入工具 Ver.0.10-alpha by Hyururu" << std::endl;
		if (PathInit())
		{
			system("pause");
			exit(-1);
		};

		if (fs::exists(progRoot / "config.ini"))
		{
			// 读取配置文件
			std::wcout << L"读取配置文件中..." << std::endl;
			std::wifstream configFile(progRoot / "config.ini");
			std::wstring line;
			while (std::getline(configFile, line))
			{
				// game_path=
				// in_situ=
				// english_mode=
				// output_path=
				
				// 读取有效 INI 配置，忽略注释空行，允许空格
				line = std::regex_replace(line, std::wregex(L"^\\s+|\\s+$"), L""); // trim
				if (line.empty() || line[0] == L';' || line[0] == L'#')
					continue;
				auto delimiterPos = line.find(L'=');
				if (delimiterPos == std::wstring::npos)
					continue;
				std::wstring key = line.substr(0, delimiterPos);
				std::wstring value = line.substr(delimiterPos + 1);
				key = std::regex_replace(key, std::wregex(L"^\\s+|\\s+$"), L""); // trim
				value = std::regex_replace(value, std::wregex(L"^\\s+|\\s+$"), L""); // trim
				if (key == L"game_path")
				{
					gameRoot = value;
					std::wcout << L"使用配置文件中的游戏路径：" << gameRoot << std::endl;
				}
				else if (key == L"in_situ")
				{
					in_situ = (value == L"1" || value == L"true" || value == L"yes");
					in_situ_noprompt = true;
				}
				else if (key == L"english_mode")
				{
					englishMode = (value == L"1" || value == L"true" || value == L"yes");
				}
				else if (key == L"output_path")
				{
					if (!in_situ)
					{
						outRoot = value;
						std::wcout << L"使用配置文件中的输出路径：" << gameRoot << std::endl;
					}
				}
				else if (key == L"no_mismatch_log")
				{
					no_mismatch_log = (value == L"1" || value == L"true" || value == L"yes");
				}
			}
			configFile.close();
		}
		
		// 初始化失配文件（UTF-8无BOM）
		fs::path mismatchPath = progRoot / "text_mismatch.txt";
		if (no_mismatch_log) {
			std::wcout << L"配置为不输出失配文本。\n";
		}
		else {
			mismatchFile.open(mismatchPath, std::ios::out | std::ios::binary);
			if (!mismatchFile.is_open()) {
				std::wcerr << L"无法创建失配文本文件：" << mismatchPath << std::endl;
			}
		}
		
		try
		{
			CodeCvt::GetInstance().Init(progRoot / L"cp932.csv");
		}
		catch (std::exception &ex)
		{
			std::wcerr << ex.what() << std::endl;
			std::wcerr << L"处理代码页cp932.csv失败了。" << std::endl;
			system("pause");
			return -2;
		}
		try
		{
			ChsToSJis::Instance().Init(progRoot / L"chs2sjis.csv");
		}
		catch (std::exception &ex)
		{
			std::wcerr << ex.what() << std::endl;
			std::wcerr << L"处理简体汉字转换逻辑chs2sjis.csv失败了。" << std::endl;
			system("pause");
			return -3;
		}

		for (int i = 0; LoadText(i); ++i);
		std::wcout << L"共读取了 " << std::to_wstring(textMapping.size()) << L" 条文本数据。" << std::endl;

		bool backupExist = false;
		if (fs::exists(progRoot / "backup")) {
			backupExist = true;

			if (in_situ)
			{
				std::wcout << L"恢复备份中..." << std::endl;
				std::error_code ec;
				fs::copy(progRoot / "backup", gameRoot, fs::copy_options::overwrite_existing | fs::copy_options::recursive, ec);
				if (ec)
				{
					std::wcerr << L"恢复备份时发生了问题：" << xybase::string::sys_mbs_to_wcs(ec.message()) << std::endl;
					system("pause");
					return -3;
				}
				std::wcout << L"备份的恢复完成了。" << std::endl;
			}
			else
			{
				int key = YesNoPrompt(L"发现了备份数据。您希望先恢复备份吗？");

				if (key == 'Y')
				{
					std::error_code ec;
					fs::copy(progRoot / "backup", gameRoot, fs::copy_options::overwrite_existing | fs::copy_options::recursive, ec);
					/*if ('Y' == YesNoPrompt(L"您希望删除备份吗？"))
						fs::remove_all(progRoot / "backup");*/

					if (ec)
					{
						std::wcerr << L"恢复备份时发生了问题：" << xybase::string::sys_mbs_to_wcs(ec.message()) << std::endl;
						system("pause");
						return -3;
					}
					std::wcout << L"备份的恢复完成了。" << std::endl;
					if ('Y' == YesNoPrompt(L"要退出程序吗？"))
					{
						return 0;
					}
				}
			}
		}

		std::wcout << L"可以在原位修改游戏文件，或将翻译后的数据输出到 output。" << std::endl;
		std::wcout << L"如果不希望使用插件，请在原位修改。" << std::endl;
		bool overwrite;
		
		if (in_situ)
			overwrite = true;
		else 
			overwrite = in_situ_noprompt ? false : (YesNoPrompt(L"要在原位修改游戏文件吗？") == 'Y');

		if (overwrite)
		{
			std::wcout << L"将在原位修改游戏文件。文件修改前将被备份。" << std::endl;
		}
		std::wcout << L"开始处理文件，请勿关闭程序。" << std::endl;

		CsvFile def(progRoot / "defs.csv", std::ios::in | std::ios::binary);
		// No cross-language mapping pass. We'll process only the detected language:
		// if englishMode == true -> only process entries with lang == "en"
		// else -> only process entries with lang == "jp"
		if (englishMode) {
			// First pass: build commentToJpPath mapping from jp entries
			while (!def.IsEof())
			{ 
				std::u8string path = def.NextCell();
				std::u8string type = def.NextCell();
				std::u8string lang = def.NextCell();
				std::u8string comm = def.NextCell();
				def.NextLine();
				if (lang == u8"jp") {
					commentToJpPath[comm] = path;
				}
			}
			def.Rewind();
		}

		while (!def.IsEof())
		{ 
			std::u8string path = def.NextCell();
			std::u8string type = def.NextCell();
			std::u8string lang = def.NextCell();
			std::u8string comm = def.NextCell();
			
			// 读取可选的第5个cell，用于指定dmsg的特定cell进行翻译
			std::u8string cellIndicesStr;
			if (!def.IsEol()) {
				cellIndicesStr = def.NextCell();
			}
			def.NextLine();

			// 仅处理日文和英文文件
			if (englishMode)
			{
				if (lang != u8"en")
					continue;
			}
			else
			{
				if (lang != u8"jp")
					continue;
			}
			
			// 输出处理信息（语言 + 文件路径 + 类型 + 注释）
			std::wcout << L"处理中：" << L" 文件 "
				<< xybase::string::to_wstring(path)
				<< L"(" << xybase::string::to_wstring(type)
				<< L") [" << xybase::string::to_wstring(comm) << L"]\r";

			fs::path relaPath = path + u8".DAT";
			fs::path datPath = gameRoot / relaPath;
			fs::path outPath = overwrite ? datPath : outRoot / relaPath;
			if (!fs::exists(datPath)) continue;
			if (!fs::exists(outPath.parent_path()))
			{
				fs::create_directories(outPath.parent_path());
			}
			if (overwrite)
				BackupGameFile(relaPath);
			if (type == u8"xis")
			{
				XiString xis(datPath);
				xis.Read();
				int rowNum = 1;
				for (auto &str : xis)
				{
					std::u8string text = xybase::string::escape(xybase::string::to_utf8(xis.Decode(str.str)));

					str.str = xis.Encode(xybase::string::to_string(xybase::string::unescape(GetTranslation(text))));
				}
				xis.path = outPath;
				xis.Write();
			}
			else if (type == u8"evsb")
			{
				EventStringBase evsb(datPath);
				evsb.Read();
				for (auto &s : evsb)
				{
					auto res = GetTranslation(s);
					s = res;
				}
				evsb.path = outPath;
				evsb.Write();
			}
			else if (type == u8"dmsg")
			{
				DMsg dmsg(datPath);
				dmsg.Read();
				
				// 解析要翻译的cell索引
				std::set<int> targetCells = ParseCellIndices(cellIndicesStr);
				bool translateAllCells = targetCells.empty(); // 如果没有指定索引，翻译所有cell
				
				int rowNum = 1;
				for (auto &row : dmsg)
				{
					int colNum = 1;
					for (auto &cell : row)
					{
						if (cell.GetType() == 0) // str
						{
							// 检查是否需要翻译这个cell
							bool shouldTranslate = translateAllCells || targetCells.count(colNum) > 0;
							
							if (shouldTranslate) {
								std::u8string text = xybase::string::escape(cell.Get<std::u8string>());
								cell.Set(xybase::string::unescape(GetTranslation(text)));
							}
						}
						++colNum;
					}
					++rowNum;
				}
				dmsg.path = outPath;
				dmsg.Write();
			}
			else if (type == u8"sd")
			{
				StatusData statusData;
				statusData.Read(datPath);
				for (auto &datum : statusData.data)
				{
					if (!datum.description.empty())
					{
						std::u8string text = xybase::string::escape(datum.description);
						datum.description = xybase::string::unescape(GetTranslation(text));
					}
				}
				statusData.Write(outPath);
			}
			else if (type == u8"fp")
			{
				FixedPhrase fixedPhrase;
				fixedPhrase.Read(datPath);
				for (auto& category : fixedPhrase.categories)
				{
					category.categoryName = GetTranslation(category.categoryName);
					category.categoryPron = GetTranslation(category.categoryPron);
					for (auto&& entry : category.entries)
					{
						std::u8string text = entry.text;
						std::u8string pron = entry.pron;

						if (!text.empty())
						{
							entry.text = GetTranslation(text);
						}
						if (!pron.empty())
						{
							entry.pron = GetTranslation(pron);
						}
					}
				}
				fixedPhrase.Write(outPath);
			}
			else if (type == u8"iab" || type == u8"iwb" || type == u8"iub" || type == u8"inb" || type == u8"ipb" || type == u8"isb" || type == u8"icb")
			{

				// 解析要翻译的cell索引
				std::set<int> targetCells = ParseCellIndices(cellIndicesStr);
				bool translateAllCells = targetCells.empty(); // 如果没有指定索引，翻译所有cell

				ItemData itemData;
				ItemSpecType specType = ItemSpecType::NORMAL;
				
				// Determine spec type based on type parameter
				if (type == u8"iab") {
					specType = ItemSpecType::ARMOUR;
				}
				else if (type == u8"iwb") {
					specType = ItemSpecType::WEAPON;
				}
				else if (type == u8"iub") {
					specType = ItemSpecType::USABLE;
				}
				else if (type == u8"inb") {
					specType = ItemSpecType::NORMAL;
				}
				else if (type == u8"ipb") {
					specType = ItemSpecType::PUPPET;
				}
				else if (type == u8"isb") {
					specType = ItemSpecType::SLIP;
				}
				else if (type == u8"icb") {
					specType = ItemSpecType::CURRENCY;
				}
				
				itemData.Read(datPath, specType);
				for (auto &datum : itemData.data)
				{
					int cellIndex = 1;
					for (auto& cell : datum.row())
					{
						if (cell.GetType() == 0) // str
						{
							bool shouldTranslate = translateAllCells || targetCells.count(cellIndex) > 0;
							if (shouldTranslate) {
								std::u8string text = xybase::string::escape(cell.Get<std::u8string>());
								cell.Set(xybase::string::unescape(GetTranslation(text)));
							}
						}
						++cellIndex;
					}
				}
				itemData.Write(outPath);
			}
			else if (type == u8"mbd")
			{
				MonBridge monBridge;
				monBridge.Read(datPath);
				
				for (auto &datum : monBridge.data)
				{
					// Only translate display name (internal name must NOT be translated)
					// Internal name is the ASCII identifier that the game uses to find entries
					if (!datum.displayName.empty())
					{
						std::u8string text = xybase::string::escape(datum.displayName);
						datum.displayName = xybase::string::unescape(GetTranslation(text));
					}
				}
				monBridge.Write(outPath);
			}
			else if (type == u8"erq") // ROM/307/15 - Quest entries
			{
				RecordsOfEminence roe;
				roe.ReadQuest(datPath);

				std::set<int> targetCells = ParseCellIndices(cellIndicesStr);
				
				for (auto &datum : roe.questData)
				{
					int cellIndex = 1;
					for (auto& cell : datum.row())
					{
						if (cell.GetType() == 0) // str
						{
							bool shouldTranslate = targetCells.empty() || targetCells.count(cellIndex) > 0;
							if (shouldTranslate) {
								std::u8string text = xybase::string::escape(cell.Get<std::u8string>());
								cell.Set(xybase::string::unescape(GetTranslation(text)));
							}
						}
						++cellIndex;
					}
				}
				roe.WriteQuest(outPath);
			}
			else if (type == u8"erc") // ROM/307/23 - Category entries
			{
				RecordsOfEminence roe;
				roe.ReadCategory(datPath);
				
				for (auto &datum : roe.categoryData)
				{
					// Translate category name
					try {
						std::u8string catName = datum.categoryName();
						if (!catName.empty())
						{
							std::u8string text = xybase::string::escape(catName);
							datum.setCategoryName(xybase::string::unescape(GetTranslation(text)));
						}
					} catch (...) { /* Ignore if field doesn't exist */ }
				}
				roe.WriteCategory(outPath);
			}
		}
		
		// 关闭失配文件
		if (mismatchFile.is_open()) {
			mismatchFile.close();
		}
		
		std::wcout << L"处理完毕。" << std::endl;
		std::wcout << L"共有 " << std::to_wstring(mismatchCount) << L" 条文本失配。失配文本已经保存到 text_mismatch.txt 中。" << std::endl;
		system("pause");
	}
	catch (std::exception &ex)
	{
		// 确保在异常情况下也关闭文件
		if (mismatchFile.is_open()) {
			mismatchFile.close();
		}
		std::wcerr << L"发生了意外错误。" << std::endl;
		std::wcerr << xybase::string::sys_mbs_to_wcs(ex.what()) << std::endl;
		system("pause");
	}

}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
