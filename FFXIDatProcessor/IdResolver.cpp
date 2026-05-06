#include "IdResolver.h"
#include "DataManager.h"
#include <fstream>
#include <sstream>

void IdResolver::Initialise()
{
    m_gameRootPath = PathUtil::gameRootPath;
    m_vtableCache.clear();
    m_ftableCache.clear();
}

std::filesystem::path IdResolver::GetFileRootPath() const
{
    return m_gameRootPath;
}

std::vector<uint8_t> IdResolver::ReadVTable(int romNumber) const
{
    auto it = m_vtableCache.find(romNumber);
    if (it != m_vtableCache.end())
    {
        return it->second;
    }

    std::vector<uint8_t> vtable;

    std::filesystem::path vtablePath;
    if (romNumber == 1)
    {
        vtablePath = m_gameRootPath / "VTABLE.DAT";
    }
    else
    {
        std::string romDir = "ROM" + std::to_string(romNumber);
        std::string vtableFile = "VTABLE" + std::to_string(romNumber) + ".DAT";
        vtablePath = m_gameRootPath / romDir / vtableFile;
    }

    if (std::filesystem::exists(vtablePath))
    {
        std::ifstream file(vtablePath, std::ios::binary);
        if (file.is_open())
        {
            file.seekg(0, std::ios::end);
            size_t fileSize = file.tellg();
            file.seekg(0, std::ios::beg);

            vtable.resize(fileSize);
            file.read(reinterpret_cast<char*>(vtable.data()), fileSize);
        }
    }

    m_vtableCache[romNumber] = vtable;
    return vtable;
}

std::vector<uint16_t> IdResolver::ReadFTable(int romNumber) const
{
    auto it = m_ftableCache.find(romNumber);
    if (it != m_ftableCache.end())
    {
        return it->second;
    }

    std::vector<uint16_t> ftable;

    std::filesystem::path ftablePath;
    if (romNumber == 1)
    {
        ftablePath = m_gameRootPath / "FTABLE.DAT";
    }
    else
    {
        std::string romDir = "ROM" + std::to_string(romNumber);
        std::string ftableFile = "FTABLE" + std::to_string(romNumber) + ".DAT";
        ftablePath = m_gameRootPath / romDir / ftableFile;
    }

    if (std::filesystem::exists(ftablePath))
    {
        std::ifstream file(ftablePath, std::ios::binary);
        if (file.is_open())
        {
            file.seekg(0, std::ios::end);
            size_t fileSize = file.tellg();
            file.seekg(0, std::ios::beg);

            size_t elementCount = fileSize / sizeof(uint16_t);
            ftable.resize(elementCount);
            file.read(reinterpret_cast<char*>(ftable.data()), fileSize);
        }
    }

    m_ftableCache[romNumber] = ftable;
    return ftable;
}

bool IdResolver::ResolveGlobalId(uint32_t globalId, std::string& romFolder, uint32_t& localFileId) const
{
    for (int romNumber = 1; romNumber <= 19; ++romNumber)
    {
        std::vector<uint8_t> vtable = ReadVTable(romNumber);
        std::vector<uint16_t> ftable = ReadFTable(romNumber);

        if (vtable.empty() || ftable.empty())
            continue;

        if (globalId >= vtable.size() || globalId >= ftable.size())
            continue;

        uint8_t romVolume = vtable[globalId];

        if (romVolume != romNumber)
            continue;

        localFileId = ftable[globalId];

        if (romNumber == 1)
        {
            romFolder = "ROM";
        }
        else
        {
            romFolder = "ROM" + std::to_string(romNumber);
        }

        std::filesystem::path datPath = GetDatFilePath(localFileId, romFolder);
        if (std::filesystem::exists(datPath))
        {
            return true;
        }
    }

    return false;
}

uint32_t IdResolver::GetGlobalFileId(const std::string& romFolder, uint32_t localFileId) const
{
    if (romFolder.substr(0, 3) != "ROM")
        return static_cast<uint32_t>(-1);

    int romNumber = 1;
    if (romFolder.length() > 3)
    {
        try
        {
            romNumber = std::stoi(romFolder.substr(3));
        }
        catch (const std::exception&)
        {
            return static_cast<uint32_t>(-1);
        }
    }

    std::vector<uint8_t> vtable = ReadVTable(romNumber);
    std::vector<uint16_t> ftable = ReadFTable(romNumber);

    for (size_t globalId = 0; globalId < ftable.size(); ++globalId)
    {
        if (ftable[globalId] == static_cast<uint16_t>(localFileId))
        {
            if (globalId < vtable.size() && vtable[globalId] == romNumber)
            {
                return static_cast<uint32_t>(globalId);
            }
        }
    }

    return (romNumber << 24) | (localFileId & 0xFFFFFF);
}

std::pair<int, int> IdResolver::CalculateDatPath(uint32_t fileId)
{
    int dir = fileId / 128;
    int file = fileId % 128;
    return { dir, file };
}

std::filesystem::path IdResolver::GetDatFilePath(uint32_t localFileId, const std::string& romFolder) const
{
    auto [dir, file] = CalculateDatPath(localFileId);

    std::ostringstream oss;
    oss << dir << "/" << file << ".DAT";

    return m_gameRootPath / romFolder / oss.str();
}

std::filesystem::path IdResolver::GetDatFilePath(uint32_t globalId) const
{
    std::string romFolder;
    uint32_t localFileId;
    if (!ResolveGlobalId(globalId, romFolder, localFileId))
    {
        return std::filesystem::path();
    }
    return GetDatFilePath(localFileId, romFolder);
}

std::filesystem::path IdResolver::Resolve(uint32_t globalId) const
{
    return GetDatFilePath(globalId);
}
