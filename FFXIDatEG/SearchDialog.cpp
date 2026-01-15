#include "SearchDialog.h"
#include "Localization.h"
#include <CommCtrl.h>

SearchDialog::SearchDialog()
    : m_hwnd(nullptr)
    , m_hParent(nullptr)
    , m_hEditSearch(nullptr)
    , m_hCheckCase(nullptr)
    , m_hRadioDown(nullptr)
    , m_hRadioUp(nullptr)
    , m_caseSensitive(false)
    , m_searchDown(true)
{
}

SearchDialog::~SearchDialog()
{
    if (m_hwnd)
    {
        DestroyWindow(m_hwnd);
    }
}

bool SearchDialog::Create(HWND hParent)
{
    m_hParent = hParent;
    
    // Register window class for custom dialog
    static bool registered = false;
    if (!registered)
    {
        WNDCLASSEXW wc = { 0 };
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = L"FFXIDatEGSearchDialog";
        
        if (!RegisterClassExW(&wc))
            return false;
        registered = true;
    }
    
    // Get parent window position to center the search dialog
    RECT rcParent;
    GetWindowRect(hParent, &rcParent);
    int x = rcParent.left + (rcParent.right - rcParent.left - 400) / 2;
    int y = rcParent.top + (rcParent.bottom - rcParent.top - 180) / 2;
    
    // Get localized window title
    std::wstring title = LOC(L"Search.Dialog.Title");
    
    // Create modeless dialog window (initially hidden)
    m_hwnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        L"FFXIDatEGSearchDialog",
        title.c_str(),
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        x, y,
        400, 180,
        hParent,
        nullptr,
        GetModuleHandle(nullptr),
        this
    );
    
    if (!m_hwnd)
        return false;
    
    OnInitDialog();
    
    return true;
}

