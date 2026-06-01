#include "BackupManager.h"
#include "Config.h"
#include "Logger.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <conio.h>
#include <xystring.h>

namespace fs = std::filesystem;

namespace
{
	constexpr wchar_t kMetadataDirectoryName[] = L".ffxitrans_crc";
	constexpr wchar_t kBackupInfoFileName[] = L"backup.info";
 constexpr wchar_t kCrcIndexFileName[] = L"crc_index.txt";
	constexpr int kBackupInfoFormatVersion = 3;
	using CrcIndex = std::map<std::string, std::uint32_t>;

	bool IsInsideMetadataRoot(const fs::path& path, const fs::path& metadataRoot)
	{
		std::error_code ec;
		const auto relativePath = fs::relative(path, metadataRoot, ec);
	  return !ec && !relativePath.empty() && *relativePath.begin() != fs::path(L"..");
	}

	bool BackupRootContainsBackupFiles(const fs::path& backupRoot, const fs::path& metadataRoot)
	{
		if (!fs::exists(backupRoot))
		{
			return false;
		}

		for (fs::recursive_directory_iterator iter(backupRoot), end; iter != end; ++iter)
		{
			const auto& entry = *iter;
			const auto entryPath = entry.path();

			if (entry.is_directory() && entryPath == metadataRoot)
			{
				iter.disable_recursion_pending();
				continue;
			}

			if (entry.is_regular_file() && !IsInsideMetadataRoot(entryPath, metadataRoot))
			{
				return true;
			}
		}

		return false;
	}

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

	std::string MakeRelativePathKey(const fs::path& relativePath)
	{
		return Logger::ToUtf8(relativePath.generic_wstring());
	}

   CrcIndex LoadCrcIndex(const fs::path& indexPath)
	{
		CrcIndex index;
		if (!fs::exists(indexPath))
		{
			return index;
		}

		std::ifstream input(indexPath, std::ios::in | std::ios::binary);
		if (!input.is_open())
		{
			throw std::runtime_error("failed to open CRC index file");
		}

		std::string line;
		while (std::getline(input, line))
		{
			const auto separator = line.find('\t');
			if (separator == std::string::npos)
			{
				continue;
			}

			const auto key = line.substr(0, separator);
			const auto value = line.substr(separator + 1);
			if (key.empty() || value.empty())
			{
				continue;
			}

			size_t parsedLength = 0;
			const auto crc = static_cast<std::uint32_t>(std::stoul(value, &parsedLength, 16));
			if (parsedLength != value.size())
			{
				throw std::runtime_error("CRC index file contains invalid data");
			}

			index[key] = crc;
		}

		return index;
	}

	void WriteCrcIndex(const fs::path& indexPath, const CrcIndex& index)
	{
     if (!fs::exists(indexPath.parent_path()))
		{
          fs::create_directories(indexPath.parent_path());
		}

      std::ofstream output(indexPath, std::ios::out | std::ios::trunc | std::ios::binary);
		if (!output.is_open())
		{
         throw std::runtime_error("failed to create CRC index file");
		}

      for (const auto& [key, crc] : index)
		{
			output << key << '\t' << FormatCrc(crc) << '\n';
		}
	}

  void WriteOriginalCrc(const fs::path& indexPath, const fs::path& relativePath, std::uint32_t crc)
	{
      auto index = LoadCrcIndex(indexPath);
		index[MakeRelativePathKey(relativePath)] = crc;
		WriteCrcIndex(indexPath, index);
	}

  std::uint32_t ReadOriginalCrc(const fs::path& indexPath, const fs::path& relativePath)
	{
		const auto index = LoadCrcIndex(indexPath);
		const auto it = index.find(MakeRelativePathKey(relativePath));
		if (it == index.end())
		{
         throw std::runtime_error("CRC entry is missing from index file");
		}

        return it->second;
	}

	bool HasLegacyCrcFiles(const fs::path& metadataRoot)
	{
		if (!fs::exists(metadataRoot))
		{
            return false;
		}

     for (fs::recursive_directory_iterator iter(metadataRoot), end; iter != end; ++iter)
		{
			if (iter->is_regular_file() && iter->path().extension() == L".crc32")
			{
				return true;
			}
		}

		return false;
	}

