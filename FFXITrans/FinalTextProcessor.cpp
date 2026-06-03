#include "FinalTextProcessor.h"
#include "FinalTextProcessor.h"

#include "ChsToSJis.h"
#include "Config.h"
#include "Logger.h"
#include <fstream>
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
}

FinalTextProcessor::FinalTextProcessor(const std::u8string& comment, const std::u8string& type)
	: comment(comment), type(type)
{
	LoadRules();
}

std::u8string FinalTextProcessor::Process(
	const std::u8string& translatedText,
	const std::u8string& originalText,
	std::optional<int64_t> rowOrId,
	std::optional<int64_t> colOrColId)
{
	std::u8string result = translatedText;
	const std::string originalTextStr = ToString(originalText);

	for (auto& rule : rules)
	{
		if (!rule.valid || !MatchesOriginal(rule, originalTextStr))
			continue;

		++rule.occurrence;
		if (!IsOccurrenceEnabled(rule))
			continue;

		result = ApplyRule(rule, result);
	}

	// TODO: Invoke Lua script for arbitrary per-text processing.
	result = ChsToSJis::Instance().ReplaceHanzi(result);

	// TODO: Validate format by type. On failure, report with row/id and col/colId and block this translation only.
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
			if (cells.size() > 1) translatedPattern = cells[1];
			if (cells.size() > 2) targetText = cells[2];
			if (cells.size() > 3) originalPattern = cells[3];
			if (cells.size() > 4) originalExcludePattern = cells[4];
			if (cells.size() > 5) occurrenceText = cells[5];
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
	if (rule.translatedPattern.empty())
		return rule.targetText;

	if (rule.command == RuleCommand::Rep)
		return ReplaceAll(translatedText, rule.translatedPattern, rule.targetText);

	return xybase::string::to_utf8(std::regex_replace(
		ToString(translatedText),
		*rule.translatedRegex,
		ToString(rule.targetText)));
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
