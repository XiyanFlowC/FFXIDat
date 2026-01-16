#include "MainFrame.h"
#include "FFXIDatEGApp.h"
#include "Localization.h"
#include "Config.h"
#include <windowsx.h>
#include <shobjidl.h>
#include <commdlg.h>
#include <string>
#include <xystring.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")

// Helper macro to convert localized string to LPCWSTR
#define LOCS(key) LOC(key).c_str()

MainFrame::MainFrame()
{
}

MainFrame::~MainFrame()
{
}

bool MainFrame::Create(HINSTANCE hInstance)
{
	// Register window class
	WNDCLASSEXW wc = { 0 };
	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszClassName = L"FFXIDatEGMainFrame";
	wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
	
	if (!RegisterClassExW(&wc))
		return false;
	
	// Create main window
	m_hwnd = CreateWindowExW(
		0,
		L"FFXIDatEGMainFrame",
		L"FFXI DAT Viewer",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		1200, 800,
		nullptr,
		nullptr,
		hInstance,
		this
	);
	
	if (!m_hwnd)
		return false;
	
	return true;
}

void MainFrame::Show()
{
	ShowWindow(m_hwnd, SW_SHOW);
	UpdateWindow(m_hwnd);
}

LRESULT CALLBACK MainFrame::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	MainFrame* pThis = nullptr;
	
	if (uMsg == WM_NCCREATE)
	{
		CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
		pThis = reinterpret_cast<MainFrame*>(pCreate->lpCreateParams);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
		pThis->m_hwnd = hwnd;
	}
	else
	{
		pThis = reinterpret_cast<MainFrame*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	}
	
	if (pThis)
	{
		switch (uMsg)
		{
		case WM_CREATE:
			pThis->OnCreate();
			return 0;
			
		case WM_SIZE:
			pThis->OnSize(LOWORD(lParam), HIWORD(lParam));
			return 0;
			
		case WM_COMMAND:
			pThis->OnCommand(wParam);
			return 0;
			
		case WM_NOTIFY:
		{
			LPNMHDR pnmh = reinterpret_cast<LPNMHDR>(lParam);
			if (pnmh->idFrom == IDC_TREEVIEW && pnmh->code == NM_DBLCLK)
			{
				HTREEITEM hItem = TreeView_GetSelection(pThis->m_hTreeView);
				if (hItem)
					pThis->OnTreeItemActivated(hItem);
			}
			return 0;
		}
		
		case WM_KEYDOWN:
			if (wParam == VK_F3)
			{
				pThis->OnFindNext();
				return 0;
			}
			break;
		
		case WM_CHAR:
			if (wParam == 6 && GetKeyState(VK_CONTROL) < 0)  // Ctrl+F
			{
				pThis->OnFind();
				return 0;
			}
			break;
		
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		}
	}
	
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void MainFrame::OnCreate()
{
	// Initialize UI language from config
	m_uiLanguage = FFXIDatEGApp::Instance().GetConfig().GetUILanguage();
	
	// Initialize category hierarchy setting from config
	m_enableCategoryHierarchy = FFXIDatEGApp::Instance().GetConfig().GetEnableCategoryHierarchy();
	
	// Get exe path and set local directory
	wchar_t exePath[MAX_PATH];
	GetModuleFileNameW(nullptr, exePath, MAX_PATH);
	std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
	m_localDir = exeDir / L"local";
	
	// Scan available UI languages
	Localization& loc = Localization::Instance();
	m_availableUILanguages = loc.ScanAvailableLanguages(m_localDir);
	
	// Create menu bar
	m_hMenu = CreateMenu();

	// File menu
	HMENU hFileMenu = CreateMenu();
	AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_SAVE, LOCS(L"menu_file_save"));
	AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_SAVEAS, LOCS(L"menu_file_saveas"));
	AppendMenuW(hFileMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_CHANGEPATH, LOCS(L"menu_file_changepath"));
	AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_RESETPATH, LOCS(L"menu_file_resetpath"));
	AppendMenuW(hFileMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_CLEANPREFS, LOCS(L"menu_help_cleanprefs"));
	AppendMenuW(hFileMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_EXIT, LOCS(L"menu_file_exit"));

	// Edit menu
	HMENU hEditMenu = CreateMenu();
	AppendMenuW(hEditMenu, MF_STRING, IDM_EDIT_FIND, LOCS(L"menu_edit_find"));
	AppendMenuW(hEditMenu, MF_STRING, IDM_EDIT_FINDNEXT, LOCS(L"menu_edit_findnext"));

	// View menu - with Language Filter submenu
	HMENU hViewMenu = CreateMenu();
	AppendMenuW(hViewMenu, MF_STRING, IDM_VIEW_FONT, LOCS(L"menu_view_font"));
	AppendMenuW(hViewMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenuW(hViewMenu, MF_STRING, IDM_VIEW_ENABLE_CATEGORY_HIERARCHY, LOCS(L"menu_view_enable_category_hierarchy"));
	CheckMenuItem(hViewMenu, IDM_VIEW_ENABLE_CATEGORY_HIERARCHY,
		m_enableCategoryHierarchy ? MF_CHECKED : MF_UNCHECKED);
	AppendMenuW(hViewMenu, MF_SEPARATOR, 0, nullptr);

	// Language Filter submenu
	m_hLanguageFilterMenu = CreateMenu();
	AppendMenuW(m_hLanguageFilterMenu, MF_STRING, IDM_VIEW_FILTER_ALL, LOCS(L"menu_view_filter_all"));
	AppendMenuW(m_hLanguageFilterMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenuW(m_hLanguageFilterMenu, MF_STRING, IDM_VIEW_FILTER_JP, LOCS(L"menu_view_filter_jp"));
	AppendMenuW(m_hLanguageFilterMenu, MF_STRING, IDM_VIEW_FILTER_EN, LOCS(L"menu_view_filter_en"));
	AppendMenuW(m_hLanguageFilterMenu, MF_STRING, IDM_VIEW_FILTER_FR, LOCS(L"menu_view_filter_fr"));
	AppendMenuW(m_hLanguageFilterMenu, MF_STRING, IDM_VIEW_FILTER_DE, LOCS(L"menu_view_filter_de"));

	// Set default checked item (All Languages)
	CheckMenuItem(m_hLanguageFilterMenu, IDM_VIEW_FILTER_ALL, MF_CHECKED);

	AppendMenuW(hViewMenu, MF_POPUP, (UINT_PTR)m_hLanguageFilterMenu, LOCS(L"menu_view_langfilter"));

	// UI Language submenu - Build dynamically
	m_hUILanguageMenu = CreateMenu();
	BuildUILanguageMenu(m_hUILanguageMenu);
	AppendMenuW(hViewMenu, MF_POPUP, (UINT_PTR)m_hUILanguageMenu, LOCS(L"menu_view_uilang"));

	// Help menu
	HMENU hHelpMenu = CreateMenu();
	AppendMenuW(hHelpMenu, MF_STRING, IDM_HELP_QUICKSTART, LOCS(L"menu_help_quickstart"));
	AppendMenuW(hHelpMenu, MF_STRING, IDM_HELP_ABOUT, LOCS(L"menu_help_about"));

	// Add to menu bar
	AppendMenuW(m_hMenu, MF_POPUP, (UINT_PTR)hFileMenu, LOCS(L"menu_file"));
	AppendMenuW(m_hMenu, MF_POPUP, (UINT_PTR)hEditMenu, LOCS(L"menu_edit"));
	AppendMenuW(m_hMenu, MF_POPUP, (UINT_PTR)hViewMenu, LOCS(L"menu_view"));
	AppendMenuW(m_hMenu, MF_POPUP, (UINT_PTR)hHelpMenu, LOCS(L"menu_help"));

	SetMenu(m_hwnd, m_hMenu);

	// Create TreeView (left panel)
	m_hTreeView = CreateWindowExW(
		WS_EX_CLIENTEDGE,
		WC_TREEVIEWW,
		nullptr,
		WS_CHILD | WS_VISIBLE | WS_BORDER | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT,
		0, 0, 250, 600,
		m_hwnd,
		reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_TREEVIEW)),
		FFXIDatEGApp::Instance().GetInstance(),
		nullptr
	);

	// Create ContentView (right panel)
	m_contentView = std::make_unique<ContentView>();
	m_contentView->Create(m_hwnd, 250, 0, 950, 600);
	
	// Load saved font from config
	std::wstring savedFont = FFXIDatEGApp::Instance().GetConfig().GetFontName();
	m_contentView->SetFontName(savedFont);
	m_contentView->SetFontSize(FFXIDatEGApp::Instance().GetConfig().GetInt(L"Font", L"Size", 16));

	// Create Status Bar
	m_hStatusBar = CreateWindowExW(
		0,
		STATUSCLASSNAMEW,
		nullptr,
		WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
		0, 0, 0, 0,
		m_hwnd,
		reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_STATUSBAR)),
		FFXIDatEGApp::Instance().GetInstance(),
		nullptr
	);

	SendMessageW(m_hStatusBar, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(LOC(L"Status.Ready").c_str()));

	// Initialize file manager
	m_fileManager = std::make_unique<DatFileManager>(
		FFXIDatEGApp::Instance().GetGamePath());

	// Initialize search dialog
	m_searchDialog = std::make_unique<SearchDialog>();
	m_searchDialog->Create(m_hwnd);

	LoadROMDefinitions();
}

