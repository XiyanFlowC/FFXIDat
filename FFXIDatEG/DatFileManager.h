#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <CommCtrl.h>
#include <filesystem>
#include <map>
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
    int fileId;
    std::string friendlyName;
    std::string fileType;
    std::string language;
    std::string category;
    std::string romFolder;
};

class DatFileManager
{
public:
    explicit DatFileManager(const std::filesystem::path& gamePath);
    ~DatFileManager();
    
    // Load ROM definition from CSV
    void LoadROMDefinition(const std::filesystem::path& csvPath,
                          HWND hTreeView, HTREEITEM parentNode);
    
    // Load all ROM definition files (ROM.csv, ROM2.csv, ROM3.csv, etc.)
    void LoadAllROMDefinitions(const std::filesystem::path& csvDir,
                              HWND hTreeView);
    
    // Get file info by tree item
    const DatFileInfo* GetFileInfo(HTREEITEM itemId) const;
    
    // Convert file ID to actual file path
    std::filesystem::path GetDatFilePath(int fileId, const std::string& romFolder) const;
    
    // Check if a cell contains image data
    bool IsImageCell(int row, int col) const;
    
    // Check if current file is a menu file
    bool IsCurrentFileMenu() const;
    
    // Load DAT file and populate content view
    bool LoadDatFile(const DatFileInfo& info, ContentView* contentView);


    void SetLanguageFilter(const std::string& language) { m_languageFilter = language; }
    std::string GetLanguageFilter() const { return m_languageFilter; }
private:
    std::filesystem::path m_gamePath;
    std::map<int, DatFileInfo> m_fileRegistry;
    std::map<HTREEITEM, int> m_treeItemToFileId;
    DatFileInfo m_currentFile;

    std::string m_languageFilter = "";  // Empty string means show all
    
    // Current loaded file data
    std::unique_ptr<DMsg> m_currentDMsg;
    std::unique_ptr<XiString> m_currentXiString;
    std::unique_ptr<EventStringBase> m_currentEventString;
    std::unique_ptr<StatusData> m_currentStatusData;
    std::unique_ptr<ItemData> m_currentItemData;
    std::unique_ptr<FixedPhrase> m_currentFixedPhrase;
    std::unique_ptr<MonBridge> m_currentMonBridge;
    std::unique_ptr<RecordsOfEminence> m_currentRoe;
    
    // Calculate DAT file path from ID
    static std::pair<int, int> CalculateDatPath(int fileId);
    
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
