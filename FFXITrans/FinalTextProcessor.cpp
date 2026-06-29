#include "FinalTextProcessor.h"
#include "FinalTextProcessor.h"

#include "ChsToSJis.h"
#include "Config.h"
#include "Logger.h"
#include <fstream>
#include <regex>
#include <xystring.h>
#include <iostream>
#include <limits>
#include <sstream>

namespace
{
	std::u8string RawBytesToUtf8(const std::string& text)
	{
		return std::u8string(reinterpret_cast<const char8_t*>(text.data()), text.size());
	}

	bool TryParseRuleCsvLine(const std::string& line, std::vector<std::u8string>& cells, std::string& error)
	{
		cells.clear();
		std::string current;
		bool inQuotes = false;

		for (size_t i = 0; i < line.size(); ++i)
		{
			const char ch = line[i];
			if (inQuotes)
			{
				if (ch == '"')
				{
					if (i + 1 < line.size() && line[i + 1] == '"')
					{
						current.push_back('"');
						++i;
					}
					else
					{
						inQuotes = false;
					}
				}
				else
				{
					current.push_back(ch);
				}
			}
			else
			{
				if (ch == ',')
				{
					cells.push_back(RawBytesToUtf8(current));
					current.clear();
				}
				else if (ch == '"')
				{
					inQuotes = true;
				}
				else
				{
					current.push_back(ch);
				}
			}
		}

		if (inQuotes)
		{
			error = "unexpected EOF inside quoted cell";
			return false;
		}

		cells.push_back(RawBytesToUtf8(current));
		return true;
	}

	struct SwitchUsageInfo
	{
		size_t optionCount = 0;
	};

	std::vector<SwitchUsageInfo> ParseSwitchUsageInfos(const std::string& text, bool& syntaxValid)
	{
		std::vector<SwitchUsageInfo> infos;
		syntaxValid = true;

		constexpr std::string_view tagPrefix = "<switch:";
		size_t pos = 0;
		while ((pos = text.find(tagPrefix, pos)) != std::string::npos)
		{
			const auto tagEnd = text.find('>', pos + tagPrefix.size());
			if (tagEnd == std::string::npos)
			{
				syntaxValid = false;
				return infos;
			}

			const size_t listStart = tagEnd + 1;
			if (listStart >= text.size() || text[listStart] != '[')
			{
				syntaxValid = false;
				return infos;
			}

			const auto listEnd = text.find(']', listStart + 1);
			if (listEnd == std::string::npos)
			{
				syntaxValid = false;
				return infos;
			}

			const auto optionsText = text.substr(listStart + 1, listEnd - listStart - 1);
			size_t optionCount = 1;
			for (char ch : optionsText)
			{
				if (ch == '/')
				{
					++optionCount;
				}
			}

			infos.push_back(SwitchUsageInfo{ optionCount });
			pos = listEnd + 1;
		}

		return infos;
	}

	bool ValidateGenderLists(const std::string& text)
	{
		constexpr std::string_view tag = "<gender>";
		size_t pos = 0;
		while ((pos = text.find(tag, pos)) != std::string::npos)
		{
			const size_t listStart = pos + tag.size();
			if (listStart >= text.size() || text[listStart] != '[')
			{
				return false;
			}

			const auto listEnd = text.find(']', listStart + 1);
			if (listEnd == std::string::npos)
			{
				return false;
			}

			const auto optionsText = text.substr(listStart + 1, listEnd - listStart - 1);
			size_t separatorCount = 0;
			for (char ch : optionsText)
			{
				if (ch == '/')
				{
					++separatorCount;
				}
			}

			if (separatorCount != 1)
			{
				return false;
			}

			pos = listEnd + 1;
		}

		return true;
	}
}

size_t FinalTextProcessor::skippedValidationCount = 0;

FinalTextProcessor::FinalTextProcessor(const std::u8string& comment, const std::u8string& type)
	: comment(comment), type(type)
{
	LoadRules();
}

