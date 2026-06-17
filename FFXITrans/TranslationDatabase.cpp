#include "TranslationDatabase.h"
#include "Config.h"
#include "Logger.h"
#include <Windows.h>
#include <iostream>
#include <xystring.h>

TranslationDatabase& TranslationDatabase::Instance()
{
	static TranslationDatabase instance;
	return instance;
}

void TranslationDatabase::InitializeMismatchLog(const std::filesystem::path& logPath)
{
	if (Config::Instance().IsNoMismatchLog())
	{
		std::wcout << L"配置为不输出失配文本。\n";
		Logger::Instance().Info("Mismatch text output is disabled by config.");
		return;
	}

	mismatchFile.open(logPath, std::ios::out | std::ios::binary);
	if (!mismatchFile.is_open())
	{
		std::wcerr << L"无法创建失配文本文件：" << logPath << std::endl;
		Logger::Instance().Error("Failed to open mismatch output file: " + Logger::ToUtf8(logPath));
		return;
	}

	Logger::Instance().Info("Mismatch output file initialized: " + Logger::ToUtf8(logPath));
}

void TranslationDatabase::CloseMismatchLog()
{
	if (mismatchFile.is_open())
	{
		Logger::Instance().Info("Mismatch output file closed.");
		mismatchFile.close();
	}
}

void TranslationDatabase::Clear()
{
	textMapping.clear();
	mismatchSet.clear();
	mismatchCount = 0;
}

bool TranslationDatabase::PrepareTextStream(std::ifstream& eye)
{
	if (!eye.is_open())
		return false;

	char bom[3] = { 0 };
	eye.read(bom, 3);
	std::streamsize bytesRead = eye.gcount();

	if (bytesRead == 3 && bom[0] == char(0xEF) && bom[1] == char(0xBB) && bom[2] == char(0xBF))
	{
		return true;
	}
	if (bytesRead >= 2 && bom[0] == char(0xFF) && bom[1] == char(0xFE))
	{
		std::wcerr << L"不支持的编码格式（UTF-16 LE），请转换为UTF-8编码。\n";
		return false;
	}
	if (bytesRead >= 2 && bom[0] == char(0xFE) && bom[1] == char(0xFF))
	{
		std::wcerr << L"不支持的编码格式（UTF-16 BE），请转换为UTF-8编码。\n";
		return false;
	}

	eye.clear();
	eye.seekg(0);
	return true;
}

bool TranslationDatabase::ConfirmContinueOnLineCountMismatch(
	const std::filesystem::path& textPath,
	const std::filesystem::path& transPath,
	int loadedLineCount,
	bool translationEndedEarly)
{
	const std::wstring sideText = translationEndedEarly
		? L"译文文件行数少于原文文件。"
		: L"译文文件行数多于原文文件。";

	const std::wstring message =
		L"平行文本库行数不一致。\n\n"
		+ sideText + L"\n\n"
		+ L"原文文件：\n" + textPath.wstring() + L"\n\n"
		+ L"译文文件：\n" + transPath.wstring() + L"\n\n"
		+ L"已加载行数：" + std::to_wstring(loadedLineCount) + L"\n\n"
		+ L"是否继续？";

	std::wcerr << L"平行文本库行数不一致：\n"
		<< L"  原文文件: " << textPath << L"\n"
		<< L"  译文文件: " << transPath << L"\n"
		<< L"  已加载行数: " << loadedLineCount << std::endl;
	Logger::Instance().Warning(
		"Parallel text line count mismatch detected. source=" + Logger::ToUtf8(textPath)
		+ ", target=" + Logger::ToUtf8(transPath)
		+ ", loadedLines=" + std::to_string(loadedLineCount)
		+ ", translationEndedEarly=" + std::string(translationEndedEarly ? "true" : "false"));

	const int result = MessageBoxW(
		nullptr,
		message.c_str(),
		L"FFXI翻译工具",
		MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2 | MB_SETFOREGROUND | MB_TOPMOST);

	const bool shouldContinue = result == IDYES;
	Logger::Instance().Info(std::string("User selected ") + (shouldContinue ? "continue" : "stop")
		+ " after parallel text line count mismatch.");
	return shouldContinue;
}

