#include "TranslationDatabase.h"
#include "Config.h"
#include "ChsToSJis.h"
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
        return;
    }

    mismatchFile.open(logPath, std::ios::out | std::ios::binary);
    if (!mismatchFile.is_open())
    {
        std::wcerr << L"无法创建失配文本文件：" << logPath << std::endl;
    }
}

void TranslationDatabase::CloseMismatchLog()
{
    if (mismatchFile.is_open())
    {
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
}

std::u8string TranslationDatabase::GetTranslation(const std::u8string& text)
{
    auto itr = textMapping.find(text);
    if (itr == textMapping.end())
    {
        RecordMismatch(text);
        return text;
    }

    std::u8string translation = itr->second;
    translation = ChsToSJis::Instance().ReplaceHanzi(translation);
    return translation;
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

    std::u8string translation = itr->second;
    translation = ChsToSJis::Instance().ReplaceHanzi(translation);
    return translation;
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
    translation = ChsToSJis::Instance().ReplaceHanzi(translation);
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
        return -1;
    }

    if (Config::Instance().IsVerbose())
    {
        std::wcout << L"读取：" << textPath << L" -=- " << transPath << std::endl;
    }

    std::ifstream oEye(textPath, std::ios::in | std::ios::binary);
    std::ifstream tEye(transPath, std::ios::in | std::ios::binary);
    std::string text;
    std::string trans;

    if (!PrepareTextStream(oEye) || !PrepareTextStream(tEye))
        return -1;

    int i = 0;
    while (std::getline(oEye, text))
    {
        if (!std::getline(tEye, trans))
        {
            std::wcerr << L"翻译文件和原文文件的行数不一致。\n";
            return i;
        }

        if (!text.empty() && text.back() == '\r')
            text.pop_back();
        if (!trans.empty() && trans.back() == '\r')
            trans.pop_back();

        textMapping[reinterpret_cast<const char8_t*>(text.c_str())] = reinterpret_cast<const char8_t*>(trans.c_str());
        ++i;
    }

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

    return LoadTextPair(textPath, transPath);
}

int TranslationDatabase::LoadSourceData()
{
    namespace fs = std::filesystem;
    const auto& progRoot = Config::Instance().GetProgRoot();
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