	std::uint32_t ReadLegacyOriginalCrc(const fs::path& crcPath)
	{
		std::ifstream input(crcPath, std::ios::in | std::ios::binary);
		if (!input.is_open())
		{
			throw std::runtime_error("failed to open legacy CRC metadata file");
		}

		std::string value;
		input >> value;
		if (value.empty())
		{
			throw std::runtime_error("legacy CRC metadata file is empty");
		}

		size_t parsedLength = 0;
		const auto crc = static_cast<std::uint32_t>(std::stoul(value, &parsedLength, 16));
		if (parsedLength != value.size())
		{
			throw std::runtime_error("legacy CRC metadata file contains invalid data");
		}

		return crc;
	}

	void RemoveEmptyMetadataDirectories(const fs::path& metadataRoot)
	{
		std::vector<fs::path> directories;
		for (fs::recursive_directory_iterator iter(metadataRoot), end; iter != end; ++iter)
		{
			if (iter->is_directory())
			{
				directories.push_back(iter->path());
			}
		}

		std::sort(directories.begin(), directories.end(), [](const fs::path& left, const fs::path& right)
			{
				return left.native().size() > right.native().size();
			});

		for (const auto& directory : directories)
		{
			std::error_code ec;
			fs::remove(directory, ec);
		}
	}

	bool TryMigrateLegacyCrcFiles(const fs::path& metadataRoot, const fs::path& indexPath)
	{
		if (!HasLegacyCrcFiles(metadataRoot))
		{
			return false;
		}

		CrcIndex index;
     std::vector<fs::path> legacyFiles;
		for (fs::recursive_directory_iterator iter(metadataRoot), end; iter != end; ++iter)
		{
			if (!iter->is_regular_file() || iter->path().extension() != L".crc32")
			{
				continue;
			}

           legacyFiles.push_back(iter->path());

			auto relativePath = fs::relative(iter->path(), metadataRoot);
			relativePath.replace_extension();
			index[MakeRelativePathKey(relativePath)] = ReadLegacyOriginalCrc(iter->path());
		}

		WriteCrcIndex(indexPath, index);

     for (const auto& legacyFile : legacyFiles)
		{
           std::error_code ec;
			fs::remove(legacyFile, ec);
		}

		RemoveEmptyMetadataDirectories(metadataRoot);
		Logger::Instance().Info("Migrated legacy per-file CRC metadata to consolidated CRC index.");
		return true;
	}

	void WriteBackupInfo(const fs::path& infoPath)
	{
		if (!fs::exists(infoPath.parent_path()))
		{
			fs::create_directories(infoPath.parent_path());
		}

		std::ofstream output(infoPath, std::ios::out | std::ios::trunc | std::ios::binary);
		if (!output.is_open())
		{
			throw std::runtime_error("failed to create backup info file");
		}

		output << "format=" << kBackupInfoFormatVersion << "\n";
		output << "game_root=" << Logger::ToUtf8(Config::Instance().GetGameRoot()) << "\n";
	}

	std::string ReadBackupInfoValue(const fs::path& infoPath, const std::string& key)
	{
		std::ifstream input(infoPath, std::ios::in | std::ios::binary);
		if (!input.is_open())
		{
			throw std::runtime_error("failed to open backup info file");
		}

		std::string line;
		while (std::getline(input, line))
		{
			const auto separator = line.find('=');
			if (separator == std::string::npos)
			{
				continue;
			}

			if (line.substr(0, separator) == key)
			{
				return line.substr(separator + 1);
			}
		}

		return {};
	}

	[[noreturn]] void ThrowUntrustedBackupDirectory()
	{
		std::wcerr << L"\n危险：检测到现有 backup 目录不是由当前安全版本自动创建的可信备份。" << std::endl;
		std::wcerr << L"这通常意味着它来自旧版本程序，或被手动复制/修改过。" << std::endl;
		std::wcerr << L"为避免把已修改的 DAT 当成“原始备份”，程序将立即停止。" << std::endl;
		std::wcerr << L"请先完整删除 backup 目录，并确认游戏目录内是当前版本的原始文件后再重新运行。" << std::endl;
		std::wcerr << L"如果游戏刚更新过，旧 backup 已经全部失效，必须删除后重新创建。" << std::endl;
		throw std::runtime_error("untrusted backup directory");
	}

