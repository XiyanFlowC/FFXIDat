#include "Application.h"
#include "Config.h"
#include "TranslationDatabase.h"
#include "BackupManager.h"
#include "ProcessorFactory.h"
#include "ChsToSJis.h"
#include "../FFXIDatProcessor/codepage.h"
#include <CsvFile.h>
#include <iostream>
#include <conio.h>
#include <xystring.h>

Application& Application::Instance()
{
    static Application instance;
    return instance;
}

// Use extern function from FFXITrans.cpp
int YesNoPrompt(const std::wstring& prompt)
{
	std::wcout << prompt << L" (Y/N): ";
    while (true)
    {
        int key = _getch();
        if (key == 'Y' || key == 'y')
            return 'Y';
        else if (key == 'N' || key == 'n')
            return 'N';
	}
}

void Application::ShowUsage()
{
    std::wcout << L"FFXI汉化插入工具 Ver." VERSION " by Hyururu\n"
        L"用法：FFXITrans [insitu]\n"
        L"  insitu：直接在游戏目录修改文件，否则输出到output目录\n"
        L"  prepare：输出要准备的游戏数据文件（翻译用）\n"
        L"  无参数则进入交互模式\n";
}

bool Application::InitializeCodePages()
{
    try
    {
        const auto& progRoot = Config::Instance().GetProgRoot();
        CodeCvt::GetInstance().Init(progRoot / L"cp932.csv");
    }
    catch (std::exception& ex)
    {
        std::wcerr << ex.what() << std::endl;
        std::wcerr << L"处理代码页cp932.csv失败了。" << std::endl;
        return false;
    }

    try
    {
        const auto& progRoot = Config::Instance().GetProgRoot();
        ChsToSJis::Instance().Init(progRoot / L"chs2sjis.csv");
    }
    catch (std::exception& ex)
    {
        std::wcerr << ex.what() << std::endl;
        std::wcerr << L"处理简体汉字转换逻辑chs2sjis.csv失败了。" << std::endl;
        return false;
    }

    return true;
}

bool Application::LoadTranslations()
{
    auto& db = TranslationDatabase::Instance();

    // Load numbered text files
    for (int i = 0;; ++i)
    {
        int loaded = db.LoadText(i);
        if (loaded < 0)
        {
            std::wcerr << L"加载文本文件失败。" << std::endl;
            return false;
        }
        if (loaded == 0)
            break;
    }

    // Load source data
    int sourceCount = db.LoadSourceData();
    if (sourceCount < 0)
    {
        std::wcerr << L"加载 text\\src / text\\tgt 结构失败。" << std::endl;
        return false;
    }

    if (sourceCount > 0)
    {
        std::wcout << L"从 text\\src / text\\tgt 额外覆盖了 " << sourceCount << L" 条文本数据。" << std::endl;
    }

    std::wcout << L"共读取了 " << std::to_wstring(db.GetTranslationCount()) << L" 条文本数据。" << std::endl;
    return true;
}

bool Application::Initialize()
{
    setlocale(LC_ALL, "");

    std::wcout << L"FFXI汉化插入工具 Ver." VERSION " by Hyururu" << std::endl;

    // Initialize configuration
    if (!Config::Instance().Initialize())
    {
        return false;
    }

    // Load config file if exists
    const auto& progRoot = Config::Instance().GetProgRoot();
    if (std::filesystem::exists(progRoot / "config.ini"))
    {
        Config::Instance().LoadFromFile(progRoot / "config.ini");
    }

    // Initialize code pages
    if (!InitializeCodePages())
    {
        return false;
    }

    // Initialize mismatch log
    TranslationDatabase::Instance().InitializeMismatchLog(progRoot / "text_mismatch.txt");

    return true;
}

std::vector<FileProcessDef> Application::LoadFileDefinitions()
{
    const auto& progRoot = Config::Instance().GetProgRoot();
    CsvFile def(progRoot / "defs.csv", std::ios::in | std::ios::binary);
    std::vector<FileProcessDef> fileDefs;

    int excludedCount = 0;

    while (!def.IsEof())
    {
        FileProcessDef fileDef;
        fileDef.path = def.NextCell();
        fileDef.type = def.NextCell();
        fileDef.lang = def.NextCell();
        fileDef.comment = def.NextCell();
        if (!def.IsEol())
        {
            fileDef.cellIndicesStr = def.NextCell();
        }
        def.NextLine();

        if (fileDef.path.empty() || fileDef.type.empty() || fileDef.lang.empty() || fileDef.comment.empty())
            continue;

        // Check if this definition should be excluded
        if (Config::Instance().IsExcluded(fileDef.comment))
        {
            if (Config::Instance().IsVerbose())
            {
                std::wcout << L"跳过已排除项：" << xybase::string::to_wstring(fileDef.comment) << std::endl;
            }
            excludedCount++;
            continue;
        }

        fileDefs.push_back(fileDef);
    }

    if (excludedCount > 0)
    {
        std::wcout << L"根据配置已排除 " << excludedCount << L" 项文件定义。" << std::endl;
    }

    return fileDefs;
}

