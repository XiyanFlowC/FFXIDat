// FFXIDatProcessor.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <clocale>
#include <format>
#include <exception>

#include "DMsg.h"

#include "xystring.h"
#include "DataManager.h"
#include "FFXIDatProcessor.h"
#include "codepage.h"
#include "XiString.h"

#include "liteopt.h"

#include "SQLiteDataSource.h"

#include "EventStringBase.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#undef GetMessage
void CsvToEventStringData(const char *src, const char *out);

int cfg_block = 0, cfg_xor = 0;

int help(const char *para)
{
    std::wcout << L"FFXI 数据文件处理器" << std::endl
        << std::endl;
    for (int i = 0; i < 32; ++i)
    {
        if (!LOPT_FLG_CHK(_reged_opt[i].flg, LOPT_FLG_DESC_VLD)) continue;
        if (LOPT_FLG_CHK(_reged_opt[i].flg, LOPT_FLG_CH_VLD))
            std::wcout << std::format(L"-{}  ", (wchar_t)_reged_opt[i].ch_opt);
        else
            std::wcout << L"    ";
        if (LOPT_FLG_CHK(_reged_opt[i].flg, LOPT_FLG_STR_VLD))
            std::wcout << std::format(L"--{:16} ", xybase::string::to_wstring(_reged_opt[i].long_opt));
        else
            std::wcout << L"                  ";

        if (LOPT_FLG_CHK(_reged_opt[i].flg, LOPT_FLG_VAL_NEED))
            std::wcout << L"* ";
        else
            std::wcout << L"  ";

        auto ch = (wchar_t *)_reged_opt[i].desc;
        int cur = 0;
        int lfc = 24;
        while (*ch)
        {
            if (*ch == L'\n')
            {
                cur = 0;
                if (*++ch)
                    std::wcout << L"\n                             ";
                else
                    std::wcout << std::endl;
                continue;
            }
            std::wcout << *ch++;
            if (cur++ > lfc)
            {
                cur = 0;
                if (*ch)
                    std::wcout << L"\n                             ";
                else
                    std::wcout << std::endl;
            }
        }
        if (cur) std::wcout << L"\n";
    }
    std::wcout << L"标记了 * 的项目，表示该开关需要后随一个参数。" << std::endl;
    return 0;
}