void MainFrame::OnSize(int width, int height)
{
	if (m_hStatusBar)
	{
		SendMessage(m_hStatusBar, WM_SIZE, 0, 0);
	}
	
	RECT rcStatus;
	GetWindowRect(m_hStatusBar, &rcStatus);
	int statusHeight = rcStatus.bottom - rcStatus.top;
	
	int contentHeight = height - statusHeight;
	
	if (m_hTreeView)
	{
		SetWindowPos(m_hTreeView, nullptr, 
					0, 0, 250, contentHeight,
					SWP_NOZORDER);
	}
	
	if (m_contentView && m_contentView->GetHandle())
	{
		SetWindowPos(m_contentView->GetHandle(), nullptr,
					250, 0, width - 250, contentHeight,
					SWP_NOZORDER);
	}
}

void MainFrame::OnTreeItemActivated(HTREEITEM hItem)
{
	if (!hItem)
		return;
	
	const DatFileInfo* info = m_fileManager->GetFileInfo(hItem);
	if (!info)
		return;
	
	// Check if this item is a folder (has children) by checking if it has child items
	// Leaf items (actual files) should not have children in the tree
	HTREEITEM hFirstChild = TreeView_GetChild(m_hTreeView, hItem);
	if (hFirstChild != nullptr)
	{
		return;
	}

	if (m_contentView->IsModified())
	{
		if (!CheckAndPromptSave())
		{
			// User cancelled loading new file
			return;
		}
	}
	
	std::wstring statusText = LOC(L"Status.Loading");
	statusText += L" ";
	std::string nameUtf8 = info->friendlyName;
	auto name = xybase::string::to_wstring((char8_t *) nameUtf8.c_str());
	statusText += name;
	statusText += L"...";
	SendMessageW(m_hStatusBar, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(statusText.c_str()));
	
	if (m_fileManager->LoadDatFile(*info, m_contentView.get()))
	{
		statusText = LOC(L"Status.Loaded");
		statusText += L": ";
		statusText += name;
		statusText += L" (" + m_fileManager->GetDatFilePath(info->fileId, info->romFolder).wstring() + L")";
		SendMessageW(m_hStatusBar, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(statusText.c_str()));
		
		// Store current file path for save operation
		m_currentFilePath = m_fileManager->GetDatFilePath(info->fileId, info->romFolder);
	}
	else
	{
		SendMessageW(m_hStatusBar, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(LOC(L"Status.LoadFailed").c_str()));
		m_currentFilePath.clear();
	}
}

