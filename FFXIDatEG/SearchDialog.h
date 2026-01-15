#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>

class SearchDialog
{
public:
    SearchDialog();
    ~SearchDialog();
    
    bool Create(HWND hParent);
    void Show();
    void Hide();
    bool IsVisible() const;
    
    std::wstring GetSearchText() const { return m_searchText; }
    bool IsCaseSensitive() const { return m_caseSensitive; }
    bool IsSearchDown() const { return m_searchDown; }
    
    void SetSearchText(const std::wstring& text);
    void RefreshUIText();  // Refresh localized text
    
private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    void OnInitDialog();
    void OnFindNext();
    void OnClose();
    
    HWND m_hwnd;
    HWND m_hParent;
    HWND m_hEditSearch;
    HWND m_hCheckCase;
    HWND m_hRadioDown;
    HWND m_hRadioUp;
    
    std::wstring m_searchText;
    bool m_caseSensitive;
    bool m_searchDown;
    
    static constexpr int IDC_EDIT_SEARCH = 1001;
    static constexpr int IDC_CHECK_CASE = 1002;
    static constexpr int IDC_RADIO_DOWN = 1003;
    static constexpr int IDC_RADIO_UP = 1004;
    static constexpr int IDC_BTN_FINDNEXT = 1005;
    static constexpr int IDC_BTN_CLOSE = 1006;
};
