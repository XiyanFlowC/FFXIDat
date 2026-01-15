#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <filesystem>
#include <string>

class FFXIDatEGApp
{
public:
    static FFXIDatEGApp& Instance();
    
    bool Initialize(HINSTANCE hInstance);
    int Run();
    void Shutdown();
    
    HINSTANCE GetInstance() const { return m_hInstance; }
    std::filesystem::path GetGamePath() const { return m_gamePath; }
    void SetGamePath(const std::filesystem::path& path);
    class Config& GetConfig();
    
private:
    FFXIDatEGApp() = default;
    ~FFXIDatEGApp() = default;
    FFXIDatEGApp(const FFXIDatEGApp&) = delete;
    FFXIDatEGApp& operator=(const FFXIDatEGApp&) = delete;
    
    bool InitializeGamePath();
    bool PromptForGamePath(HWND hParent = nullptr);
    bool LoadConfig();
    void SaveConfig();
    
    HINSTANCE m_hInstance = nullptr;
    std::filesystem::path m_gamePath;
    std::filesystem::path m_configPath;
};