void MainFrame::LoadROMDefinitions()
{
	// Set language filter
	m_fileManager->SetLanguageFilter(m_languageFilter);

	// Get exe path
	wchar_t exePath[MAX_PATH];
	GetModuleFileNameW(nullptr, exePath, MAX_PATH);
	std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
	
	// Load all ROM definition files (ROM.csv, ROM2.csv, ROM3.csv, etc.)
	m_fileManager->LoadAllROMDefinitions(exeDir, m_hTreeView, m_enableCategoryHierarchy);
}

void MainFrame::OnCommand(WPARAM wParam)
{
	// Check if message is from search dialog
	if (m_searchDialog && LOWORD(wParam) == 1005)  // IDC_BTN_FINDNEXT from SearchDialog
	{
		OnFindNext();
		return;
	}

	switch (LOWORD(wParam))
	{
	case IDM_FILE_SAVE:
		OnSave();
		break;
	case IDM_FILE_SAVEAS:
		OnSaveAs();
		break;
	case IDM_FILE_CHANGEPATH:
		OnChangeGamePath();
		break;
	case IDM_FILE_RESETPATH:
		OnResetGamePath();
		break;
	case IDM_FILE_EXIT:
		PostMessage(m_hwnd, WM_CLOSE, 0, 0);
		break;
	case IDM_EDIT_FIND:
		OnFind();
		break;
	case IDM_EDIT_FINDNEXT:
		OnFindNext();
		break;
	case IDM_VIEW_FONT:
		OnSelectFont();
		break;
	case IDM_VIEW_ENABLE_CATEGORY_HIERARCHY:
		OnToggleCategoryHierarchy();
		break;
	case IDM_VIEW_FILTER_ALL:
		OnFilterLanguage("");
		break;
	case IDM_VIEW_FILTER_JP:
		OnFilterLanguage("jp");
		break;
	case IDM_VIEW_FILTER_EN:
		OnFilterLanguage("en");
		break;
	case IDM_VIEW_FILTER_FR:
		OnFilterLanguage("fr");
		break;
	case IDM_VIEW_FILTER_DE:
		OnFilterLanguage("de");
		break;
	case IDM_HELP_QUICKSTART:
		OnQuickStartHelp();
		break;
	case IDM_HELP_ABOUT:
		OnAbout();
		break;
	case IDM_FILE_CLEANPREFS:
		OnCleanPreferencesAndExit();
		break;
	default:
		// Handle dynamic UI language menu items
		if (LOWORD(wParam) >= IDM_VIEW_UILANG_BASE && 
			LOWORD(wParam) < IDM_VIEW_UILANG_BASE + 10)
		{
			size_t langIndex = LOWORD(wParam) - IDM_VIEW_UILANG_BASE;
			if (langIndex < m_availableUILanguages.size())
			{
				OnChangeUILanguage(m_availableUILanguages[langIndex].code);
			}
		}
		break;
	}
}

