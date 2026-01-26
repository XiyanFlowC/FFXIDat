#include "DatFileManager.h"
#include "ContentView.h"
#include <sstream>
#include <fstream>
#include <ItemData.h>
#include <DMsg.h>
#include <XiString.h>
#include <EventStringBase.h>
#include <StatusData.h>
#include <FixedPhrase.h>
#include <MonBridge.h>
#include <RecordsOfEminence.h>
#include <CsvFile.h>
#include <xystring.h>

namespace {
	// Helper function to check if text contains line breaks
	bool ContainsLineBreaks(const std::wstring& text) {
		return text.find(L'\n') != std::wstring::npos || text.find(L'\r') != std::wstring::npos;
	}

	// Helper function to count lines in text
	int CountLines(const std::wstring& text) {
		if (text.empty()) return 1;
		
		int lineCount = 1;
		for (size_t i = 0; i < text.length(); ++i) {
			wchar_t ch = text[i];
			if (ch == L'\n') {
				lineCount++;
			}
			else if (ch == L'\r') {
				lineCount++;
				// Skip LF if it follows CR (CRLF sequence)
				if (i + 1 < text.length() && text[i + 1] == L'\n') {
					i++;
				}
			}
		}
		return lineCount;
	}
}

DatFileManager::DatFileManager(const std::filesystem::path& gamePath)
	: m_gamePath(gamePath)
{
}

DatFileManager::~DatFileManager()
{
}

std::vector<DatFileInfo> DatFileManager::LoadROMFileInfos(const std::filesystem::path& csvPath)
{
	// Extract ROM folder name from CSV filename
	std::string csvName = csvPath.stem().string();
	std::string romFolder = csvName;

	std::vector<DatFileInfo> fileInfos;
	CsvFile csv(csvPath, std::ios::in | std::ios::binary);

	while (!csv.IsEof())
	{
		// Read file ID
		std::u8string fileIdStr = csv.NextCell();
		if (fileIdStr.empty())
		{
			csv.NextLine();
			continue;
		}

		// Read file type
		std::u8string fileType = csv.IsEol() ? u8"" : csv.NextCell();

		// Read language
		std::u8string language = csv.IsEol() ? u8"" : csv.NextCell();

		// Read category (friendly name)
		std::u8string category = csv.IsEol() ? u8"" : csv.NextCell();

		csv.NextLine();

		if (fileType.empty())
			continue;

		// Apply language filter
		if (!m_languageFilter.empty() && language != xybase::string::to_utf8(m_languageFilter))
		{
			continue;
		}

		try
		{
			// Parse file ID
			int fileId = std::stoi(xybase::string::to_string(fileIdStr));

			// Create file info
			DatFileInfo info;
			info.localFileId = fileId;
			info.fileType = xybase::string::to_string(fileType);
			info.language = xybase::string::to_string(language);
			info.category = xybase::string::to_string(category);
			info.romFolder = romFolder;

			// Create friendly name
			std::string friendlyName = info.category;
			if (!friendlyName.empty() && !info.fileType.empty())
				friendlyName += " [" + info.fileType + "]";
			if (!friendlyName.empty() && !info.language.empty())
				friendlyName += " (" + info.language + ")";
			if (!friendlyName.empty())
				friendlyName += " - " + xybase::string::to_string(fileIdStr);
			if (friendlyName.empty())
			{
				friendlyName = romFolder + "/" + std::to_string(fileId);
			}

			info.friendlyName = friendlyName;
			fileInfos.push_back(info);
		}
		catch (const std::exception&)
		{
			continue;
		}
	}

	csv.Close();
	return fileInfos;
}

void DatFileManager::BuildFileTree(const std::vector<DatFileInfo>& fileInfos,
	HWND hTreeView, HTREEITEM parentNode, bool cateSub)
{
	if (!cateSub)
	{
		// Original logic for flat list (or when categories disabled)
		std::vector<DatFileInfo> sortedInfos = fileInfos;
		std::sort(sortedInfos.begin(), sortedInfos.end(),
			[](const DatFileInfo& a, const DatFileInfo& b) {
				return a.localFileId < b.localFileId;
			});

		for (const auto& info : sortedInfos)
		{
			TVINSERTSTRUCTW tvis = { 0 };
			tvis.hParent = parentNode;
			tvis.hInsertAfter = TVI_LAST;
			tvis.item.mask = TVIF_TEXT | TVIF_PARAM;

			std::wstring wName(info.friendlyName.begin(), info.friendlyName.end());
			tvis.item.pszText = const_cast<LPWSTR>(wName.c_str());
			tvis.item.lParam = GetGlobalFileId(info.romFolder, info.localFileId);

			HTREEITEM hItem = TreeView_InsertItem(hTreeView, &tvis);
			m_treeItemToFileId[hItem] = GetGlobalFileId(info.romFolder, info.localFileId);
		}
		return;
	}

	// 1. Build temporary tree structure
	struct Node {
		std::map<std::string, std::unique_ptr<Node>> children;
		std::vector<const DatFileInfo*> files;
		std::string name;
	};
	
	Node root;

	for (const auto& info : fileInfos)
	{
		Node* current = &root;
		if (!info.category.empty())
		{
			size_t start = 0, pos;
			while ((pos = info.category.find('/', start)) != std::string::npos)
			{
				std::string part = info.category.substr(start, pos - start);
				if (!part.empty())
				{
					if (current->children.find(part) == current->children.end()) {
						auto newNode = std::make_unique<Node>();
						newNode->name = part;
						current->children[part] = std::move(newNode);
					}
					current = current->children[part].get();
				}
				start = pos + 1;
			}
			// The part after last '/' is treated as the file name, not a subdirectory
		}
		current->files.push_back(&info);
	}

	// 2. Recursive function to populate UI
	struct TreeBuilder {
		DatFileManager* mgr;
		HWND hTree;
		
		void Populate(Node* node, HTREEITEM hParent) {
			
			// Part A: Process Folders (Subcategories)
			// Sort Order: Nodes with sub-sub-folders come first (Non-Leaf Categories), then others
			std::vector<Node*> sortedInternalNodes;
			for (auto& pair : node->children) sortedInternalNodes.push_back(pair.second.get());

			std::sort(sortedInternalNodes.begin(), sortedInternalNodes.end(), 
				[](const Node* a, const Node* b) {
					bool aHasChildren = !a->children.empty();
					bool bHasChildren = !b->children.empty();
					if (aHasChildren != bHasChildren) return aHasChildren; // True before False
					return a->name < b->name;
				});

			for (Node* child : sortedInternalNodes)
			{
				TVINSERTSTRUCTW tvis = { 0 };
				tvis.hParent = hParent;
				tvis.hInsertAfter = TVI_LAST;
				tvis.item.mask = TVIF_TEXT;
				
				std::wstring wName(child->name.begin(), child->name.end());
				tvis.item.pszText = const_cast<LPWSTR>(wName.c_str());

				HTREEITEM hChild = TreeView_InsertItem(hTree, &tvis);
				Populate(child, hChild);
			}

			// Part B: Process Files (Leafs)
			std::vector<const DatFileInfo*> sortedFiles = node->files;
			std::sort(sortedFiles.begin(), sortedFiles.end(), 
				[](const DatFileInfo* a, const DatFileInfo* b) {
					return a->friendlyName == b->friendlyName ? a->localFileId < b->localFileId : a->friendlyName < b->friendlyName;
				});

			for (const DatFileInfo* info : sortedFiles)
			{
				TVINSERTSTRUCTW tvis = { 0 };
				tvis.hParent = hParent;
				tvis.hInsertAfter = TVI_LAST;
				tvis.item.mask = TVIF_TEXT | TVIF_PARAM;

				// Generate optimized display name
				std::string leafName;
				size_t lastSlash = info->category.find_last_of('/');
				if (lastSlash != std::string::npos)
					leafName = info->category.substr(lastSlash + 1);
				else
					leafName = info->category;

				std::string dispName = leafName;
				if (!dispName.empty()) dispName += " - ";
				dispName += std::to_string(info->localFileId);
				
				if (!info->fileType.empty()) dispName += " [" + info->fileType + "]";
				if (!info->language.empty()) dispName += " (" + info->language + ")";
				
				// Fallback if empty
				if (dispName.empty()) dispName = std::to_string(info->localFileId);

				std::wstring wName(dispName.begin(), dispName.end());
				tvis.item.pszText = const_cast<LPWSTR>(wName.c_str());
				
				int globalId = mgr->GetGlobalFileId(info->romFolder, info->localFileId);
				tvis.item.lParam = globalId;

				HTREEITEM hItem = TreeView_InsertItem(hTree, &tvis);
				mgr->m_treeItemToFileId[hItem] = globalId;
			}
		}
	};

	TreeBuilder builder = { this, hTreeView };
	builder.Populate(&root, parentNode);
}

