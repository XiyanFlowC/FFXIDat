#include "SpecialProcessor.h"
#include "Config.h"
#include "DMsg.h"
#include "FinalTextProcessor.h"
#include "TranslationDatabase.h"
#include "xystring.h"

bool SpecialProcessor::Process(const FileProcessDef& fileDef, const std::filesystem::path& datPath, const std::filesystem::path& outPath, const std::map<std::u8string, FileProcessDef>& jpDefsByComment)
{
	if (fileDef.comment == u8"sys/job")
	{
		return TryProcessJobName(fileDef, datPath, outPath, jpDefsByComment);
	}
	return false;
}

std::u8string SpecialProcessor::GetSupportedType() const
{
	return std::u8string();
}



bool SpecialProcessor::TryProcessJobName(const FileProcessDef& fileDef, const std::filesystem::path& inputPath, const std::filesystem::path& outputPath, const std::map<std::u8string, FileProcessDef>& jpDefsByComment)
{
	if (fileDef.lang == u8"en") return true; // 英语JobName用于UI显示，修改会导致UI无法正确显示职业名称，若不慎传入，忽略

	if (fileDef.lang != u8"jp") return false;

	DMsg jobDmsg(inputPath);
	jobDmsg.Read();
 FinalTextProcessor finalTextProcessor(fileDef.comment, fileDef.type);
	/*std::set<int> targetCells = ProcessorUtils::ParseCellIndices(fileDef.cellIndicesStr);
	if (targetCells.empty())
		return true;*/

 auto& db = TranslationDatabase::Instance();
	int64_t rowIndex = 0;
	for (auto& row : jobDmsg)
	{
	   ++rowIndex;
		auto& cells = row.GetCells();
		if (cells.size() < 1 || cells[0].GetType() != 0)
			continue;

		std::u8string text = xybase::string::escape(cells[0].Get<std::u8string>());
		std::u8string trans = xybase::string::unescape(finalTextProcessor.ProcessEscaped(
			db.GetTranslation(text),
			text,
			rowIndex,
			1));
		if (Config::Instance().IsSamuraiJobTransNot() && text == u8"侍")
			continue; // 特例：侍翻译为武士则简称和武僧冲突，故不翻译侍这个职业名称，玩家能看懂
		if (Config::Instance().IsSamuraiJobSpecial() && text == u8"侍")
			trans = u8"侍（武士）";  // 特例：侍翻译为侍（武士），既保证了简称不冲突，在显示全名时玩家也能看懂（虽然有点冗长）
		if (Config::Instance().IsMonkJobAbbreviated() && text == u8"モンク")
			trans = u8"僧"; // 特例：モンク翻译为僧，玩家能看懂且更简洁（虽然显示全称时会有点奇怪，但总比武僧更好）
		cells[0].Set(trans);
	}
	jobDmsg.path = outputPath;
	jobDmsg.Write();

	return true;
}

