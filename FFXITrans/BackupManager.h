#pragma once
#include <filesystem>

class BackupManager
{
public:
    static BackupManager& Instance();

    // Check if backup exists
    bool BackupExists() const;

    // Backup a single game file
    void BackupGameFile(const std::filesystem::path& relativePath);

    // Restore all backups
    bool RestoreBackups();

    // Prompt user for backup operations
    bool PromptAndRestore();

private:
    BackupManager() = default;
    BackupManager(const BackupManager&) = delete;
    BackupManager& operator=(const BackupManager&) = delete;

    static std::filesystem::path GetBackupRoot();
    static std::filesystem::path GetMetadataRoot();
    static std::filesystem::path GetBackupPath(const std::filesystem::path& relativePath);
    static std::filesystem::path GetCrcPath(const std::filesystem::path& relativePath);
};