void MainFrame::OnChangeGamePath()
{
	FFXIDatEGApp& app = FFXIDatEGApp::Instance();
	
	// Initialize COM for this thread if needed
	HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	bool comInitialized = SUCCEEDED(hr);
	
	IFileOpenDialog* pFileDialog = nullptr;
	hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL,
						 IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileDialog));
	
	if (SUCCEEDED(hr))
	{
		// Set options for folder picker
		DWORD dwOptions;
		if (SUCCEEDED(pFileDialog->GetOptions(&dwOptions)))
		{
			pFileDialog->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_PATHMUSTEXIST);
		}
		
		pFileDialog->SetTitle(LOCS(L"select_ffxi_dir"));
		
		// Show the dialog
		hr = pFileDialog->Show(m_hwnd);
		if (SUCCEEDED(hr))
		{
			IShellItem* pItem = nullptr;
			hr = pFileDialog->GetResult(&pItem);
			if (SUCCEEDED(hr))
			{
				PWSTR pszPath = nullptr;
				hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
				if (SUCCEEDED(hr))
				{
					std::filesystem::path selectedPath(pszPath);
					
					// Verify it's a valid FFXI installation
					if (!std::filesystem::exists(selectedPath / "ROM"))
					{
						MessageBoxW(m_hwnd,
								   LOCS(L"invalid_ffxi_dir_msg"),
								   LOCS(L"error_msg_title"), MB_OK | MB_ICONERROR);
						CoTaskMemFree(pszPath);
						pItem->Release();
						pFileDialog->Release();
						if (comInitialized && hr != RPC_E_CHANGED_MODE) CoUninitialize();
						return;
					}
					
					app.SetGamePath(selectedPath);
					
					// Reload file manager
					m_fileManager = std::make_unique<DatFileManager>(selectedPath);
					
					// Clear and reload tree
					TreeView_DeleteAllItems(m_hTreeView);
					LoadROMDefinitions();
					
					SendMessageW(m_hStatusBar, SB_SETTEXTW, 0, 
							reinterpret_cast<LPARAM>(LOC(L"Status.GamePathChanged").c_str()));
					
					CoTaskMemFree(pszPath);
				}
				pItem->Release();
			}
		}
		pFileDialog->Release();
	}
	
	if (comInitialized && hr != RPC_E_CHANGED_MODE)
	{
		CoUninitialize();
	}
}

void MainFrame::OnSelectFont()
{
	CHOOSEFONTW cf = { 0 };
	LOGFONTW lf = { 0 };
	
	cf.lStructSize = sizeof(CHOOSEFONTW);
	cf.hwndOwner = m_hwnd;
	cf.lpLogFont = &lf;
	cf.Flags = CF_SCREENFONTS | CF_EFFECTS | CF_INITTOLOGFONTSTRUCT;
	cf.nFontType = SCREEN_FONTTYPE;
	
	// Set current font
	std::wstring currentFont = m_contentView->GetFontName();
	int fontSize = m_contentView->GetFontSize();
	wcscpy_s(lf.lfFaceName, LF_FACESIZE, currentFont.c_str());
	lf.lfHeight = -MulDiv(fontSize, GetDeviceCaps(GetDC(m_hwnd), LOGPIXELSY), 72);
	lf.lfCharSet = SHIFTJIS_CHARSET;
	
	if (ChooseFontW(&cf))
	{
		// User selected a font
		std::wstring newFontName = lf.lfFaceName;
		m_contentView->SetFontName(newFontName);
		m_contentView->SetFontSize(
			abs(MulDiv(lf.lfHeight, 72, GetDeviceCaps(GetDC(m_hwnd), LOGPIXELSY))));
		
		// Save font to config
		FFXIDatEGApp::Instance().GetConfig().SetFontName(newFontName);
		FFXIDatEGApp::Instance().GetConfig().SetInt(L"Font", L"Size", 
			m_contentView->GetFontSize());
		
		std::wstring statusText = L"Font changed to: ";
		statusText += newFontName;
		SendMessageW(m_hStatusBar, SB_SETTEXTW, 0,
					reinterpret_cast<LPARAM>(statusText.c_str()));
	}
}

void MainFrame::OnAbout()
{
	std::wstring message = 
		L"FFXI DAT Viewer\n\n"
		L"Version: 1.0\n\n"
		L"A tool for viewing and editing(not yet) FFXI DAT files.\n\n"
		L"Built with Win32 API\n"
		L"https://github.com/XiyanFlowC/FFXIDat";
	
	MessageBoxW(m_hwnd, message.c_str(), L"About", MB_OK | MB_ICONINFORMATION);
}

void MainFrame::OnQuickStartHelp()
{
	std::wstring title = LOC(L"quickstart_title");
	std::wstring message = LOC(L"quickstart_message");
	
	MessageBoxW(m_hwnd, message.c_str(), title.c_str(), MB_OK | MB_ICONINFORMATION);
}

