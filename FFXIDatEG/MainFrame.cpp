#include "MainFrame.h"
#include "FFXIDatEGApp.h"
#include "Localization.h"
#include "Config.h"
#include <windowsx.h>
#include <shobjidl.h>
#include <commdlg.h>
#include <string>
#include <vector>
#include <xystring.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")

// Helper macro to convert localized string to LPCWSTR
#define LOCS(key) LOC(key).c_str()

namespace {
	constexpr int IDC_PROMPT_TYPE = 3001;
	constexpr int IDC_PROMPT_OK = 3002;
	constexpr int IDC_PROMPT_CANCEL = 3003;
	constexpr int IDC_PROMPT_GLOBAL_RADIO = 3004;
	constexpr int IDC_PROMPT_LOCAL_RADIO = 3005;
	constexpr int IDC_PROMPT_GLOBAL_EDIT = 3006;
	constexpr int IDC_PROMPT_ROM_EDIT = 3007;
	constexpr int IDC_PROMPT_LOCAL_EDIT = 3008;

	const wchar_t* kPromptClass = L"FFXIDatEGPrompt";

	std::wstring LocalizedOrDefault(const wchar_t* key, const wchar_t* fallback)
	{
		return Localization::Instance().GetString(key, fallback);
	}

	struct PromptState
	{
		bool done = false;
		bool accepted = false;
		bool isGlobal = true;
		std::wstring fileType;
		std::wstring romFolder;
		std::wstring globalIdText;
		std::wstring localIdText;
		HWND comboType = nullptr;
		HWND editGlobal = nullptr;
		HWND editRom = nullptr;
		HWND editLocal = nullptr;
		HWND radioGlobal = nullptr;
		HWND radioLocal = nullptr;
		bool showIdFields = false;
	};

	void SetPromptFieldsEnabled(PromptState* state)
	{
		if (!state)
			return;
		BOOL globalMode = state->isGlobal ? TRUE : FALSE;
		if (state->editGlobal)
			EnableWindow(state->editGlobal, globalMode);
		if (state->editRom)
			EnableWindow(state->editRom, !globalMode);
		if (state->editLocal)
			EnableWindow(state->editLocal, !globalMode);
	}

	void CapturePromptData(HWND hwnd, PromptState* state)
	{
		if (!state)
			return;

		wchar_t buffer[256] = {};
		if (state->comboType)
		{
			GetWindowTextW(state->comboType, buffer, 256);
			state->fileType = buffer;
		}
		if (state->editGlobal)
		{
			GetWindowTextW(state->editGlobal, buffer, 256);
			state->globalIdText = buffer;
		}
		if (state->editRom)
		{
			GetWindowTextW(state->editRom, buffer, 256);
			state->romFolder = buffer;
		}
		if (state->editLocal)
		{
			GetWindowTextW(state->editLocal, buffer, 256);
			state->localIdText = buffer;
		}
	}

	LRESULT CALLBACK PromptWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		PromptState* state = reinterpret_cast<PromptState*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

		switch (msg)
		{
		case WM_NCCREATE:
		{
			CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
			return TRUE;
		}
		case WM_COMMAND:
		{
			int id = LOWORD(wParam);
			int code = HIWORD(wParam);
			if (id == IDC_PROMPT_OK && code == BN_CLICKED)
			{
				CapturePromptData(hwnd, state);
				state->accepted = true;
				state->done = true;
				DestroyWindow(hwnd);
				return 0;
			}
			if (id == IDC_PROMPT_CANCEL && code == BN_CLICKED)
			{
				state->accepted = false;
				state->done = true;
				DestroyWindow(hwnd);
				return 0;
			}
			if ((id == IDC_PROMPT_GLOBAL_RADIO || id == IDC_PROMPT_LOCAL_RADIO) && code == BN_CLICKED)
			{
				state->isGlobal = (id == IDC_PROMPT_GLOBAL_RADIO);
				SetPromptFieldsEnabled(state);
				return 0;
			}
			break;
		}
		case WM_CLOSE:
			if (state)
			{
				state->accepted = false;
				state->done = true;
			}
			DestroyWindow(hwnd);
			return 0;
		}
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	bool EnsurePromptClassRegistered(HINSTANCE instance)
	{
		static bool registered = false;
		if (registered)
			return true;
		WNDCLASSEXW wc = {};
		wc.cbSize = sizeof(wc);
		wc.lpfnWndProc = PromptWndProc;
		wc.hInstance = instance;
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
		wc.lpszClassName = kPromptClass;
		if (!RegisterClassExW(&wc))
			return false;
		registered = true;
		return true;
	}

