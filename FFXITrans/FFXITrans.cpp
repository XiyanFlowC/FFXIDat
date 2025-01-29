// FFXITrans.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <string>
#include <fstream>
#include <map>
#include <conio.h>
#include <exception>
#include <filesystem>
#include <xystring.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <CsvFile.h>

#include <XiString.h>
#include <DMsg.h>
#include <EventStringBase.h>

namespace fs = std::filesystem;

fs::path gameRoot, progRoot;

#include "../FFXIDatProcessor/codepage.h"
#include "ChsToSJis.h"

std::map<std::u8string, std::u8string> textMapping;

std::u8string GetTranslation(const std::u8string &text) {
    std::u8string translation;

    auto itr = textMapping.find(text);
    if (itr == textMapping.end())
    {
        std::wcout << L"\n文本：" << xybase::string::to_wstring(text) << L"失配了。" << std::endl;
        return text;
    }

    translation = itr->second;
    translation = ChsToSJis::Instance().ReplaceHanzi(translation);

    return translation;
}

int PathInit()
{
    HKEY hKey;
    const wchar_t *subKey = L"SOFTWARE\\WOW6432Node\\PlayOnline\\InstallFolder";
    const wchar_t *valueName = L"0001";
    wchar_t valueData[MAX_PATH];
    DWORD bufferSize = sizeof(valueData);

    DWORD valueType;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, subKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExW(hKey, valueName, nullptr, &valueType, reinterpret_cast<LPBYTE>(valueData), &bufferSize) == ERROR_SUCCESS) {
            if (valueType == REG_SZ) {
                gameRoot = valueData;
            }
            else
            {
                std::wcerr << L"未能正确读取注册表信息，请检查游戏安装信息。\n";
                return -1;
            }
        }
        else
        {
            std::wcerr << L"发现了POL的安装信息，但是没有找到游戏的。\n";
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
    return 0;
}

void LoadText()
{
    std::ifstream oEye(progRoot / "text.txt", std::ios::in | std::ios::binary),
        tEye(progRoot / "text_translated.txt", std::ios::in | std::ios::binary);
    std::string text;
    std::string trans;

    int i = 0;
    while (std::getline(oEye, text)) {
        if (!std::getline(tEye, trans))
        {
            std::wcerr << L"翻译文件和原文文件的行数不一致。\n";
            return;
        }
        textMapping[(char8_t *)text.c_str()] = (char8_t *)trans.c_str();
        ++i;
    }
    std::wcout << L"读取了" << i << L"条文本翻译数据。\n";
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

int main()
{
    setlocale(LC_ALL, "");
    try
    {
        std::wcout << L"FFXI汉化插入工具 Ver.0.1-alpha by Hyururu" << std::endl;
        if (PathInit())
        {
            system("pause");
            exit(-1);
        };
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
        ChsToSJis::Instance().Init(progRoot / L"chs2sjis.csv");

        LoadText();

        bool backupExist = false;
        if (fs::exists(progRoot / "backup")) {
            backupExist = true;
            int key = YesNoPrompt(L"发现了备份数据。您希望先恢复备份吗？");

            if (key == 'Y')
            {
                std::error_code ec;
                fs::copy(progRoot / "backup", gameRoot, fs::copy_options::overwrite_existing | fs::copy_options::recursive, ec);
                /*if ('Y' == YesNoPrompt(L"您希望删除备份吗？"))
                    fs::remove_all(progRoot / "backup");*/

                if (ec)
                {
                    std::wcerr << L"恢复备份时发生了问题：" << ec.message().c_str();
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

        std::wcout << L"可以在原位修改游戏文件，或将翻译后的数据输出到 output。" << std::endl;
        std::wcout << L"如果不希望使用插件，请在原位修改。" << std::endl;
        bool overwrite;
        
        overwrite = YesNoPrompt(L"要在原位修改游戏文件吗？") == 'Y';

        if (overwrite)
        {
            std::wcout << L"将在原位修改游戏文件。文件修改前将被备份。" << std::endl;
        }
        std::wcout << L"开始处理文件，请勿关闭程序。" << std::endl;

        CsvFile def(progRoot / "defs.csv", std::ios::in | std::ios::binary);
        while (!def.IsEof())
        { 
            std::u8string path = def.NextCell();
            std::u8string type = def.NextCell();
            std::u8string lang = def.NextCell();
            std::u8string comm = def.NextCell();
            std::wcout << L"处理中：文件 "
                << xybase::string::to_wstring(path)
                << L"(" << xybase::string::to_wstring(type)
                << L") [" << xybase::string::to_wstring(comm) << L"]\r";
            def.NextLine();

            fs::path relaPath = path + u8".DAT";
            fs::path datPath = gameRoot / relaPath;
            fs::path outPath = overwrite ? datPath : "output" / relaPath;
            if (!fs::exists(datPath)) continue;
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
                for (auto &str : evsb)
                {
                    auto res = GetTranslation(str);
                    str = res;
                }
                evsb.path = outPath;
                evsb.Write();
            }
            else if (type == u8"dmsg")
            {
                DMsg dmsg(datPath);
                dmsg.Read();
                int rowNum = 1;
                for (auto &row : dmsg)
                {
                    int colNum = 1;
                    for (auto &cell : row)
                    {
                        if (cell.GetType() == 0) // str
                        {
                            std::u8string text = xybase::string::escape(cell.Get<std::u8string>());

                            cell.Set(xybase::string::unescape(GetTranslation(text)));
                        }
                        ++colNum;
                    }
                    ++rowNum;
                }
                dmsg.path = outPath;
                dmsg.Write();
            }
        }
        std::wcout << L"处理完毕。" << std::endl;
        system("pause");
    }
    catch (std::exception &ex)
    {
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