void DatFileManager::LoadAllROMDefinitions(const std::filesystem::path& csvDir,
	HWND hTreeView, bool cateSub)
{
	// Add root item
	TVINSERTSTRUCTW tvis = { 0 };
	tvis.hParent = TVI_ROOT;
	tvis.hInsertAfter = TVI_LAST;
	tvis.item.mask = TVIF_TEXT;
	tvis.item.pszText = const_cast<LPWSTR>(L"FFXI Data Files");
	HTREEITEM hRoot = TreeView_InsertItem(hTreeView, &tvis);

	// Collect all file infos from different sources
	std::vector<DatFileInfo> allFileInfos;

	// Priority 1: Load from FLIST.csv first
	std::filesystem::path flistPath = csvDir / "FLIST.csv";
	if (std::filesystem::exists(flistPath))
	{
		auto flistInfos = LoadFLISTFileInfos(flistPath);
		allFileInfos.insert(allFileInfos.end(), flistInfos.begin(), flistInfos.end());
	}

	// Priority 2: Load ROM.csv, ROM2.csv, ROM3.csv, etc. as fallback
	std::vector<std::pair<std::string, std::filesystem::path>> romFiles;

	for (const auto& entry : std::filesystem::directory_iterator(csvDir))
	{
		if (entry.is_regular_file() && entry.path().extension() == L".csv")
		{
			std::string filename = entry.path().stem().string();
			// Check if filename matches ROM pattern (ROM, ROM2, ROM3, etc.)
			if (filename == "ROM" || (filename.length() > 3 &&
				filename.substr(0, 3) == "ROM" &&
				std::all_of(filename.begin() + 3, filename.end(), ::isdigit)))
			{
				romFiles.push_back({ filename, entry.path() });
			}
		}
	}

	// Sort so ROM comes first, then ROM2, ROM3, etc.
	std::sort(romFiles.begin(), romFiles.end(),
		[](const auto& a, const auto& b) {
			if (a.first == "ROM") return true;
			if (b.first == "ROM") return false;
			return a.first < b.first;
		});

	// Load legacy ROM files and add entries that don't exist in FLIST
	for (const auto& [romName, csvPath] : romFiles)
	{
		if (std::filesystem::exists(csvPath))
		{
			auto legacyInfos = LoadROMFileInfos(csvPath);

			// Only add legacy entries that don't conflict with FLIST entries
			for (const auto& legacyInfo : legacyInfos)
			{
				// Check if this entry already exists (by comparing category+language+type)
				bool exists = std::any_of(allFileInfos.begin(), allFileInfos.end(),
					[&legacyInfo](const DatFileInfo& existing) {
						return existing.category == legacyInfo.category &&
							existing.language == legacyInfo.language &&
							existing.fileType == legacyInfo.fileType;
					});

				if (!exists)
				{
					allFileInfos.push_back(legacyInfo);
					m_fileRegistry[GetGlobalFileId(legacyInfo.romFolder, legacyInfo.localFileId)] = legacyInfo;
				}
			}
		}
	}

	// Now build tree from combined file infos using the same structure as LoadROMDefinition
	BuildFileTree(allFileInfos, hTreeView, hRoot, cateSub);

	TreeView_Expand(hTreeView, hRoot, TVE_EXPAND);
}

std::vector<DatFileInfo> DatFileManager::LoadFLISTFileInfos(const std::filesystem::path& flistPath)
{
	std::vector<DatFileInfo> fileInfos;
	std::map<std::string, int> nameToGlobalId; // Track duplicate names by language+name

	CsvFile csv(flistPath, std::ios::in | std::ios::binary);

	while (!csv.IsEof())
	{
		// Read global ID
		std::u8string globalIdStr = csv.NextCell();
		if (globalIdStr.empty())
		{
			csv.NextLine();
			continue;
		}

		// Read file type
		std::u8string fileType = csv.IsEol() ? u8"" : csv.NextCell();

		// Read language
		std::u8string language = csv.IsEol() ? u8"" : csv.NextCell();

		// Read category (friendly name)
		std::u8string category = csv.IsEol() ? u8"" : csv.NextCell();

		csv.NextLine();

		if (fileType.empty())
			continue;

		// Apply language filter
		if (!m_languageFilter.empty() && language != xybase::string::to_utf8(m_languageFilter))
		{
			continue;
		}

		try
		{
			// Parse global ID
			int globalId = std::stoi(xybase::string::to_string(globalIdStr));

			// Create unique key for duplicate detection (language + category)
			std::string uniqueKey = xybase::string::to_string(language) + "|" + xybase::string::to_string(category);

			// Handle duplicates - use the last occurrence
			if (nameToGlobalId.find(uniqueKey) != nameToGlobalId.end())
			{
				// Remove previous entry with same name+language
				int prevGlobalId = nameToGlobalId[uniqueKey];
				auto it = std::find_if(fileInfos.begin(), fileInfos.end(),
					[prevGlobalId](const DatFileInfo& info) {
						return info.localFileId == prevGlobalId;
					});
				if (it != fileInfos.end())
				{
					fileInfos.erase(it);
				}
			}
			nameToGlobalId[uniqueKey] = globalId;

			// Validate global ID and get ROM info
			std::string romFolder;
			int localFileId;
			if (!ResolveGlobalId(globalId, romFolder, localFileId))
			{
				// Skip this entry if global ID cannot be resolved
				continue;
			}

			// Create file info
			DatFileInfo info;
			info.localFileId = localFileId;  // Keep global ID as the identifier
			info.fileType = xybase::string::to_string(fileType);
			info.language = xybase::string::to_string(language);
			info.category = xybase::string::to_string(category);
			info.romFolder = romFolder;

			// Create friendly name
			std::string friendlyName = info.category;
			if (!friendlyName.empty() && !info.fileType.empty())
				friendlyName += " [" + info.fileType + "]";
			if (!friendlyName.empty() && !info.language.empty())
				friendlyName += " (" + info.language + ")";
			if (!friendlyName.empty())
				friendlyName += " - " + std::to_string(globalId);
			if (friendlyName.empty())
			{
				friendlyName = romFolder + "/" + std::to_string(localFileId);
			}

			info.friendlyName = friendlyName;
			fileInfos.push_back(info);
			m_fileRegistry[globalId] = info;
		}
		catch (const std::exception&)
		{
			continue;
		}
	}

	csv.Close();
	return fileInfos;
}

