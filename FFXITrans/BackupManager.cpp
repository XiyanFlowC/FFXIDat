#include "BackupManager.h"
#include "BackupManager.h"
#include "Config.h"
#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <conio.h>
#include <xystring.h>

namespace fs = std::filesystem;

namespace
{
    constexpr wchar_t kMetadataDirectoryName[] = L".ffxitrans_crc";

    const std::array<std::uint32_t, 256>& GetCrc32Table()
    {
        static const std::array<std::uint32_t, 256> table = []
        {
            std::array<std::uint32_t, 256> values{};
            for (std::uint32_t i = 0; i < values.size(); ++i)
            {
                std::uint32_t crc = i;
                for (int bit = 0; bit < 8; ++bit)
                {
                    crc = (crc & 1U) ? (0xEDB88320U ^ (crc >> 1U)) : (crc >> 1U);
                }
                values[i] = crc;
            }
            return values;
        }();

        return table;
    }

    std::uint32_t ComputeFileCrc32(const fs::path& path)
    {
        std::ifstream input(path, std::ios::binary);
        if (!input.is_open())
        {
            throw std::runtime_error("failed to open file for CRC calculation");
        }

        const auto& table = GetCrc32Table();
        std::uint32_t crc = 0xFFFFFFFFU;
        std::array<char, 8192> buffer{};

        while (input)
        {
            input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
            const auto bytesRead = input.gcount();
            for (std::streamsize i = 0; i < bytesRead; ++i)
            {
                const auto byte = static_cast<std::uint8_t>(buffer[static_cast<size_t>(i)]);
                crc = table[(crc ^ byte) & 0xFFU] ^ (crc >> 8U);
            }
        }

        return crc ^ 0xFFFFFFFFU;
    }

    std::string FormatCrc(std::uint32_t crc)
    {
        std::ostringstream stream;
        stream << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << crc;
        return stream.str();
    }

    void WriteOriginalCrc(const fs::path& crcPath, std::uint32_t crc)
    {
        if (!fs::exists(crcPath.parent_path()))
        {
            fs::create_directories(crcPath.parent_path());
        }

        std::ofstream output(crcPath, std::ios::out | std::ios::trunc | std::ios::binary);
        if (!output.is_open())
        {
            throw std::runtime_error("failed to create CRC metadata file");
        }

        const auto formatted = FormatCrc(crc);
        output << formatted;
    }

    std::uint32_t ReadOriginalCrc(const fs::path& crcPath)
    {
        std::ifstream input(crcPath, std::ios::in | std::ios::binary);
        if (!input.is_open())
        {
            throw std::runtime_error("failed to open CRC metadata file");
        }

        std::string value;
        input >> value;
        if (value.empty())
        {
            throw std::runtime_error("CRC metadata file is empty");
        }

        size_t parsedLength = 0;
        const auto crc = static_cast<std::uint32_t>(std::stoul(value, &parsedLength, 16));
        if (parsedLength != value.size())
        {
            throw std::runtime_error("CRC metadata file contains invalid data");
        }

        return crc;
    }

    [[noreturn]] void ThrowBackupMismatch(const fs::path& relativePath, std::uint32_t expectedCrc, std::uint32_t actualCrc)
    {
        std::wcerr << L"\n警告：检测到 backup 中记录的原始文件 CRC 与当前游戏文件不一致："
            << relativePath.wstring() << std::endl;
        std::wcerr << L"备份记录 CRC: 0x" << xybase::string::sys_mbs_to_wcs(FormatCrc(expectedCrc))
            << L"，当前文件 CRC: 0x" << xybase::string::sys_mbs_to_wcs(FormatCrc(actualCrc)) << std::endl;
        std::wcerr << L"这通常表示当前游戏文件已经被修改，继续运行可能进一步破坏游戏数据。程序将立即停止。" << std::endl;
        std::wcerr << L"如果游戏已经更新，请先删除 backup 目录后再重新运行。" << std::endl;
        throw std::runtime_error("backup CRC mismatch");
    }

    [[noreturn]] void ThrowBackupMetadataMissing(const fs::path& relativePath)
    {
        std::wcerr << L"\n警告：无法在 backup 中找到该文件对应的 CRC 记录："
            << relativePath.wstring() << std::endl;
        std::wcerr << L"该备份无法在恢复前验证完整性。为避免进一步破坏游戏数据，程序将立即停止。" << std::endl;
        std::wcerr << L"如果这是旧版本程序留下的 backup，或游戏已经更新，请先删除 backup 目录后再重新运行。" << std::endl;
        throw std::runtime_error("backup CRC metadata missing");
    }