	[[noreturn]] void ThrowBackupInfoMismatch(const std::wstring& expectedGameRoot, const std::wstring& actualGameRoot)
	{
		std::wcerr << L"\n危险：backup 目录记录的游戏路径与当前配置不一致。" << std::endl;
		std::wcerr << L"backup 记录路径：" << expectedGameRoot << std::endl;
		std::wcerr << L"当前配置路径：" << actualGameRoot << std::endl;
		std::wcerr << L"为避免把别的安装目录或别的版本的文件当成原始备份，程序将立即停止。" << std::endl;
		std::wcerr << L"请删除 backup 目录，并确认当前 game_path 正确后重新运行。" << std::endl;
		throw std::runtime_error("backup info game root mismatch");
	}

	void EnsureTrustedBackupDirectory(bool allowCreateMetadata)
	{
		const auto backupRoot = Config::Instance().GetProgRoot() / "backup";
		const auto metadataRoot = backupRoot / kMetadataDirectoryName;
		const auto infoPath = metadataRoot / kBackupInfoFileName;
		const auto crcIndexPath = metadataRoot / kCrcIndexFileName;

		if (!fs::exists(backupRoot))
		{
			return;
		}

		const bool hasBackupFiles = BackupRootContainsBackupFiles(backupRoot, metadataRoot);
		if (!hasBackupFiles)
		{
			if (allowCreateMetadata && !fs::exists(infoPath))
			{
				WriteBackupInfo(infoPath);
			}
			return;
		}

		if (!fs::exists(infoPath))
		{
			ThrowUntrustedBackupDirectory();
		}

		const auto formatValue = ReadBackupInfoValue(infoPath, "format");
		const auto gameRootValue = ReadBackupInfoValue(infoPath, "game_root");
		if (formatValue.empty() || gameRootValue.empty())
		{
			ThrowUntrustedBackupDirectory();
		}

		const int formatVersion = std::stoi(formatValue);
		if (formatVersion < kBackupInfoFormatVersion)
		{
			ThrowUntrustedBackupDirectory();
		}

		const auto currentGameRoot = Logger::ToUtf8(Config::Instance().GetGameRoot());
		if (gameRootValue != currentGameRoot)
		{
			ThrowBackupInfoMismatch(
				xybase::string::sys_mbs_to_wcs(gameRootValue),
				xybase::string::sys_mbs_to_wcs(currentGameRoot));
		}
	}

	[[noreturn]] void ThrowBackupMismatch(const fs::path& relativePath, std::uint32_t expectedCrc, std::uint32_t actualCrc)
	{
		std::wcerr << L"\n警告：检测到 backup 中记录的原始文件 CRC 与当前游戏文件不一致："
			<< relativePath.wstring() << std::endl;
		std::wcerr << L"备份记录 CRC: 0x" << xybase::string::sys_mbs_to_wcs(FormatCrc(expectedCrc))
			<< L"，当前文件 CRC: 0x" << xybase::string::sys_mbs_to_wcs(FormatCrc(actualCrc)) << std::endl;
		std::wcerr << L"这通常表示当前游戏文件已经被修改，或者游戏已经更新。继续运行会把错误版本继续覆盖。" << std::endl;
		std::wcerr << L"程序将立即停止。请先恢复原始文件，或删除整个 backup 目录后用当前版本的原始游戏文件重新创建备份。" << std::endl;
		throw std::runtime_error("backup CRC mismatch");
	}

	[[noreturn]] void ThrowBackupMetadataMissing(const fs::path& relativePath)
	{
		std::wcerr << L"\n警告：无法在 backup 中找到该文件对应的 CRC 记录："
			<< relativePath.wstring() << std::endl;
	 std::wcerr << L"该备份无法在恢复前验证完整性。它可能来自旧版本程序，或被手工拼接过。" << std::endl;
		std::wcerr << L"为避免把已修改文件当成原始备份，程序将立即停止。请删除整个 backup 目录后重新运行。" << std::endl;
		std::wcerr << L"如果游戏已经更新，旧 backup 同样必须全部删除后重建。" << std::endl;
		throw std::runtime_error("backup CRC metadata missing");
	}