void MainFrame::OnToggleCategoryHierarchy()
{
	m_enableCategoryHierarchy = !m_enableCategoryHierarchy;
	
	// Update menu check state
	UINT checkState = m_enableCategoryHierarchy ? MF_CHECKED : MF_UNCHECKED;
	CheckMenuItem(m_hMenu, IDM_VIEW_ENABLE_CATEGORY_HIERARCHY, checkState);
	
	// Save to config
	FFXIDatEGApp::Instance().GetConfig().SetEnableCategoryHierarchy(m_enableCategoryHierarchy);
	
	// Reload tree view
	TreeView_DeleteAllItems(m_hTreeView);
	LoadROMDefinitions();
}

void MainFrame::OnCleanPreferencesAndExit()
{
	// Confirm with user
	std::wstring confirmTitle = LOC(L"cleanprefs_confirm_title");
	std::wstring confirmMessage = LOC(L"cleanprefs_confirm_message");
	
	int result = MessageBoxW(m_hwnd, confirmMessage.c_str(), confirmTitle.c_str(), 
							MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
	
	if (result != IDYES)
		return;
	
	// Get config path from app
	FFXIDatEGApp& app = FFXIDatEGApp::Instance();
	std::filesystem::path configPath = app.GetConfigPath();
	
	try
	{
		// Delete config file if exists
		if (std::filesystem::exists(configPath))
		{
			std::filesystem::remove(configPath);
		}
		
		// Delete config directory if exists and is empty
		std::filesystem::path configDir = configPath.parent_path();
		if (std::filesystem::exists(configDir) && std::filesystem::is_empty(configDir))
		{
			std::filesystem::remove(configDir);
		}
		
		// Show success message
		std::wstring successMsg = LOC(L"cleanprefs_success");
		MessageBoxW(m_hwnd, successMsg.c_str(), confirmTitle.c_str(), MB_OK | MB_ICONINFORMATION);
		
		// Exit application
		PostMessage(m_hwnd, WM_CLOSE, 0, 0);
	}
	catch (const std::exception& e)
	{
		// Show error message
		std::wstring failedMsg = LOC(L"cleanprefs_failed");
		std::string errStr = e.what();
		std::wstring errMsg = failedMsg + std::wstring(errStr.begin(), errStr.end());
		MessageBoxW(m_hwnd, errMsg.c_str(), confirmTitle.c_str(), MB_OK | MB_ICONERROR);
	}
}

void MainFrame::OnFind()
{
	if (!m_searchDialog)
		return;
	
	// Show the search dialog
	m_searchDialog->Show();
}

void MainFrame::OnFindNext()
{
	if (!m_searchDialog || !m_contentView)
		return;
	
	std::wstring searchText = m_searchDialog->GetSearchText();
	if (searchText.empty())
	{
		OnFind();
		return;
	}
	
	bool found = m_contentView->SearchText(
		searchText,
		m_searchDialog->IsCaseSensitive(),
		m_searchDialog->IsSearchDown(),
		true  // findNext = true
	);
	
	if (!found)
	{
		std::wstring msg = LOC(L"Status.TextNotFound");
		msg += L": \"" + searchText + L"\"";
		MessageBoxW(m_hwnd, msg.c_str(), LOC(L"Search.Dialog.Title").c_str(), MB_OK | MB_ICONINFORMATION);
	}
	else
	{
		std::wstring statusMsg = LOC(L"Status.Found");
		statusMsg += L": \"" + searchText + L"\"";
		SendMessageW(m_hStatusBar, SB_SETTEXTW, 0, 
					reinterpret_cast<LPARAM>(statusMsg.c_str()));
	}
}
bool MainFrame::CheckAndPromptSave()
{
	// Check if content view has modifications
	if (!m_contentView || !m_contentView->IsModified())
		return true;  // No modifications, proceed
	
	// Ask user if they want to save changes
	std::wstring message = LOC(L"save_changes_prompt");
	if (message.empty())
		message = L"Do you want to save changes to the current file?";
	
	std::wstring title = LOC(L"save_changes_title");
	if (title.empty())
		title = L"Save Changes";
	
	int result = MessageBoxW(m_hwnd, message.c_str(), title.c_str(), 
							MB_YESNOCANCEL | MB_ICONQUESTION);
	
	if (result == IDCANCEL)
		return false;  // User cancelled, don't proceed
	
	if (result == IDYES)
	{
		// User wants to save, trigger save
		OnSave();
		return true;  // Proceed after save
	}
	
	// User chose "No", discard changes and proceed
	return true;
}

void MainFrame::OnSave()
{
	if (!m_contentView || !m_fileManager)
		return;
	
	// Check if we have a current file path
	if (m_currentFilePath.empty())
	{
		// No file currently open, use Save As instead
		OnSaveAs();
		return;
	}
	
	try
	{
		// Collect data from ContentView back to file manager
		if (!m_fileManager->SaveCurrentFile(m_contentView.get(), m_currentFilePath))
		{
			std::wstring msg = LOC(L"save_failed_msg");
			if (msg.empty())
				msg = L"Failed to save file.";
			MessageBoxW(m_hwnd, msg.c_str(), L"Error", MB_OK | MB_ICONERROR);
			return;
		}
		
		// Mark as unmodified
		m_contentView->SetModified(false);
		
		// Update status bar
		std::wstring statusMsg = LOC(L"save_success_msg");
		if (statusMsg.empty())
			statusMsg = L"File saved successfully.";
		SendMessageW(m_hStatusBar, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(statusMsg.c_str()));
	}
	catch (const std::exception& e)
	{
		std::string errStr = e.what();
		std::wstring errMsg = L"Error saving file: ";
		errMsg += std::wstring(errStr.begin(), errStr.end());
		MessageBoxW(m_hwnd, errMsg.c_str(), L"Error", MB_OK | MB_ICONERROR);
	}
}

void MainFrame::OnSaveAs()
{
	if (!m_contentView || !m_fileManager)
		return;
	
	// Initialize COM for this thread if needed
	HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	bool comInitialized = SUCCEEDED(hr);
	
	IFileSaveDialog* pFileSaveDialog = nullptr;
	hr = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_ALL,
						 IID_IFileSaveDialog, reinterpret_cast<void**>(&pFileSaveDialog));
	
	if (SUCCEEDED(hr))
	{
		// Set file type filters
		COMDLG_FILTERSPEC rgSpec[] = {
			{ L"DAT Files", L"*.DAT" },
			{ L"All Files", L"*.*" }
		};
		pFileSaveDialog->SetFileTypes(ARRAYSIZE(rgSpec), rgSpec);
		pFileSaveDialog->SetFileTypeIndex(1);
		pFileSaveDialog->SetDefaultExtension(L"DAT");
		
		// Set title
		std::wstring title = LOC(L"save_as_title");
		if (title.empty())
			title = L"Save File As";
		pFileSaveDialog->SetTitle(title.c_str());
		
		// Set default filename if we have a current file
		if (!m_currentFilePath.empty())
		{
			pFileSaveDialog->SetFileName(m_currentFilePath.filename().c_str());
		}
		
		// Show the dialog
		hr = pFileSaveDialog->Show(m_hwnd);
		if (SUCCEEDED(hr))
		{
			IShellItem* pItem = nullptr;
			hr = pFileSaveDialog->GetResult(&pItem);
			if (SUCCEEDED(hr))
			{
				PWSTR pszPath = nullptr;
				hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
				if (SUCCEEDED(hr))
				{
					std::filesystem::path savePath(pszPath);
					
					try
					{
						// Save to the specified path
						if (!m_fileManager->SaveCurrentFile(m_contentView.get(), savePath))
						{
							std::wstring msg = LOC(L"save_failed_msg");
							if (msg.empty())
								msg = L"Failed to save file.";
							MessageBoxW(m_hwnd, msg.c_str(), L"Error", MB_OK | MB_ICONERROR);
						}
						else
						{
							// Update current file path
							m_currentFilePath = savePath;
							
							// Mark as unmodified
							m_contentView->SetModified(false);
							
							// Update status bar
							std::wstring statusMsg = LOC(L"save_success_msg");
							if (statusMsg.empty())
								statusMsg = L"File saved successfully.";
			SendMessageW(m_hStatusBar, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(statusMsg.c_str()));
						}
					}
					catch (const std::exception& e)
					{
						std::string errStr = e.what();
						std::wstring errMsg = L"Error saving file: ";
						errMsg += std::wstring(errStr.begin(), errStr.end());
						MessageBoxW(m_hwnd, errMsg.c_str(), L"Error", MB_OK | MB_ICONERROR);
					}
					
					CoTaskMemFree(pszPath);
				}
				pItem->Release();
			}
		}
		pFileSaveDialog->Release();
	}
	
	if (comInitialized && hr != RPC_E_CHANGED_MODE)
	{
		CoUninitialize();
	}
}

