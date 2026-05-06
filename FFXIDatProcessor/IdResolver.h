#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <cstdint>
#include <map>
#include <utility>

class IdResolver
{
public:
	std::filesystem::path Resolve(uint32_t globalId) const;

	void Initialise();

	bool ResolveGlobalId(uint32_t globalId, std::string& romFolder, uint32_t& localFileId) const;

	uint32_t GetGlobalFileId(const std::string& romFolder, uint32_t localFileId) const;

	std::filesystem::path GetDatFilePath(uint32_t localFileId, const std::string& romFolder) const;

	std::filesystem::path GetDatFilePath(uint32_t globalId) const;

private:
	std::filesystem::path GetFileRootPath() const;

	std::vector<uint8_t> ReadVTable(int romNumber) const;

	std::vector<uint16_t> ReadFTable(int romNumber) const;

	static std::pair<int, int> CalculateDatPath(uint32_t fileId);

	std::filesystem::path m_gameRootPath;

	mutable std::map<int, std::vector<uint8_t>> m_vtableCache;
	mutable std::map<int, std::vector<uint16_t>> m_ftableCache;
};
