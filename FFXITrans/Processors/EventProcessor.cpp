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
#include <CsvFile.h>

namespace fs = std::filesystem;

bool EventProcessor::aliasMapLoaded_ = false;
std::map<std::string, std::string> EventProcessor::aliasMap_;

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

static std::vector<std::u8string> ReadTextLines(const fs::path& filePath)
{
	std::vector<std::u8string> lines;
	std::ifstream f(filePath);
	if (!f.is_open())
		return lines;
	char possibleBOM[3] = { 0 };
	f.read(possibleBOM, 3);
	if (possibleBOM[0] == char(0xEF) && possibleBOM[1] == char(0xBB) && possibleBOM[2] == char(0xBF))
	{
		// UTF-8 BOM detected, continue reading after BOM
	}
	else
	{
		// No BOM, reset to beginning
		f.seekg(0);
	}
	std::string line;
	while (std::getline(f, line))
		lines.push_back(reinterpret_cast<const char8_t*>(line.c_str()));
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
			else if (type == "evsb" && lang == "ja")
				zp.evsb_ja_path = path;
			else if (type == "evsb" && lang == "en")
				zp.evsb_en_path = path;
		}
	}

private:
	bool loaded_ = false;
	std::unordered_map<std::string, ZonePaths> zones_;
};

void EventProcessor::LoadAliasMap()
{
	if (aliasMapLoaded_) return;
	aliasMapLoaded_ = true;
	auto aliasFile = Config::Instance().GetProgRoot() / L"text" / L"tgt" / L"event" / L"event_aliases.csv";
	if (std::filesystem::exists(aliasFile) == false)
	{
		aliasFile = Config::Instance().GetProgRoot() / L"text" / L"src" / L"event" / L"event_aliases.csv";
	}
	if (fs::exists(aliasFile) == false)
	{
		Logger::Instance().Warning("Alias file not found: " + Logger::ToUtf8(aliasFile) + ". Event path aliasing will be unavailable.");
		return;
	}
	CsvFile csv(aliasFile, std::ios::in | std::ios::binary);
	std::string line;
	while (!csv.IsEof())
	{
		std::string src = Logger::ToUtf8(csv.NextCell());
		if (src[0] == '#')
		{
			csv.NextLine();
			continue;
		}
		std::string dst = Logger::ToUtf8(csv.NextCell());
		if (!src.empty() && !dst.empty())
			aliasMap_[src] = dst;
		csv.NextLine();
	}
}

std::filesystem::path EventProcessor::ResolveEventSrcPath(const std::string& actorName, int actorId, int eventIndex, const std::string& zoneName)
{
	LoadAliasMap();
	auto srcEventBase = Config::Instance().GetProgRoot() / L"text" / L"src" / L"event";

	// most precise and without ambiguity: zoneName/actorId/eventIndex.txt
	auto p4 = fs::path(zoneName) / (std::to_string(actorId)) / (std::to_string(eventIndex) + ".txt");
	if (aliasMap_.find(p4.string()) != aliasMap_.end())
		p4 = aliasMap_[p4.string()];
	if (fs::exists(srcEventBase / p4))
		return srcEventBase / p4;

	auto p2 = fs::path(zoneName) / SafeName(actorName) / (std::to_string(eventIndex) + ".txt");
	if (aliasMap_.find(p2.string()) != aliasMap_.end())
		p2 = aliasMap_[p2.string()];
	if (fs::exists(srcEventBase / p2))
		return srcEventBase / p2;

	auto p1 = fs::path(zoneName) / (SafeName(actorName) + "_" + std::to_string(actorId)) / (std::to_string(eventIndex) + ".txt");
	if (aliasMap_.find(p1.string()) != aliasMap_.end())
		p1 = aliasMap_[p1.string()];
	if (fs::exists(srcEventBase / p1))
		return srcEventBase / p1;

	auto p3 = fs::path("common") / (SafeName(actorName)) / (std::to_string(eventIndex) + ".txt");
	if (aliasMap_.find(p3.string()) != aliasMap_.end())
		p3 = aliasMap_[p3.string()];
	if (fs::exists(srcEventBase / p3))
		return srcEventBase / p3;

	return {};
}

