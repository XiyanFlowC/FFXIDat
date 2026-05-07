#include "BackupManager.h"
#include "Config.h"
#include <iostream>
#include <conio.h>
#include <xystring.h>

namespace fs = std::filesystem;

BackupManager& BackupManager::Instance()
{
    static BackupManager instance;
    return instance;
}

bool BackupManager::BackupExists() const
{
    const auto& progRoot = Config::Instance().GetProgRoot();
    return fs::exists(progRoot / "backup");
}

void BackupManager::BackupGameFile(const fs::path& relativePath)
{
    const auto& config = Config::Instance();
    fs::path gamePath = config.GetGameRoot() / relativePath;
    fs::path backPath = config.GetProgRoot() / "backup" / relativePath;

    if (!fs::exists(backPath.parent_path()))
        fs::create_directories(backPath.parent_path());

    fs::copy(gamePath, backPath, fs::copy_options::skip_existing);
}

bool BackupManager::RestoreBackups()
{
    const auto& config = Config::Instance();
    fs::path backupRoot = config.GetProgRoot() / "backup";
    const auto& gameRoot = config.GetGameRoot();

    std::wcout << L"恢复备份中..." << std::endl;
    std::error_code ec;
    fs::copy(backupRoot, gameRoot, fs::copy_options::overwrite_existing | fs::copy_options::recursive, ec);

    if (ec)
    {
        std::wcerr << L"恢复备份时发生了问题：" << ec.message().c_str() << std::endl;
        return false;
    }

    std::wcout << L"备份的恢复完成了。" << std::endl;
    return true;
}

bool BackupManager::PromptAndRestore()
{
    if (!BackupExists())
        return false;

    const auto& config = Config::Instance();

    if (config.IsInSituMode())
    {
        return RestoreBackups();
    }
    else
    {
        // Use extern function from FFXITrans.cpp
        extern int YesNoPrompt(const std::wstring& prompt);
        int key = YesNoPrompt(L"发现了备份数据。您希望先恢复备份吗？");
        if (key == 'Y')
        {
            if (!RestoreBackups())
                return false;

            if (YesNoPrompt(L"要退出程序吗？") == 'Y')
            {
                exit(0);
            }
        }
    }

    return true;
}
