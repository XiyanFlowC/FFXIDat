#include "DatFileManager.h"
#include "ContentView.h"
#include <sstream>
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

void DatFileManager::LoadROMDefinition(const std::filesystem::path& csvPath,
									   HWND hTreeView, HTREEITEM parentNode, bool cateSub)
{
	// Extract ROM folder name from CSV filename
	// ROM.csv -> "ROM"
	// ROM2.csv -> "ROM2"
	// ROM3.csv -> "ROM3"
	std::string csvName = csvPath.stem().string();
	std::string romFolder = csvName;  // Assume filename without extension is the ROM folder
	
	CsvFile csv(csvPath, std::ios::in | std::ios::binary);
	
	// Map to track category hierarchy nodes when cateSub is enabled
	std::map<std::string, HTREEITEM> categoryNodes;
	
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
			info.fileId = fileId;
			info.fileType = xybase::string::to_string(fileType);
			info.language = xybase::string::to_string(language);
			info.category = xybase::string::to_string(category);
			info.romFolder = romFolder;  // Set from filename, not from CSV
			
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
			
			m_fileRegistry[fileId] = info;
			
			// Determine parent node for this file
			HTREEITEM hParentNode = parentNode;
			
			// If cateSub is enabled and category is not empty, create/use category hierarchy
			if (cateSub && !info.category.empty())
			{
				// Split category by "/" to create hierarchy
				// e.g., "sys/mis/sd" -> creates nodes: sys -> mis -> sd
				std::string categoryPath = info.category;
				std::string currentPath;
				size_t lastPos = 0;
				size_t pos = 0;
				
				while ((pos = categoryPath.find('/', lastPos)) != std::string::npos)
				{
					std::string pathPart = categoryPath.substr(lastPos, pos - lastPos);
					if (!pathPart.empty())
					{
						// Build the full path for this level
						if (!currentPath.empty())
							currentPath += "/" + pathPart;
						else
							currentPath = pathPart;
						
						// Check if this node already exists
						if (categoryNodes.find(currentPath) == categoryNodes.end())
						{
							// Create new category node
							TVINSERTSTRUCTW tvis = { 0 };
							tvis.hParent = (currentPath == pathPart) ? parentNode : categoryNodes[currentPath.substr(0, currentPath.rfind('/'))];
							tvis.hInsertAfter = TVI_LAST;
							tvis.item.mask = TVIF_TEXT;
							
							std::wstring wCategoryName(pathPart.begin(), pathPart.end());
							tvis.item.pszText = const_cast<LPWSTR>(wCategoryName.c_str());
							
							HTREEITEM hCategoryNode = TreeView_InsertItem(hTreeView, &tvis);
							categoryNodes[currentPath] = hCategoryNode;
						}
						
						hParentNode = categoryNodes[currentPath];
					}
					lastPos = pos + 1;
				}
				
				//// Handle the last part of the path
				//std::string lastPart = categoryPath.substr(lastPos);
				//if (!lastPart.empty())
				//{
				//	if (!currentPath.empty())
				//		currentPath += "/" + lastPart;
				//	else
				//		currentPath = lastPart;
				//	
				//	if (categoryNodes.find(currentPath) == categoryNodes.end())
				//	{
				//		TVINSERTSTRUCTW tvis = { 0 };
				//		tvis.hParent = (currentPath == lastPart) ? parentNode : categoryNodes[categoryPath.substr(0, categoryPath.rfind('/'))];
				//		tvis.hInsertAfter = TVI_LAST;
				//		tvis.item.mask = TVIF_TEXT;
				//		
				//		std::wstring wCategoryName(lastPart.begin(), lastPart.end());
				//		tvis.item.pszText = const_cast<LPWSTR>(wCategoryName.c_str());
				//		
				//		HTREEITEM hCategoryNode = TreeView_InsertItem(hTreeView, &tvis);
				//		categoryNodes[currentPath] = hCategoryNode;
				//	}
				//	
				//	hParentNode = categoryNodes[currentPath];
				//}
			}
			
			// Add file item to tree under appropriate parent
			TVINSERTSTRUCTW tvis = { 0 };
			tvis.hParent = hParentNode;
			tvis.hInsertAfter = TVI_LAST;
			tvis.item.mask = TVIF_TEXT | TVIF_PARAM;
			
			std::wstring wName(friendlyName.begin(), friendlyName.end());
			tvis.item.pszText = const_cast<LPWSTR>(wName.c_str());
			tvis.item.lParam = fileId;
			
			HTREEITEM hItem = TreeView_InsertItem(hTreeView, &tvis);
			m_treeItemToFileId[hItem] = fileId;
		}
		catch (const std::exception&)
		{
			continue;
		}
	}
	
	csv.Close();
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
	
	// Load ROM.csv, ROM2.csv, ROM3.csv, etc.
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
	
	// Load each ROM file
	for (const auto& [romName, csvPath] : romFiles)
	{
		if (std::filesystem::exists(csvPath))
		{
			std::wstring wRomName = xybase::string::to_wstring(romName);
			tvis.hParent = hRoot;
			tvis.item.pszText = const_cast<LPWSTR>(wRomName.c_str());
			HTREEITEM hRomNode = TreeView_InsertItem(hTreeView, &tvis);

			// ╪сть CSV нд╪Ч
			LoadROMDefinition(csvPath, hTreeView, hRomNode, cateSub);
		}
	}
	
	TreeView_Expand(hTreeView, hRoot, TVE_EXPAND);
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
	
	std::filesystem::path filePath = GetDatFilePath(info.fileId, info.romFolder);
	
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
	contentView->SetColumnTitle(3, L"Flag2");
	contentView->SetColumnTitle(4, L"Flag3");
	
	// Add items
	int index = 0;
	for (const auto& entry : *m_currentXiString)
	{
		auto item = std::make_unique<ContentItem>();
		
		std::wstring wstr = xybase::string::to_wstring(entry.str);
		
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


	contentView->SetColumnCount(maxCols + 2);
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
	contentView->SetColumnCount(3);
	contentView->SetColumnTitle(0, L"ID");
	contentView->SetColumnWidth(0, 60);
	contentView->SetColumnTitle(1, L"Internal Name");
	contentView->SetColumnWidth(1, 150);
	contentView->SetColumnTitle(2, L"Display Name");
	contentView->SetColumnWidth(2, 350);

	for (const auto& datum : m_currentMonBridge->data)
	{
		auto item = std::make_unique<ContentItem>();
		item->type = ContentItemType::Multiline;

		item->columns.push_back(ColumnData::MakeInteger(datum.id));
		std::wstring internalName = xybase::string::to_wstring(datum.internalName);
		item->columns.push_back(ColumnData::MakeText(internalName));
		std::wstring displayName = xybase::string::to_wstring(datum.displayName);
		item->columns.push_back(ColumnData::MakeMultilineText(displayName));

		if (ContainsLineBreaks(displayName))
		{
			item->customHeight = CountLines(displayName) * 24;
		}

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

	contentView->SetColumnCount(maxCols);
	for (int col = 0; col < maxCols; ++col)
	{
		std::wstring colName = L"Field " + std::to_wstring(col);
		contentView->SetColumnTitle(col, colName);
		contentView->SetColumnWidth(col, 150);
	}

	if (maxCols > 3)
	{
		contentView->SetColumnWidth(1, 40);
	}
	else {
		contentView->SetColumnWidth(1, 300);
		contentView->SetColumnWidth(2, 300);
	}

	for (const auto& datum : m_currentRoe->questData)
	{
		auto item = std::make_unique<ContentItem>();
		item->type = ContentItemType::Multiline;

		bool hasMultilineText = false;
		int maxLines = 1;

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

		item->columns.push_back(ColumnData::MakeInteger(index++));

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