void TranslationDatabase::RecordMismatch(const std::u8string& text)
{
	if (mismatchSet.find(text) != mismatchSet.end())
		return;

	mismatchCount++;
	mismatchSet.insert(text);

	if (Config::Instance().IsVerbose())
	{
		std::wcout << L"\n失配：" << xybase::string::to_wstring(text) << std::endl;
	}

	if (mismatchFile.is_open())
	{
		mismatchFile.write(reinterpret_cast<const char*>(text.c_str()), text.length());
		mismatchFile << "\n";
	}

	Logger::Instance().Warning("Translation mismatch recorded: " + Logger::ToUtf8(text));
}

std::u8string TranslationDatabase::GetTranslation(const std::u8string& text)
{
	auto lit = localMapping.find(text);
	if (lit != localMapping.end())
		return lit->second;

	auto itr = textMapping.find(text);
	if (itr == textMapping.end())
	{
		RecordMismatch(text);
		return text;
	}

	return itr->second;
}

void TranslationDatabase::LoadLocalScope(const std::filesystem::path& srcPath, const std::filesystem::path& tgtPath)
{
	localMapping.clear();

	if (!std::filesystem::exists(srcPath) || !std::filesystem::exists(tgtPath))
		return;

	std::ifstream src(srcPath, std::ios::in | std::ios::binary);
	std::ifstream tgt(tgtPath, std::ios::in | std::ios::binary);
	if (!src || !tgt) return;

	src.seekg(0); tgt.seekg(0);
	std::string s, t;
	while (std::getline(src, s))
	{
		if (!s.empty() && s.back() == '\r') s.pop_back();
		if (!std::getline(tgt, t)) break;
		if (!t.empty() && t.back() == '\r') t.pop_back();
		localMapping[reinterpret_cast<const char8_t*>(s.c_str())] =
			reinterpret_cast<const char8_t*>(t.c_str());
	}
}

void TranslationDatabase::ClearLocalScope()
{
	localMapping.clear();
}

std::u8string TranslationDatabase::GetTranslationFromReference(const std::u8string& sourceText, const std::u8string& referenceText)
{
	auto itr = textMapping.find(referenceText);
	if (itr == textMapping.end())
	{
		if (mismatchSet.find(referenceText) == mismatchSet.end())
		{
			mismatchCount++;
			mismatchSet.insert(referenceText);

			if (Config::Instance().IsVerbose())
			{
				std::wcout << L"\n参考失配：" << xybase::string::to_wstring(referenceText) << std::endl;
			}

			if (mismatchFile.is_open())
			{
				mismatchFile.write(reinterpret_cast<const char*>(referenceText.c_str()), referenceText.length());
				mismatchFile << "\n";
			}
		}
		return sourceText;
	}

	return itr->second;
}

bool TranslationDatabase::TryGetTranslationFromReference(const std::u8string& sourceText, const std::u8string& referenceText, std::u8string& translation)
{
	auto itr = textMapping.find(referenceText);
	if (itr == textMapping.end())
	{
		if (mismatchSet.find(referenceText) == mismatchSet.end())
		{
			mismatchCount++;
			mismatchSet.insert(referenceText);

			if (Config::Instance().IsVerbose())
			{
				std::wcout << L"\n参考失配：" << xybase::string::to_wstring(referenceText) << std::endl;
			}

			if (mismatchFile.is_open())
			{
				mismatchFile.write(reinterpret_cast<const char*>(referenceText.c_str()), referenceText.length());
				mismatchFile << "\n";
			}
		}
		translation = sourceText;
		return false;
	}

	translation = itr->second;
	return true;
}