int Application::Run(int argc, char** argv)
{
    try
    {
        // Parse command line
        bool inSitu = false;
        if (argc > 1)
        {
            if (argc != 2)
            {
                std::wcerr << L"参数错误。\n";
                return -1;
            }

            std::string cmd{ argv[1] };
            if (cmd == "prepare")
            {
                if (!Config::Instance().Initialize())
                {
                    system("pause");
                    return -1;
                }
                int ret = PrepareSourceData();
                if (ret != 0)
                {
                    std::wcerr << L"prepare 执行失败。" << std::endl;
                    system("pause");
                    return ret;
                }
                std::wcout << L"prepare 执行成功。" << std::endl;
                return 0;
            }
            else if (cmd == "insitu")
            {
                inSitu = true;
                Config::Instance().SetInSituMode(true);
            }
            else
            {
                ShowUsage();
                return 0;
            }
        }

        // Initialize application
        if (!Initialize())
        {
            system("pause");
            return -1;
        }

        // Load translations
        if (!LoadTranslations())
        {
            system("pause");
            return -4;
        }

        // Handle backup
        BackupManager::Instance().PromptAndRestore();

        // Determine output mode
        bool overwrite;
        if (inSitu)
        {
            overwrite = true;
        }
        else
        {
            overwrite = Config::Instance().IsInSituNoPrompt() ? false : (YesNoPrompt(L"要在原位修改游戏文件吗？") == 'Y');
        }

        if (overwrite)
        {
            Config::Instance().SetInSituMode(true); // Update config for processors
            std::wcout << L"将在原位修改游戏文件。文件修改前将被备份。" << std::endl;
        }

        std::wcout << L"开始处理文件，请勿关闭程序。" << std::endl;

        // Process translations
        int ret = ProcessTranslations();

        // Close mismatch log
        TranslationDatabase::Instance().CloseMismatchLog();

        std::wcout << L"处理完毕。" << std::endl;
        std::wcout << L"共有 " << std::to_wstring(TranslationDatabase::Instance().GetMismatchCount())
            << L" 条文本失配。失配文本已经保存到 text_mismatch.txt 中。" << std::endl;

        system("pause");
        return ret;
    }
    catch (std::exception& ex)
    {
        TranslationDatabase::Instance().CloseMismatchLog();
        std::wcerr << L"发生了意外错误。" << std::endl;
        std::wcerr << xybase::string::sys_mbs_to_wcs(ex.what()) << std::endl;
        system("pause");
        return -1;
    }
}

int Application::ProcessTranslations()
{
    auto fileDefs = LoadFileDefinitions();
    std::map<std::u8string, FileProcessDef> jpDefsByComment;

    // Build JP definition mapping
    for (const auto& def : fileDefs)
    {
        if (def.lang == u8"jp")
            jpDefsByComment[def.comment] = def;
    }

    // Determine output mode
    bool overwrite = Config::Instance().IsInSituMode();
    const auto& gameRoot = Config::Instance().GetGameRoot();
    const auto& outRoot = overwrite ? gameRoot : Config::Instance().GetOutRoot();

    int fileCounter = 0;
    int totalFiles = 0;

    // Count files to process
    for (const auto& def : fileDefs)
    {
        if (Config::Instance().IsEnglishMode())
        {
            if (def.lang == u8"en") totalFiles++;
        }
        else
        {
            if (def.lang == u8"jp") totalFiles++;
        }
    }

    for (const auto& fileDef : fileDefs)
    {
        // Filter by language
        if (Config::Instance().IsEnglishMode())
        {
            if (fileDef.lang != u8"en")
                continue;
        }
        else
        {
            if (fileDef.lang != u8"jp")
                continue;
        }

        fileCounter++;

        // Display progress
        wchar_t progress[128];
        swprintf_s(progress, L"[%d/%d] %d%%", fileCounter, totalFiles, fileCounter * 100 / totalFiles);
        std::wcout << L"\r处理中：" << progress << L" "
            << xybase::string::to_wstring(fileDef.comment) << L"          ";

        // Prepare paths
        std::filesystem::path datPath = gameRoot / (fileDef.path + u8".DAT");
        std::filesystem::path outputPath = overwrite ? datPath : outRoot / (fileDef.path + u8".DAT");

        if (!std::filesystem::exists(datPath))
            continue;

        // Create output directory if needed
        if (!std::filesystem::exists(outputPath.parent_path()))
        {
            std::filesystem::create_directories(outputPath.parent_path());
        }

        // Backup if overwriting
        if (overwrite)
        {
            BackupManager::Instance().BackupGameFile(fileDef.path + u8".DAT");
        }

        // Try ejref_tolerance special processor first
        bool processed = false;
        auto ejrefProcessor = ProcessorFactory::Instance().GetEjrefToleranceProcessor();
        if (ejrefProcessor)
        {
            try
            {
                processed = ejrefProcessor->Process(fileDef, datPath, outputPath, jpDefsByComment);
            }
            catch (const std::exception& ex)
            {
                // Continue to regular processor if ejref fails
                if (Config::Instance().IsVerbose())
                {
                    std::wcerr << L"\nEjref处理器失败，回退到常规处理器：" 
                        << xybase::string::sys_mbs_to_wcs(ex.what()) << std::endl;
                }
            }
        }

        // If not processed by ejref, use regular processor
        if (!processed)
        {
            auto processor = ProcessorFactory::Instance().GetProcessor(fileDef.type);
            if (processor)
            {
                try
                {
                    processor->Process(fileDef, datPath, outputPath, jpDefsByComment);
                }
                catch (const std::exception& ex)
                {
                    std::wcerr << L"\n处理失败：" << xybase::string::to_wstring(fileDef.path)
                        << L" - " << xybase::string::sys_mbs_to_wcs(ex.what()) << std::endl;
                }
            }
            else
            {
                if (Config::Instance().IsVerbose())
                {
                    std::wcerr << L"\n未找到处理器：" << xybase::string::to_wstring(fileDef.type)
                        << L" [" << xybase::string::to_wstring(fileDef.comment) << L"]" << std::endl;
                }
            }
        }
    }

    std::wcout << std::endl;
    return 0;
}

int Application::PrepareSourceData()
{
    // TODO: Move prepare logic from original file
    std::wcout << L"注意：Prepare 功能正在重构中，请使用原始 FFXITrans.cpp" << std::endl;
    return -1;
}
