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
bool en_as_ja = false; 
bool ejref_tolerance = false;
bool verbose = false;

#include "../FFXIDatProcessor/codepage.h"
#include "ChsToSJis.h"

std::map<std::u8string, std::u8string> textMapping;
std::map<std::u8string, std::u8string> commentToJpPath;
std::unordered_set<std::u8string> mismatchSet;

struct FileProcessDef {
	std::u8string path;
	std::u8string type;
	std::u8string lang;
	std::u8string comment;
	std::u8string cellIndicesStr;
};

// 失配文本统计和文件输出
int mismatchCount = 0;
std::ofstream mismatchFile;

std::set<int> ParseCellIndices(const std::u8string &cellIndicesStr);

bool IsQuestDMsg(const std::u8string& comment)
{
	return comment.starts_with(u8"sys/mis/") || comment.starts_with(u8"sys/qst/");
}

bool IsEjrefShorterReferenceComment(const std::u8string& comment)
{
	return comment == u8"gev/action"
		|| comment == u8"sys/text_command_help"
		|| comment == u8"sys/weapon_skill"
		|| comment == u8"sys/weapon_skill_help";
}

bool IsEjrefSameRowCell0Comment(const std::u8string& comment)
{
	return comment == u8"sys/status" || comment == u8"sys/mob_race";
}

bool IsEjrefSpecialComment(const std::u8string& comment)
{
	return comment == u8"gev/status"
		|| comment == u8"sys/key_item"
		|| IsEjrefShorterReferenceComment(comment)
		|| IsEjrefSameRowCell0Comment(comment);
}

