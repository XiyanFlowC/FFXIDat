#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <CommCtrl.h>
#include <memory>
#include <string>
#include <vector>
#include <filesystem>
#include "DatFileManager.h"
#include "ContentView.h"
#include "SearchDialog.h"
#include "Localization.h"  // Include for LanguageInfo

class MainFrame
{
public:
    MainFrame();
    ~MainFrame();
    
    bool Create(HINSTANCE hInstance);
    void Show();
    
private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    void OnCreate();
    void OnSize(int width, int height);
    void OnTreeItemActivated(HTREEITEM hItem);
    void OnCommand(WPARAM wParam);
    
    void LoadROMDefinitions();
    void OnChangeGamePath();
    void OnResetGamePath();
    void OnFilterLanguage(const std::string& language);
    void OnChangeUILanguage(const std::wstring& language);
    void RefreshUIText();
    void BuildUILanguageMenu(HMENU parentMenu);  // Build dynamic UI language menu
    void OnSelectFont();
    void OnAbout();
    void OnFind();
    void OnFindNext();
    
    HWND m_hwnd = nullptr;
    HWND m_hTreeView = nullptr;
    HWND m_hStatusBar = nullptr;
    HMENU m_hMenu = nullptr;
    HMENU m_hLanguageFilterMenu = nullptr;
    HMENU m_hUILanguageMenu = nullptr;
    
    std::unique_ptr<ContentView> m_contentView;
    std::unique_ptr<DatFileManager> m_fileManager;
    std::unique_ptr<SearchDialog> m_searchDialog;
    
    std::string m_languageFilter = "";  // Empty string means show all languages
    std::wstring m_uiLanguage = L"en";  // Current UI language
    std::vector<LanguageInfo> m_availableUILanguages;  // Available UI languages
    std::filesystem::path m_localDir;  // Path to local directory
    
    static constexpr int IDC_TREEVIEW = 1001;
    static constexpr int IDC_CONTENTVIEW = 1002;
    static constexpr int IDC_STATUSBAR = 1003;
    
    // Menu IDs
    static constexpr int IDM_FILE_CHANGEPATH = 2001;
    static constexpr int IDM_FILE_RESETPATH = 2002;
    static constexpr int IDM_FILE_EXIT = 2003;
    static constexpr int IDM_EDIT_FIND = 2051;
    static constexpr int IDM_EDIT_FINDNEXT = 2052;
    static constexpr int IDM_VIEW_FONT = 2101;
    static constexpr int IDM_VIEW_FILTER_JP = 2151;
    static constexpr int IDM_VIEW_FILTER_EN = 2152;
    static constexpr int IDM_VIEW_FILTER_FR = 2153;
    static constexpr int IDM_VIEW_FILTER_DE = 2154;
    static constexpr int IDM_VIEW_FILTER_ALL = 2155;
    static constexpr int IDM_VIEW_UILANG_BASE = 2160;  // Base ID for dynamic UI language menu items
    static constexpr int IDM_HELP_ABOUT = 2201;
};