std::filesystem::path EventProcessor::ResolveEventTgtPath(const std::string& actorName, int actorId, int eventIndex, const std::string& zoneName)
{
	LoadAliasMap();
	auto dstEventBase = Config::Instance().GetProgRoot() / L"text" / L"tgt" / L"event";
	auto p1 = fs::path(zoneName) / (SafeName(actorName) + "_" + std::to_string(actorId)) / (std::to_string(eventIndex) + ".txt");
	if (aliasMap_.find(p1.string()) != aliasMap_.end())
		p1 = aliasMap_[p1.string()];
	if (fs::exists(dstEventBase / p1))
		return dstEventBase / p1;

	auto p2 = fs::path(zoneName) / SafeName(actorName) / (std::to_string(eventIndex) + ".txt");
	if (aliasMap_.find(p2.string()) != aliasMap_.end())
		p2 = aliasMap_[p2.string()];
	if (fs::exists(dstEventBase / p2))
		return dstEventBase / p2;

	auto p4 = fs::path(zoneName) / (std::to_string(actorId)) / (std::to_string(eventIndex) + ".txt");
	if (aliasMap_.find(p4.string()) != aliasMap_.end())
		p4 = aliasMap_[p4.string()];
	if (fs::exists(dstEventBase / p4))
		return dstEventBase / p4;

	auto p3 = fs::path("common") / (SafeName(actorName)) / (std::to_string(eventIndex) + ".txt");
	if (aliasMap_.find(p3.string()) != aliasMap_.end())
		p3 = aliasMap_[p3.string()];
	if (fs::exists(dstEventBase / p3))
		return dstEventBase / p3;

	return {};
}

bool EventProcessor::StripEndingCtrlSeq(const std::u8string& text,
	std::u8string& strippedText,
	std::u8string& endingCtrlSeq)
{
	// 已知的行结束标记（<-> 系列），按长度降序排列（最长优先匹配）
	static const std::vector<std::u8string> kEndingTags = {
		u8"<-:20:20:20:20>",
		u8"<-:20:20>",
		u8"<-:20>",
		u8"<->",
	};

	for (const auto& tag : kEndingTags) {
		// 检查文本是否以当前 tag 结尾
		if (!text.ends_with(tag))
			continue;

		size_t pos = text.size() - tag.size();          // tag 在 text 中的起始位置
		bool found7F = false;
		size_t start7F = std::u8string::npos;

		// 检查 tag 前面是否紧邻一个 7F 标记（即 tag 前一个字符是 '>'）
		if (pos > 0 && text[pos - 1] == u8'>') {
			// 从 tag 起始位置向前查找最近的 '<'
			size_t lt = text.rfind(u8'<', pos - 1);
			if (lt != std::u8string::npos) {
				// 检查是否为 "<7F:...>" 格式
				if (lt + 4 < pos && text[lt] == u8'<' &&
					text[lt + 1] == u8'7' && text[lt + 2] == u8'F' &&
					text[lt + 3] == u8':') {
					// 验证中间部分（lt+4 到 pos-2）是否只包含合法字符（十六进制数字或冒号）
					bool valid = true;
					for (size_t i = lt + 4; i < pos - 1; ++i) {
						char8_t c = text[i];
						if (!((c >= u8'0' && c <= u8'9') ||
							(c >= u8'A' && c <= u8'F') ||
							(c >= u8'a' && c <= u8'f') ||
							c == u8':')) {
							valid = false;
							break;
						}
					}
					if (valid && (pos - lt) > 5) { // 至少 "<7F:>" 长度为5
						found7F = true;
						start7F = lt;
					}
				}
			}
		}

		if (found7F) {
			// 剥离 7F 标记 + 行结束标记
			strippedText = text.substr(0, start7F) + text.substr(pos + tag.size());
			endingCtrlSeq = text.substr(start7F, (pos + tag.size()) - start7F);
		}
		else {
			// 仅剥离行结束标记
			strippedText = text.substr(0, pos) + text.substr(pos + tag.size());
			endingCtrlSeq = tag;
		}
		return true;
	}

	// 未找到任何行结束标记
	strippedText = text;
	endingCtrlSeq = u8"";
	return false;
}