std::vector<uint8_t> DatFileManager::ReadVTable(int romNumber) const
{
	// Check cache first
	auto it = m_vtableCache.find(romNumber);
	if (it != m_vtableCache.end())
	{
		return it->second;
	}

	std::vector<uint8_t> vtable;

	// Construct VTABLE filename based on ROM number
	std::filesystem::path vtablePath;
	if (romNumber == 1)
	{
		// For ROM 1, VTABLE is in game root directory
		vtablePath = m_gamePath / "VTABLE.DAT";
	}
	else
	{
		// For ROM 2+, VTABLE is in ROM{i} directory
		std::string romDir = "ROM" + std::to_string(romNumber);
		std::string vtableFile = "VTABLE" + std::to_string(romNumber) + ".DAT";
		vtablePath = m_gamePath / romDir / vtableFile;
	}

	// Read VTABLE file
	if (std::filesystem::exists(vtablePath))
	{
		std::ifstream file(vtablePath, std::ios::binary);
		if (file.is_open())
		{
			// Get file size
			file.seekg(0, std::ios::end);
			size_t fileSize = file.tellg();
			file.seekg(0, std::ios::beg);

			// Read entire file
			vtable.resize(fileSize);
			file.read(reinterpret_cast<char*>(vtable.data()), fileSize);
		}
	}

	// Cache the result
	m_vtableCache[romNumber] = vtable;
	return vtable;
}

std::vector<uint16_t> DatFileManager::ReadFTable(int romNumber) const
{
	// Check cache first
	auto it = m_ftableCache.find(romNumber);
	if (it != m_ftableCache.end())
	{
		return it->second;
	}

	std::vector<uint16_t> ftable;

	// Construct FTABLE filename based on ROM number
	std::filesystem::path ftablePath;
	if (romNumber == 1)
	{
		// For ROM 1, FTABLE is in game root directory
		ftablePath = m_gamePath / "FTABLE.DAT";
	}
	else
	{
		// For ROM 2+, FTABLE is in ROM{i} directory
		std::string romDir = "ROM" + std::to_string(romNumber);
		std::string ftableFile = "FTABLE" + std::to_string(romNumber) + ".DAT";
		ftablePath = m_gamePath / romDir / ftableFile;
	}

	// Read FTABLE file
	if (std::filesystem::exists(ftablePath))
	{
		std::ifstream file(ftablePath, std::ios::binary);
		if (file.is_open())
		{
			// Get file size
			file.seekg(0, std::ios::end);
			size_t fileSize = file.tellg();
			file.seekg(0, std::ios::beg);

			// Read entire file as 16-bit integers
			size_t elementCount = fileSize / sizeof(uint16_t);
			ftable.resize(elementCount);
			file.read(reinterpret_cast<char*>(ftable.data()), fileSize);
		}
	}

	// Cache the result
	m_ftableCache[romNumber] = ftable;
	return ftable;
}

