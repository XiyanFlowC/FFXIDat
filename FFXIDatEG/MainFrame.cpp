#include "MainFrame.h"
#include "FFXIDatEGApp.h"
#include <windowsx.h>
#include <shobjidl.h>
#include <commdlg.h>
#include <string>
#include <xystring.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")

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
	// Create menu bar
	m_hMenu = CreateMenu();

	// File menu
	HMENU hFileMenu = CreateMenu();
	AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_CHANGEPATH, L"&Change Game Path...");
	AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_RESETPATH, L"&Reset Game Path from Registry");
	AppendMenuW(hFileMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_EXIT, L"E&xit");

	// Edit menu
	HMENU hEditMenu = CreateMenu();
	AppendMenuW(hEditMenu, MF_STRING, IDM_EDIT_FIND, L"&Find...\tCtrl+F");
	AppendMenuW(hEditMenu, MF_STRING, IDM_EDIT_FINDNEXT, L"Find &Next\tF3");

	// View menu - with Language Filter submenu
	HMENU hViewMenu = CreateMenu();
	AppendMenuW(hViewMenu, MF_STRING, IDM_VIEW_FONT, L"&Font...");
	AppendMenuW(hViewMenu, MF_SEPARATOR, 0, nullptr);

	// Language Filter submenu
	m_hLanguageFilterMenu = CreateMenu();
	AppendMenuW(m_hLanguageFilterMenu, MF_STRING, IDM_VIEW_FILTER_ALL, L"&All Languages");
	AppendMenuW(m_hLanguageFilterMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenuW(m_hLanguageFilterMenu, MF_STRING, IDM_VIEW_FILTER_JP, L"&Japanese (jp)");
	AppendMenuW(m_hLanguageFilterMenu, MF_STRING, IDM_VIEW_FILTER_EN, L"&English (en)");
	AppendMenuW(m_hLanguageFilterMenu, MF_STRING, IDM_VIEW_FILTER_FR, L"&French (fr)");
	AppendMenuW(m_hLanguageFilterMenu, MF_STRING, IDM_VIEW_FILTER_DE, L"&German (de)");

	// Set default checked item (All Languages)
	CheckMenuItem(m_hLanguageFilterMenu, IDM_VIEW_FILTER_ALL, MF_CHECKED);

	AppendMenuW(hViewMenu, MF_POPUP, (UINT_PTR)m_hLanguageFilterMenu, L"&Language Filter");

	// Help menu
	HMENU hHelpMenu = CreateMenu();
	AppendMenuW(hHelpMenu, MF_STRING, IDM_HELP_ABOUT, L"&About");

	// Add to menu bar
	AppendMenuW(m_hMenu, MF_POPUP, (UINT_PTR)hFileMenu, L"&File");
	AppendMenuW(m_hMenu, MF_POPUP, (UINT_PTR)hEditMenu, L"&Edit");
	AppendMenuW(m_hMenu, MF_POPUP, (UINT_PTR)hViewMenu, L"&View");
	AppendMenuW(m_hMenu, MF_POPUP, (UINT_PTR)hHelpMenu, L"&Help");

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

	SendMessageW(m_hStatusBar, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(L"Ready"));

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
	
	std::wstring statusText = L"Loading ";
	std::string nameUtf8 = info->friendlyName;
	auto name = xybase::string::to_wstring((char8_t *) nameUtf8.c_str());
	statusText += name;
	statusText += L"...";
	SendMessageW(m_hStatusBar, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(statusText.c_str()));
	
	if (m_fileManager->LoadDatFile(*info, m_contentView.get()))
	{
		statusText = L"Loaded: ";
		statusText += name;
		statusText += L" (" + std::to_wstring(info->fileId) + L")";
		SendMessageW(m_hStatusBar, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(statusText.c_str()));
	}
	else
	{
		SendMessageW(m_hStatusBar, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(L"Failed to load file"));
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
	m_fileManager->LoadAllROMDefinitions(exeDir, m_hTreeView);
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
	case IDM_HELP_ABOUT:
		OnAbout();
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
		
		pFileDialog->SetTitle(L"Select FFXI Installation Directory");
		
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
								   L"Invalid FFXI installation directory. ROM folder not found.",
								   L"Error", MB_OK | MB_ICONERROR);
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
								reinterpret_cast<LPARAM>(L"Game path changed successfully"));
					
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
	wcscpy_s(lf.lfFaceName, LF_FACESIZE, currentFont.c_str());
	lf.lfHeight = -16;
	lf.lfCharSet = SHIFTJIS_CHARSET;
	
	if (ChooseFontW(&cf))
	{
		// User selected a font
		std::wstring newFontName = lf.lfFaceName;
		m_contentView->SetFontName(newFontName);
		
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
		std::wstring msg = L"Cannot find \"" + searchText + L"\"";
		MessageBoxW(m_hwnd, msg.c_str(), L"Find", MB_OK | MB_ICONINFORMATION);
	}
	else
	{
		SendMessageW(m_hStatusBar, SB_SETTEXTW, 0, 
					reinterpret_cast<LPARAM>(L"SingleLine found"));
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
			L"Could not find FFXI installation path in Windows Registry.\n"
			L"Please install FFXI or use 'Change Game Path' to select the directory manually.",
			L"Registry Error", MB_OK | MB_ICONERROR);
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

	std::wstring statusText = L"Game path reset to: " + newPath.wstring();
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