void SearchDialog::OnInitDialog()
{
    // Create controls
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    
    // Label - use localized text
    HWND hLabel = CreateWindowExW(
        0, L"STATIC", LOC(L"Search.Dialog.FindWhat").c_str(),
        WS_CHILD | WS_VISIBLE,
        10, 15, 80, 20,
        m_hwnd, nullptr, GetModuleHandle(nullptr), nullptr
    );
    SendMessage(hLabel, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    // Search text edit
    m_hEditSearch = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        100, 12, 280, 24,
        m_hwnd, (HMENU)(INT_PTR)IDC_EDIT_SEARCH, GetModuleHandle(nullptr), nullptr
    );
    SendMessage(m_hEditSearch, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    // Case sensitive checkbox - use localized text
    m_hCheckCase = CreateWindowExW(
        0, L"BUTTON", LOC(L"Search.Dialog.MatchCase").c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        10, 50, 150, 20,
        m_hwnd, (HMENU)(INT_PTR)IDC_CHECK_CASE, GetModuleHandle(nullptr), nullptr
    );
    SendMessage(m_hCheckCase, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    // Direction group - use localized text
    HWND hGroupBox = CreateWindowExW(
        0, L"BUTTON", LOC(L"Search.Dialog.Direction").c_str(),
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        10, 80, 150, 60,
        m_hwnd, nullptr, GetModuleHandle(nullptr), nullptr
    );
    SendMessage(hGroupBox, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    // Radio buttons - use localized text
    m_hRadioUp = CreateWindowExW(
        0, L"BUTTON", LOC(L"Search.Dialog.DirectionUp").c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON,
        20, 100, 60, 20,
        m_hwnd, (HMENU)(INT_PTR)IDC_RADIO_UP, GetModuleHandle(nullptr), nullptr
    );
    SendMessage(m_hRadioUp, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    m_hRadioDown = CreateWindowExW(
        0, L"BUTTON", LOC(L"Search.Dialog.DirectionDown").c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON,
        90, 100, 60, 20,
        m_hwnd, (HMENU)(INT_PTR)IDC_RADIO_DOWN, GetModuleHandle(nullptr), nullptr
    );
    SendMessage(m_hRadioDown, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    // Set down as default
    SendMessage(m_hRadioDown, BM_SETCHECK, BST_CHECKED, 0);
    
    // Buttons - use localized text
    HWND hBtnFindNext = CreateWindowExW(
        0, L"BUTTON", LOC(L"Search.Dialog.ButtonFindNext").c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
        200, 80, 90, 25,
        m_hwnd, (HMENU)(INT_PTR)IDC_BTN_FINDNEXT, GetModuleHandle(nullptr), nullptr
    );
    SendMessage(hBtnFindNext, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    HWND hBtnClose = CreateWindowExW(
        0, L"BUTTON", LOC(L"Search.Dialog.ButtonClose").c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        295, 80, 90, 25,
        m_hwnd, (HMENU)(INT_PTR)IDC_BTN_CLOSE, GetModuleHandle(nullptr), nullptr
    );
    SendMessage(hBtnClose, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    // Set focus to search edit
    SetFocus(m_hEditSearch);
}

void SearchDialog::Show()
{
    if (m_hwnd)
    {
        ShowWindow(m_hwnd, SW_SHOW);
        SetForegroundWindow(m_hwnd);
        SetFocus(m_hEditSearch);
        
        // Select all text in search box
        SendMessage(m_hEditSearch, EM_SETSEL, 0, -1);
    }
}

void SearchDialog::Hide()
{
    if (m_hwnd)
    {
        ShowWindow(m_hwnd, SW_HIDE);
    }
}

bool SearchDialog::IsVisible() const
{
    return m_hwnd && IsWindowVisible(m_hwnd);
}

void SearchDialog::SetSearchText(const std::wstring& text)
{
    m_searchText = text;
    if (m_hEditSearch)
    {
        SetWindowTextW(m_hEditSearch, text.c_str());
    }
}

void SearchDialog::OnFindNext()
{
    // Get search text
    int len = GetWindowTextLengthW(m_hEditSearch);
    if (len > 0)
    {
        m_searchText.resize(len + 1);
        GetWindowTextW(m_hEditSearch, &m_searchText[0], len + 1);
        m_searchText.resize(len);
    }
    else
    {
        m_searchText.clear();
    }
    
    // Get case sensitive option
    m_caseSensitive = (SendMessage(m_hCheckCase, BM_GETCHECK, 0, 0) == BST_CHECKED);
    
    // Get direction
    m_searchDown = (SendMessage(m_hRadioDown, BM_GETCHECK, 0, 0) == BST_CHECKED);
    
    // Notify parent
    SendMessage(m_hParent, WM_COMMAND, MAKEWPARAM(IDC_BTN_FINDNEXT, BN_CLICKED), 
                reinterpret_cast<LPARAM>(m_hwnd));
}

void SearchDialog::OnClose()
{
    Hide();
}

void SearchDialog::RefreshUIText()
{
    if (!m_hwnd)
        return;
    
    // Update window title
    SetWindowTextW(m_hwnd, LOC(L"Search.Dialog.Title").c_str());

    // Update group box and label by enumerating child windows
    EnumChildWindows(m_hwnd, [](HWND hwnd, LPARAM lParam) -> BOOL {
        wchar_t className[256];
        GetClassNameW(hwnd, className, 256);

        if (wcscmp(className, L"Button") == 0)
        {
            LONG style = GetWindowLong(hwnd, GWL_STYLE);
            if (style & BS_GROUPBOX)
            {
                SetWindowTextW(hwnd, LOC(L"Search.Dialog.Direction").c_str());
            }
        }
        else if (wcscmp(className, L"Static") == 0)
        {
            SetWindowTextW(hwnd, LOC(L"Search.Dialog.FindWhat").c_str());
        }

        return TRUE;
        }, 0);
    
    // Update controls text
    HWND hLabel = GetDlgItem(m_hwnd, -1);  // Labels don't have IDs, need to find by order
    // For simplicity, recreate controls or store handles
    
    // Update checkbox text
    if (m_hCheckCase)
        SetWindowTextW(m_hCheckCase, LOC(L"Search.Dialog.MatchCase").c_str());
    
    // Update radio buttons
    if (m_hRadioUp)
        SetWindowTextW(m_hRadioUp, LOC(L"Search.Dialog.DirectionUp").c_str());
    
    if (m_hRadioDown)
        SetWindowTextW(m_hRadioDown, LOC(L"Search.Dialog.DirectionDown").c_str());
    
    // Find and update buttons
    HWND hBtnFindNext = GetDlgItem(m_hwnd, IDC_BTN_FINDNEXT);
    if (hBtnFindNext)
        SetWindowTextW(hBtnFindNext, LOC(L"Search.Dialog.ButtonFindNext").c_str());
    
    HWND hBtnClose = GetDlgItem(m_hwnd, IDC_BTN_CLOSE);
    if (hBtnClose)
        SetWindowTextW(hBtnClose, LOC(L"Search.Dialog.ButtonClose").c_str());
}

LRESULT CALLBACK SearchDialog::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    SearchDialog* pThis = nullptr;
    
    if (uMsg == WM_NCCREATE)
    {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<SearchDialog*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hwnd = hwnd;
    }
    else
    {
        pThis = reinterpret_cast<SearchDialog*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    
    if (pThis)
    {
        return pThis->DialogProc(hwnd, uMsg, wParam, lParam);
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

INT_PTR CALLBACK SearchDialog::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_BTN_FINDNEXT:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                OnFindNext();
                return TRUE;
            }
            break;
            
        case IDC_BTN_CLOSE:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                OnClose();
                return TRUE;
            }
            break;
            
        case IDC_EDIT_SEARCH:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                // Could enable/disable Find Next button based on text
                return TRUE;
            }
            break;
        }
        break;
        
    case WM_CLOSE:
        OnClose();
        return TRUE;
        
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            OnClose();
            return TRUE;
        }
        break;
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
