#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <CommCtrl.h>
#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <memory>

// Forward declarations
class ItemData;
class DMsg;
class XiString;
class EventStringBase;
class StatusData;
class FixedPhrase;
class MonBridge;
class RecordsOfEminence;
class ContentView;
class Image;

struct DatFileInfo
{
	int localFileId;
	std::string friendlyName;
	std::string fileType;
	std::string language;
	std::string category;
	std::string romFolder;
};

class DatFileManager
{
public:
	bool SaveCurrentFile(ContentView* contentView, const std::filesystem::path& filePath);

	explicit DatFileManager(const std::filesystem::path& gamePath);
	~DatFileManager();
	
	// Load all ROM definition files (FLIST.csv first, then ROM.csv, ROM2.csv, ROM3.csv, etc. as fallback)
	void LoadAllROMDefinitions(const std::filesystem::path& csvDir,
							  HWND hTreeView, bool cateSub);
	
	// Get file info by tree item
	const DatFileInfo* GetFileInfo(HTREEITEM itemId) const;
	
	// Convert file ID to actual file path
	std::filesystem::path GetDatFilePath(int fileId, const std::string& romFolder) const;

	std::filesystem::path GetDatFilePath(int fileId) const;
	
	// Check if a cell contains image data
	bool IsImageCell(int row, int col) const;
	
	// Check if current file is a menu file
	bool IsCurrentFileMenu() const;
	
	// Load DAT file and populate content view
	bool LoadDatFile(const DatFileInfo& info, ContentView* contentView);

	// Load arbitrary file with specified type
	bool LoadArbitraryFile(const std::filesystem::path& path, const std::string& fileType, ContentView* contentView);

	// Load file by global ID
	bool LoadGlobalId(int globalId, const std::string& fileType, ContentView* contentView);

	// Load file by local ID
	bool LoadLocalId(const std::string& romFolder, int localId, const std::string& fileType, ContentView* contentView);

	void SetLanguageFilter(const std::string& language)
	{
		m_languageFilters.clear();
		if (!language.empty())
		{
			m_languageFilters.insert(language);
		}
	}
	std::string GetLanguageFilter() const
	{
		if (m_languageFilters.empty())
		{
			return "";
		}
		return *m_languageFilters.begin();
	}
	void SetLanguageFilters(const std::set<std::string>& languages) { m_languageFilters = languages; }
	const std::set<std::string>& GetLanguageFilters() const { return m_languageFilters; }
	const std::vector<DatFileInfo>& GetAllFileInfos() const { return m_allFileInfos; }
	bool ExportDatToCsv(const DatFileInfo& info, const std::filesystem::path& csvPath, std::wstring* errorMessage = nullptr) const;
	bool ImportCsvToDat(const DatFileInfo& info, const std::filesystem::path& csvPath, std::wstring* errorMessage = nullptr) const;
private:
	std::filesystem::path m_gamePath;
	std::map<int, DatFileInfo> m_fileRegistry;
	std::map<HTREEITEM, int> m_treeItemToFileId;
	DatFileInfo m_currentFile;
	mutable std::map<int, std::vector<uint8_t>> m_vtableCache;
	mutable std::map<int, std::vector<uint16_t>> m_ftableCache;

	std::set<std::string> m_languageFilters;  // Empty means show all
	std::vector<DatFileInfo> m_allFileInfos;
	
	// Current loaded file data
	std::unique_ptr<DMsg> m_currentDMsg;
	std::unique_ptr<XiString> m_currentXiString;
	std::unique_ptr<EventStringBase> m_currentEventString;
	std::unique_ptr<StatusData> m_currentStatusData;
	std::unique_ptr<ItemData> m_currentItemData;
	std::unique_ptr<FixedPhrase> m_currentFixedPhrase;
	std::unique_ptr<MonBridge> m_currentMonBridge;
	std::unique_ptr<RecordsOfEminence> m_currentRoe;

	int GetGlobalFileId(const std::string& romFolder, int localFileId) const;
	
	// Calculate DAT file path from ID
	static std::pair<int, int> CalculateDatPath(int fileId);
	
	// Helper methods for VTABLE/FTABLE handling
	std::vector<uint8_t> ReadVTable(int romNumber) const;
	std::vector<uint16_t> ReadFTable(int romNumber) const;
	bool ResolveGlobalId(int globalId, std::string& romFolder, int& localFileId) const;
	bool IsLanguageAllowed(const std::u8string& language) const;
	
	// Load FLIST.csv definition
	std::vector<DatFileInfo> LoadFLISTFileInfos(const std::filesystem::path& flistPath);

	void BuildFileTree(const std::vector<DatFileInfo>& fileInfos,
		HWND hTreeView, HTREEITEM parentNode, bool cateSub);
	std::vector<DatFileInfo> LoadROMFileInfos(const std::filesystem::path& csvPath);
	
	// Load specific file types
	bool LoadDMsgFile(const std::filesystem::path& filePath, ContentView* contentView);
	bool LoadXiStringFile(const std::filesystem::path& filePath, ContentView* contentView);
	bool LoadEventStringFile(const std::filesystem::path& filePath, ContentView* contentView);
	bool LoadStatusDataFile(const std::filesystem::path& filePath, ContentView* contentView);
	bool LoadItemDataFile(const std::filesystem::path& filePath, const std::string& fileType, ContentView* contentView);
	bool LoadFixedPhraseFile(const std::filesystem::path& filePath, ContentView* contentView);
	bool LoadMonBridgeFile(const std::filesystem::path& filePath, ContentView* contentView);
	bool LoadRoeQuestFile(const std::filesystem::path& filePath, ContentView* contentView);
	bool LoadRoeCategoryFile(const std::filesystem::path& filePath, ContentView* contentView);
};
