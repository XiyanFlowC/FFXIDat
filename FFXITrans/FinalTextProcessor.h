#pragma once
#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <regex>
#include <string>
#include <vector>

class FinalTextProcessor
{
public:
	FinalTextProcessor(const std::u8string& comment, const std::u8string& type);

	static void ResetValidationSummary();
	static size_t GetSkippedValidationCount();

	std::u8string Process(
		const std::u8string& translatedText,
		const std::u8string& originalText,
		std::optional<int64_t> rowOrId = std::nullopt,
		std::optional<int64_t> colOrColId = std::nullopt);

	std::u8string ProcessEscaped(
		const std::u8string& translatedText,
		const std::u8string& originalText,
		std::optional<int64_t> rowOrId = std::nullopt,
		std::optional<int64_t> colOrColId = std::nullopt);

private:
	enum class RuleCommand
	{
		Rep,
		RepRe,
		Set,
		SetNxt,
	};

	struct OccurrenceRange
	{
		size_t start = 0;
		size_t end = 0;
	};

	struct Rule
	{
		RuleCommand command = RuleCommand::Rep;
		std::u8string translatedPattern;
		std::u8string targetText;
		std::u8string originalPattern;
		std::u8string originalExcludePattern;
		std::vector<OccurrenceRange> occurrenceRanges;
		std::optional<std::regex> translatedRegex;
		std::optional<std::regex> originalRegex;
		std::optional<std::regex> originalExcludeRegex;
		size_t occurrence = 0;
		std::filesystem::path sourcePath;
		size_t sourceLine = 0;
		bool valid = true;
		std::u8string nextTextOverride;
	};

	void LoadRules();
	size_t LoadRuleFile(const std::filesystem::path& path);
	bool TryBuildRule(
		const std::filesystem::path& path,
		size_t lineNumber,
		const std::u8string& commandText,
		const std::u8string& translatedPattern,
		const std::u8string& targetText,
		const std::u8string& originalPattern,
		const std::u8string& originalExcludePattern,
		const std::u8string& occurrenceText,
		Rule& rule);
	bool TryParseOccurrenceRanges(const std::u8string& occurrenceText, std::vector<OccurrenceRange>& ranges) const;
	bool MatchesOriginal(Rule& rule, const std::string& originalText);
	bool IsOccurrenceEnabled(const Rule& rule) const;
	std::u8string ApplyRule(const Rule& rule, const std::u8string& translatedText) const;
  std::u8string ValidateResult(
		const std::u8string& processedText,
		const std::u8string& originalText,
		std::optional<int64_t> rowOrId,
		std::optional<int64_t> colOrColId) const;
	bool TryValidateEvsbSwitchUsage(const std::u8string& processedText, const std::u8string& originalText, std::wstring& message) const;
   bool TryValidateEvsbGenderUsage(const std::u8string& processedText, std::wstring& message) const;
	std::wstring BuildValidationContext(std::optional<int64_t> rowOrId, std::optional<int64_t> colOrColId) const;
	void ReportRuleError(const std::filesystem::path& path, size_t lineNumber, const std::wstring& message) const;
	static std::string ToString(const std::u8string& text);
	static std::u8string ReplaceAll(const std::u8string& text, const std::u8string& from, const std::u8string& to);

  static size_t skippedValidationCount;

	std::u8string comment;
	std::u8string type;
	std::vector<Rule> rules;
	std::u8string pendingNextTextOverride;
};