int main(int argc, const char **argv)
{
    // setlocale(LC_ALL, "ja_JP");
    setlocale(LC_ALL, "");

    PathUtil::Init();

    try
    {
        CodeCvt::GetInstance().Init(PathUtil::progRootPath + L"/cp932.csv");
    }
    catch (std::exception &ex)
    {
        std::wcerr << ex.what() << std::endl;
        std::wcerr << L"处理代码页cp932.csv失败了。" << std::endl;
        return -2;
    }

    lopt_regopt("sqlite-init", 0, LOPT_FLG_VAL_NEED, [](const char *str)->int {
        std::filesystem::remove(PathUtil::progRootPath + L"/text.db");
        SQLiteDataSource ds;
        ds.Initialise();
        CsvFile def(str, std::ios_base::in | std::ios_base::binary);
        ds.InitialiseFileDefinition(def);
        return 0;
        }, L"用指定的定义初始化SQLite数据库。（已经存在的数据库会被删除）");
    lopt_regopt("sqlite-trans-dat", 'T', 0, [](const char *str)->int {
        SQLiteDataSource ds;
        ds.TransAndOut();
        return 0;
        }, L"按SQLite中的定义和翻译数据，试图翻译游戏Dat并输出。");
    lopt_regopt("sqlite-text-purge", 0, 0, [](const char *str)->int {
        SQLiteDataSource ds;
        ds.Purge();
        return 0;
        }, L"清除SQLite数据库中的无引用文本。");
    lopt_regopt("sqlite-trans-dump", 0, 0, [](const char *str)->int {
        SQLiteDataSource ds;
        ds.DumpTranslationData();
        return 0;
        }, L"导出SQLite数据库中的原文和翻译数据到文本文件。");
    lopt_regopt("sqlite-trans-import", 0, 0, [](const char *str)->int {
        SQLiteDataSource ds;
        ds.ImportTranslation();
        return 0;
        }, L"导入文本文件中的翻译数据到SQLite数据库中。");
    lopt_regopt("sqlite-trans-dump-empty", 0, 0, [](const char *str)->int {
        SQLiteDataSource ds;
        ds.ExportNoTranslation();
        return 0;
        }, L"导出SQLite数据库中没有翻译的数据到文本文件。");
    lopt_regopt("sqlite-dat-to-sql", 'q', LOPT_FLG_VAL_NEED, [](const char *str)->int {
        SQLiteDataSource ds;
        ds.DatToDatabase(str, nullptr, nullptr);
        return 0;
        }, L"从游戏安装目录抽取指定语言的文本，并存入SQLite数据库。");
    lopt_regopt("do-xor", 'x', 0, [](const char *str)->int {cfg_xor = 1; return 0; }, L"要求DMsg进行Xor保护。");
    lopt_regopt("block", 'b', 0, [](const char *str)->int {cfg_block = 1; return 0; }, L"要求DMsg以块形式保存。。");
    lopt_regopt("dmsg-to-csv", 'm', LOPT_FLG_VAL_NEED, [](const char *str) -> int {
        std::string path(str);
        DMsg f(str);
        try {
            f.Read();
            f.ToCsv(path.replace(path.find(".DAT"), 4, ".csv"));
        }
        catch (std::exception &ex) {
            std::wcerr << L"发生错误：" << ex.what() << std::endl;
        }
        return 0;
        }, L"转换一个D_Msg文件到CSV。");
    lopt_regopt("csv-to-dmsg", 'M', LOPT_FLG_VAL_NEED, [](const char *str) -> int {CsvToDMsg(str); return 0; }, L"转换一个CSV文件到D_Msg。");
    lopt_regopt("xis-to-csv", 's', LOPT_FLG_VAL_NEED, [](const char *str) -> int {
        std::string path(str);
        XiString f(str);
        try {
            f.Read();
            f.ToCsv(path.replace(path.find(".DAT"), 4, ".csv"));
        }
        catch (std::exception &ex) {
            std::wcerr << L"发生错误：" << ex.what() << std::endl;
        }
        return 0;
        }, L"转换一个XiString文件到CSV。");
    lopt_regopt("csv-to-xis", 'S', LOPT_FLG_VAL_NEED, [](const char *str) -> int {CsvToXiString(str); return 0; }, L"转换一个CSV文件到XiString。");
    lopt_regopt("install-path", 'I', LOPT_FLG_VAL_NEED, [](const char *str) -> int {
        PathUtil::gameRootPath = xybase::string::sys_mbs_to_wcs(str);
        return 0;
        }, L"指定游戏安装目录。应为第一个开关。");
    lopt_regopt("scan-extract", 'X', 0, [](const char *str)-> int {
        ExtractSysText();
        return 0;
        }, L"扫描并导出游戏目录。");
    lopt_regopt("help", '?', 0, help, L"显示本信息。");
    if (argc == 1) help(nullptr);

    if (argv[1][0] != '-')
    {
        // 智能多文件处理
        for (int i = 1; i < argc; ++i)
        {
            std::string path(argv[i]);
            if (path.ends_with(".dmsg.csv"))
            {
                CsvToDMsg(argv[i], path.substr(0, path.size() - 9).c_str());
            }
            if (path.ends_with(".xis.csv"))
            {
                CsvToXiString(argv[i], path.substr(0, path.size() - 8).c_str());
            }
            if (path.ends_with(".evsb.csv"))
            {
                CsvToEventStringData(argv[i], path.substr(0, path.size() - 9).c_str());
            }
            if (path.ends_with(".DAT"))
            {
                static char m[8];
                std::ifstream eye(path, std::ios::binary);
                auto size = std::filesystem::file_size(path);
                eye.read(m, 8);
                int flag = *((int32_t *)m);

                if (strcmp(m, "d_msg") == 0)
                {
                    std::wcout << "xistr p=" << path.c_str() << std::endl;
                    try
                    {
                        DMsg f(path);
                        f.Read();
                        f.ToCsv(path + ".dmsg.csv");
                    }
                    catch (std::exception &ex)
                    {
                        std::wcout << "Failed: " << ex.what() << std::endl;
                    }
                }
                else if (strcmp(m, "XISTRING") == 0)
                {
                    std::wcout << "xistr p=" << path.c_str() << std::endl;
                    try
                    {
                        XiString s(path);
                        s.Read();
                        s.ToCsv(path + ".xis.csv");
                    }
                    catch (std::exception &ex)
                    {
                        std::wcout << "Failed: " << ex.what() << std::endl;
                    }
                }
                else if ((flag & 0xFF000000) == 0x10000000)
                {
                    if ((flag & 0xFFFFFF) == size - 4)
                    {
                        std::wcout << "evsb p=" << path.c_str() << std::endl;
                        try
                        {
                            EventStringBase esb(path);
                            esb.Read();
                            esb.ToCsv(path + ".evsb.csv");
                        }
                        catch (std::exception &ex)
                        {
                            std::wcout << "Failed: " << ex.what() << std::endl;
                        }
                    }
                }
            }
        }
        return 0;
    }

    try
    {
        int ret = lopt_parse(argc, argv);
        if (ret)
        {
            if (ret < 0)
            {
                std::wcerr << std::format(L"{}: 语法有误或是不存在的开关。\n", xybase::string::to_wstring(argv[-ret]));
            }
            exit(ret);
        }
    }
    catch (std::exception &ex)
    {
        std::wcout << ex.what() << std::endl;
    }

    lopt_finalize();

    /*DMsg t(DataManager::GetOutPathConf(1, 165, 64));
    t.FromCsv(DataManager::GetOutPathConf(1, 165, 64) + ".dmsg.csv");
    t.Write();*/

    // ExtractSysText();

    /*DMsg test(DataManager::GetPath(1, 0, 14));
    test.Read();
    test.ToCsv("test.csv");*/
    // read when launch:
    // 165 85 Area Name
    // 175 31 Equip Slot
    // 165 87 Job Name
    // 165 53/54 dmsg 附近 XISTRING 也有 系统文本很多
    // 175 32 
    //
    // 
    // 
    // 168 24 定型文辞书
    //
  /*  DMsg daijinaMono(DataManager::GetPath(1, 175, 34));
    daijinaMono.Read();
    daijinaMono.ToCsv("dm.csv");*/
}