    void ValidateBackupFile(const fs::path& relativePath, const fs::path& backupPath, bool allowCreateMissingMetadata)
    {
        fs::path crcPath = Config::Instance().GetProgRoot() / "backup" / kMetadataDirectoryName / relativePath;
        crcPath += L".crc32";
        if (!fs::exists(crcPath))
        {
            if (!allowCreateMissingMetadata)
            {
                ThrowBackupMetadataMissing(relativePath);
            }

            WriteOriginalCrc(crcPath, ComputeFileCrc32(backupPath));
        }

        const auto expectedCrc = ReadOriginalCrc(crcPath);
        const auto actualCrc = ComputeFileCrc32(backupPath);
        if (expectedCrc != actualCrc)
        {
            std::wcerr << L"\n警告：检测到 backup 中的备份文件 CRC 与记录不一致："
                << relativePath.wstring() << std::endl;
            std::wcerr << L"备份记录 CRC: 0x" << xybase::string::sys_mbs_to_wcs(FormatCrc(expectedCrc))
                << L"，备份文件 CRC: 0x" << xybase::string::sys_mbs_to_wcs(FormatCrc(actualCrc)) << std::endl;
            std::wcerr << L"备份文件可能已损坏或被手动修改。为避免进一步破坏游戏数据，程序将立即停止。" << std::endl;
            std::wcerr << L"如果游戏已经更新，请先删除 backup 目录后再重新运行。" << std::endl;
            throw std::runtime_error("backup file CRC mismatch");
        }
    }
}

BackupManager& BackupManager::Instance()
{
    static BackupManager instance;
    return instance;
}

fs::path BackupManager::GetBackupRoot()
{
    return Config::Instance().GetProgRoot() / "backup";
}

fs::path BackupManager::GetMetadataRoot()
{
    return GetBackupRoot() / kMetadataDirectoryName;
}

fs::path BackupManager::GetBackupPath(const fs::path& relativePath)
{
    return GetBackupRoot() / relativePath;
}

fs::path BackupManager::GetCrcPath(const fs::path& relativePath)
{
    fs::path crcPath = GetMetadataRoot() / relativePath;
    crcPath += L".crc32";
    return crcPath;
}

bool BackupManager::BackupExists() const
{
    return fs::exists(GetBackupRoot());
}

void BackupManager::BackupGameFile(const fs::path& relativePath)
{
    const auto& config = Config::Instance();
    fs::path gamePath = config.GetGameRoot() / relativePath;
    fs::path backPath = GetBackupPath(relativePath);
    fs::path crcPath = GetCrcPath(relativePath);

    if (!fs::exists(gamePath))
        return;

    if (fs::exists(backPath))
    {
        ValidateBackupFile(relativePath, backPath, true);
        const auto expectedCrc = ReadOriginalCrc(crcPath);
        const auto actualCrc = ComputeFileCrc32(gamePath);
        if (expectedCrc != actualCrc)
        {
            ThrowBackupMismatch(relativePath, expectedCrc, actualCrc);
        }

        return;
    }

    if (!fs::exists(backPath.parent_path()))
        fs::create_directories(backPath.parent_path());

    const auto originalCrc = ComputeFileCrc32(gamePath);
    fs::copy(gamePath, backPath, fs::copy_options::none);
    WriteOriginalCrc(crcPath, originalCrc);
}

bool BackupManager::RestoreBackups()
{
    const auto& config = Config::Instance();
    fs::path backupRoot = GetBackupRoot();
    const auto& gameRoot = config.GetGameRoot();
    const auto metadataRoot = GetMetadataRoot();

    std::wcout << L"恢复备份中..." << std::endl;

    for (fs::recursive_directory_iterator iter(backupRoot), end; iter != end; ++iter)
    {
        const auto& entry = *iter;
        const auto entryPath = entry.path();

        if (entry.is_directory() && entryPath == metadataRoot)
        {
            iter.disable_recursion_pending();
            continue;
        }

        if (!entry.is_regular_file())
            continue;

        std::error_code ec;
        const auto relativePath = fs::relative(entryPath, backupRoot, ec);
        if (ec)
        {
            std::wcerr << L"恢复备份时发生了问题：" << ec.message().c_str() << std::endl;
            return false;
        }

        ValidateBackupFile(relativePath, entryPath, false);

        const auto outputPath = gameRoot / relativePath;
        if (!fs::exists(outputPath.parent_path()))
        {
            fs::create_directories(outputPath.parent_path(), ec);
            if (ec)
            {
                std::wcerr << L"恢复备份时发生了问题：" << ec.message().c_str() << std::endl;
                return false;
            }
        }

        fs::copy_file(entryPath, outputPath, fs::copy_options::overwrite_existing, ec);
        if (ec)
        {
            std::wcerr << L"恢复备份时发生了问题：" << ec.message().c_str() << std::endl;
            return false;
        }
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