void FinalTextProcessor::ResetValidationSummary()
{
	skippedValidationCount = 0;
}

size_t FinalTextProcessor::GetSkippedValidationCount()
{
	return skippedValidationCount;
}

std::u8string FinalTextProcessor::Process(
	const std::u8string& translatedText,
	const std::u8string& originalText,
	std::optional<int64_t> rowOrId,
	std::optional<int64_t> colOrColId)
{
	std::u8string result = translatedText;

	// Check if there's a pending override from previous SETNXT
	if (!pendingNextTextOverride.empty())
	{
		result = pendingNextTextOverride;
		pendingNextTextOverride.clear();
		result = ChsToSJis::Instance().ReplaceHanzi(result);
		result = ValidateResult(result, originalText, rowOrId, colOrColId);
		return result;
	}

	const std::string originalTextStr = ToString(originalText);

	for (auto& rule : rules)
	{
		if (!rule.valid || !MatchesOriginal(rule, originalTextStr))
			continue;

		++rule.occurrence;
		if (!IsOccurrenceEnabled(rule))
			continue;

		// Handle SETNXT command
		if (rule.command == RuleCommand::SetNxt)
		{
			pendingNextTextOverride = rule.targetText;
			continue;
		}

		// Handle SET command
		if (rule.command == RuleCommand::Set)
		{
			result = rule.targetText;
			break; // SET directly replaces the entire text, no further processing
		}

		result = ApplyRule(rule, result);
	}

	// TODO: Invoke Lua script for arbitrary per-text processing.
	result = ChsToSJis::Instance().ReplaceHanzi(result);
	result = ValidateResult(result, originalText, rowOrId, colOrColId);

	(void)rowOrId;
	(void)colOrColId;

	return result;
}

std::u8string FinalTextProcessor::ProcessEscaped(
	const std::u8string& translatedText,
	const std::u8string& originalText,
	std::optional<int64_t> rowOrId,
	std::optional<int64_t> colOrColId)
{
	return xybase::string::escape(Process(
		xybase::string::unescape(translatedText),
		xybase::string::unescape(originalText),
		rowOrId,
		colOrColId));
}

void FinalTextProcessor::LoadRules()
{
	const auto rulesRoot = Config::Instance().GetProgRoot() / "rules";
	const size_t commonRuleCount = LoadRuleFile(rulesRoot / "common.csv");
	auto fileRulePath = rulesRoot / std::filesystem::path(comment);
	fileRulePath += ".csv";
	const size_t fileRuleCount = LoadRuleFile(fileRulePath);
	if (commonRuleCount > 0 || fileRuleCount > 0)
	{
		Logger::Instance().Info(
			"Loaded final text rules for comment='" + ToString(comment)
			+ "': common=" + std::to_string(commonRuleCount)
			+ ", file=" + std::to_string(fileRuleCount));
	}
}