std::u8string GetTranslation(const std::u8string &text) {
	std::u8string translation;

	auto itr = textMapping.find(text);
	if (itr == textMapping.end())
	{
		
		// 将失配文本写入文件
		if (mismatchSet.find(text) == mismatchSet.end()) {
			// 统计失配文本数量
			mismatchCount++;
			mismatchSet.insert(text);
			if (verbose) {
				std::wcout << L"\n失配：" << xybase::string::to_wstring(text) << std::endl;
			}
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

ItemSpecType GetItemSpecType(const std::u8string& type)
{
	if (type == u8"iab") return ItemSpecType::ARMOUR;
	if (type == u8"iwb") return ItemSpecType::WEAPON;
	if (type == u8"iub") return ItemSpecType::USABLE;
	if (type == u8"ipb") return ItemSpecType::PUPPET;
	if (type == u8"isb") return ItemSpecType::SLIP;
	if (type == u8"icb") return ItemSpecType::CURRENCY;
	return ItemSpecType::NORMAL;
}

std::map<uint32_t, std::vector<std::u8string>> CollectItemTextsById(const fs::path& datPath, const std::u8string& type)
{
	std::map<uint32_t, std::vector<std::u8string>> result;
	ItemData itemData;
	itemData.Read(datPath, GetItemSpecType(type));
	for (const auto& datum : itemData.data)
	{
		auto& texts = result[datum.id];
		try {
			auto name = datum.name();
			if (!name.empty()) texts.push_back(xybase::string::escape(name));
		} catch (...) {}
		/*try {
			auto nameSg = datum.name_sg();
			if (!nameSg.empty()) texts.push_back(xybase::string::escape(nameSg));
		} catch (...) {}
		try {
			auto namePl = datum.name_pl();
			if (!namePl.empty()) texts.push_back(xybase::string::escape(namePl));
		} catch (...) {}*/
		try {
			auto desc = datum.description();
			if (!desc.empty()) texts.push_back(xybase::string::escape(desc));
		} catch (...) {}
	}
	return result;
}

std::map<uint32_t, std::u8string> CollectMonBridgeTextsById(const fs::path& datPath)
{
	std::map<uint32_t, std::u8string> result;
	MonBridge monBridge;
	monBridge.Read(datPath);
	for (const auto& datum : monBridge.data)
	{
		if (!datum.displayName.empty())
		{
			result[datum.id] = xybase::string::escape(datum.displayName);
		}
	}
	return result;
}

std::map<int, std::vector<std::u8string>> CollectDMsgTextsById(const fs::path& datPath, const std::u8string& cellIndicesStr)
{
	std::map<int, std::vector<std::u8string>> result;
	DMsg dmsg(datPath);
	dmsg.Read();
	std::set<int> targetCells = ParseCellIndices(cellIndicesStr);
	bool translateAllCells = targetCells.empty();
	for (auto& row : dmsg)
	{
		const auto& cells = row.GetCellsConst();
		if (cells.empty() || cells[0].GetType() != 1)
			continue;
		int rowId = cells[0].Get<int>();
		auto& texts = result[rowId];
		int colNum = 1;
		for (const auto& cell : cells)
		{
			if (cell.GetType() == 0)
			{
				bool shouldTranslate = translateAllCells || targetCells.count(colNum) > 0;
				if (shouldTranslate) {
					texts.push_back(xybase::string::escape(cell.Get<std::u8string>()));
				}
			}
			++colNum;
		}
	}
	return result;
}

std::u8string GetTranslationFromReference(const std::u8string& sourceText, const std::u8string& referenceText)
{
	auto itr = textMapping.find(referenceText);
	if (itr == textMapping.end())
	{
		if (mismatchSet.find(referenceText) == mismatchSet.end()) {
			mismatchCount++;
			mismatchSet.insert(referenceText);
			if (verbose) {
				std::wcout << L"\n参考失配：" << xybase::string::to_wstring(referenceText) << std::endl;
			}
			if (mismatchFile.is_open()) {
				mismatchFile.write(reinterpret_cast<const char*>(referenceText.c_str()), referenceText.length());
				mismatchFile << "\n";
			}
		}
		return sourceText;
	}

	auto translation = itr->second;
	translation = ChsToSJis::Instance().ReplaceHanzi(translation);
	return translation;
}

bool TryGetTranslationFromReference(const std::u8string& sourceText, const std::u8string& referenceText, std::u8string& translation)
{
	auto itr = textMapping.find(referenceText);
	if (itr == textMapping.end())
	{
		if (mismatchSet.find(referenceText) == mismatchSet.end()) {
			mismatchCount++;
			mismatchSet.insert(referenceText);
			if (verbose) {
				std::wcout << L"\n参考失配：" << xybase::string::to_wstring(referenceText) << std::endl;
			}
			if (mismatchFile.is_open()) {
				mismatchFile.write(reinterpret_cast<const char*>(referenceText.c_str()), referenceText.length());
				mismatchFile << "\n";
			}
		}
		translation = sourceText;
		return false;
	}

	translation = itr->second;
	translation = ChsToSJis::Instance().ReplaceHanzi(translation);
	return true;
}

struct InsToken
{
	size_t start = 0;
	size_t end = 0;
	std::vector<std::u8string> parts;
};

std::vector<InsToken> ParseInsTokens(const std::u8string& text)
{
	std::vector<InsToken> result;
	const std::u8string prefix = u8"<ins:";
	size_t pos = 0;
	while ((pos = text.find(prefix, pos)) != std::u8string::npos)
	{
		size_t end = text.find(u8">", pos);
		if (end == std::u8string::npos)
			break;

		InsToken token;
		token.start = pos;
		token.end = end + 1;
		std::u8string body = text.substr(pos + prefix.size(), end - pos - prefix.size());
		std::string bodyStr = xybase::string::to_string(body);
		std::stringstream ss(bodyStr);
		std::string part;
		while (std::getline(ss, part, ':'))
		{
			token.parts.push_back(xybase::string::to_utf8(part));
		}
		result.push_back(std::move(token));
		pos = end + 1;
	}
	return result;
}

std::u8string BuildInsToken(const std::vector<std::u8string>& parts)
{
	std::u8string token = u8"<ins:";
	for (size_t i = 0; i < parts.size(); ++i)
	{
		if (i > 0) token += u8":";
		token += parts[i];
	}
	token += u8">";
	return token;
}

std::u8string BuildInsKey(const std::vector<std::u8string>& parts)
{
	if (parts.size() < 5)
		return {};
	std::u8string key;
	for (size_t i = parts.size() - 4; i < parts.size(); ++i)
	{
		if (!key.empty()) key += u8":";
		key += parts[i];
	}
	return key;
}

bool TryAdaptInsCategoryForEnglish(const std::u8string& englishSource, std::u8string& translated)
{
	auto sourceTokens = ParseInsTokens(englishSource);
	auto translatedTokens = ParseInsTokens(translated);
	if (translatedTokens.empty())
		return true;

	std::map<std::u8string, std::vector<std::u8string>> sourcePartsByVar;
	std::set<std::u8string> sourceShortTokens;
	for (const auto& token : sourceTokens)
	{
		if (token.parts.size() < 5)
		{
			sourceShortTokens.insert(BuildInsToken(token.parts));
			continue;
		}
		auto key = BuildInsKey(token.parts);
		if (!key.empty())
		{
			sourcePartsByVar[key] = token.parts;
		}
	}

	for (const auto& translatedToken : translatedTokens)
	{
		if (translatedToken.parts.size() < 5)
		{
			auto tokenStr = BuildInsToken(translatedToken.parts);
			if (!sourceShortTokens.contains(tokenStr))
				return false;
		}
	}

	for (auto it = translatedTokens.rbegin(); it != translatedTokens.rend(); ++it)
	{
		auto& token = *it;
		if (token.parts.size() < 5)
			continue;

		auto key = BuildInsKey(token.parts);
		auto sourceItr = sourcePartsByVar.find(key);
		if (sourceItr == sourcePartsByVar.end())
			return false;

		const auto& sourceParts = sourceItr->second;
		if (sourceParts.size() < 5)
			return false;

		auto newToken = BuildInsToken(sourceParts);
		translated.replace(token.start, token.end - token.start, newToken);
	}

	return true;
}

std::vector<std::u8string> CollectStrings(const fs::path& datPath, const std::u8string& type, const std::u8string& cellIndicesStr)
{
	std::vector<std::u8string> result;

	if (type == u8"xis")
	{
		XiString xis(datPath);
		xis.Read();
		for (auto& str : xis)
		{
			result.push_back(xybase::string::escape(xybase::string::to_utf8(xis.Decode(str.str))));
		}
	}
	else if (type == u8"evsb")
	{
		EventStringBase evsb(datPath);
		evsb.Read();
		for (auto& s : evsb)
		{
			result.push_back(s);
		}
	}
	else if (type == u8"dmsg")
	{
		DMsg dmsg(datPath);
		dmsg.Read();
		std::set<int> targetCells = ParseCellIndices(cellIndicesStr);
		bool translateAllCells = targetCells.empty();
		for (auto& row : dmsg)
		{
			int colNum = 1;
			for (auto& cell : row)
			{
				if (cell.GetType() == 0)
				{
					bool shouldTranslate = translateAllCells || targetCells.count(colNum) > 0;
					if (shouldTranslate) {
						result.push_back(xybase::string::escape(cell.Get<std::u8string>()));
					}
				}
				++colNum;
			}
		}
	}
	else if (type == u8"sd")
	{
		StatusData statusData;
		statusData.Read(datPath);
		for (auto& datum : statusData.data)
		{
			if (!datum.description.empty())
			{
				result.push_back(xybase::string::escape(datum.description));
			}
		}
	}
	else if (type == u8"fp")
	{
		FixedPhrase fixedPhrase;
		fixedPhrase.Read(datPath);
		for (auto& category : fixedPhrase.categories)
		{
			result.push_back(category.categoryName);
			result.push_back(category.categoryPron);
			for (auto&& entry : category.entries)
			{
				if (!entry.text.empty())
				{
					result.push_back(entry.text);
				}
				if (!entry.pron.empty())
				{
					result.push_back(entry.pron);
				}
			}
		}
	}
	else if (type == u8"iab" || type == u8"iwb" || type == u8"iub" || type == u8"inb" || type == u8"ipb" || type == u8"isb" || type == u8"icb")
	{
		std::set<int> targetCells = ParseCellIndices(cellIndicesStr);
		bool translateAllCells = targetCells.empty();

		ItemData itemData;
		ItemSpecType specType = ItemSpecType::NORMAL;
		if (type == u8"iab") {
			specType = ItemSpecType::ARMOUR;
		}
		else if (type == u8"iwb") {
			specType = ItemSpecType::WEAPON;
		}
		else if (type == u8"iub") {
			specType = ItemSpecType::USABLE;
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
		for (auto& datum : itemData.data)
		{
			int cellIndex = 1;
			for (auto& cell : datum.row())
			{
				if (cell.GetType() == 0)
				{
					bool shouldTranslate = translateAllCells || targetCells.count(cellIndex) > 0;
					if (shouldTranslate) {
						result.push_back(xybase::string::escape(cell.Get<std::u8string>()));
					}
				}
				++cellIndex;
			}
		}
	}
	else if (type == u8"mbd")
	{
		MonBridge monBridge;
		monBridge.Read(datPath);
		for (auto& datum : monBridge.data)
		{
			if (!datum.displayName.empty())
			{
				result.push_back(xybase::string::escape(datum.displayName));
			}
		}
	}
	else if (type == u8"erq")
	{
		RecordsOfEminence roe;
		roe.ReadQuest(datPath);
		std::set<int> targetCells = ParseCellIndices(cellIndicesStr);
		for (auto& datum : roe.questData)
		{
			int cellIndex = 1;
			for (auto& cell : datum.row())
			{
				if (cell.GetType() == 0)
				{
					bool shouldTranslate = targetCells.empty() || targetCells.count(cellIndex) > 0;
					if (shouldTranslate) {
						result.push_back(xybase::string::escape(cell.Get<std::u8string>()));
					}
				}
				++cellIndex;
			}
		}
	}
	else if (type == u8"erc")
	{
		RecordsOfEminence roe;
		roe.ReadCategory(datPath);
		for (auto& datum : roe.categoryData)
		{
			try {
				std::u8string catName = datum.categoryName();
				if (!catName.empty())
				{
					result.push_back(xybase::string::escape(catName));
				}
			}
			catch (...) {}
		}
	}

	return result;
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

bool PrepareTextStream(std::ifstream& eye)
{
	if (!eye.is_open())
		return false;

	char bom[3] = { 0 };
	eye.read(bom, 3);
	std::streamsize bytesRead = eye.gcount();

	if (bytesRead == 3 && bom[0] == char(0xEF) && bom[1] == char(0xBB) && bom[2] == char(0xBF)) {
		return true;
	}
	if (bytesRead >= 2 && bom[0] == char(0xFF) && bom[1] == char(0xFE)) {
		std::wcerr << L"不支持的编码格式（UTF-16 LE），请转换为UTF-8编码。\n";
		return false;
	}
	if (bytesRead >= 2 && bom[0] == char(0xFE) && bom[1] == char(0xFF)) {
		std::wcerr << L"不支持的编码格式（UTF-16 BE），请转换为UTF-8编码。\n";
		return false;
	}

	eye.clear();
	eye.seekg(0);
	return true;
}

int LoadTextPair(const fs::path& textPath, const fs::path& transPath)
{
	bool textExists = fs::exists(textPath) && fs::is_regular_file(textPath);
	bool transExists = fs::exists(transPath) && fs::is_regular_file(transPath);
	if (!textExists && !transExists)
		return 0;
	if (textExists != transExists)
	{
		std::wcerr << L"原文文件和翻译文件未成对出现：" << textPath << L" / " << transPath << std::endl;
		return -1;
	}

	if (verbose) {
		std::wcout << L"读取：" << textPath << L" -=- " << transPath << std::endl;
	}

	std::ifstream
		oEye(textPath, std::ios::in | std::ios::binary),
		tEye(transPath, std::ios::in | std::ios::binary);
	std::string text;
	std::string trans;

	if (!PrepareTextStream(oEye) || !PrepareTextStream(tEye))
		return -1;

	int i = 0;
	while (std::getline(oEye, text)) {
		if (!std::getline(tEye, trans))
		{
			std::wcerr << L"翻译文件和原文文件的行数不一致。\n";
			return i;
		}
		if (!text.empty() && text.back() == '\r') {
			text.pop_back();
		}
		if (!trans.empty() && trans.back() == '\r') {
			trans.pop_back();
		}
		textMapping[reinterpret_cast<const char8_t*>(text.c_str())] = reinterpret_cast<const char8_t*>(trans.c_str());
		++i;
	}
	return i;
}

int LoadText(int seq)
{
	fs::path textPath = progRoot / (std::string("text") + std::to_string(seq) + ".txt");
	fs::path transPath = progRoot / (std::string("text") + std::to_string(seq) + "_translated.txt");
	if (seq == 0) {
		textPath = progRoot / "text.txt";
		transPath = progRoot / "text_translated.txt";
	}
	return LoadTextPair(textPath, transPath);
}

bool HasTranslatedCsv(const std::u8string& comment)
{
	fs::path csvPath = progRoot / L"text" / L"tgt" / xybase::string::to_wstring(comment + u8".csv");
	return fs::exists(csvPath) && fs::is_regular_file(csvPath);
}

fs::path GetTranslatedCsvPath(const std::u8string& comment)
{
	return progRoot / L"text" / L"tgt" / xybase::string::to_wstring(comment + u8".csv");
}

void ValidateCsvHeader(CsvFile& csv, const std::vector<std::u8string>& expectedHeader, const fs::path& csvPath)
{
	if (csv.IsEof())
		throw std::runtime_error("CSV is empty > " + csvPath.string());

	std::vector<std::u8string> actualHeader;
	actualHeader.reserve(expectedHeader.size());
	for (size_t i = 0; i < expectedHeader.size(); ++i)
	{
		actualHeader.push_back(csv.NextCell());
		if (csv.IsEol() && i + 1 < expectedHeader.size())
			break;
	}
	csv.NextLine();

	if (actualHeader.size() != expectedHeader.size())
		throw std::runtime_error("CSV header column count mismatch > " + csvPath.string());

	for (size_t i = 0; i < expectedHeader.size(); ++i)
	{
		if (actualHeader[i] != expectedHeader[i])
			throw std::runtime_error("CSV header mismatch > " + csvPath.string());
	}
}

std::map<uint32_t, std::pair<std::u8string, std::u8string>> LoadItemCsvTranslations(const fs::path& csvPath)
{
	std::map<uint32_t, std::pair<std::u8string, std::u8string>> result;
	CsvFile csv(csvPath, std::ios::in | std::ios::binary);
	ValidateCsvHeader(csv, { u8"ID", u8"Name", u8"Description" }, csvPath);
	while (!csv.IsEof())
	{
		std::u8string idStr = csv.NextCell();
		std::u8string name = csv.IsEol() ? u8"" : csv.NextCell();
		std::u8string description = csv.IsEol() ? u8"" : csv.NextCell();
		csv.NextLine();

		if (idStr.empty())
			continue;

		try {
			uint32_t id = static_cast<uint32_t>(xybase::string::stoi(idStr));
			result[id] = { name, description };
		}
		catch (const std::exception&) {
		}
	}
	return result;
}

struct RoeQuestCsvTranslation
{
	std::u8string questName;
	std::u8string description;
	std::u8string note;
};

struct QuestDMsgCsvTranslation
{
	std::u8string name;
	std::u8string description;
};

std::map<uint32_t, RoeQuestCsvTranslation> LoadRoeQuestCsvTranslations(const fs::path& csvPath)
{
	std::map<uint32_t, RoeQuestCsvTranslation> result;
	CsvFile csv(csvPath, std::ios::in | std::ios::binary);
	ValidateCsvHeader(csv, { u8"ID", u8"QuestName", u8"Description", u8"Note" }, csvPath);
	while (!csv.IsEof())
	{
		std::u8string idStr = csv.NextCell();
		std::u8string questName = csv.IsEol() ? u8"" : csv.NextCell();
		std::u8string description = csv.IsEol() ? u8"" : csv.NextCell();
		std::u8string note = csv.IsEol() ? u8"" : csv.NextCell();
		csv.NextLine();

		if (idStr.empty())
			continue;

		try {
			uint32_t id = static_cast<uint32_t>(xybase::string::stoi(idStr));
			result[id] = { questName, description, note };
		}
		catch (const std::exception&) {
		}
	}
	return result;
}

std::map<int, QuestDMsgCsvTranslation> LoadQuestDMsgCsvTranslations(const fs::path& csvPath)
{
	std::map<int, QuestDMsgCsvTranslation> result;
	CsvFile csv(csvPath, std::ios::in | std::ios::binary);
	ValidateCsvHeader(csv, { u8"ID", u8"Name", u8"Desc" }, csvPath);
	while (!csv.IsEof())
	{
		std::u8string idStr = csv.NextCell();
		std::u8string name = csv.IsEol() ? u8"" : csv.NextCell();
		std::u8string description = csv.IsEol() ? u8"" : csv.NextCell();
		csv.NextLine();

		if (idStr.empty())
			continue;

		try {
			int id = xybase::string::stoi(idStr);
			result[id] = { name, description };
		}
		catch (const std::exception&) {
		}
	}
	return result;
}

std::map<uint32_t, std::u8string> LoadRoeCategoryCsvTranslations(const fs::path& csvPath)
{
	std::map<uint32_t, std::u8string> result;
	CsvFile csv(csvPath, std::ios::in | std::ios::binary);
	ValidateCsvHeader(csv, { u8"ID", u8"CategoryName" }, csvPath);
	while (!csv.IsEof())
	{
		std::u8string idStr = csv.NextCell();
		std::u8string categoryName = csv.IsEol() ? u8"" : csv.NextCell();
		csv.NextLine();

		if (idStr.empty())
			continue;

		try {
			uint32_t id = static_cast<uint32_t>(xybase::string::stoi(idStr));
			result[id] = categoryName;
		}
		catch (const std::exception&) {
		}
	}
	return result;
}

bool TryApplyEjrefToleranceSpecialTranslation(
	const FileProcessDef& fileDef,
	const fs::path& datPath,
	const fs::path& outPath,
	const std::map<std::u8string, FileProcessDef>& jpDefsByComment)
{
	if (!(englishMode && en_as_ja && ejref_tolerance))
		return false;
	if (!IsEjrefSpecialComment(fileDef.comment))
		return false;

	auto jpItr = jpDefsByComment.find(fileDef.comment);
	if (jpItr == jpDefsByComment.end() || jpItr->second.type != fileDef.type)
		return false;

	fs::path jpDatPath = gameRoot / (jpItr->second.path + u8".DAT");
	if (!fs::exists(jpDatPath))
		return false;

	if (fileDef.type == u8"evsb" && fileDef.comment == u8"gev/status")
	{
		EventStringBase evsb(datPath);
		EventStringBase jpEvsb(jpDatPath);
		evsb.Read();
		jpEvsb.Read();
		if (jpEvsb.size() * 2 != evsb.size() || jpEvsb.size() == 0)
			return false;

		for (size_t i = 0; i < evsb.size(); ++i)
		{
			evsb[i] = GetTranslationFromReference(evsb[i], jpEvsb[i % jpEvsb.size()]);
		}

		evsb.path = outPath;
		evsb.Write();
		return true;
	}

	if (fileDef.type == u8"evsb" && fileDef.comment == u8"gev/action")
	{
		EventStringBase evsb(datPath);
		EventStringBase jpEvsb(jpDatPath);
		evsb.Read();
		jpEvsb.Read();

		for (size_t i = 0; i < evsb.size(); ++i)
		{
			if (i < jpEvsb.size())
				evsb[i] = GetTranslationFromReference(evsb[i], jpEvsb[i]);
			else
				evsb[i] = GetTranslation(evsb[i]);
		}

		evsb.path = outPath;
		evsb.Write();
		return true;
	}

	if (fileDef.type == u8"dmsg" && IsEjrefShorterReferenceComment(fileDef.comment))
	{
		DMsg dmsg(datPath);
		dmsg.Read();
		auto jpTexts = CollectStrings(jpDatPath, jpItr->second.type, jpItr->second.cellIndicesStr);
		std::set<int> targetCells = ParseCellIndices(fileDef.cellIndicesStr);
		bool translateAllCells = targetCells.empty();
		size_t textIdx = 0;

		for (auto& row : dmsg)
		{
			int colNum = 1;
			for (auto& cell : row)
			{
				if (cell.GetType() == 0)
				{
					bool shouldTranslate = translateAllCells || targetCells.count(colNum) > 0;
					if (shouldTranslate)
					{
						std::u8string text = xybase::string::escape(cell.Get<std::u8string>());
						std::u8string translated = textIdx < jpTexts.size()
							? GetTranslationFromReference(text, jpTexts[textIdx])
							: GetTranslation(text);
						cell.Set(xybase::string::unescape(translated));
						++textIdx;
					}
				}
				++colNum;
			}
		}

		dmsg.path = outPath;
		dmsg.Write();
		return true;
	}

	if (fileDef.type == u8"dmsg" && IsEjrefSameRowCell0Comment(fileDef.comment))
	{
		DMsg dmsg(datPath);
		DMsg jpDmsg(jpDatPath);
		dmsg.Read();
		jpDmsg.Read();

		auto jpRowItr = jpDmsg.begin();
		for (auto& row : dmsg)
		{
			if (jpRowItr == jpDmsg.end())
				break;

			const auto& jpCells = jpRowItr->GetCellsConst();
			if (!jpCells.empty() && jpCells[0].GetType() == 0)
			{
				std::u8string jpReference = xybase::string::escape(jpCells[0].Get<std::u8string>());
				auto& cells = row.GetCells();
				for (size_t i = 0; i < cells.size() && i < 2; ++i)
				{
					if (cells[i].GetType() != 0)
						continue;
					std::u8string text = xybase::string::escape(cells[i].Get<std::u8string>());
					cells[i].Set(xybase::string::unescape(GetTranslationFromReference(text, jpReference)));
				}
			}
			++jpRowItr;
		}

		dmsg.path = outPath;
		dmsg.Write();
		return true;
	}

	if (fileDef.type == u8"dmsg" && fileDef.comment == u8"sys/key_item")
	{
		DMsg dmsg(datPath);
		DMsg jpDmsg(jpDatPath);
		dmsg.Read();
		jpDmsg.Read();

		struct KeyItemReference
		{
			std::u8string name;
			std::u8string description;
		};
		std::map<int, KeyItemReference> jpById;
		for (auto& row : jpDmsg)
		{
			const auto& cells = row.GetCellsConst();
			if (cells.size() < 3 || cells[0].GetType() != 1)
				continue;

			int id = cells[0].Get<int>();
			KeyItemReference ref;
			if (cells[1].GetType() == 0)
				ref.name = xybase::string::escape(cells[1].Get<std::u8string>());
			if (cells[2].GetType() == 0)
				ref.description = xybase::string::escape(cells[2].Get<std::u8string>());
			jpById[id] = std::move(ref);
		}

		for (auto& row : dmsg)
		{
			auto& cells = row.GetCells();
			if (cells.size() < 7 || cells[0].GetType() != 1)
				continue;

			int id = cells[0].Get<int>();
			auto refItr = jpById.find(id);
			if (refItr == jpById.end())
				continue;

			if (!refItr->second.name.empty())
			{
				if (cells[4].GetType() == 0)
				{
					std::u8string text = xybase::string::escape(cells[4].Get<std::u8string>());
					cells[4].Set(xybase::string::unescape(GetTranslationFromReference(text, refItr->second.name)));
				}
				if (cells[5].GetType() == 0)
				{
					std::u8string text = xybase::string::escape(cells[5].Get<std::u8string>());
					cells[5].Set(xybase::string::unescape(GetTranslationFromReference(text, refItr->second.name)));
				}
			}

			if (!refItr->second.description.empty() && cells[6].GetType() == 0)
			{
				std::u8string text = xybase::string::escape(cells[6].Get<std::u8string>());
				cells[6].Set(xybase::string::unescape(GetTranslationFromReference(text, refItr->second.description)));
			}
		}

		dmsg.path = outPath;
		dmsg.Write();
		return true;
	}

	return false;
}

int LoadSourceData()
{
	fs::path srcRoot = progRoot / L"text" / L"src";
	fs::path tgtRoot = progRoot / L"text" / L"tgt";
	if (!fs::exists(srcRoot) || !fs::exists(tgtRoot))
		return 0;

	int total = 0;
	for (const auto& entry : fs::recursive_directory_iterator(srcRoot))
	{
		if (!entry.is_regular_file() || entry.path().extension() != L".txt")
			continue;

		fs::path relativePath = fs::relative(entry.path(), srcRoot);
		fs::path targetPath = tgtRoot / relativePath;
		if (!fs::exists(targetPath) || !fs::is_regular_file(targetPath))
			continue;

		int loaded = LoadTextPair(entry.path(), targetPath);
		if (loaded < 0)
			return loaded;
		total += loaded;
	}

	return total;
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
	fs::path outputPath = outputDir / xybase::string::to_wstring(def.comment + u8".txt");
	if (!fs::exists(outputPath.parent_path()))
		fs::create_directories(outputPath.parent_path());
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
		if (!out.is_open())
		{
			std::wcerr << L"无法创建输出文件：" << outputPath << std::endl;
			return;
		}
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

int PrepareSourceData()
{
	try
	{
		CodeCvt::GetInstance().Init(progRoot / L"cp932.csv");
	}
	catch (std::exception& ex)
	{
		std::wcerr << ex.what() << std::endl;
		std::wcerr << L"处理代码页cp932.csv失败了。" << std::endl;
		system("pause");
		return -2;
	}
	std::wcout << L"开始准备源数据..." << std::endl;
	fs::path outputDir{ progRoot / L"text" / L"src"};
	//fs::path zh = outputDir / L"zh";
	try
	{
		if (!fs::exists(outputDir))
		{
			fs::create_directories(outputDir);
			std::wcout << L"已创建输出目录：" << outputDir << std::endl;
		}

		CsvFile defs(progRoot / L"defs.csv", std::ios::in | std::ios::binary);

		std::list<FileDef> fileDefs;

		while (!defs.IsEof())
		{
			std::u8string path = defs.NextCell();
			path = path.ends_with(u8".DAT") ? path : path + u8".DAT";
			std::u8string type = defs.NextCell();
			std::u8string lang = defs.NextCell();
			std::u8string comment = defs.NextCell();
			if (!defs.IsEol()) {
				defs.NextCell();
			}
			defs.NextLine();

			if (comment.empty() || lang.empty() || path.empty() || type.empty())
				continue;
			if (lang != u8"jp")
				continue;
			if (!fs::exists(gameRoot / path))
			{
				std::wcout << L"警告：游戏文件不存在，跳过 " << (gameRoot / path) << std::endl;
				continue;
			}
			fileDefs.push_back({ comment, lang, path, type });
			commentToJpPath[comment] = path;
		}

		std::wcout << L"已加载 " << fileDefs.size() << L" 个 JP 文件定义。" << std::endl;

		// 处理 ev/* 和 evx/* 文件
		// 首先处理 ev/dbg_scene，
		std::set<std::u8string> processedStrings; // 避免重复处理同一文本
		auto itr = std::find_if(fileDefs.begin(), fileDefs.end(), [](const FileDef &def) {
			return def.comment == u8"ev/dbg_scene" && def.lang == u8"jp";
			});
		if (itr != fileDefs.end())
		{
			auto dbg_scene_jp = *itr;
			itr = fileDefs.erase(itr);
			ExtractEventStringBase(dbg_scene_jp, processedStrings, outputDir);
		}
		else
		{
			std::wcout << L"未找到 ev/dbg_scene，跳过预处理。" << std::endl;
			itr = fileDefs.begin();
		}

		// 现在处理剩下的 ev/* 和 evx/* 文件
		itr = fileDefs.begin();
		while (itr != fileDefs.end())
		{
			if ((itr->comment.starts_with(u8"ev/") || itr->comment.starts_with(u8"evx/") || itr->comment.starts_with(u8"gev/")) && itr->lang == u8"jp")
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
			if (itr->comment.starts_with(u8"itm/"))
			{

				fs::path csvOutputPath = outputDir / xybase::string::to_wstring(itr->comment + u8".csv");
				if (!fs::exists(csvOutputPath.parent_path()))
					fs::create_directories(csvOutputPath.parent_path());
				std::wcout << L"正在导出 CSV " << (gameRoot / itr->path) << L" -> " << csvOutputPath << std::endl;

				ItemData id;
				ItemSpecType stype = ItemSpecType::NORMAL;
				if (itr->type == u8"iab") {
					stype = ItemSpecType::ARMOUR;
				}
				else if (itr->type == u8"iwb") {
					stype = ItemSpecType::WEAPON;
				}
				else if (itr->type == u8"iub") {
					stype = ItemSpecType::USABLE;
				}
				else if (itr->type == u8"ipb") {
					stype = ItemSpecType::PUPPET;
				}
				else if (itr->type == u8"isb") {
					stype = ItemSpecType::SLIP;
				}
				else if (itr->type == u8"icb") {
					stype = ItemSpecType::CURRENCY;
				}
				id.Read(gameRoot / itr->path, stype);
				CsvFile output(csvOutputPath, std::ios::out | std::ios::binary);

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
				std::wcout << L"CSV 导出完成：" << csvOutputPath << std::endl;
				itr = fileDefs.erase(itr);
			}
			else
				++itr;
		}

		// 处理剩下的文件
		for (const auto &def : fileDefs)
		{
			fs::path inputPath = gameRoot / def.path;
			fs::path outputPath = outputDir / xybase::string::to_wstring(def.comment + u8".txt");
			if (!fs::exists(outputPath.parent_path()))
				fs::create_directories(outputPath.parent_path());
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
				fs::path csvOutputPath = outputDir / xybase::string::to_wstring(def.comment + u8".csv");
				std::wcout << L"正在导出 CSV " << inputPath << L" -> " << csvOutputPath << std::endl;
				FixedPhrase data;
				data.Read(inputPath);
				data.ToCsv(csvOutputPath);
				std::wcout << L"CSV 导出完成：" << csvOutputPath << std::endl;
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
				fs::path csvOutputPath = outputDir / xybase::string::to_wstring(def.comment + u8".csv");
				std::wcout << L"正在导出 CSV " << inputPath << L" -> " << csvOutputPath << std::endl;
				RecordsOfEminence data;
				data.ReadQuest(inputPath);
				CsvFile output(csvOutputPath, std::ios::out | std::ios::binary);
				output.NewCell(u8"ID");
				output.NewCell(u8"QuestName");
				output.NewCell(u8"Description");
				output.NewCell(u8"Note");
				output.NewLine();
				for (const auto &entry : data.questData)
				{
					output.NewCell(xybase::string::itos<char8_t>(entry.id));
					output.NewCell(entry.questName());
					output.NewCell(entry.description());
					output.NewCell(entry.note());
					output.NewLine();
				}
				std::wcout << L"CSV 导出完成：" << csvOutputPath << std::endl;
			}
			else if (def.type == u8"erc")
			{
				fs::path csvOutputPath = outputDir / xybase::string::to_wstring(def.comment + u8".csv");
				std::wcout << L"正在导出 CSV " << inputPath << L" -> " << csvOutputPath << std::endl;
				RecordsOfEminence data;
				data.ReadCategory(inputPath);
				CsvFile output(csvOutputPath, std::ios::out | std::ios::binary);
				output.NewCell(u8"ID");
				output.NewCell(u8"CategoryName");
				output.NewLine();
				for (const auto &entry : data.categoryData)
				{
					output.NewCell(xybase::string::itos<char8_t>(entry.id));
					output.NewCell(entry.categoryName());
					output.NewLine();
				}
				std::wcout << L"CSV 导出完成：" << csvOutputPath << std::endl;
			}
			else if (def.type == u8"dmsg")
			{
				if (IsQuestDMsg(def.comment))
				{
					DMsg data(inputPath);
					data.Read();

					CsvFile output(outputDir / xybase::string::to_wstring(def.comment + u8".csv"), std::ios::out | std::ios::binary);
					output.NewCell(u8"ID");
					output.NewCell(u8"Name");
					output.NewCell(u8"Desc");
					output.NewLine();
					for (auto& datum : data)
					{
						if (datum[0].GetType() != 1)
							continue;
						int id = datum[0].Get<int>();
						std::u8string title = datum[1].Get<std::u8string>();
						std::u8string description = datum[2].Get<std::u8string>();

						output.NewCell(xybase::string::itos<char8_t>(id));
						output.NewCell(title);
						output.NewCell(description);
						output.NewLine();
					}
					output.Close();
					std::wcout << L"CSV 导出完成：" << outputDir / xybase::string::to_wstring(def.comment + u8".csv") << std::endl;
					continue;
				}

				DMsg data(inputPath);
				data.Read();
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
			else if (def.type == u8"mbd")
			{
				MonBridge data;
				data.Read(inputPath);
				for (const auto &entry : data.data)
				{
					extractedStrings.push_back(entry.displayName);
				}
			}
			else
			{
				std::wcout << L"未支持的 prepare 类型，已跳过：" << xybase::string::to_wstring(def.type)
					<< L" [" << xybase::string::to_wstring(def.comment) << L"]" << std::endl;
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
				std::wcout << L"提取了 " << extractedStrings.size() << L" 条文本。" << std::endl;
			}
			else if (def.type == u8"sd" || def.type == u8"mb" || def.type == u8"dmsg" || def.type == u8"xis")
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
			int ret = PrepareSourceData();
			if (ret != 0)
			{
				std::wcerr << L"prepare 执行失败。" << std::endl;
				system("pause");
				exit(ret);
			}
			std::wcout << L"prepare 执行成功。" << std::endl;
			exit(0);
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
				else if (key == L"en_as_ja")
				{
					en_as_ja = (value == L"1" || value == L"true" || value == L"yes");
				}
				else if (key == L"ejref_tolerance")
				{
					ejref_tolerance = (value == L"1" || value == L"true" || value == L"yes");
				}
				else if (key == L"verbose")
				{
					verbose = (value == L"1" || value == L"true" || value == L"yes");
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

		for (int i = 0;; ++i)
		{
			int loaded = LoadText(i);
			if (loaded < 0)
			{
				std::wcerr << L"加载文本文件失败。" << std::endl;
				return -4;
			}
			if (loaded == 0)
				break;
		}
		int sourceCount = LoadSourceData();
		if (sourceCount < 0)
		{
			std::wcerr << L"加载 text\\src / text\\tgt 结构失败。" << std::endl;
			return -4;
		}
		if (sourceCount > 0) {
			std::wcout << L"从 text\\src / text\\tgt 额外覆盖了 " << sourceCount << L" 条文本数据。" << std::endl;
		}
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
		std::vector<FileProcessDef> fileDefs;
		std::map<std::u8string, FileProcessDef> jpDefsByComment;
		while (!def.IsEof())
		{
			FileProcessDef fileDef;
			fileDef.path = def.NextCell();
			fileDef.type = def.NextCell();
			fileDef.lang = def.NextCell();
			fileDef.comment = def.NextCell();
			if (!def.IsEol()) {
				fileDef.cellIndicesStr = def.NextCell();
			}
			def.NextLine();

			if (fileDef.path.empty() || fileDef.type.empty() || fileDef.lang.empty() || fileDef.comment.empty())
				continue;

			fileDefs.push_back(fileDef);
			if (fileDef.lang == u8"jp") {
				commentToJpPath[fileDef.comment] = fileDef.path;
				jpDefsByComment[fileDef.comment] = fileDef;
			}
		}

		for (const auto& fileDef : fileDefs)
		{
			std::u8string path = fileDef.path;
			std::u8string type = fileDef.type;
			std::u8string lang = fileDef.lang;
			std::u8string comm = fileDef.comment;
			std::u8string cellIndicesStr = fileDef.cellIndicesStr;

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
			if (TryApplyEjrefToleranceSpecialTranslation(fileDef, datPath, outPath, jpDefsByComment))
				continue;

			std::vector<std::u8string> referenceTexts;
			bool useJaReference = false;
			bool useIdMappedDMsgReference = type == u8"dmsg" && IsQuestDMsg(comm);
			bool useIdMappedItemReference = type == u8"iab" || type == u8"iwb" || type == u8"iub" || type == u8"inb" || type == u8"ipb" || type == u8"isb" || type == u8"icb";
			bool useIdMappedMonBridgeReference = type == u8"mbd";
			if (englishMode && en_as_ja && !useIdMappedDMsgReference && !useIdMappedItemReference && !useIdMappedMonBridgeReference)
			{
				auto jpItr = jpDefsByComment.find(comm);
				if (jpItr != jpDefsByComment.end() && jpItr->second.type == type)
				{
					fs::path jpDatPath = gameRoot / (jpItr->second.path + u8".DAT");
					if (fs::exists(jpDatPath))
					{
						auto currentTexts = CollectStrings(datPath, type, cellIndicesStr);
						auto jpTexts = CollectStrings(jpDatPath, jpItr->second.type, jpItr->second.cellIndicesStr);
						if (currentTexts.size() == jpTexts.size())
						{
							referenceTexts = std::move(jpTexts);
							useJaReference = true;
						}
						else
						{
							std::wcout << L"\n检测到 en_as_ja，但同 comment 的 EN/JA 记录数不同，回退普通处理：["
								<< xybase::string::to_wstring(comm) << L"] EN=" << currentTexts.size()
								<< L" JA=" << jpTexts.size() << std::endl;
						}
					}
				}
			}

			if (type == u8"xis")
			{
				XiString xis(datPath);
				xis.Read();
				size_t textIdx = 0;
				for (auto &str : xis)
				{
					std::u8string text = xybase::string::escape(xybase::string::to_utf8(xis.Decode(str.str)));
					std::u8string translated = useJaReference ? GetTranslationFromReference(text, referenceTexts[textIdx]) : GetTranslation(text);
					str.str = xis.Encode(xybase::string::to_string(xybase::string::unescape(translated)));
					++textIdx;
				}
				xis.path = outPath;
				xis.Write();
			}
			else if (type == u8"evsb")
			{
				EventStringBase evsb(datPath);
				evsb.Read();
				size_t textIdx = 0;
				for (auto &s : evsb)
				{
					std::u8string res;
					if (useJaReference)
					{
						std::u8string refTranslated;
						if (TryGetTranslationFromReference(s, referenceTexts[textIdx], refTranslated) && TryAdaptInsCategoryForEnglish(s, refTranslated))
						{
							res = refTranslated;
						}
						else
						{
							res = GetTranslation(s);
						}
					}
					else
					{
						res = GetTranslation(s);
					}
					s = res;
					++textIdx;
				}
				evsb.path = outPath;
				evsb.Write();
			}
			else if (type == u8"dmsg")
			{
				DMsg dmsg(datPath);
				dmsg.Read();
				if (IsQuestDMsg(comm) && HasTranslatedCsv(comm))
				{
					auto csvTranslations = LoadQuestDMsgCsvTranslations(GetTranslatedCsvPath(comm));
					for (auto& row : dmsg)
					{
						const auto& cells = row.GetCellsConst();
						if (cells.empty() || cells[0].GetType() != 1)
							continue;

						int rowId = cells[0].Get<int>();
						auto itrCsv = csvTranslations.find(rowId);
						if (itrCsv == csvTranslations.end())
							continue;

						auto& mutableCells = row.GetCells();
						if (!itrCsv->second.name.empty() && mutableCells.size() >= 2 && mutableCells[1].GetType() == 0)
						{
							mutableCells[1].Set(ChsToSJis::Instance().ReplaceHanzi(itrCsv->second.name));
						}
						if (!itrCsv->second.description.empty() && mutableCells.size() >= 3 && mutableCells[2].GetType() == 0)
						{
							mutableCells[2].Set(ChsToSJis::Instance().ReplaceHanzi(itrCsv->second.description));
						}
					}
					dmsg.path = outPath;
					dmsg.Write();
					continue;
				}
				
				// 解析要翻译的cell索引
			 std::set<int> targetCells = ParseCellIndices(cellIndicesStr);
			 bool translateAllCells = targetCells.empty(); // 如果没有指定索引，翻译所有cell
			 std::map<int, std::vector<std::u8string>> jpTextsById;
			 if (englishMode && en_as_ja && useIdMappedDMsgReference)
			 {
				 auto jpItr = jpDefsByComment.find(comm);
				 if (jpItr != jpDefsByComment.end() && jpItr->second.type == type)
				 {
					 fs::path jpDatPath = gameRoot / (jpItr->second.path + u8".DAT");
					 if (fs::exists(jpDatPath))
					 {
						 jpTextsById = CollectDMsgTextsById(jpDatPath, jpItr->second.cellIndicesStr);
					 }
				 }
			 }
				
			 size_t textIdx = 0;
			 for (auto &row : dmsg)
			 {
				 int rowId = 0;
				 bool hasRowId = !row.GetCellsConst().empty() && row.GetCellsConst()[0].GetType() == 1;
				 if (hasRowId) {
					 rowId = row.GetCellsConst()[0].Get<int>();
				 }
				 size_t rowTextIdx = 0;
				 int colNum = 1;
				 for (auto &cell : row)
				 {
					 if (cell.GetType() == 0) // str
					 {
						 // 检查是否需要翻译这个cell
						 bool shouldTranslate = translateAllCells || targetCells.count(colNum) > 0;
							
						 if (shouldTranslate) {
							 std::u8string text = xybase::string::escape(cell.Get<std::u8string>());
							 std::u8string translated;
							 if (englishMode && en_as_ja && useIdMappedDMsgReference && hasRowId)
							 {
								 auto jpTextItr = jpTextsById.find(rowId);
								 if (jpTextItr != jpTextsById.end() && rowTextIdx < jpTextItr->second.size()) {
									 translated = GetTranslationFromReference(text, jpTextItr->second[rowTextIdx]);
								 }
								 else {
									 translated = GetTranslation(text);
								 }
							 }
							 else {
								 translated = useJaReference ? GetTranslationFromReference(text, referenceTexts[textIdx]) : GetTranslation(text);
							 }
							 cell.Set(xybase::string::unescape(translated));
							 ++textIdx;
							 ++rowTextIdx;
						 }
					 }
					 ++colNum;
				 }
			 }
			 dmsg.path = outPath;
			 dmsg.Write();
			}
			else if (type == u8"sd")
			{
				StatusData statusData;
				statusData.Read(datPath);
				size_t textIdx = 0;
				for (auto &datum : statusData.data)
				{
					if (!datum.description.empty())
					{
						std::u8string text = xybase::string::escape(datum.description);
						std::u8string translated = useJaReference ? GetTranslationFromReference(text, referenceTexts[textIdx]) : GetTranslation(text);
						datum.description = xybase::string::unescape(translated);
						++textIdx;
					}
				}
				statusData.Write(outPath);
			}
			else if (type == u8"fp")
			{
				FixedPhrase fixedPhrase;
				fixedPhrase.Read(datPath);
				fs::path csvPath = GetTranslatedCsvPath(comm);
				if (HasTranslatedCsv(comm))
				{
					fixedPhrase.FromCsv(csvPath.wstring());
					for (auto& category : fixedPhrase.categories)
					{
						category.categoryName = ChsToSJis::Instance().ReplaceHanzi(category.categoryName);
						category.categoryPron = ChsToSJis::Instance().ReplaceHanzi(category.categoryPron);
						for (auto& entry : category.entries)
						{
							entry.text = ChsToSJis::Instance().ReplaceHanzi(entry.text);
							entry.pron = ChsToSJis::Instance().ReplaceHanzi(entry.pron);
						}
					}
					fixedPhrase.Write(outPath);
					continue;
				}
				size_t textIdx = 0;
				for (auto& category : fixedPhrase.categories)
				{
					category.categoryName = useJaReference ? GetTranslationFromReference(category.categoryName, referenceTexts[textIdx++]) : GetTranslation(category.categoryName);
					category.categoryPron = useJaReference ? GetTranslationFromReference(category.categoryPron, referenceTexts[textIdx++]) : GetTranslation(category.categoryPron);
					for (auto&& entry : category.entries)
					{
						std::u8string text = entry.text;
						std::u8string pron = entry.pron;

						if (!text.empty())
						{
							entry.text = useJaReference ? GetTranslationFromReference(text, referenceTexts[textIdx++]) : GetTranslation(text);
						}
						if (!pron.empty())
						{
							entry.pron = useJaReference ? GetTranslationFromReference(pron, referenceTexts[textIdx++]) : GetTranslation(pron);
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
				ItemSpecType specType = GetItemSpecType(type);
				
				itemData.Read(datPath, specType);
				if (HasTranslatedCsv(comm))
				{
					auto csvTranslations = LoadItemCsvTranslations(GetTranslatedCsvPath(comm));
					for (auto& datum : itemData.data)
					{
						auto itrCsv = csvTranslations.find(datum.id);
						if (itrCsv == csvTranslations.end())
							continue;

						if (!itrCsv->second.first.empty()) {
						auto convertedName = ChsToSJis::Instance().ReplaceHanzi(itrCsv->second.first);
						datum.setName(convertedName);
						datum.setName_sg(convertedName);
						datum.setName_pl(convertedName);
						}
						if (!itrCsv->second.second.empty()) {
						datum.setDescription(ChsToSJis::Instance().ReplaceHanzi(itrCsv->second.second));
						}
					}
					itemData.Write(outPath);
					
					continue;
				}

				std::map<uint32_t, std::vector<std::u8string>> jpTextsById;
				if (englishMode && en_as_ja)
				{
					auto jpItr = jpDefsByComment.find(comm);
					if (jpItr != jpDefsByComment.end() && jpItr->second.type == type)
					{
						fs::path jpDatPath = gameRoot / (jpItr->second.path + u8".DAT");
						if (fs::exists(jpDatPath))
						{
							jpTextsById = CollectItemTextsById(jpDatPath, type);
						}
					}
				}

				size_t textIdx = 0;
				for (auto &datum : itemData.data)
				{
					if (englishMode && en_as_ja)
					{
						auto jpTextItr = jpTextsById.find(datum.id);
						if (jpTextItr != jpTextsById.end())
						{
							if (!jpTextItr->second.empty()) {
								auto translatedName = GetTranslationFromReference(xybase::string::escape(datum.name()), jpTextItr->second[0]);
								auto unescapedName = xybase::string::unescape(translatedName);
								datum.setName(unescapedName);
								datum.setName_sg(unescapedName);
								datum.setName_pl(unescapedName);
							}
							if (jpTextItr->second.size() >= 2) {
								auto translatedDesc = GetTranslationFromReference(xybase::string::escape(datum.description()), jpTextItr->second.back());
								datum.setDescription(xybase::string::unescape(translatedDesc));
							}
							continue;
						}
					}

					int cellIndex = 1;
					for (auto& cell : datum.row())
					{
						if (cell.GetType() == 0) // str
						{
							bool shouldTranslate = translateAllCells || targetCells.count(cellIndex) > 0;
							if (shouldTranslate) {
								std::u8string text = xybase::string::escape(cell.Get<std::u8string>());
								std::u8string translated = useJaReference ? GetTranslationFromReference(text, referenceTexts[textIdx]) : GetTranslation(text);
								cell.Set(xybase::string::unescape(translated));
								++textIdx;
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
				size_t textIdx = 0;
				std::map<uint32_t, std::u8string> jpTextsById;
				if (englishMode && en_as_ja)
				{
					auto jpItr = jpDefsByComment.find(comm);
					if (jpItr != jpDefsByComment.end() && jpItr->second.type == type)
					{
						fs::path jpDatPath = gameRoot / (jpItr->second.path + u8".DAT");
						if (fs::exists(jpDatPath))
						{
							jpTextsById = CollectMonBridgeTextsById(jpDatPath);
						}
					}
				}
				
				for (auto &datum : monBridge.data)
				{
					// Only translate display name (internal name must NOT be translated)
					// Internal name is the ASCII identifier that the game uses to find entries
					if (!datum.displayName.empty())
					{
						std::u8string text = xybase::string::escape(datum.displayName);
						std::u8string translated;
						if (englishMode && en_as_ja)
						{
							auto jpTextItr = jpTextsById.find(datum.id);
							if (jpTextItr != jpTextsById.end())
							{
								translated = GetTranslationFromReference(text, jpTextItr->second);
							}
							else
							{
								translated = GetTranslation(text);
							}
						}
						else
						{
							translated = useJaReference ? GetTranslationFromReference(text, referenceTexts[textIdx]) : GetTranslation(text);
						}
						datum.displayName = xybase::string::unescape(translated);
						++textIdx;
					}
				}
				monBridge.Write(outPath);
			}
			else if (type == u8"erq") // ROM/307/15 - Quest entries
			{
				RecordsOfEminence roe;
				roe.ReadQuest(datPath);
				if (HasTranslatedCsv(comm))
				{
					auto csvTranslations = LoadRoeQuestCsvTranslations(GetTranslatedCsvPath(comm));
					for (auto& datum : roe.questData)
					{
						auto itrCsv = csvTranslations.find(datum.id);
						if (itrCsv == csvTranslations.end())
							continue;

						if (!itrCsv->second.questName.empty())
						datum.setQuestName(ChsToSJis::Instance().ReplaceHanzi(itrCsv->second.questName));
						if (!itrCsv->second.description.empty())
						datum.setDescription(ChsToSJis::Instance().ReplaceHanzi(itrCsv->second.description));
						if (!itrCsv->second.note.empty())
						datum.setNote(ChsToSJis::Instance().ReplaceHanzi(itrCsv->second.note));
					}
					roe.WriteQuest(outPath);
					continue;
				}

				std::set<int> targetCells = ParseCellIndices(cellIndicesStr);
				size_t textIdx = 0;
				
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
								std::u8string translated = useJaReference ? GetTranslationFromReference(text, referenceTexts[textIdx]) : GetTranslation(text);
								cell.Set(xybase::string::unescape(translated));
								++textIdx;
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
				if (HasTranslatedCsv(comm))
				{
					auto csvTranslations = LoadRoeCategoryCsvTranslations(GetTranslatedCsvPath(comm));
					for (auto& datum : roe.categoryData)
					{
						auto itrCsv = csvTranslations.find(datum.id);
						if (itrCsv == csvTranslations.end())
							continue;
					if (!itrCsv->second.empty())
						datum.setCategoryName(ChsToSJis::Instance().ReplaceHanzi(itrCsv->second));
					}
					roe.WriteCategory(outPath);
					continue;
				}
				size_t textIdx = 0;
				
				for (auto &datum : roe.categoryData)
				{
					// Translate category name
					try {
						std::u8string catName = datum.categoryName();
						if (!catName.empty())
						{
							std::u8string text = xybase::string::escape(catName);
							std::u8string translated = useJaReference ? GetTranslationFromReference(text, referenceTexts[textIdx]) : GetTranslation(text);
							datum.setCategoryName(xybase::string::unescape(translated));
							++textIdx;
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