bool DatFileManager::ResolveGlobalId(int globalId, std::string& romFolder, int& localFileId) const
{
	// Try all ROM numbers from 1 to 19
	for (int romNumber = 1; romNumber <= 19; ++romNumber)
	{
		// Read VTABLE and FTABLE for this ROM
		std::vector<uint8_t> vtable = ReadVTable(romNumber);
		std::vector<uint16_t> ftable = ReadFTable(romNumber);
		
		// Check if files exist and are valid
		if (vtable.empty() || ftable.empty())
			continue;
		
		// Check bounds
		if (globalId >= static_cast<int>(vtable.size()) || 
			(globalId) >= static_cast<int>(ftable.size()))
			continue;
		
		// Get ROM volume from VTABLE
		uint8_t romVolume = vtable[globalId];
		
		// Check if this ROM matches
		if (romVolume != romNumber)
			continue;
		
		// Get encoded value from FTABLE
		localFileId = ftable[globalId];
		
		// Set ROM folder name
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

const DatFileInfo* DatFileManager::GetFileInfo(HTREEITEM itemId) const
{
	auto it = m_treeItemToFileId.find(itemId);
	if (it == m_treeItemToFileId.end())
		return nullptr;
	
	auto infoIt = m_fileRegistry.find(it->second);
	if (infoIt == m_fileRegistry.end())
		return nullptr;
	
	return &infoIt->second;
}

std::filesystem::path DatFileManager::GetDatFilePath(int fileId, 
													  const std::string& romFolder) const
{
	auto [dir, file] = CalculateDatPath(fileId);
	
	std::ostringstream oss;
	oss << dir << "/" << file << ".DAT";
	
	return m_gamePath / romFolder / oss.str();
}

std::filesystem::path DatFileManager::GetDatFilePath(int fileId) const
{
	auto registryIt = m_fileRegistry.find(fileId);
	if (registryIt != m_fileRegistry.end())
	{
		return GetDatFilePath(registryIt->second.localFileId,
			registryIt->second.romFolder);
	}

	std::string romFolder = "ROM";
	int localFileId;
	ResolveGlobalId(fileId, romFolder, localFileId);
	return GetDatFilePath(localFileId, romFolder);
}

int DatFileManager::GetGlobalFileId(const std::string& romFolder, int localFileId) const
{
	// search in corresponding ROM
	if (romFolder.substr(0, 3) != "ROM")
		return -1;
	int romNumber = 1;
	if (romFolder.length() > 3)
	{
		try
		{
			romNumber = std::stoi(romFolder.substr(3));
		}
		catch (const std::exception&)
		{
			return -1;
		}
	}

	// Read VTABLE and FTABLE for this ROM
	std::vector<uint8_t> vtable = ReadVTable(romNumber);
	std::vector<uint16_t> ftable = ReadFTable(romNumber);
	// Search for matching localFileId in FTABLE
	for (size_t globalId = 0; globalId < ftable.size(); ++globalId)
	{
		if (ftable[globalId] == static_cast<uint16_t>(localFileId))
		{
			// Check if VTABLE matches ROM number
			if (globalId < vtable.size() && vtable[globalId] == romNumber)
			{
				return static_cast<int>(globalId);
			}
		}
	}

	// if vtable/ftable not up, return a make up global id
	return (romNumber << 24) | (localFileId & 0xFFFFFF);
}

std::pair<int, int> DatFileManager::CalculateDatPath(int fileId)
{
	int dir = fileId / 128;
	int file = fileId % 128;
	
	return {dir, file};
}

bool DatFileManager::IsImageCell(int row, int col) const
{
	// TODO: Implement based on current file type and column
	return false;
}

bool DatFileManager::IsCurrentFileMenu() const
{
	return m_currentFile.fileType == "menu";
}

bool DatFileManager::LoadDatFile(const DatFileInfo& info, ContentView* contentView)
{
	m_currentFile = info;
	
	std::filesystem::path filePath = GetDatFilePath(info.localFileId, info.romFolder);
	
	if (!std::filesystem::exists(filePath))
	{
		std::wstring msg = L"File not found: " + filePath.wstring();
		MessageBoxW(nullptr, msg.c_str(), L"Error", MB_OK | MB_ICONERROR);
		return false;
	}
	
	// Clear previous data
	m_currentDMsg.reset();
	m_currentXiString.reset();
	m_currentEventString.reset();
	m_currentStatusData.reset();
	m_currentItemData.reset();
	m_currentFixedPhrase.reset();
	m_currentMonBridge.reset();
	m_currentRoe.reset();
	
	// Load appropriate file type
	try
	{
		if (info.fileType == "dmsg")
		{
			return LoadDMsgFile(filePath, contentView);
		}
		else if (info.fileType == "xis")
		{
			return LoadXiStringFile(filePath, contentView);
		}
		else if (info.fileType == "evsb")
		{
			return LoadEventStringFile(filePath, contentView);
		}
		else if (info.fileType == "sd")
		{
			return LoadStatusDataFile(filePath, contentView);
		}
		else if (info.fileType == "fp")
		{
			return LoadFixedPhraseFile(filePath, contentView);
		}
		else if (info.fileType == "iab" || info.fileType == "iwb" || info.fileType == "iub" ||
			info.fileType == "inb" || info.fileType == "ipb" || info.fileType == "isb" ||
			info.fileType == "icb")
		{
			return LoadItemDataFile(filePath, info.fileType, contentView);
		}
		else if (info.fileType == "mbd")
		{
			return LoadMonBridgeFile(filePath, contentView);
		}
		else if (info.fileType == "erq")
		{
			return LoadRoeQuestFile(filePath, contentView);
		}
		else if (info.fileType == "erc")
		{
			return LoadRoeCategoryFile(filePath, contentView);
		}
		else
		{
			std::wstring msg = L"Unsupported file type: ";
			std::string typeStr = info.fileType;
			msg += std::wstring(typeStr.begin(), typeStr.end());
			MessageBoxW(nullptr, msg.c_str(), L"Error", MB_OK | MB_ICONERROR);
			return false;
		}
	}
	catch (const std::exception& e)
	{
		std::string errMsg = "Error loading file: ";
		errMsg += e.what();
		std::wstring wErrMsg(errMsg.begin(), errMsg.end());
		MessageBoxW(nullptr, wErrMsg.c_str(), L"Error", MB_OK | MB_ICONERROR);
		return false;
	}
}

bool DatFileManager::LoadDMsgFile(const std::filesystem::path& filePath, ContentView* contentView)
{
	m_currentDMsg = std::make_unique<DMsg>(filePath);
	m_currentDMsg->Read();
	
	contentView->Clear();
	
	// Find maximum number of columns
	int maxCols = 0;
	for (const auto& row : *m_currentDMsg)
	{
		int cols = static_cast<int>(row.GetCellsConst().size());
		if (cols > maxCols)
			maxCols = cols;
	}
	
	if (maxCols == 0)
		maxCols = 1;
	
	// Set columns
	contentView->SetColumnCount(maxCols + 1);
	contentView->SetColumnTitle(0, L"Index");
	contentView->SetColumnWidth(0, 60);
	for (int col = 0; col < maxCols; ++col)
	{
		std::wstring colName = L"Cell " + std::to_wstring(col);
		contentView->SetColumnTitle(col + 1, colName);

		if (m_currentDMsg->begin()->GetCellsConst()[col].GetType() == 1)  // Integer
			contentView->SetColumnWidth(col + 1, 40);
		else
			contentView->SetColumnWidth(col + 1, 300);

	}

	int index = 0;
	
	// Add items
	for (const auto& row : *m_currentDMsg)
	{
		auto item = std::make_unique<ContentItem>();
		
		bool hasMultilineText = false;
		int maxLines = 1;

		item->columns.push_back(ColumnData::MakeInteger(index++));
		
		for (const auto& cell : row.GetCellsConst())
		{
			if (cell.GetType() == 1)  // Integer
			{
				int value = cell.Get<int>();
				item->columns.push_back(ColumnData::MakeInteger(value));
				continue;
			}

			std::u8string cellStr = cell.Get<std::u8string>();
			std::wstring wstr = xybase::string::to_wstring(cellStr);
			item->columns.push_back(ColumnData::MakeMultilineText(wstr));
			
			// Check if this cell contains line breaks
			if (ContainsLineBreaks(wstr))
			{
				hasMultilineText = true;
				int lineCount = CountLines(wstr);
				if (lineCount > maxLines)
					maxLines = lineCount;
			}
		}
		
		// Set type and custom height for multi-line items
		if (hasMultilineText)
		{
			item->type = ContentItemType::Multiline;
			item->customHeight = maxLines * 24;  // 24 pixels per line
		}
		else
		{
			item->type = ContentItemType::Multiline;
		}
		
		contentView->AddItem(std::move(item));
	}
	
	return true;
}

bool DatFileManager::LoadXiStringFile(const std::filesystem::path& filePath, ContentView* contentView)
{
	m_currentXiString = std::make_unique<XiString>(filePath);
	m_currentXiString->Read();
	
	contentView->Clear();
	
	// Set columns: Index, String, Flag1, Flag2, Flag3
	contentView->SetColumnCount(5);
	contentView->SetColumnTitle(0, L"Index");
	contentView->SetColumnWidth(0, 60);
	contentView->SetColumnTitle(1, L"String");
	contentView->SetColumnWidth(1, 600);
	contentView->SetColumnTitle(2, L"Flag1");
	contentView->SetColumnWidth(2, 40);
	contentView->SetColumnTitle(3, L"Flag2");
	contentView->SetColumnWidth(3, 40);
	contentView->SetColumnTitle(4, L"Flag3");
	contentView->SetColumnWidth(4, 40);
	
	// Add items
	int index = 0;
	for (const auto& entry : *m_currentXiString)
	{
		auto item = std::make_unique<ContentItem>();
		
		std::wstring wstr = xybase::string::to_wstring(XiString::Decode(entry.str));
		
		item->columns.push_back(ColumnData::MakeInteger( (index)) );
		item->columns.push_back(ColumnData::MakeMultilineText( wstr ));
		item->columns.push_back(ColumnData::MakeText(std::to_wstring(entry.flag1)));
		item->columns.push_back(ColumnData::MakeText(std::to_wstring(entry.flag2)));
		item->columns.push_back(ColumnData::MakeText(std::to_wstring(entry.flag3)));
		
		// Check if the string contains line breaks
		if (ContainsLineBreaks(wstr))
		{
			item->type = ContentItemType::Multiline;
			int lineCount = CountLines(wstr);
			item->customHeight = lineCount * 24;  // 24 pixels per line
		}
		else
		{
			item->type = ContentItemType::Multiline;
		}
		
		contentView->AddItem(std::move(item));
		index++;
	}
	
	return true;
}

bool DatFileManager::LoadEventStringFile(const std::filesystem::path& filePath, ContentView* contentView)
{
	m_currentEventString = std::make_unique<EventStringBase>(filePath);
	m_currentEventString->Read();
	
	contentView->Clear();
	
	// Set columns: Index, String
	contentView->SetColumnCount(2);
	contentView->SetColumnTitle(0, L"Index");
	contentView->SetColumnWidth(0, 60);
	contentView->SetColumnTitle(1, L"String");
	contentView->SetColumnWidth(1, 600);
	
	// Add items
	for (size_t i = 0; i < m_currentEventString->Size(); ++i)
	{
		auto item = std::make_unique<ContentItem>();
		item->type = ContentItemType::Multiline;
		
		item->columns.push_back(ColumnData::MakeInteger(i));
		
		const auto& str = (*m_currentEventString)[i];
		item->columns.push_back(ColumnData::MakeMultilineText( xybase::string::to_wstring(str) ));
		
		contentView->AddItem(std::move(item));
	}
	
	return true;
}

bool DatFileManager::LoadStatusDataFile(const std::filesystem::path& filePath, ContentView* contentView)
{
	m_currentStatusData = std::make_unique<StatusData>();
	m_currentStatusData->Read(filePath.wstring());
	
	contentView->Clear();
	
	// Set columns: ID, Flag, Description
	contentView->SetColumnCount(4);
	contentView->SetColumnTitle(0, L"ID");
	contentView->SetColumnWidth(0, 60);
	contentView->SetColumnTitle(1, L"Flag");
	contentView->SetColumnWidth(1, 60);
	contentView->SetColumnTitle(2, L"Description");
	contentView->SetColumnWidth(2, 400);
	contentView->SetColumnTitle(3, L"Image");
	
	// Add items
	for (auto& datum : m_currentStatusData->data)
	{
		auto item = std::make_unique<ContentItem>();
		
		// ID column
		item->columns.push_back(ColumnData::MakeInteger(datum.id));
		
		// Flag column (displayed as hex)
		wchar_t flagStr[32];
		swprintf_s(flagStr, L"0x%04X", datum.flg);
		item->columns.push_back(ColumnData::MakeText(flagStr));
		
		// Description column
		std::wstring description = xybase::string::to_wstring(datum.description);
		item->columns.push_back(ColumnData::MakeMultilineText(description));
		
		// Check if description contains line breaks
		if (ContainsLineBreaks(description))
		{
			item->type = ContentItemType::Multiline;
			int lineCount = CountLines(description);
			item->customHeight = lineCount * 24;
		}
		else
		{
			item->type = ContentItemType::Multiline;
		}

		item->columns.push_back(ColumnData::MakeImage(std::make_shared<Image>(datum.image)));

		contentView->AddItem(std::move(item));
	}
	
	return true;
}

bool DatFileManager::LoadFixedPhraseFile(const std::filesystem::path& filePath, ContentView* contentView)
{
	m_currentFixedPhrase = std::make_unique<FixedPhrase>();
	m_currentFixedPhrase->Read(filePath);

	contentView->Clear();
	contentView->SetColumnCount(3);
	contentView->SetColumnTitle(0, L"Category");
	contentView->SetColumnWidth(0, 120);
	contentView->SetColumnTitle(1, L"Name");
	contentView->SetColumnWidth(1, 250);
	contentView->SetColumnTitle(2, L"Pronunciation");
	contentView->SetColumnWidth(2, 250);

	for (const auto& category : m_currentFixedPhrase->categories)
	{
		for (const auto& entry : category.entries)
		{
			auto item = std::make_unique<ContentItem>();
			item->type = ContentItemType::Multiline;

			std::wstring categoryName = xybase::string::to_wstring(category.categoryName);
			item->columns.push_back(ColumnData::MakeText(categoryName));

			std::wstring entryText = xybase::string::to_wstring(entry.text);
			item->columns.push_back(ColumnData::MakeMultilineText(entryText));

			std::wstring entryPron = xybase::string::to_wstring(entry.pron);
			item->columns.push_back(ColumnData::MakeMultilineText(entryPron));
			int maxLineCount = max(
				ContainsLineBreaks(entryText) ? CountLines(entryText) : 1,
				ContainsLineBreaks(entryPron) ? CountLines(entryPron) : 1
			);
			if (maxLineCount > 1)
			{
				item->customHeight = maxLineCount * 24;
			}

			contentView->AddItem(std::move(item));
		}
	}

	return true;
}

// ==================== ItemData (iab, iwb, iub, inb, ipb, isb, icb) ====================
bool DatFileManager::LoadItemDataFile(const std::filesystem::path& filePath, const std::string& fileType, ContentView* contentView)
{
	m_currentItemData = std::make_unique<ItemData>();

	ItemSpecType specType = ItemSpecType::NORMAL;
	if (fileType == "iab") specType = ItemSpecType::ARMOUR;
	else if (fileType == "iwb") specType = ItemSpecType::WEAPON;
	else if (fileType == "iub") specType = ItemSpecType::USABLE;
	else if (fileType == "ipb") specType = ItemSpecType::PUPPET;
	else if (fileType == "isb") specType = ItemSpecType::SLIP;
	else if (fileType == "icb") specType = ItemSpecType::CURRENCY;

	m_currentItemData->Read(filePath, specType);

	contentView->Clear();

	int maxCols = 0;
	for (const auto& datum : m_currentItemData->data)
	{
		int cols = static_cast<int>(datum.row().GetCellsConst().size());
		if (cols > maxCols) maxCols = cols;
	}
	if (maxCols == 0) maxCols = 1;


	// extra columns for non-editable metadata (Flags/Stack/Type)
	const int extraCols = 2;
	contentView->SetColumnCount(maxCols + 2 + extraCols);
	contentView->SetColumnTitle(0, L"ID");
	contentView->SetColumnWidth(0, 60);
	contentView->SetColumnTitle(1, L"Icon");
	contentView->SetColumnWidth(1, 40);
	for (int col = 0; col < maxCols; ++col)
	{
		std::wstring colName = L"Field " + std::to_wstring(col);
		contentView->SetColumnTitle(col + 2, colName);
		contentView->SetColumnWidth(col + 2, 300);
	}

	// Append non-editable columns at the end
	int appendedBase = 2 + maxCols;
	contentView->SetColumnTitle(appendedBase + 0, L"Flags");
	contentView->SetColumnWidth(appendedBase + 0, 150);
	contentView->SetColumnTitle(appendedBase + 1, L"Stack");
	contentView->SetColumnWidth(appendedBase + 1, 60);
	/*contentView->SetColumnTitle(appendedBase + 2, L"Type");
	contentView->SetColumnWidth(appendedBase + 2, 80);*/

	for (const auto& datum : m_currentItemData->data)
	{
		auto item = std::make_unique<ContentItem>();
		item->type = ContentItemType::Multiline;

		bool hasMultilineText = false;
		int maxLines = 1;

		item->columns.push_back(ColumnData::MakeInteger(datum.id));
		item->columns.push_back(ColumnData::MakeImage(std::make_shared<Image>(datum.image)));

		for (const auto& cell : datum.row())
		{
			if (cell.GetType() == 0)
			{
				std::u8string cellStr = cell.Get<std::u8string>();
				std::wstring wstr = xybase::string::to_wstring(cellStr);
				item->columns.push_back(ColumnData::MakeMultilineText(wstr));

				if (ContainsLineBreaks(wstr))
				{
					hasMultilineText = true;
					int lineCount = CountLines(wstr);
					if (lineCount > maxLines) maxLines = lineCount;
				}
			}
			else if (cell.GetType() == 1)
			{
				int64_t value = cell.Get<int>();
				item->columns.push_back(ColumnData::MakeInteger(value));
			}
			else
			{
				item->columns.push_back(ColumnData::MakeText(L"[Binary]"));
			}
		}

		// Append non-editable metadata columns
		{
			// Combined flags: Alt / Ex / Rare (others can be added later)
			std::wstring flagsStr;
			const auto& flg = datum.flags();
			if (flg.is_alt) {
				if (!flagsStr.empty()) flagsStr += L" ";
				flagsStr += L"Alt";
			}
			if (flg.is_ex) {
				if (!flagsStr.empty()) flagsStr += L" ";
				flagsStr += L"Ex";
			}
			if (flg.is_rare) {
				if (!flagsStr.empty()) flagsStr += L" ";
				flagsStr += L"Rare";
			}
			if (flagsStr.empty()) flagsStr = L"-";

			ColumnData flagsCol = ColumnData::MakeText(flagsStr);
			flagsCol.editable = false;
			item->columns.push_back(flagsCol);

			// Stack size
			ColumnData stackCol = ColumnData::MakeInteger(datum.stack_size());
			stackCol.editable = false;
			item->columns.push_back(stackCol);

			// Spec type as text
			/*std::wstring typeStr;
			switch (datum.spec_type) {
			case ItemSpecType::WEAPON: typeStr = L"WEAPON"; break;
			case ItemSpecType::ARMOUR: typeStr = L"ARMOUR"; break;
			case ItemSpecType::USABLE: typeStr = L"USABLE"; break;
			case ItemSpecType::PUPPET: typeStr = L"PUPPET"; break;
			case ItemSpecType::SLIP:   typeStr = L"SLIP"; break;
			case ItemSpecType::CURRENCY: typeStr = L"CURRENCY"; break;
			case ItemSpecType::NORMAL: default: typeStr = L"NORMAL"; break;
			}
			ColumnData typeCol = ColumnData::MakeText(typeStr);
			typeCol.editable = false;
			item->columns.push_back(typeCol);*/
		}

		item->customHeight = 3 * 24;
		contentView->AddItem(std::move(item));
	}

	return true;
}

// ==================== MonBridge (mbd) ====================
bool DatFileManager::LoadMonBridgeFile(const std::filesystem::path& filePath, ContentView* contentView)
{
	m_currentMonBridge = std::make_unique<MonBridge>();
	m_currentMonBridge->Read(filePath);

	contentView->Clear();
	contentView->SetColumnCount(4);
	contentView->SetColumnTitle(0, L"ID");
	contentView->SetColumnWidth(0, 60);
	contentView->SetColumnTitle(1, L"Icon");
	contentView->SetColumnWidth(1, 40);
	contentView->SetColumnTitle(2, L"Internal Name");
	contentView->SetColumnWidth(2, 150);
	contentView->SetColumnTitle(3, L"Display Name");
	contentView->SetColumnWidth(3, 350);

	for (const auto& datum : m_currentMonBridge->data)
	{
		auto item = std::make_unique<ContentItem>();
		item->type = ContentItemType::Multiline;

		item->columns.push_back(ColumnData::MakeInteger(datum.id));

		item->columns.push_back(ColumnData::MakeImage(std::make_shared<Image>(datum.image)));

		std::wstring internalName = xybase::string::to_wstring(datum.internalName);
		item->columns.push_back(ColumnData::MakeText(internalName));
		std::wstring displayName = xybase::string::to_wstring(datum.displayName);
		item->columns.push_back(ColumnData::MakeMultilineText(displayName));

		item->customHeight = 32;

		contentView->AddItem(std::move(item));
	}

	return true;
}

// ==================== RecordsOfEminence - Quest (erq) ====================
bool DatFileManager::LoadRoeQuestFile(const std::filesystem::path& filePath, ContentView* contentView)
{
	m_currentRoe = std::make_unique<RecordsOfEminence>();
	m_currentRoe->ReadQuest(filePath);

	contentView->Clear();

	int maxCols = 0;
	for (const auto& datum : m_currentRoe->questData)
	{
		int cols = static_cast<int>(datum.row().GetCellsConst().size());
		if (cols > maxCols) maxCols = cols;
	}
	if (maxCols == 0) maxCols = 1;

	contentView->SetColumnCount(maxCols + 1 + 4 + 1);
	contentView->SetColumnTitle(0, L"ID");
	contentView->SetColumnWidth(0, 60);
	for (int col = 0; col < maxCols; ++col)
	{
		std::wstring colName = L"Field " + std::to_wstring(col);
		contentView->SetColumnTitle(col + 1, colName);
		contentView->SetColumnWidth(col +1 , 150);
	}
	contentView->SetColumnTitle(maxCols + 1, L"EXP");
	contentView->SetColumnWidth(maxCols + 1, 50);
	contentView->SetColumnTitle(maxCols + 2, L"UNI");
	contentView->SetColumnWidth(maxCols + 2, 50);
	contentView->SetColumnTitle(maxCols + 3, L"CAP");
	contentView->SetColumnWidth(maxCols + 3, 50);
	contentView->SetColumnTitle(maxCols + 4, L"EMI");
	contentView->SetColumnWidth(maxCols + 4, 50);
	contentView->SetColumnTitle(maxCols + 5, L"Release Date");
	contentView->SetColumnWidth(maxCols + 5, 150);


	if (maxCols > 3)
	{
		contentView->SetColumnWidth(2, 40);
	}
	else {
		contentView->SetColumnWidth(2, 300);
		contentView->SetColumnWidth(3, 300);
	}

	for (const auto& datum : m_currentRoe->questData)
	{
		auto item = std::make_unique<ContentItem>();
		item->type = ContentItemType::Multiline;

		bool hasMultilineText = false;
		int maxLines = 1;

		item->columns.push_back(ColumnData::MakeInteger(datum.id));

		for (const auto& cell : datum.row())
		{
			if (cell.GetType() == 0)
			{
				std::u8string cellStr = cell.Get<std::u8string>();
				std::wstring wstr = xybase::string::to_wstring(cellStr);
				item->columns.push_back(ColumnData::MakeMultilineText(wstr));

				if (ContainsLineBreaks(wstr))
				{
					hasMultilineText = true;
					int lineCount = CountLines(wstr);
					if (lineCount > maxLines) maxLines = lineCount;
				}
			}
			else if (cell.GetType() == 1)
			{
				int64_t value = cell.Get<int>();
				item->columns.push_back(ColumnData::MakeInteger(value));
			}
			else
			{
				item->columns.push_back(ColumnData::MakeText(L"[Binary]"));
			}
		}

		item->columns.push_back(ColumnData::MakeInteger(datum.originalEntry.exp_reward));
		item->columns.push_back(ColumnData::MakeInteger(datum.originalEntry.uni_reward));
		item->columns.push_back(ColumnData::MakeInteger(datum.originalEntry.cap_reward));
		item->columns.push_back(ColumnData::MakeInteger(datum.originalEntry.emi_reward));
		item->columns.push_back(ColumnData::MakeInteger(datum.originalEntry.release_date));

		if (hasMultilineText) item->customHeight = maxLines * 24;
		contentView->AddItem(std::move(item));
	}

	return true;
}

// ==================== RecordsOfEminence - Category (erc) ====================
bool DatFileManager::LoadRoeCategoryFile(const std::filesystem::path& filePath, ContentView* contentView)
{
	m_currentRoe = std::make_unique<RecordsOfEminence>();
	m_currentRoe->ReadCategory(filePath);

	contentView->Clear();
	contentView->SetColumnCount(2);
	contentView->SetColumnTitle(0, L"ID");
	contentView->SetColumnWidth(0, 60);
	contentView->SetColumnTitle(1, L"Category Name");
	contentView->SetColumnWidth(1, 500);

	int index = 0;
	for (const auto& datum : m_currentRoe->categoryData)
	{
		auto item = std::make_unique<ContentItem>();
		item->type = ContentItemType::Multiline;

		item->columns.push_back(ColumnData::MakeInteger(datum.id));

		std::wstring categoryName = L"";
		try {
			categoryName = xybase::string::to_wstring(datum.categoryName());
		}
		catch (...) {
			categoryName = L"<Error>";
		}
		item->columns.push_back(ColumnData::MakeMultilineText(categoryName));

		if (ContainsLineBreaks(categoryName))
		{
			item->customHeight = CountLines(categoryName) * 24;
		}

		contentView->AddItem(std::move(item));
	}

	return true;
}

bool DatFileManager::SaveCurrentFile(ContentView* contentView, const std::filesystem::path& filePath)
{
	if (!contentView)
		return false;

	try
	{
		// Determine which file type is currently loaded based on m_current* members
		if (m_currentDMsg)
		{
			// Update existing rows with data from ContentView
			size_t rowCount = min(contentView->GetItemCount(), m_currentDMsg->Count());
			
			for (size_t i = 0; i < rowCount; ++i)
			{
				const ContentItem* item = contentView->GetItem(i);
				if (!item) continue;
				
				Row& row = m_currentDMsg->operator[](i);
				
				// Skip first column (index), update remaining columns
				size_t cellCount = min(item->columns.size() - 1, row.GetCells().size());
				for (size_t col = 0; col < cellCount; ++col)
				{
					const ColumnData& colData = item->columns[col + 1]; // +1 to skip index column
					Cell& cell = row.GetCells()[col];
					
					if (cell.GetType() == 1) // Integer
					{
						if (colData.type == ColumnDataType::Integer)
						{
							cell.Set(static_cast<int>(colData.intValue));
						}
					}
					else if (cell.GetType() == 0) // String
					{
						if (colData.type == ColumnDataType::Text || colData.type == ColumnDataType::MultilineText)
						{
							std::u8string u8str = xybase::string::to_utf8(colData.textValue);
							cell.Set(u8str);
						}
					}
				}
			}
			
			m_currentDMsg->path = filePath;
			m_currentDMsg->Write();
			return true;
		}
		else if (m_currentXiString)
		{
			// Update existing entries with data from ContentView
			size_t entryCount = min(contentView->GetItemCount(), (size_t)std::distance(m_currentXiString->begin(), m_currentXiString->end()));
			
			auto it = m_currentXiString->begin();
			for (size_t i = 0; i < entryCount; ++i, ++it)
			{
				const ContentItem* item = contentView->GetItem(i);
				if (!item || item->columns.size() < 5) continue;
				
				// Column 1 is the string
				std::u8string u8str = xybase::string::to_utf8(item->columns[1].textValue);
				it->str = m_currentXiString->Encode(xybase::string::to_string(u8str));
				
				// Columns 2, 3, 4 are flags
				try {
					if (item->columns[2].type == ColumnDataType::Text)
						it->flag1 = static_cast<uint16_t>(std::stoi(xybase::string::to_string(xybase::string::to_utf8(item->columns[2].textValue))));
					if (item->columns[3].type == ColumnDataType::Text)
						it->flag2 = static_cast<uint16_t>(std::stoi(xybase::string::to_string(xybase::string::to_utf8(item->columns[3].textValue))));
					if (item->columns[4].type == ColumnDataType::Text)
						it->flag3 = static_cast<uint16_t>(std::stoi(xybase::string::to_string(xybase::string::to_utf8(item->columns[4].textValue))));
				} catch (...) {
					// Ignore parse errors for flags
				}
			}
			
			m_currentXiString->path = filePath;
			m_currentXiString->Write();
			return true;
		}
		else if (m_currentEventString)
		{
			// Update existing strings with data from ContentView
			size_t stringCount = min(contentView->GetItemCount(), m_currentEventString->Size());
			
			for (size_t i = 0; i < stringCount; ++i)
			{
				const ContentItem* item = contentView->GetItem(i);
				if (!item || item->columns.size() < 2) continue;
				
				std::u8string u8str = xybase::string::to_utf8(item->columns[1].textValue);
				(*m_currentEventString)[i] = u8str;
			}
			
			m_currentEventString->path = filePath;
			m_currentEventString->Write();
			return true;
		}
		else if (m_currentStatusData)
		{
			// Update existing data with data from ContentView
			size_t dataCount = min(contentView->GetItemCount(), m_currentStatusData->data.size());
			
			for (size_t i = 0; i < dataCount; ++i)
			{
				const ContentItem* item = contentView->GetItem(i);
				if (!item || item->columns.size() < 3) continue;
				
				StatusData::StatusDatum& datum = m_currentStatusData->data[i];
				
				// ID is in column 0
				if (item->columns[0].type == ColumnDataType::Integer)
					datum.id = static_cast<uint32_t>(item->columns[0].intValue);
				
				// Parse flag from hex string in column 1
				if (item->columns[1].type == ColumnDataType::Text)
				{
					try {
						std::wstring flagStr = item->columns[1].textValue;
						datum.flg = static_cast<uint16_t>(std::stoi(flagStr, nullptr, 16));
					} catch (...) {
						// Keep original flag on parse error
					}
				}
				
				// Description in column 2
				if (item->columns[2].type == ColumnDataType::Text || item->columns[2].type == ColumnDataType::MultilineText)
					datum.description = xybase::string::to_utf8(item->columns[2].textValue);
				
				// Image in column 3 (if exists) - but we preserve original image since it can't be edited
				// No action needed - image is preserved from original data
			}
			
			m_currentStatusData->Write(filePath.wstring());
			return true;
		}
		else if (m_currentItemData)
		{
			// Update existing item data with data from ContentView
			size_t itemCount = min(contentView->GetItemCount(), m_currentItemData->data.size());
			
			for (size_t i = 0; i < itemCount; ++i)
			{
				const ContentItem* item = contentView->GetItem(i);
				if (!item) continue;
				
				auto& datum = m_currentItemData->data[i];
				
			// Skip columns 0 (ID) and 1 (Icon), and stop before non-editable columns
			// Non-editable columns are the last 2 columns (Flags, Stack)
			size_t editableColCount = item->columns.size() >= 2 ? item->columns.size() - 2 : 0;
			size_t cellIndex = 0;
			for (size_t col = 2; col < editableColCount && cellIndex < datum.row().GetCells().size(); ++col, ++cellIndex)
				{
					const ColumnData& colData = item->columns[col];
					Cell& cell = datum.row().GetCells()[cellIndex];
					
					if (cell.GetType() == 0) // String type
					{
						if (colData.type == ColumnDataType::Text || colData.type == ColumnDataType::MultilineText)
						{
							std::u8string u8str = xybase::string::to_utf8(colData.textValue);
							cell.Set(u8str);
						}
					}
					else if (cell.GetType() == 1) // Integer type
					{
						if (colData.type == ColumnDataType::Integer)
						{
							cell.Set(static_cast<int>(colData.intValue));
						}
					}
				}
			}
			
			m_currentItemData->Write(filePath);
			return true;
		}
		else if (m_currentFixedPhrase)
		{
			// Update existing entries with data from ContentView
			size_t itemIndex = 0;
			for (auto& category : m_currentFixedPhrase->categories)
			{
				for (auto& entry : category.entries)
				{
					if (itemIndex >= contentView->GetItemCount()) break;
					
					const ContentItem* item = contentView->GetItem(itemIndex++);
					if (!item) continue;
					
					// Update entry text (column 1) and pronunciation (column 2)
					if (item->columns[1].type == ColumnDataType::Text || item->columns[1].type == ColumnDataType::MultilineText)
						entry.text = xybase::string::to_utf8(item->columns[1].textValue);
					
					if (item->columns[2].type == ColumnDataType::Text || item->columns[2].type == ColumnDataType::MultilineText)
						entry.pron = xybase::string::to_utf8(item->columns[2].textValue);
				}
			}
			
			m_currentFixedPhrase->Write(filePath.wstring());
			return true;
		}
		else if (m_currentMonBridge)
		{
			// Update existing data with data from ContentView
			size_t dataCount = min(contentView->GetItemCount(), m_currentMonBridge->data.size());
			
			for (size_t i = 0; i < dataCount; ++i)
			{
				const ContentItem* item = contentView->GetItem(i);
			if (!item || item->columns.size() < 4) continue; // ID, Icon, Internal, Display
				
				auto& datum = m_currentMonBridge->data[i];
				
			// Update internal name (column 2)
			if (item->columns[2].type == ColumnDataType::Text)
				datum.internalName = xybase::string::to_utf8(item->columns[2].textValue);
				
			// Update display name (column 3)
			if (item->columns[3].type == ColumnDataType::Text || item->columns[3].type == ColumnDataType::MultilineText)
				datum.displayName = xybase::string::to_utf8(item->columns[3].textValue);
			}
			
			m_currentMonBridge->Write(filePath.wstring());
			return true;
		}
		else if (m_currentRoe)
		{
			// Collect data from ContentView back to RecordsOfEminence
			if (!m_currentRoe->questData.empty())
			{
				// It's a quest file - update existing quest data
				size_t questCount = min(contentView->GetItemCount(), m_currentRoe->questData.size());
				
				for (size_t i = 0; i < questCount; ++i)
				{
					const ContentItem* item = contentView->GetItem(i);
					if (!item) continue;
					
					auto& datum = m_currentRoe->questData[i];
					
					// Update cells in the row
					size_t cellIndex = 0;
					for (size_t col = 1; col < item->columns.size() && cellIndex < datum.row().GetCells().size(); ++col, ++cellIndex)
					{
						const ColumnData& colData = item->columns[col];
						Cell& cell = datum.row().GetCells()[cellIndex];
						
						if (cell.GetType() == 0) // String type
						{
							if (colData.type == ColumnDataType::Text || colData.type == ColumnDataType::MultilineText)
							{
								std::u8string u8str = xybase::string::to_utf8(colData.textValue);
								cell.Set(u8str);
							}
						}
						else if (cell.GetType() == 1) // Integer type
						{
							if (colData.type == ColumnDataType::Integer)
							{
								cell.Set(static_cast<int>(colData.intValue));
							}
						}
					}
				}
				
				m_currentRoe->WriteQuest(filePath.wstring());
				return true;
			}
			else if (!m_currentRoe->categoryData.empty())
			{
				// It's a category file - update existing category data
				size_t catCount = min(contentView->GetItemCount(), m_currentRoe->categoryData.size());
				
				for (size_t i = 0; i < catCount; ++i)
				{
					const ContentItem* item = contentView->GetItem(i);
					if (!item || item->columns.size() < 2) continue;
					
					auto& datum = m_currentRoe->categoryData[i];
					
					// Update category name (column 1)
					if (item->columns[1].type == ColumnDataType::Text || item->columns[1].type == ColumnDataType::MultilineText)
					{
						std::u8string categoryName = xybase::string::to_utf8(item->columns[1].textValue);
						datum.setCategoryName(categoryName);
					}
				}
				
				m_currentRoe->WriteCategory(filePath.wstring());
				return true;
			}
		}
		
		// Unknown or unsupported file type
		return false;
	}
	catch (const std::exception&)
	{
		return false;
	}
}