size_t FinalTextProcessor::LoadRuleFile(const std::filesystem::path& path)
{
	namespace fs = std::filesystem;

	if (!fs::exists(path) || !fs::is_regular_file(path))
		return 0;

	std::ifstream input(path, std::ios::in | std::ios::binary);
	if (!input.is_open())
	{
		throw std::runtime_error("failed to open rule file");
	}

	size_t lineNumber = 0;
	size_t loadedCount = 0;
	std::string line;
	bool firstLine = true;

	while (std::getline(input, line))
	{
		++lineNumber;

		if (!line.empty() && line.back() == '\r')
		{
			line.pop_back();
		}

		if (firstLine)
		{
			firstLine = false;
			if (line.size() >= 3
				&& static_cast<unsigned char>(line[0]) == 0xEF
				&& static_cast<unsigned char>(line[1]) == 0xBB
				&& static_cast<unsigned char>(line[2]) == 0xBF)
			{
				line.erase(0, 3);
			}
		}

		std::u8string commandText;
		std::u8string translatedPattern;
		std::u8string targetText;
		std::u8string originalPattern;
		std::u8string originalExcludePattern;
		std::u8string occurrenceText;
		std::vector<std::u8string> cells;

		try
		{
			std::string error;
			if (!TryParseRuleCsvLine(line, cells, error))
			{
				throw std::runtime_error(error);
			}

			if (cells.size() > 0) commandText = cells[0];

			if (commandText == u8"REP" || commandText == u8"REPRE")
			{
				if (cells.size() > 1) translatedPattern = cells[1];
				if (cells.size() > 2) targetText = cells[2];
				if (cells.size() > 3) originalPattern = cells[3];
				if (cells.size() > 4) originalExcludePattern = cells[4];
				if (cells.size() > 5) occurrenceText = cells[5];
			}
			else
			{
				translatedPattern = u8""; // Not used for non-regex commands, but set to empty to avoid confusion
				if (cells.size() > 1) targetText = cells[1];
				if (cells.size() > 2) originalPattern = cells[2];
				if (cells.size() > 3) originalExcludePattern = cells[3];
				if (cells.size() > 4) occurrenceText = cells[4];
			}
		}
		catch (const std::exception& ex)
		{
			ReportRuleError(path, lineNumber, L"failed to parse csv row");
			Logger::Instance().Error(
				"FinalTextProcessor CSV parse error: file=" + Logger::ToUtf8(path)
				+ ", line=" + std::to_string(lineNumber)
				+ ", error=" + ex.what());
			throw;
		}

		if (commandText.empty() || commandText[0] == u8'#')
			continue;

		Rule rule;
		if (TryBuildRule(
			path,
			lineNumber,
			commandText,
			translatedPattern,
			targetText,
			originalPattern,
			originalExcludePattern,
			occurrenceText,
			rule))
		{
			rules.push_back(std::move(rule));
			++loadedCount;
		}
	}

	Logger::Instance().Info("Loaded " + std::to_string(loadedCount) + " final text rules from " + Logger::ToUtf8(path));
	return loadedCount;
}

bool FinalTextProcessor::TryBuildRule(
	const std::filesystem::path& path,
	size_t lineNumber,
	const std::u8string& commandText,
	const std::u8string& translatedPattern,
	const std::u8string& targetText,
	const std::u8string& originalPattern,
	const std::u8string& originalExcludePattern,
	const std::u8string& occurrenceText,
	Rule& rule)
{
	rule.translatedPattern = translatedPattern;
	rule.targetText = targetText;
	rule.originalPattern = originalPattern;
	rule.originalExcludePattern = originalExcludePattern;
	rule.sourcePath = path;
	rule.sourceLine = lineNumber;

	const std::string command = ToString(commandText);
	if (command == "REP")
	{
		rule.command = RuleCommand::Rep;
	}
	else if (command == "REPRE")
	{
		rule.command = RuleCommand::RepRe;
	}
	else if (command == "SET")
	{
		rule.command = RuleCommand::Set;
	}
	else if (command == "SETNXT")
	{
		rule.command = RuleCommand::SetNxt;
	}
	else
	{
		ReportRuleError(path, lineNumber, L"unknown rule command");
		return false;
	}

	if (!TryParseOccurrenceRanges(occurrenceText, rule.occurrenceRanges))
	{
		ReportRuleError(path, lineNumber, L"invalid occurrence range");
		return false;
	}

	try
	{
		if (!originalPattern.empty())
			rule.originalRegex.emplace(ToString(originalPattern), std::regex::ECMAScript);
		if (!originalExcludePattern.empty())
			rule.originalExcludeRegex.emplace(ToString(originalExcludePattern), std::regex::ECMAScript);
		if (rule.command == RuleCommand::RepRe && !translatedPattern.empty())
			rule.translatedRegex.emplace(ToString(translatedPattern), std::regex::ECMAScript);
	}
	catch (const std::regex_error&)
	{
		ReportRuleError(path, lineNumber, L"invalid regex");
		return false;
	}

	return true;
}

