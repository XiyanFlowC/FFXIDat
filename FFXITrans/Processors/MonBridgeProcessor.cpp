#include "MonBridgeProcessor.h"
#include "../FinalTextProcessor.h"
#include "../TranslationDatabase.h"
#include "../Config.h"
#include "../ProcessorUtils.h"
#include <MonBridge.h>
#include <xystring.h>

bool MonBridgeProcessor::Process(
	const FileProcessDef& fileDef,
	const std::filesystem::path& datPath,
	const std::filesystem::path& outPath,
	const std::map<std::u8string, FileProcessDef>& jpDefsByComment)
{
	MonBridge monBridge;
	monBridge.Read(datPath);
	FinalTextProcessor finalTextProcessor(fileDef.comment, fileDef.type);

	// Check for ID-mapped reference (en_as_ja mode)
	std::map<uint32_t, std::u8string> jpTextsById;
	if (Config::Instance().IsEnglishMode() && Config::Instance().IsEnAsJa())
	{
		auto jpItr = jpDefsByComment.find(fileDef.comment);
		if (jpItr != jpDefsByComment.end() && jpItr->second.type == fileDef.type)
		{
			std::filesystem::path jpDatPath = Config::Instance().GetGameRoot() / (jpItr->second.path + u8".DAT");
			if (std::filesystem::exists(jpDatPath))
			{
				jpTextsById = ProcessorUtils::CollectMonBridgeTextsById(jpDatPath);
			}
		}
	}

	// Try regular Japanese reference
	std::vector<std::u8string> referenceTexts;
	bool useJaReference = jpTextsById.empty() && TryGetJapaneseReference(fileDef, jpDefsByComment, referenceTexts);

	auto& db = TranslationDatabase::Instance();
	size_t textIdx = 0;

	for (auto& datum : monBridge.data)
	{
		// Only translate display name (internal name must NOT be translated)
		// Internal name is the ASCII identifier that the game uses to find entries
		if (!datum.displayName.empty())
		{
			std::u8string text = xybase::string::escape(datum.displayName);
			std::u8string translated;

			if (!jpTextsById.empty())
			{
				auto jpTextItr = jpTextsById.find(datum.id);
				if (jpTextItr != jpTextsById.end())
				{
					translated = db.GetTranslationFromReference(text, jpTextItr->second);
				}
				else
				{
					translated = db.GetTranslation(text);
				}
			}
			else if (useJaReference && textIdx < referenceTexts.size())
			{
				translated = db.GetTranslationFromReference(text, referenceTexts[textIdx]);
			}
			else
			{
				translated = db.GetTranslation(text);
			}

			datum.displayName = xybase::string::unescape(finalTextProcessor.ProcessEscaped(
				translated,
				text,
				datum.id,
				1));
			++textIdx;
		}
	}

	monBridge.Write(outPath);
	return true;
}
