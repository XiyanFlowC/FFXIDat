#include "FFXIDatEGApp.h"
#include "MainFrame.h"
#include "StringValidator.h"
#include "../FFXIDatProcessor/codepage.h"
#include <shlobj.h>
#include <fstream>
#include <sstream>

#pragma comment(lib, "comctl32.lib")

FFXIDatEGApp& FFXIDatEGApp::Instance()
{
    static FFXIDatEGApp instance;
    return instance;
}

bool FFXIDatEGApp::Initialize(HINSTANCE hInstance)
{
    m_hInstance = hInstance;
    
    // Get exe path for code page initialization
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();
    
    // Initialize CodeCvt with cp932.csv
    std::filesystem::path cp932Path = exeDir / L"cp932.csv";
    if (std::filesystem::exists(cp932Path))
    {
        try
        {
            CodeCvt::GetInstance().Init(cp932Path);
        }
        catch (const std::exception& e)
        {
            std::string errMsg = "Failed to initialize code page: ";
            errMsg += e.what();
            std::wstring wErrMsg(errMsg.begin(), errMsg.end());
            MessageBoxW(nullptr, wErrMsg.c_str(), L"Warning", MB_OK | MB_ICONWARNING);
        }
    }
    
    // Initialize string validator
    StringValidator::Initialize(exePath);
    
    // Get config path (AppData/Local/FFXIDatEG)
    wchar_t appDataPath[MAX_PATH];
    if (SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, appDataPath) == S_OK)
    {
        m_configPath = std::filesystem::path(appDataPath) / L"FFXIDatEG";
        std::filesystem::create_directories(m_configPath);
        m_configPath /= L"config.ini";
    }
    
    LoadConfig();
    
    // Try to get game path
    if (!InitializeGamePath())
    {
        if (!PromptForGamePath())
        {
            MessageBoxW(nullptr, 
                       L"Failed to locate FFXI installation. The application will exit.",
                       L"Error", MB_OK | MB_ICONERROR);
            return false;
        }
    }
    
    return true;
}

int FFXIDatEGApp::Run()
{
    // Create and show main window
    MainFrame* mainFrame = new MainFrame();
    if (!mainFrame->Create(m_hInstance))
    {
        delete mainFrame;
        return 1;
    }
    
    mainFrame->Show();
    
    // Message loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    delete mainFrame;
    return static_cast<int>(msg.wParam);
}

void FFXIDatEGApp::Shutdown()
{
    SaveConfig();
}

bool FFXIDatEGApp::LoadConfig()
{
    if (!std::filesystem::exists(m_configPath))
        return false;
    
    std::ifstream file(m_configPath);
    if (!file.is_open())
        return false;
    
    std::string line;
    while (std::getline(file, line))
    {
        size_t pos = line.find('=');
        if (pos != std::string::npos)
        {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            if (key == "GamePath")
            {
                m_gamePath = std::filesystem::path(
                    std::wstring(value.begin(), value.end()));
            }
        }
    }
    
    return true;
}

void FFXIDatEGApp::SaveConfig()
{
    std::ofstream file(m_configPath);
    if (!file.is_open())
        return;
    
    std::wstring pathStr = m_gamePath.wstring();
    std::string pathUtf8(pathStr.begin(), pathStr.end());
    file << "GamePath=" << pathUtf8 << std::endl;
}

bool FFXIDatEGApp::InitializeGamePath()
{
    // Check if loaded path is valid
    if (!m_gamePath.empty() && std::filesystem::exists(m_gamePath / "ROM"))
        return true;
    
    // Try to read from registry
    HKEY hKey;
    const wchar_t* subKeys[] = {
        L"SOFTWARE\\WOW6432Node\\PlayOnline\\InstallFolder",
        L"SOFTWARE\\WOW6432Node\\PlayOnlineEU\\InstallFolder",
        L"SOFTWARE\\WOW6432Node\\PlayOnlineUS\\InstallFolder"
    };
    
    const wchar_t* valueName = L"0001";
    wchar_t valueData[MAX_PATH];
    DWORD bufferSize = sizeof(valueData);
    DWORD valueType;

    for (const auto* subKey : subKeys)
    {
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, subKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            if (RegQueryValueExW(hKey, valueName, nullptr, &valueType, 
                                reinterpret_cast<LPBYTE>(valueData), &bufferSize) == ERROR_SUCCESS)
            {
                if (valueType == REG_SZ)
                {
                    m_gamePath = std::filesystem::path(valueData);
                    RegCloseKey(hKey);
                    
                    if (std::filesystem::exists(m_gamePath / "ROM"))
                    {
                        SetGamePath(m_gamePath);
                        return true;
                    }
                }
            }
            RegCloseKey(hKey);
        }
    }

    return false;
}

bool FFXIDatEGApp::PromptForGamePath(HWND hParent)
{
    BROWSEINFOW bi = { 0 };
    bi.hwndOwner = hParent;
    bi.lpszTitle = L"Select FFXI Installation Directory";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    
    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (pidl != nullptr)
    {
        wchar_t path[MAX_PATH];
        if (SHGetPathFromIDListW(pidl, path))
        {
            std::filesystem::path selectedPath(path);
            
            // Verify it's a valid FFXI installation
            if (!std::filesystem::exists(selectedPath / "ROM"))
            {
                MessageBoxW(hParent,
                           L"Invalid FFXI installation directory. ROM folder not found.",
                           L"Error", MB_OK | MB_ICONERROR);
                CoTaskMemFree(pidl);
                return false;
            }
            
            SetGamePath(selectedPath);
            CoTaskMemFree(pidl);
            return true;
        }
        CoTaskMemFree(pidl);
    }
    
    return false;
}

void FFXIDatEGApp::SetGamePath(const std::filesystem::path& path)
{
    m_gamePath = path;
    SaveConfig();
}

// WinMain entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Initialize common controls
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icc);
    
    // Initialize COM for folder browser
    CoInitialize(nullptr);
    
    FFXIDatEGApp& app = FFXIDatEGApp::Instance();
    
    if (!app.Initialize(hInstance))
    {
        CoUninitialize();
        return 1;
    }
    
    int result = app.Run();
    
    app.Shutdown();
    CoUninitialize();
    
    return result;
}