bool EventProcessor::IsSelectPrompt(const std::u8string& text)
{
	// <sel> 是选项菜单标记
	return text.find(u8"<sel>") != std::u8string::npos;
}
bool IsValidUTF8(const std::u8string& str) {
	int lang = 1;
	for (int p = 0; p < str.size(); ++p) {
		unsigned char c = str[p];
		if (c <= 0x7F) {
			// ASCII
			continue;
		}
		else if ((c & 0xE0) == 0xC0) {
			// 2-byte sequence
			if (p + 1 >= str.size() || (str[p + 1] & 0xC0) != 0x80)
				return false;
			p += 1;
		}
		else if ((c & 0xF0) == 0xE0) {
			// 3-byte sequence
			if (p + 2 >= str.size() || (str[p + 1] & 0xC0) != 0x80 || (str[p + 2] & 0xC0) != 0x80)
				return false;
			p += 2;
		}
		else if ((c & 0xF8) == 0xF0) {
			// 4-byte sequence
			if (p + 3 >= str.size() || (str[p + 1] & 0xC0) != 0x80 || (str[p + 2] & 0xC0) != 0x80 || (str[p + 3] & 0xC0) != 0x80)
				return false;
			p += 3;
		}
		else {
			return false; // Invalid byte
		}
	}
	return true;
}

static std::vector<std::u8string> SplitLines(const std::u8string& text)
{
	std::vector<std::u8string> result;

	size_t begin = 0;
	while (true)
	{
		size_t pos = text.find(u8"<lf>", begin);

		if (pos == std::u8string::npos)
		{
			result.emplace_back(text.substr(begin));
			break;
		}

		result.emplace_back(text.substr(begin, pos - begin));
		begin = pos + 4;
	}

	return result;
}

// ASCII 宽度 = 1，其它 UTF-8 字符宽度 = 2
static int DisplayWidth(std::u8string_view text)
{
	int width = 0;

	for (size_t i = 0; i < text.size();)
	{
		unsigned char c = static_cast<unsigned char>(text[i]);

		if (c < 0x80)
		{
			++width;
			++i;
		}
		else if ((c & 0xE0) == 0xC0)
		{
			width += 2;
			i += 2;
		}
		else if ((c & 0xF0) == 0xE0)
		{
			width += 2;
			i += 3;
		}
		else if ((c & 0xF8) == 0xF0)
		{
			width += 2;
			i += 4;
		}
		else
		{
			++i;
		}
	}

	return width;
}