	bool RunPromptDialog(HWND owner, HWND hwnd, PromptState& state)
	{
		if (!hwnd)
			return false;

		ShowWindow(hwnd, SW_SHOW);
		UpdateWindow(hwnd);
		EnableWindow(owner, FALSE);

		MSG msg;
		while (!state.done && GetMessage(&msg, nullptr, 0, 0))
		{
			if (!IsDialogMessage(hwnd, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		EnableWindow(owner, TRUE);
		SetActiveWindow(owner);
		return state.accepted;
	}
}
bool MainFrame::PromptForFileType(std::string& outType)
{
	PromptState state;
	HINSTANCE instance = FFXIDatEGApp::Instance().GetInstance();
	if (!EnsurePromptClassRegistered(instance))
		return false;
	HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

	RECT rcParent;
	GetWindowRect(m_hwnd, &rcParent);
	int x = rcParent.left + (rcParent.right - rcParent.left - 400) / 2;
	int y = rcParent.top + (rcParent.bottom - rcParent.top - 180) / 2;

	std::wstring title = LocalizedOrDefault(L"dialog_filetype_title", L"Select File Type");
	HWND hwnd = CreateWindowExW(
		WS_EX_DLGMODALFRAME,
		kPromptClass,
		title.c_str(),
		WS_POPUP | WS_CAPTION | WS_SYSMENU,
		x, y,
		340, 170,
		m_hwnd,
		nullptr,
		instance,
		&state);
	if (!hwnd)
		return false;
	SetWindowTextW(hwnd, title.c_str()); // why?

	std::wstring label = LocalizedOrDefault(L"dialog_filetype_label", L"File Type:");
	HWND hFileType = CreateWindowExW(0, L"STATIC", label.c_str(), WS_CHILD | WS_VISIBLE,
		12, 18, 110, 20, hwnd, nullptr, instance, nullptr);
	SendMessage(hFileType, WM_SETFONT, (WPARAM)hFont, TRUE);

	state.comboType = CreateWindowExW(0, WC_COMBOBOXW, nullptr,
		WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
		120, 14, 185, 200, hwnd, reinterpret_cast<HMENU>(IDC_PROMPT_TYPE), instance, nullptr);
	SendMessage(state.comboType, WM_SETFONT, (WPARAM)hFont, TRUE);

	const std::vector<std::wstring> types = {
		L"dmsg", L"xis", L"evsb", L"sd", L"fp",
		L"iab", L"iwb", L"iub", L"inb", L"ipb", L"isb", L"icb",
		L"mbd", L"erq", L"erc"
	};
	for (const auto& type : types)
	{
		SendMessageW(state.comboType, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(type.c_str()));
	}
	SendMessageW(state.comboType, CB_SETCURSEL, 0, 0);

	std::wstring okText = LocalizedOrDefault(L"dialog_ok", L"OK");
	std::wstring cancelText = LocalizedOrDefault(L"dialog_cancel", L"Cancel");
	HWND hOkBtn = CreateWindowExW(0, L"BUTTON", okText.c_str(), WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
		80, 110, 80, 26, hwnd, reinterpret_cast<HMENU>(IDC_PROMPT_OK), instance, nullptr);
	HWND hCancelBtn = CreateWindowExW(0, L"BUTTON", cancelText.c_str(), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		170, 110, 80, 26, hwnd, reinterpret_cast<HMENU>(IDC_PROMPT_CANCEL), instance, nullptr);
	SendMessage(hOkBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
	SendMessage(hCancelBtn, WM_SETFONT, (WPARAM)hFont, TRUE);

	if (!RunPromptDialog(m_hwnd, hwnd, state))
		return false;

	if (state.fileType.empty())
		return false;

	std::u8string u8str = xybase::string::to_utf8(state.fileType);
	outType = xybase::string::to_string(u8str);
	return true;
}

bool MainFrame::PromptForFileId(int& outGlobalId, std::string& outRomFolder, int& outLocalId, std::string& outType, bool& isGlobal)
{
	PromptState state;
	state.isGlobal = true;
	HINSTANCE instance = FFXIDatEGApp::Instance().GetInstance();
	if (!EnsurePromptClassRegistered(instance))
		return false;
	HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

	RECT rcParent;
	GetWindowRect(m_hwnd, &rcParent);
	int x = rcParent.left + (rcParent.right - rcParent.left - 400) / 2;
	int y = rcParent.top + (rcParent.bottom - rcParent.top - 180) / 2;

	std::wstring title = LocalizedOrDefault(L"dialog_open_by_id_title", L"Open File by ID");
	HWND hwnd = CreateWindowExW(
		WS_EX_DLGMODALFRAME,
		kPromptClass,
		title.c_str(),
		WS_POPUP | WS_CAPTION | WS_SYSMENU,
		x, y,
		420, 260,
		m_hwnd,
		nullptr,
		instance,
		&state);
	if (!hwnd)
		return false;
	SetWindowTextW(hwnd, title.c_str());

	std::wstring typeLabel = LocalizedOrDefault(L"dialog_filetype_label", L"File Type:");
	HWND hTypeLabel = CreateWindowExW(0, L"STATIC", typeLabel.c_str(), WS_CHILD | WS_VISIBLE,
		12, 18, 110, 20, hwnd, nullptr, instance, nullptr);
	SendMessage(hTypeLabel, WM_SETFONT, (WPARAM)hFont, TRUE);

	state.comboType = CreateWindowExW(0, WC_COMBOBOXW, nullptr,
		WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
		120, 14, 185, 200, hwnd, reinterpret_cast<HMENU>(IDC_PROMPT_TYPE), instance, nullptr);
	SendMessage(state.comboType, WM_SETFONT, (WPARAM)hFont, TRUE);

	const std::vector<std::wstring> types = {
		L"dmsg", L"xis", L"evsb", L"sd", L"fp",
		L"iab", L"iwb", L"iub", L"inb", L"ipb", L"isb", L"icb",
		L"mbd", L"erq", L"erc"
	};
	for (const auto& type : types)
	{
		SendMessageW(state.comboType, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(type.c_str()));
	}
	SendMessageW(state.comboType, CB_SETCURSEL, 0, 0);

	std::wstring globalLabel = LocalizedOrDefault(L"dialog_global_id", L"Global ID");
	std::wstring romLabel = LocalizedOrDefault(L"dialog_rom_folder", L"ROM Folder");
	std::wstring localLabel = LocalizedOrDefault(L"dialog_local_id", L"Local ID");

	state.radioGlobal = CreateWindowExW(0, L"BUTTON", globalLabel.c_str(),
		WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
		12, 60, 100, 20, hwnd, reinterpret_cast<HMENU>(IDC_PROMPT_GLOBAL_RADIO), instance, nullptr);
	state.editGlobal = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", nullptr,
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
		120, 58, 260, 22, hwnd, reinterpret_cast<HMENU>(IDC_PROMPT_GLOBAL_EDIT), instance, nullptr);
	SendMessageW(state.editGlobal, WM_SETFONT, (WPARAM)hFont, TRUE);
	SendMessageW(state.radioGlobal, WM_SETFONT, (WPARAM)hFont, TRUE);

	state.radioLocal = CreateWindowExW(0, L"BUTTON", LocalizedOrDefault(L"dialog_local_mode", L"ROM + Local ID").c_str(),
		WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
		12, 95, 140, 20, hwnd, reinterpret_cast<HMENU>(IDC_PROMPT_LOCAL_RADIO), instance, nullptr);
	SendMessageW(state.radioLocal, WM_SETFONT, (WPARAM)hFont, TRUE);

	HWND hRomLabel = CreateWindowExW(0, L"STATIC", romLabel.c_str(), WS_CHILD | WS_VISIBLE,
		40, 125, 80, 18, hwnd, nullptr, instance, nullptr);
	state.editRom = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", nullptr,
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
		120, 122, 120, 22, hwnd, reinterpret_cast<HMENU>(IDC_PROMPT_ROM_EDIT), instance, nullptr);
	SendMessageW(state.editRom, WM_SETFONT, (WPARAM)hFont, TRUE);
	SendMessageW(hRomLabel, WM_SETFONT, (WPARAM)hFont, TRUE);

	HWND hLocalLabel = CreateWindowExW(0, L"STATIC", localLabel.c_str(), WS_CHILD | WS_VISIBLE,
		40, 155, 80, 18, hwnd, nullptr, instance, nullptr);
	state.editLocal = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", nullptr,
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
		120, 152, 120, 22, hwnd, reinterpret_cast<HMENU>(IDC_PROMPT_LOCAL_EDIT), instance, nullptr);
	SendMessageW(state.editLocal, WM_SETFONT, (WPARAM)hFont, TRUE);
	SendMessageW(hLocalLabel, WM_SETFONT, (WPARAM)hFont, TRUE);

	SendMessageW(state.radioGlobal, BM_SETCHECK, BST_CHECKED, 0);
	SetPromptFieldsEnabled(&state);

	std::wstring okText = LocalizedOrDefault(L"dialog_ok", L"OK");
	std::wstring cancelText = LocalizedOrDefault(L"dialog_cancel", L"Cancel");
	HWND hOkBtn = CreateWindowExW(0, L"BUTTON", okText.c_str(), WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
		120, 195, 80, 26, hwnd, reinterpret_cast<HMENU>(IDC_PROMPT_OK), instance, nullptr);
	HWND hCancelBtn = CreateWindowExW(0, L"BUTTON", cancelText.c_str(), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		220, 195, 80, 26, hwnd, reinterpret_cast<HMENU>(IDC_PROMPT_CANCEL), instance, nullptr);
	SendMessageW(hOkBtn, WM_SETFONT, (WPARAM)hFont, TRUE);
	SendMessageW(hCancelBtn, WM_SETFONT, (WPARAM)hFont, TRUE);

	if (!RunPromptDialog(m_hwnd, hwnd, state))
		return false;

	if (state.fileType.empty())
		return false;

	std::u8string u8str = xybase::string::to_utf8(state.fileType);
	outType = xybase::string::to_string(u8str);
	isGlobal = state.isGlobal;

	try
	{
		if (isGlobal)
		{
			if (state.globalIdText.empty())
				return false;
			outGlobalId = std::stoi(state.globalIdText);
		}
		else
		{
			if (state.romFolder.empty() || state.localIdText.empty())
				return false;
			outRomFolder = xybase::string::to_string(xybase::string::to_utf8(state.romFolder));
			outLocalId = std::stoi(state.localIdText);
		}
	}
	catch (...)
	{
		std::wstring message = LocalizedOrDefault(L"dialog_invalid_input", L"Invalid input.");
		MessageBoxW(m_hwnd, message.c_str(), LocalizedOrDefault(L"error_msg_title", L"Error").c_str(), MB_OK | MB_ICONERROR);
		return false;
	}

	return true;
}

void MainFrame::OnOpenFile()
{
	if (!m_fileManager || !m_contentView)
		return;

	if (m_contentView->IsModified() && !CheckAndPromptSave())
		return;

	HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	bool comInitialized = SUCCEEDED(hr);

	IFileOpenDialog* pFileDialog = nullptr;
	hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL,
		IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileDialog));

	if (SUCCEEDED(hr))
	{
		COMDLG_FILTERSPEC rgSpec[] = {
			{ L"DAT Files", L"*.DAT" },
			{ L"All Files", L"*.*" }
		};
		pFileDialog->SetFileTypes(ARRAYSIZE(rgSpec), rgSpec);
		pFileDialog->SetFileTypeIndex(1);
		std::wstring title = LocalizedOrDefault(L"open_file_title", L"Open File");
		pFileDialog->SetTitle(title.c_str());

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
					std::filesystem::path filePath(pszPath);
					std::string fileType;
					if (PromptForFileType(fileType))
					{
						std::wstring statusText = LOC(L"Status.Loading");
						statusText += L" " + filePath.filename().wstring() + L"...";
						SendMessageW(m_hStatusBar, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(statusText.c_str()));

						if (m_fileManager->LoadArbitraryFile(filePath, fileType, m_contentView.get()))
						{
							statusText = LOC(L"Status.Loaded");
							statusText += L": " + filePath.wstring();
							SendMessageW(m_hStatusBar, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(statusText.c_str()));
							m_currentFilePath = filePath;
						}
						else
						{
							SendMessageW(m_hStatusBar, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(LOC(L"Status.LoadFailed").c_str()));
							m_currentFilePath.clear();
						}
					}
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

void MainFrame::OnOpenFileById()
{
	if (!m_fileManager || !m_contentView)
		return;

	if (m_contentView->IsModified() && !CheckAndPromptSave())
		return;

	int globalId = 0;
	int localId = 0;
	std::string romFolder;
	std::string fileType;
	bool isGlobal = true;

	if (!PromptForFileId(globalId, romFolder, localId, fileType, isGlobal))
		return;

	std::wstring statusText = LOC(L"Status.Loading");
	if (isGlobal)
		statusText += L" GlobalID " + std::to_wstring(globalId) + L"...";
	else
		statusText += L" " + xybase::string::to_wstring(romFolder) + L"/" + std::to_wstring(localId) + L"...";
	SendMessageW(m_hStatusBar, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(statusText.c_str()));

	bool loaded = false;
	if (isGlobal)
		loaded = m_fileManager->LoadGlobalId(globalId, fileType, m_contentView.get());
	else
		loaded = m_fileManager->LoadLocalId(romFolder, localId, fileType, m_contentView.get());

	if (loaded)
	{
		statusText = LOC(L"Status.Loaded");
		if (isGlobal)
		{
			m_currentFilePath = m_fileManager->GetDatFilePath(globalId);
		}
		else
		{
			m_currentFilePath = m_fileManager->GetDatFilePath(localId, romFolder);
		}
		statusText += L": " + m_currentFilePath.wstring();
		SendMessageW(m_hStatusBar, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(statusText.c_str()));
	}
	else
	{
		SendMessageW(m_hStatusBar, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(LOC(L"Status.LoadFailed").c_str()));
		m_currentFilePath.clear();
	}
}
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
	std::wstring openFileText = LocalizedOrDefault(L"menu_file_open", L"Open File...");
	std::wstring openFileIdText = LocalizedOrDefault(L"menu_file_open_by_id", L"Open File by ID...");
	AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_OPEN, openFileText.c_str());
	AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_OPEN_BY_ID, openFileIdText.c_str());
	AppendMenuW(hFileMenu, MF_SEPARATOR, 0, nullptr);
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
		statusText += L" (" + m_fileManager->GetDatFilePath(info->localFileId, info->romFolder).wstring() + L")";
		SendMessageW(m_hStatusBar, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(statusText.c_str()));
		
		// Store current file path for save operation
		m_currentFilePath = m_fileManager->GetDatFilePath(info->localFileId, info->romFolder);
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
	case IDM_FILE_OPEN:
		OnOpenFile();
		break;
	case IDM_FILE_OPEN_BY_ID:
		OnOpenFileById();
		break;
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
	
    // First-time save disclaimer
    try {
        int accepted = FFXIDatEGApp::Instance().GetConfig().GetInt(L"Safety", L"SaveDisclaimerAccepted", 0);
        if (accepted == 0)
        {
			std::wstring title = Localization::Instance().GetString(L"save_disclaimer_title", L"Disclaimer");
            std::wstring message = Localization::Instance().GetString(L"save_disclaimer_message", L"You are about to modify game files. This may break game functionality and violate the game's terms of service. Proceed at your own risk. Do you understand and want to continue?");

            int result = MessageBoxW(m_hwnd, message.c_str(), title.c_str(), MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
            if (result != IDYES)
            {
                // User declined; discard changes
                m_contentView->SetModified(false);
                SendMessageW(m_hStatusBar, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(LOC(L"save_disclaimer_declined").c_str()));
                return;
            }

            // User accepted; remember in config
            FFXIDatEGApp::Instance().GetConfig().SetInt(L"Safety", L"SaveDisclaimerAccepted", 1);
        }
    } catch (...) {
        // If config operations fail, continue without blocking save
    }

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
	
    // First-time save disclaimer
    try {
        int accepted = FFXIDatEGApp::Instance().GetConfig().GetInt(L"Safety", L"SaveDisclaimerAccepted", 0);
        if (accepted == 0)
        {
            std::wstring title = LOC(L"save_disclaimer_title");
            if (title.empty()) title = L"Disclaimer";
            std::wstring message = LOC(L"save_disclaimer_message");
            if (message.empty())
                message = L"You are about to modify game files. This may break game functionality and violate the game's terms of service. Proceed at your own risk. Do you understand and want to continue?";

            int result = MessageBoxW(m_hwnd, message.c_str(), title.c_str(), MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
            if (result != IDYES)
            {
                // User declined; discard changes
                m_contentView->SetModified(false);
                SendMessageW(m_hStatusBar, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(LOC(L"save_disclaimer_declined").c_str()));
                return;
            }

            // User accepted; remember in config
            FFXIDatEGApp::Instance().GetConfig().SetInt(L"Safety", L"SaveDisclaimerAccepted", 1);
        }
    } catch (...) {
        // If config operations fail, continue without blocking save
    }

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
	std::wstring openFileText = LocalizedOrDefault(L"menu_file_open", L"Open File...");
	std::wstring openFileIdText = LocalizedOrDefault(L"menu_file_open_by_id", L"Open File by ID...");
	AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_OPEN, openFileText.c_str());
	AppendMenuW(hFileMenu, MF_STRING, IDM_FILE_OPEN_BY_ID, openFileIdText.c_str());
	AppendMenuW(hFileMenu, MF_SEPARATOR, 0, nullptr);
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

	CheckMenuItem(hViewMenu, IDM_VIEW_ENABLE_CATEGORY_HIERARCHY,
		m_enableCategoryHierarchy ? MF_CHECKED : MF_UNCHECKED);

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