void MainFrame::OnResetGamePath()
{
	FFXIDatEGApp& app = FFXIDatEGApp::Instance();

	HKEY hKey;
	const wchar_t* subKey1 = L"SOFTWARE\\WOW6432Node\\PlayOnline\\InstallFolder";
	const wchar_t* subKey2 = L"SOFTWARE\\WOW6432Node\\PlayOnlineEU\\InstallFolder";
	const wchar_t* subKey3 = L"SOFTWARE\\WOW6432Node\\PlayOnlineUS\\InstallFolder";
	const wchar_t* valueName = L"0001";
	wchar_t valueData[MAX_PATH];
	DWORD bufferSize = sizeof(valueData);
	DWORD valueType;

	std::filesystem::path newPath;
	bool found = false;

	// Try primary key first (JP)
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, subKey1, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		if (RegQueryValueExW(hKey, valueName, nullptr, &valueType, reinterpret_cast<LPBYTE>(valueData), &bufferSize) == ERROR_SUCCESS)
		{
			newPath = valueData;
			found = true;
		}
		RegCloseKey(hKey);
	}
	// Try PlayOnlineEU
	else if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, subKey2, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		if (RegQueryValueExW(hKey, valueName, nullptr, &valueType, reinterpret_cast<LPBYTE>(valueData), &bufferSize) == ERROR_SUCCESS)
		{
			newPath = valueData;
			found = true;
		}
		RegCloseKey(hKey);
	}
	// Try PlayOnlineUS
	else if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, subKey3, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		if (RegQueryValueExW(hKey, valueName, nullptr, &valueType, reinterpret_cast<LPBYTE>(valueData), &bufferSize) == ERROR_SUCCESS)
		{
			newPath = valueData;
			found = true;
		}
		RegCloseKey(hKey);
	}

	if (!found)
	{
		MessageBoxW(m_hwnd,
			LOCS(L"registry_ffxi_path_not_found"),
			LOCS(L"registry_error_title"), MB_OK | MB_ICONERROR);
		return;
	}

	// Verify it's a valid FFXI installation
	if (!std::filesystem::exists(newPath / "ROM"))
	{
		std::wstring msg = L"Invalid FFXI installation directory found in registry:\n" + newPath.wstring();
		MessageBoxW(m_hwnd, msg.c_str(), L"Error", MB_OK | MB_ICONERROR);
		return;
	}

	// Update application path
	app.SetGamePath(newPath);

	// Reload file manager
	m_fileManager = std::make_unique<DatFileManager>(newPath);

	// Clear and reload tree
	TreeView_DeleteAllItems(m_hTreeView);
	m_contentView->Clear();
	LoadROMDefinitions();

	std::wstring statusText = LOC(L"Status.GamePathReset");
	statusText += L": " + newPath.wstring();
	SendMessageW(m_hStatusBar, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(statusText.c_str()));
}