bool FinalTextProcessor::TryParseOccurrenceRanges(
	const std::u8string& occurrenceText,
	std::vector<OccurrenceRange>& ranges) const
{
	if (occurrenceText.empty())
		return true;

	std::stringstream ss(ToString(occurrenceText));
	std::string token;

	while (std::getline(ss, token, '|'))
	{
		if (token.empty())
			continue;

		const size_t dashPos = token.find('-');
		std::string startText = token;
		std::string endText = token;
		if (dashPos != std::string::npos)
		{
			startText = token.substr(0, dashPos);
			endText = token.substr(dashPos + 1);
		}

		try
		{
			const unsigned long long startValue = std::stoull(startText);
			const unsigned long long endValue = std::stoull(endText);
			if (startValue == 0 || endValue == 0 || startValue > endValue)
				return false;

			ranges.push_back(OccurrenceRange{
				static_cast<size_t>(startValue),
				static_cast<size_t>(endValue)
				});
		}
		catch (const std::exception&)
		{
			return false;
		}
	}

	return !ranges.empty();
}

bool FinalTextProcessor::MatchesOriginal(Rule& rule, const std::string& originalText)
{
	if (rule.originalRegex.has_value() && !std::regex_search(originalText, *rule.originalRegex))
		return false;

	if (rule.originalExcludeRegex.has_value() && std::regex_search(originalText, *rule.originalExcludeRegex))
		return false;

	return true;
}

bool FinalTextProcessor::IsOccurrenceEnabled(const Rule& rule) const
{
	if (rule.occurrenceRanges.empty())
		return true;

	for (const auto& range : rule.occurrenceRanges)
	{
		if (rule.occurrence >= range.start && rule.occurrence <= range.end)
			return true;
	}

	return false;
}

std::u8string FinalTextProcessor::ApplyRule(const Rule& rule, const std::u8string& translatedText) const
{
	// SET and SETNXT are handled in Process(), not here
	if (rule.command == RuleCommand::Set || rule.command == RuleCommand::SetNxt)
		return translatedText;

	if (rule.translatedPattern.empty())
		return rule.targetText;

	if (rule.command == RuleCommand::Rep)
		return ReplaceAll(translatedText, rule.translatedPattern, rule.targetText);

	return xybase::string::to_utf8(std::regex_replace(
		ToString(translatedText),
		*rule.translatedRegex,
		ToString(rule.targetText)));
}

std::u8string FinalTextProcessor::ValidateResult(
	const std::u8string& processedText,
	const std::u8string& originalText,
	std::optional<int64_t> rowOrId,
	std::optional<int64_t> colOrColId) const
{
	const auto mode = Config::Instance().GetCtrlSeqCheckMode();
	if (mode == Config::CtrlSeqCheckMode::Off)
		return processedText;

	if (type != u8"evsb")
		return processedText;

	std::wstring message;
	if (!TryValidateEvsbSwitchUsage(processedText, originalText, message))
	{
		const auto context = BuildValidationContext(rowOrId, colOrColId);
		const auto originalTextWide = xybase::string::to_wstring(originalText);
		const auto processedTextWide = xybase::string::to_wstring(processedText);
		const auto fullMessage = L"Final Text Processor 控制序列校验失败：" + context + L"\n"
			+ L"原因：" + message + L"\n"
			+ L"原文：" + originalTextWide + L"\n"
			+ L"译文：" + processedTextWide;

		Logger::Instance().Warning(fullMessage);

		if (mode == Config::CtrlSeqCheckMode::Strict)
		{
			throw std::runtime_error("control sequence validation failed; see log.txt for details");
		}

		++skippedValidationCount;
		return originalText;
	}

	if (TryValidateEvsbGenderUsage(processedText, message))
		return processedText;

	const auto context = BuildValidationContext(rowOrId, colOrColId);
	const auto originalTextWide = xybase::string::to_wstring(originalText);
	const auto processedTextWide = xybase::string::to_wstring(processedText);
	const auto fullMessage = L"Final Text Processor 控制序列校验失败：" + context + L"\n"
		+ L"原因：" + message + L"\n"
		+ L"原文：" + originalTextWide + L"\n"
		+ L"译文：" + processedTextWide;

	Logger::Instance().Warning(fullMessage);

	if (mode == Config::CtrlSeqCheckMode::Strict)
	{
		throw std::runtime_error("control sequence validation failed; see log.txt for details");
	}

	++skippedValidationCount;
	return originalText;
}