std::u8string EventProcessor::MakeRosettaText(
	const std::u8string originalText,
	const std::u8string translatedText,
	const std::u8string& separator,
	int insMode)
{
	std::u8string bareOriginal, bareTranslated;
	std::u8string endingCtrlSeq;

	// 如果译文剥离失败，直接返回原文
	if (!StripEndingCtrlSeq(translatedText, bareTranslated, endingCtrlSeq))
		return translatedText;

	// 如果原文剥离失败，也直接返回原文
	std::u8string dummy;
	if (!StripEndingCtrlSeq(originalText, bareOriginal, dummy))
		return translatedText;

	if (IsSelectPrompt(bareOriginal))
	{
		// ===== 处理选项行 =====
		// 结构：引导文本<lf><sel>选项1<lf>选项2<lf>...<7F:XX><->

		// 提取 Prompt 前的内容（引导文本 + <lf>）
		size_t selPos = bareOriginal.find(u8"<sel>");
		if (selPos == std::u8string::npos) [[unlikely]]
			return translatedText;  // 安全兜底，不应发生
		if (translatedText.find(u8"<sel>") == std::u8string::npos)
			return translatedText;  // 原始数据异常

		std::u8string promptSec = bareOriginal.substr(0, selPos);
		std::u8string transPromptSec = bareTranslated.substr(0,
			bareTranslated.find(u8"<sel>"));

		// 提取选项部分（<sel> 之后到行尾）
		std::u8string optionsSec = bareOriginal.substr(selPos + 5);
		std::u8string transOptionsSec = bareTranslated.substr(
			bareTranslated.find(u8"<sel>") + 5);

		// 按 <lf> 分割选项
		auto splitOptions = [](const std::u8string& sec) -> std::vector<std::u8string> {
			std::vector<std::u8string> opts;
			size_t pos = 0;
			size_t found;
			while ((found = sec.find(u8"<lf>", pos)) != std::u8string::npos) {
				opts.push_back(sec.substr(pos, found - pos));
				pos = found + 4;
			}
			std::u8string last = sec.substr(pos);
			if (!last.empty())
				opts.push_back(last);
			return opts;
			};

		std::vector<std::u8string> options = splitOptions(optionsSec);
		std::vector<std::u8string> transOptions = splitOptions(transOptionsSec);

		// 如果选项数量不一致，拒绝合并，直接返回译文
		if (options.size() != transOptions.size())
			return translatedText;

		// 构建 Rosetta 文本
		std::u8string rosettaText = transPromptSec + u8"<sel>";
		for (size_t i = 0; i < options.size(); ++i)
		{
			if (insMode == 0)
			{
				// 译文 + 分隔符 + 原文
				rosettaText += transOptions[i] + u8"|" + options[i];
			}
			else  // insMode == 1
			{
				// 原文 + 分隔符 + 译文
				rosettaText += options[i] + u8"|" + transOptions[i];
			}
			if (i < options.size() - 1)
				rosettaText += u8"<lf>";
		}

		return rosettaText + endingCtrlSeq;
	}

	// ===== 处理普通行 =====

	constexpr int NAME_INDENT = 9; // 第一行预留名字宽度（ASCII）

	auto originalLines = SplitLines(bareOriginal);
	auto translatedLines = SplitLines(bareTranslated);

	if (originalLines.size() == 1 && translatedLines.size() != 1)
	{
		// 对英语文件：通常没有换行，直接输出一行原文，并将译文的<lf>脱去直接拼接返回：
		std::u8string result;
		if (insMode == 0)
		{
			std::u8string result;
			for (auto &&line : translatedLines)
			{
				result += line;
			}
			result += u8"<lf>";
			result += originalLines[0];
			return result + endingCtrlSeq;
		}
		else {
			std::u8string result;
			result += originalLines[0];
			result += u8"<lf>";
			for (auto &&line : translatedLines)
			{
				result += line;
			}
			return result + endingCtrlSeq;
		}
	}

	// 计算左侧需要占用的最大显示宽度
	int targetWidth = 0;

	if (insMode == 0)
	{
		// 左侧为译文
		for (size_t i = 0; i < translatedLines.size(); ++i)
		{
			int w = DisplayWidth(translatedLines[i]);

			targetWidth = std::max(targetWidth, w);
		}
	}
	else
	{
		// 左侧为原文
		for (size_t i = 0; i < originalLines.size(); ++i)
		{
			int w = DisplayWidth(originalLines[i]);

			targetWidth = std::max(targetWidth, w);
		}
	}

	if (targetWidth > 40)
		targetWidth = 40; // 超过 40 个字符宽度时，令超过的行直接拼接，不再对齐

	std::u8string result;

	size_t lineCount = std::max(originalLines.size(), translatedLines.size());

	for (size_t i = 0; i < lineCount; ++i)
	{
		std::u8string left;
		std::u8string right;

		if (insMode == 0)
		{
			if (i < translatedLines.size())
				left = translatedLines[i];

			if (i < originalLines.size())
				right = originalLines[i];
		}
		else
		{
			if (i < originalLines.size())
				left = originalLines[i];

			if (i < translatedLines.size())
				right = translatedLines[i];
		}

		result += left;

		int pad = targetWidth - DisplayWidth(left);
		if (i == 0)
			pad -= NAME_INDENT;

		// 至少留一个空格
		if (pad < 1)
			pad = 1;

		result.append(static_cast<size_t>(pad), u8' ');

		result += separator;
		result += right;

		if (i + 1 != lineCount)
			result += u8"<lf>";
	}

	return result + endingCtrlSeq;
}