int TranslationDatabase::LoadTextPair(const std::filesystem::path& textPath, const std::filesystem::path& transPath)
{
	namespace fs = std::filesystem;

	bool textExists = fs::exists(textPath) && fs::is_regular_file(textPath);
	bool transExists = fs::exists(transPath) && fs::is_regular_file(transPath);

	if (!textExists && !transExists)
		return 0;

	if (textExists != transExists)
	{
		std::wcerr << L"原文文件和翻译文件未成对出现：" << textPath << L" / " << transPath << std::endl;
		Logger::Instance().Error("Parallel text pair is incomplete. source=" + Logger::ToUtf8(textPath) + ", target=" + Logger::ToUtf8(transPath));
		return -1;
	}

	Logger::Instance().Info("Loading parallel text pair. source=" + Logger::ToUtf8(textPath) + ", target=" + Logger::ToUtf8(transPath));

	std::ifstream oEye(textPath, std::ios::in | std::ios::binary);
	std::ifstream tEye(transPath, std::ios::in | std::ios::binary);
	std::string text;
	std::string trans;

	if (!PrepareTextStream(oEye) || !PrepareTextStream(tEye))
	{
		Logger::Instance().Error("Failed to prepare text streams. source=" + Logger::ToUtf8(textPath) + ", target=" + Logger::ToUtf8(transPath));
		return -1;
	}

	int i = 0;
	while (std::getline(oEye, text))
	{
		if (!std::getline(tEye, trans))
		{
			if (!ConfirmContinueOnLineCountMismatch(textPath, transPath, i, true))
				return -1;

			Logger::Instance().Warning("Stopped loading pair at first shorter target EOF. loadedLines=" + std::to_string(i));
			return i;
		}

		if (!text.empty() && text.back() == '\r')
			text.pop_back();
		if (!trans.empty() && trans.back() == '\r')
			trans.pop_back();

		textMapping[reinterpret_cast<const char8_t*>(text.c_str())] = reinterpret_cast<const char8_t*>(trans.c_str());
		++i;
	}

	if (std::getline(tEye, trans))
	{
		if (!ConfirmContinueOnLineCountMismatch(textPath, transPath, i, false))
			return -1;
	}

	Logger::Instance().Info("Loaded " + std::to_string(i) + " parallel text rows from source=" + Logger::ToUtf8(textPath) + ", target=" + Logger::ToUtf8(transPath));

	return i;
}

int TranslationDatabase::LoadText(int seq)
{
	const auto& progRoot = Config::Instance().GetProgRoot();
	std::filesystem::path textPath = progRoot / (std::string("text") + std::to_string(seq) + ".txt");
	std::filesystem::path transPath = progRoot / (std::string("text") + std::to_string(seq) + "_translated.txt");

	if (seq == 0)
	{
		textPath = progRoot / "text.txt";
		transPath = progRoot / "text_translated.txt";
	}

	const int loaded = LoadTextPair(textPath, transPath);
	Logger::Instance().Info("LoadText sequence " + std::to_string(seq) + " result=" + std::to_string(loaded));
	return loaded;
}

int TranslationDatabase::LoadSourceData()
{
	namespace fs = std::filesystem;
	const auto& progRoot = Config::Instance().GetProgRoot();
	fs::path srcRoot = progRoot / L"text" / L"src";
	fs::path tgtRoot = progRoot / L"text" / L"tgt";

	if (!fs::exists(srcRoot) || !fs::exists(tgtRoot))
	{
		Logger::Instance().Info("Source data folders were not found. src=" + Logger::ToUtf8(srcRoot) + ", tgt=" + Logger::ToUtf8(tgtRoot));
		return 0;
	}

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

	Logger::Instance().Info("Loaded " + std::to_string(total) + " rows from text/src -> text/tgt parallel text data.");
	return total;
}