  void ValidateBackupFile(const fs::path& relativePath, const fs::path& backupPath)
	{
     const auto crcIndexPath = Config::Instance().GetProgRoot() / "backup" / kMetadataDirectoryName / kCrcIndexFileName;
		if (!fs::exists(crcIndexPath))
		{
			ThrowBackupMetadataMissing(relativePath);
		}

      std::uint32_t expectedCrc = 0;
		try
		{
			expectedCrc = ReadOriginalCrc(crcIndexPath, relativePath);
		}
		catch (const std::exception&)
		{
			ThrowBackupMetadataMissing(relativePath);
		}

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

fs::path BackupManager::GetCrcIndexPath()
{
    return GetMetadataRoot() / kCrcIndexFileName;
}

fs::path BackupManager::GetBackupInfoPath()
{
	return GetMetadataRoot() / kBackupInfoFileName;
}

bool BackupManager::BackupExists() const
{
 return BackupRootContainsBackupFiles(GetBackupRoot(), GetMetadataRoot());
}

void BackupManager::BackupGameFile(const fs::path& relativePath)
{
	const auto& config = Config::Instance();
	fs::path gamePath = config.GetGameRoot() / relativePath;
	fs::path backPath = GetBackupPath(relativePath);
    fs::path crcIndexPath = GetCrcIndexPath();

	if (!fs::exists(gamePath))
		return;

	Logger::Instance().Info("Ensuring backup for " + Logger::ToUtf8(relativePath));
	EnsureTrustedBackupDirectory(true);

	if (fs::exists(backPath))
	{
	   ValidateBackupFile(relativePath, backPath);
      const auto expectedCrc = ReadOriginalCrc(crcIndexPath, relativePath);
		const auto actualCrc = ComputeFileCrc32(gamePath);
		if (expectedCrc != actualCrc)
		{
			ThrowBackupMismatch(relativePath, expectedCrc, actualCrc);
		}

		return;
	}

	if (!fs::exists(backPath.parent_path()))
		fs::create_directories(backPath.parent_path());

	if (!fs::exists(GetBackupInfoPath()))
	{
		WriteBackupInfo(GetBackupInfoPath());
	}

	const auto originalCrc = ComputeFileCrc32(gamePath);
	fs::copy(gamePath, backPath, fs::copy_options::none);
 WriteOriginalCrc(crcIndexPath, relativePath, originalCrc);
	Logger::Instance().Info("Created backup file " + Logger::ToUtf8(backPath) + " with CRC 0x" + FormatCrc(originalCrc));
}

bool BackupManager::RestoreBackups()
{
	const auto& config = Config::Instance();
	fs::path backupRoot = GetBackupRoot();
	const auto& gameRoot = config.GetGameRoot();
	const auto metadataRoot = GetMetadataRoot();
	EnsureTrustedBackupDirectory(false);

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

	 ValidateBackupFile(relativePath, entryPath);

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

	Logger::Instance().Info("Backup directory detected.");

	const auto& config = Config::Instance();
	EnsureTrustedBackupDirectory(false);
	extern int YesNoPrompt(const std::wstring & prompt);

	if (config.IsInSituMode())
	{
	  std::wcout << L"检测到已有 backup，InSitu 模式会先自动恢复原始文件后再重新写入。" << std::endl;
		std::wcout << L"如果游戏已经更新，请先删除整个 backup 目录，再重新运行以创建新备份。" << std::endl;
		Logger::Instance().Info("In-place mode requires automatic backup restore before processing.");
		bool res = RestoreBackups();
		if (res)
		{
			int key = YesNoPrompt(L"备份的恢复完成了。要继续处理文件吗？");
			if (key != 'Y')
			{
				exit(0);
			}
		}
		return res;
	}
	else
	{
		// Use extern function from FFXITrans.cpp
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