bool EventProcessor::Process(
	const FileProcessDef& fileDef,
	const fs::path& datPath,
	const fs::path& outPath,
	const std::map<std::u8string, FileProcessDef>& jpDefsByComment)
{
	if (fileDef.type != u8"evsb")
		return false;

	auto& db = TranslationDatabase::Instance();
	auto& cfg = Config::Instance();

	EventStringBase evsb(datPath);
	evsb.Read();

	FinalTextProcessor finalTextProcessor(fileDef.comment, fileDef.type);

	std::string commentStr = xybase::string::to_string(fileDef.comment);
	std::string zoneName = commentStr;

	db.LoadLocalScope(
		cfg.GetProgRoot() / L"text" / L"src" / (fileDef.comment + std::u8string(u8".txt")),
		cfg.GetProgRoot() / L"text" / L"tgt" / (fileDef.comment + std::u8string(u8".txt")));

	auto& defs = DefsCache::Get();
	auto progRoot = cfg.GetProgRoot();
	defs.Load(progRoot / L"defs.csv");

	std::vector<std::u8string> referenceTexts;
	bool useJaReference = TryGetJapaneseReference(fileDef, jpDefsByComment, referenceTexts);

	auto* zone = defs.Find(zoneName);
	if (!zone || zone->evev_path.empty())
	{
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

	std::map<size_t, std::u8string> patches;
	fs::path srcEventBase = progRoot / L"text" / L"src" / L"event";
	fs::path tgtEventBase = progRoot / L"text" / L"tgt" / L"event";

	for (const auto& actor : evev.GetActors())
	{
		auto it = actorNameMap.find(actor.actor_id);
		std::string actorName = (it != actorNameMap.end()) ? it->second : std::to_string(actor.actor_id);
		std::string actorDir = SafeName(actorName) + "_" + std::to_string(actor.actor_id);

		std::vector<std::pair<uint32_t, std::u8string>> validIndices;
		for (uint32_t idx : actor.constants)
		{
			if (idx < evsb.Size() && idx >= 0)
			{
				if (useJaReference && idx < referenceTexts.size())
					validIndices.push_back({ idx, referenceTexts[idx] });
				else
					validIndices.push_back({ idx, evsb[idx] });
			}
		}

		for (const auto& evt : actor.events)
		{
			fs::path srcPath = ResolveEventSrcPath(actorName, actor.actor_id, evt.event_index, zoneName);

			if (srcPath.empty())
				continue;

			Logger::Instance().Info("EventProcessor: processing event overlay: " + Logger::ToUtf8(srcPath));

			fs::path tgtPath = tgtEventBase / srcPath.lexically_relative(srcEventBase);
			if (fs::exists(tgtPath) == false)
			{
				Logger::Instance().Warning("EventProcessor: target event file not found: " + Logger::ToUtf8(tgtPath));
				continue;
			}

			auto srcLines = ReadTextLines(srcPath);
			auto tgtLines = ReadTextLines(tgtPath);

			if (srcLines.empty() || tgtLines.empty())
			{
				Logger::Instance().Warning("EventProcessor: empty source or target event file: " + Logger::ToUtf8(srcPath) + " / " + Logger::ToUtf8(tgtPath));
				continue;
			}
			if (srcLines.size() != tgtLines.size())
			{
				Logger::Instance().Warning("EventProcessor: source and target event file line count mismatch: " + std::to_string(srcLines.size()) + "<->" + std::to_string(tgtLines.size()));
				continue;
			}

			size_t matchedInEvent = 0;
			for (size_t i = 0; i < srcLines.size(); ++i)
			{
				const std::u8string& srcLine = srcLines[i];
				for (const auto& [idx, text] : validIndices)
				{
					if (text == srcLine)
					{
						patches[idx] = tgtLines[i];
						++matchedInEvent;
					}
				}
			}
			Logger::Instance().Info("EventProcessor: Actor " + std::to_string(actor.actor_id) + " (" + actorName + ") event index " + std::to_string(evt.event_index) + " processed, " + std::to_string(matchedInEvent) + " patch entries from " + std::to_string(srcLines.size()) + " source lines.");
		}
	}
	if (patches.size() > 0)
		Logger::Instance().Info("EventProcessor: Event override collected. total " + std::to_string(patches.size()) + " lines will be patched in " + std::to_string(evsb.Size()) + " total lines.");

	for (size_t i = 0; i < evsb.Size(); ++i)
	{
		auto& s = evsb[i];
		std::u8string result;
		auto patchIt = patches.find(i);
		if (patchIt != patches.end())
		{
			result = patchIt->second;
			if (useJaReference && i < referenceTexts.size())
			{
				if (!ProcessorUtils::TryAdaptInsCategoryForEnglish(s, result))
				{
					// If adaptation fails, fallback to the original text's translation, no event override can be applied
					result = db.GetTranslation(s);
				}
			}
		}
		else
		{
			if (useJaReference && i < referenceTexts.size())
			{
				std::u8string refTranslated;
				if (db.TryGetTranslationFromReference(s, referenceTexts[i], refTranslated)
					&& ProcessorUtils::TryAdaptInsCategoryForEnglish(s, refTranslated))
					result = refTranslated;
				else
					result = db.GetTranslation(s);
			}
			else
			{
				result = db.GetTranslation(s);
			}
		}
		if (Config::Instance().GetRosettaMode() != Config::RosettaMode::Off
			&& !fileDef.comment.starts_with(u8"gev/"))
		{
			result = MakeRosettaText(s, result, u8"|", Config::Instance().GetRosettaMode() == Config::RosettaMode::AfterOriginal ? 1 : 0);
		}
		s = finalTextProcessor.Process(result, s, static_cast<int64_t>(i + 1), 1);
	}

	evsb.path = outPath;
	evsb.Write();

	db.ClearLocalScope();
	return true;
}