bool FinalTextProcessor::TryValidateEvsbSwitchUsage(
	const std::u8string& processedText,
	const std::u8string& originalText,
	std::wstring& message) const
{
	const std::string processed(reinterpret_cast<const char*>(processedText.data()), processedText.size());
	const std::string original(reinterpret_cast<const char*>(originalText.data()), originalText.size());
	bool translatedSyntaxValid = true;
	const auto translatedSwitchInfos = ParseSwitchUsageInfos(processed, translatedSyntaxValid);
	if (!translatedSyntaxValid)
	{
		message = L"译文中的 <switch:...> 后未紧跟合法的分支列表。";
		return false;
	}

	bool originalSyntaxValid = true;
	const auto originalSwitchInfos = ParseSwitchUsageInfos(original, originalSyntaxValid);
	if (!originalSyntaxValid)
	{
		message = L"原文中的 <switch:...> 结构无法解析，无法执行选项数校验。";
		return false;
	}

	if (!originalSwitchInfos.empty() && originalSwitchInfos.size() == translatedSwitchInfos.size())
	{
		for (size_t i = 0; i < originalSwitchInfos.size(); ++i)
		{
			if (originalSwitchInfos[i].optionCount != translatedSwitchInfos[i].optionCount)
			{
				message = L"原文与译文对应 <switch:...> 的选项数量不一致。";
				return false;
			}
		}
	}

	return true;
}

bool FinalTextProcessor::TryValidateEvsbGenderUsage(
	const std::u8string& processedText,
	std::wstring& message) const
{
	const std::string processed(reinterpret_cast<const char*>(processedText.data()), processedText.size());
	if (!ValidateGenderLists(processed))
	{
		message = L"译文中的 <gender> 后必须紧跟 [A/B] 形式的两项分支列表。";
		return false;
	}

	return true;
}

std::wstring FinalTextProcessor::BuildValidationContext(std::optional<int64_t> rowOrId, std::optional<int64_t> colOrColId) const
{
	std::wstringstream stream;
	stream << L"comment=" << xybase::string::to_wstring(comment);
	if (rowOrId.has_value())
		stream << L", row/id=" << *rowOrId;
	if (colOrColId.has_value())
		stream << L", col=" << *colOrColId;
	return stream.str();
}

void FinalTextProcessor::ReportRuleError(
	const std::filesystem::path& path,
	size_t lineNumber,
	const std::wstring& message) const
{
	std::wcerr << L"[FinalTextProcessor] "
		<< message
		<< L": " << path.wstring()
		<< L" (line " << lineNumber << L")"
		<< std::endl;
	Logger::Instance().Error(
		"FinalTextProcessor rule error: " + Logger::ToUtf8(message)
		+ ", file=" + Logger::ToUtf8(path)
		+ ", line=" + std::to_string(lineNumber));
}

std::string FinalTextProcessor::ToString(const std::u8string& text)
{
	return xybase::string::to_string(text);
}

std::u8string FinalTextProcessor::ReplaceAll(
	const std::u8string& text,
	const std::u8string& from,
	const std::u8string& to)
{
	if (from.empty())
		return text;

	std::u8string result = text;
	size_t pos = 0;
	while ((pos = result.find(from, pos)) != std::u8string::npos)
	{
		result.replace(pos, from.length(), to);
		pos += to.length();
	}
	return result;
}