void MainFrame::OnFilterLanguage(const std::string& language)
{
	// Update language filter
	m_languageFilter = language;

	// Update menu checkmarks
	CheckMenuItem(m_hLanguageFilterMenu, IDM_VIEW_FILTER_ALL,
		language.empty() ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(m_hLanguageFilterMenu, IDM_VIEW_FILTER_JP,
		language == "jp" ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(m_hLanguageFilterMenu, IDM_VIEW_FILTER_EN,
		language == "en" ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(m_hLanguageFilterMenu, IDM_VIEW_FILTER_FR,
		language == "fr" ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(m_hLanguageFilterMenu, IDM_VIEW_FILTER_DE,
		language == "de" ? MF_CHECKED : MF_UNCHECKED);

	// Reload ROM definitions with filter
	TreeView_DeleteAllItems(m_hTreeView);
	LoadROMDefinitions();

	// Update status bar
	std::wstring statusText;
	if (language.empty())
	{
		statusText = L"Showing all languages";
	}
	else
	{
		statusText = L"Showing only " + std::wstring(language.begin(), language.end()) + L" files";
	}
	SendMessageW(m_hStatusBar, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(statusText.c_str()));
}

void MainFrame::BuildUILanguageMenu(HMENU parentMenu)
{
	// Clear existing menu items
	while (GetMenuItemCount(parentMenu) > 0)
	{
		RemoveMenu(parentMenu, 0, MF_BYPOSITION);
	}

	// Add menu items for each available language
	for (size_t i = 0; i < m_availableUILanguages.size(); ++i)
	{
		const LanguageInfo& langInfo = m_availableUILanguages[i];
		
		// Create menu text (e.g., "English" or "ÖÐÎÄ")
		std::wstring menuText = langInfo.name;
		
		// Add menu item with dynamic ID
		UINT menuId = IDM_VIEW_UILANG_BASE + static_cast<UINT>(i);
		AppendMenuW(parentMenu, MF_STRING, menuId, menuText.c_str());
		
		// Check if this is the current language
		if (langInfo.code == m_uiLanguage)
		{
			CheckMenuItem(parentMenu, menuId, MF_CHECKED);
		}
	}
	
	// If no languages found, add a placeholder
	if (m_availableUILanguages.empty())
	{
		AppendMenuW(parentMenu, MF_STRING | MF_GRAYED, 0, L"(No languages available)");
	}
}

void MainFrame::OnChangeUILanguage(const std::wstring& language)
{
	// Update UI language
	m_uiLanguage = language;
	
	// Save to config
	FFXIDatEGApp::Instance().GetConfig().SetUILanguage(language);
	
	// Load localization strings from local directory
	Localization& loc = Localization::Instance();
	if (!loc.LoadLanguage(language, m_localDir))
	{
		// Fallback to English if loading fails
		loc.LoadLanguage(L"en", m_localDir);
		m_uiLanguage = L"en";
	}
	
	// Refresh all UI text
	RefreshUIText();
	
	// Update status bar
	std::wstring statusText = LOC(L"ui_language_changed");
	SendMessageW(m_hStatusBar, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(statusText.c_str()));
}

void MainFrame::RefreshUIText()
{
	// Rebuild menu with localized text
	if (m_hMenu)
	{
		DestroyMenu(m_hMenu);
		m_hMenu = nullptr;
	}
	
	// Create menu bar
	m_hMenu = CreateMenu();

	// File menu
	HMENU hFileMenu = CreateMenu();
	AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_SAVE, LOCS(L"menu_file_save"));
	AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_SAVEAS, LOCS(L"menu_file_saveas"));
	AppendMenuW(hFileMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_CHANGEPATH, LOCS(L"menu_file_changepath"));
	AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_RESETPATH, LOCS(L"menu_file_resetpath"));
	AppendMenuW(hFileMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_CLEANPREFS, LOCS(L"menu_help_cleanprefs"));
	AppendMenuW(hFileMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_EXIT, LOCS(L"menu_file_exit"));

	// Edit menu
	HMENU hEditMenu = CreateMenu();
	AppendMenuW(hEditMenu, MF_STRING, IDM_EDIT_FIND, LOCS(L"menu_edit_find"));
	AppendMenuW(hEditMenu, MF_STRING, IDM_EDIT_FINDNEXT, LOCS(L"menu_edit_findnext"));

	// View menu
	HMENU hViewMenu = CreateMenu();
	AppendMenuW(hViewMenu, MF_STRING, IDM_VIEW_FONT, LOCS(L"menu_view_font"));
	AppendMenuW(hViewMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenuW(hViewMenu, MF_STRING, IDM_VIEW_ENABLE_CATEGORY_HIERARCHY, LOCS(L"menu_view_enable_category_hierarchy"));
	AppendMenuW(hViewMenu, MF_SEPARATOR, 0, nullptr);

	// Language Filter submenu
	m_hLanguageFilterMenu = CreateMenu();
	AppendMenuW(m_hLanguageFilterMenu, MF_STRING, IDM_VIEW_FILTER_ALL, LOCS(L"menu_view_filter_all"));
	AppendMenuW(m_hLanguageFilterMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenuW(m_hLanguageFilterMenu, MF_STRING, IDM_VIEW_FILTER_JP, LOCS(L"menu_view_filter_jp"));
	AppendMenuW(m_hLanguageFilterMenu, MF_STRING, IDM_VIEW_FILTER_EN, LOCS(L"menu_view_filter_en"));
	AppendMenuW(m_hLanguageFilterMenu, MF_STRING, IDM_VIEW_FILTER_FR, LOCS(L"menu_view_filter_fr"));
	AppendMenuW(m_hLanguageFilterMenu, MF_STRING, IDM_VIEW_FILTER_DE, LOCS(L"menu_view_filter_de"));

	// Restore language filter checkmarks
	CheckMenuItem(m_hLanguageFilterMenu, IDM_VIEW_FILTER_ALL,
		m_languageFilter.empty() ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(m_hLanguageFilterMenu, IDM_VIEW_FILTER_JP,
		m_languageFilter == "jp" ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(m_hLanguageFilterMenu, IDM_VIEW_FILTER_EN,
		m_languageFilter == "en" ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(m_hLanguageFilterMenu, IDM_VIEW_FILTER_FR,
		m_languageFilter == "fr" ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(m_hLanguageFilterMenu, IDM_VIEW_FILTER_DE,
		m_languageFilter == "de" ? MF_CHECKED : MF_UNCHECKED);

	AppendMenuW(hViewMenu, MF_POPUP, (UINT_PTR)m_hLanguageFilterMenu, LOCS(L"menu_view_langfilter"));

	// UI Language submenu - Rebuild dynamically
	m_hUILanguageMenu = CreateMenu();
	BuildUILanguageMenu(m_hUILanguageMenu);
	AppendMenuW(hViewMenu, MF_POPUP, (UINT_PTR)m_hUILanguageMenu, LOCS(L"menu_view_uilang"));

	// Help menu
	HMENU hHelpMenu = CreateMenu();
	AppendMenuW(hHelpMenu, MF_STRING, IDM_HELP_QUICKSTART, LOCS(L"menu_help_quickstart"));
	AppendMenuW(hHelpMenu, MF_STRING, IDM_HELP_ABOUT, LOCS(L"menu_help_about"));

	// Add to menu bar
	AppendMenuW(m_hMenu, MF_POPUP, (UINT_PTR)hFileMenu, LOCS(L"menu_file"));
	AppendMenuW(m_hMenu, MF_POPUP, (UINT_PTR)hEditMenu, LOCS(L"menu_edit"));
	AppendMenuW(m_hMenu, MF_POPUP, (UINT_PTR)hViewMenu, LOCS(L"menu_view"));
	AppendMenuW(m_hMenu, MF_POPUP, (UINT_PTR)hHelpMenu, LOCS(L"menu_help"));

	SetMenu(m_hwnd, m_hMenu);
	
	// Refresh search dialog text (regardless of visibility)
	if (m_searchDialog)
	{
		m_searchDialog->RefreshUIText();
	}
	
	DrawMenuBar(m_hwnd);
}