void CsvToDMsg(const char *src, const char *out)
{
    std::string csvPath = src;
    if (!csvPath.ends_with(".csv"))
    {
        std::wcerr << L"指定的文件不是CSV。" << std::endl;
        return;
    }
    std::string datPath = csvPath;
    DMsg f(out ? out : datPath.replace(datPath.find(".csv"), 4, ".DAT"));
    try {
        f.FromCsv(csvPath);
        f.mode = cfg_block ? DMsg::Mode::Block : DMsg::Mode::Variable;
        f.obs = cfg_xor == 1;
        f.Write();
    }
    catch (xybase::RuntimeException &ex)
    {
        std::wcerr << L"发生异常：" << ex.GetMessage() << std::endl;
    }
    catch (std::exception &ex) {
        std::wcerr << L"发生错误：" << ex.what() << std::endl;
    }
}

void CsvToEventStringData(const char *src, const char *out)
{
    std::string csvPath = src;
    if (!csvPath.ends_with(".csv"))
    {
        std::wcerr << L"指定的文件不是CSV。" << std::endl;
        return;
    }
    std::string datPath = csvPath;
    EventStringBase f(out ? out : datPath.replace(datPath.find(".csv"), 4, ".DAT"));
    try {
        f.FromCsv(csvPath);
        f.Write();
    }
    catch (xybase::RuntimeException &ex)
    {
        std::wcerr << L"发生异常：" << ex.GetMessage() << std::endl;
    }
    catch (std::exception &ex) {
        std::wcerr << L"发生错误：" << ex.what() << std::endl;
    }
}

void CsvToXiString(const char *src, const char *out)
{
    std::string csvPath = src;
    if (!csvPath.ends_with(".csv"))
    {
        std::wcerr << L"指定的文件不是CSV。" << std::endl;
        return;
    }
    std::string datPath = csvPath;
    XiString f(out ? out : datPath.replace(datPath.find(".csv"), 4, ".DAT"));
    try {
        f.FromCsv(csvPath);
        f.Write();
    }
    catch (xybase::RuntimeException &ex)
    {
        std::wcerr << L"发生异常：" << ex.GetMessage() << std::endl;
    }
    catch (std::exception &ex) {
        std::wcerr << L"发生错误：" << ex.what() << std::endl;
    }
}

void ExtractSysText()
{
    for (int rom = 1; rom < 12; ++rom)
    {
        int cmax = rom == 1 ? 365 : 30;
        for (int c = 0; c < cmax; ++c)
        {
            for (int n = 0; n < 128; ++n)
            {
                std::filesystem::path p = PathUtil::GetPath(rom, c, n);
                if (std::filesystem::exists(p))
                {
                    auto size = std::filesystem::file_size(p);
                    std::ifstream eye(p, std::ios::in | std::ios::binary);
                    int flag;
                    eye.read((char *)&flag, 4);
                    if ((flag & 0xFF000000) == 0x10000000)
                    {
                        if ((flag & 0xFFFFFF) == size - 4)
                        {
                            std::wcout << "evsb p=" << p << std::endl;
                            try
                            {
                                EventStringBase esb(p);
                                esb.Read();
                                esb.ToCsv(PathUtil::GetOutPathConf(rom, c, n) + L".evsb.csv");
                            }
                            catch (std::exception &ex)
                            {
                                std::wcout << "Failed: " << ex.what() << std::endl;
                            }
                        }
                    }
                }
            }
        }

        if (rom == 1)
        for (int c = 97; c < 365; ++c)
        {
            for (int n = 0; n < 128; ++n)
            {
                std::filesystem::path p = PathUtil::GetPath(rom, c, n);
                if (std::filesystem::exists(p))
                {
                    static char m[8];
                    std::ifstream eye(p, std::ios::binary);
                    eye.read(m, 8);

                    if (strcmp(m, "d_msg") == 0)
                    {
                        std::wcout << "dmsg p=" << p << std::endl;
                        try
                        {
                            DMsg f(p);
                            f.Read();
                            f.ToCsv(PathUtil::GetOutPathConf(rom, c, n) + L".dmsg.csv");
                        }
                        catch (std::exception &ex)
                        {
                            std::wcout << "Failed: " << ex.what() << std::endl;
                        }
                    }
                    else if (strcmp(m, "XISTRING") == 0)
                    {
                        std::wcout << "xistr p=" << p << std::endl;
                        try
                        {
                            XiString s(p);
                            s.Read();
                            s.ToCsv(PathUtil::GetOutPathConf(rom, c, n) + L".xis.csv");
                        }
                        catch (std::exception &ex)
                        {
                            std::wcout << "Failed: " << ex.what() << std::endl;
                        }
                    }
                }
            }
        }
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
