#include "SetupWizard.h"
#include <Windows.h>
#include <shobjidl.h>
#include <shellapi.h>
#include <fstream>
#include <map>
#include <set>
#include <optional>
#include <vector>
#include <algorithm>
#include <regex>
#include <string>
#include <sstream>
#include <CsvFile.h>
#include <xystring.h>

#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "Shell32.lib")

namespace
{
    namespace fs = std::filesystem;

    constexpr wchar_t kWizardClassName[] = L"FFXITransSetupWizardWindow";
    constexpr int kWindowWidth = 760;
    constexpr int kWindowHeight = 560;

    enum ControlId : int
    {
        IdBack = 100,
        IdNext,
        IdCancel,
        IdTitle,
        IdDescription,
        IdStatus,
        IdPrimaryRadio1,
        IdPrimaryRadio2,
        IdPrimaryRadio3,
        IdPrimaryRadio4,
        IdSecondaryRadio1,
        IdSecondaryRadio2,
        IdCheck1,
        IdCheck2,
        IdEdit1,
        IdEdit2,
        IdBrowse1,
        IdBrowse2,
        IdAction1,
        IdAction2,
        IdAction3,
        IdList1,
        IdStatic1,
        IdStatic2,
        IdStatic3,
        IdSummary,
    };

    enum class StepId
    {
        Welcome,
        GamePath,
        Launcher,
        AshitaPath,
        PivotCheck,
        PivotConfig,
        Babel,
        Noname,
        Excludes,
        Summary,
        ManualFinish,
        InstallFinish,
    };

    enum class LauncherChoice
    {
        Ashita,
        Windower,
        NoneInstalled,
    };

    enum class BabelMode
    {
        Off,
        Bilingual,
        Exotic,
        Tower,
    };

    struct DetectedGamePath
    {
        fs::path gamePath;
        bool englishMode = false;
    };

    struct WizardConfigDraft
    {
        bool manualConfig = false;
        bool useDetectedGamePath = true;
        std::optional<fs::path> manualGamePath;
        std::optional<bool> manualEnglishMode;
        LauncherChoice launcherChoice = LauncherChoice::Ashita;
        bool willingToInstall = false;
        fs::path ashitaPath;
        fs::path pivotRoot;
        fs::path outputPath;
        BabelMode babelMode = BabelMode::Off;
        bool noname = false;
        std::vector<std::u8string> excludes;
    };

    struct IniData
    {
        std::map<std::wstring, std::map<std::wstring, std::wstring>> sections;
    };

    struct PivotConfiguration
    {
        bool valid = false;
        std::wstring errorMessage;
        fs::path pivotIniPath;
        IniData iniData;
        fs::path pivotRoot;
        fs::path outputPath;
    };

    std::wstring Trim(const std::wstring& value)
    {
        return std::regex_replace(value, std::wregex(L"^\\s+|\\s+$"), L"");
    }

    std::wstring Unquote(std::wstring value)
    {
        value = Trim(value);
        if (value.size() >= 2)
        {
            if ((value.front() == L'"' && value.back() == L'"') || (value.front() == L'\'' && value.back() == L'\''))
                value = value.substr(1, value.size() - 2);
        }
        return value;
    }

    std::wstring ToDisplayPath(const fs::path& path)
    {
        return path.empty() ? L"（未设置）" : path.wstring();
    }

    std::wstring GetBabelConfigValue(BabelMode mode)
    {
        switch (mode)
        {
        case BabelMode::Bilingual:
            return L"bilingual";
        case BabelMode::Exotic:
            return L"exotic";
        case BabelMode::Tower:
            return L"tower";
        case BabelMode::Off:
        default:
            return L"false";
        }
    }

    std::wstring GetBabelDisplayName(BabelMode mode)
    {
        switch (mode)
        {
        case BabelMode::Bilingual:
            return L"双语";
        case BabelMode::Exotic:
            return L"异国";
        case BabelMode::Tower:
            return L"塔";
        case BabelMode::Off:
        default:
            return L"关";
        }
    }

    std::wstring ReadWindowText(HWND control)
    {
        const int length = GetWindowTextLengthW(control);
        std::wstring text(static_cast<size_t>(length), L'\0');
        GetWindowTextW(control, text.data(), length + 1);
        return text;
    }

    void OpenUrl(const wchar_t* url)
    {
        ShellExecuteW(nullptr, L"open", url, nullptr, nullptr, SW_SHOWNORMAL);
    }

    void OpenPath(const fs::path& path)
    {
        ShellExecuteW(nullptr, L"open", path.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }

    std::optional<fs::path> PickFolder(const std::wstring& title, const fs::path& defaultFolder = {})
    {
        IFileDialog* fileDialog = nullptr;
        if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&fileDialog))))
            return std::nullopt;

        DWORD options = 0;
        fileDialog->GetOptions(&options);
        fileDialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
        fileDialog->SetTitle(title.c_str());

        if (!defaultFolder.empty() && fs::exists(defaultFolder))
        {
            IShellItem* defaultItem = nullptr;
            if (SUCCEEDED(SHCreateItemFromParsingName(defaultFolder.wstring().c_str(), nullptr, IID_PPV_ARGS(&defaultItem))))
            {
                fileDialog->SetDefaultFolder(defaultItem);
                defaultItem->Release();
            }
        }

        const HRESULT showResult = fileDialog->Show(nullptr);
        if (FAILED(showResult))
        {
            fileDialog->Release();
            return std::nullopt;
        }

        IShellItem* selected = nullptr;
        if (FAILED(fileDialog->GetResult(&selected)))
        {
            fileDialog->Release();
            return std::nullopt;
        }

        PWSTR rawPath = nullptr;
        std::optional<fs::path> result;
        if (SUCCEEDED(selected->GetDisplayName(SIGDN_FILESYSPATH, &rawPath)) && rawPath != nullptr)
        {
            result = fs::path(rawPath);
            CoTaskMemFree(rawPath);
        }

        selected->Release();
        fileDialog->Release();
        return result;
    }

    std::optional<DetectedGamePath> TryReadPolRegistryPath(const wchar_t* keyPath, bool englishMode)
    {
        HKEY hKey = nullptr;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
            return std::nullopt;

        wchar_t valueData[MAX_PATH] = {};
        DWORD valueType = 0;
        DWORD bufferSize = sizeof(valueData);
        const auto status = RegQueryValueExW(hKey, L"0001", nullptr, &valueType, reinterpret_cast<LPBYTE>(valueData), &bufferSize);
        RegCloseKey(hKey);

        if (status != ERROR_SUCCESS || valueType != REG_SZ)
            return std::nullopt;

        DetectedGamePath result;
        result.gamePath = fs::path(valueData);
        result.englishMode = englishMode;
        if (result.gamePath.empty() || !fs::exists(result.gamePath))
            return std::nullopt;
        return result;
    }

    std::optional<DetectedGamePath> DetectGamePathFromRegistry()
    {
        if (auto jp = TryReadPolRegistryPath(L"SOFTWARE\\WOW6432Node\\PlayOnline\\InstallFolder", false))
            return jp;
        if (auto eu = TryReadPolRegistryPath(L"SOFTWARE\\WOW6432Node\\PlayOnlineEU\\InstallFolder", true))
            return eu;
        if (auto us = TryReadPolRegistryPath(L"SOFTWARE\\WOW6432Node\\PlayOnlineUS\\InstallFolder", true))
            return us;
        return std::nullopt;
    }

    bool ContainsRequiredAshitaFiles(const fs::path& ashitaPath)
    {
        return fs::exists(ashitaPath / L"Ashita.dll") && fs::exists(ashitaPath / L"Ashita-cli.exe");
    }

    bool LoadIniFile(const fs::path& iniPath, IniData& data)
    {
        if (!fs::exists(iniPath))
            return false;

        std::wifstream file(iniPath);
        if (!file.is_open())
            return false;

        std::wstring line;
        std::wstring currentSection;
        while (std::getline(file, line))
        {
            line = Trim(line);
            if (line.empty() || line[0] == L';' || line[0] == L'#')
                continue;

            if (line.front() == L'[' && line.back() == L']' && line.size() >= 3)
            {
                currentSection = line.substr(1, line.size() - 2);
                data.sections[currentSection];
                continue;
            }

            const auto eqPos = line.find(L'=');
            if (eqPos == std::wstring::npos)
                continue;

            data.sections[currentSection][Trim(line.substr(0, eqPos))] = Trim(line.substr(eqPos + 1));
        }

        return true;
    }

    bool SaveIniFile(const fs::path& iniPath, const IniData& data)
    {
        fs::create_directories(iniPath.parent_path());
        std::wofstream file(iniPath, std::ios::trunc);
        if (!file.is_open())
            return false;

        bool firstSection = true;
        for (const auto& [section, kv] : data.sections)
        {
            if (!firstSection)
                file << L"\n";
            firstSection = false;

            if (!section.empty())
                file << L"[" << section << L"]\n";
            for (const auto& [key, value] : kv)
                file << key << L"=" << value << L"\n";
        }

        return true;
    }

    std::vector<std::u8string> LoadDefsComments(const fs::path& progRoot)
    {
        std::vector<std::u8string> comments;
        std::set<std::u8string> unique;
        const auto defsPath = progRoot / L"defs.csv";
        if (!fs::exists(defsPath))
            return comments;

        CsvFile defs(defsPath, std::ios::in | std::ios::binary);
        while (!defs.IsEof())
        {
            std::u8string path = defs.NextCell();
            std::u8string type = defs.NextCell();
            std::u8string lang = defs.NextCell();
            std::u8string comment = defs.NextCell();
            defs.NextLine();

            if (path.empty() || type.empty() || lang.empty() || comment.empty())
                continue;

            if (unique.insert(comment).second)
                comments.push_back(comment);
        }

        std::sort(comments.begin(), comments.end());
        return comments;
    }

    PivotConfiguration BuildPivotConfiguration(const fs::path& ashitaPath)
    {
        PivotConfiguration result;
        result.pivotIniPath = ashitaPath / L"config" / L"pivot" / L"pivot.ini";
        const fs::path defaultPivotRoot = ashitaPath / L"polplugins" / L"DATs";

        LoadIniFile(result.pivotIniPath, result.iniData);
        auto& settings = result.iniData.sections[L"settings"];
        auto& overlays = result.iniData.sections[L"overlays"];

        if (const auto it = settings.find(L"root_path"); it != settings.end() && !it->second.empty())
        {
            const fs::path rootPath = fs::path(Unquote(it->second));
            if (rootPath.is_relative())
            {
                result.errorMessage = L"pivot.ini 的 settings.root_path 是相对路径，必须手动设定。";
                return result;
            }
            result.pivotRoot = rootPath;
        }
        else
        {
            result.pivotRoot = defaultPivotRoot;
        }

        if (settings.find(L"debug_log") == settings.end())
            settings[L"debug_log"] = L"false";
        if (settings.find(L"redirect_fopens") == settings.end())
            settings[L"redirect_fopens"] = L"true";

        bool hasLocCNTxt = false;
        int maxIndex = -1;
        for (const auto& [key, value] : overlays)
        {
            if (value == L"LocCNTxt")
                hasLocCNTxt = true;

            try
            {
                size_t consumed = 0;
                const int idx = std::stoi(key, &consumed);
                if (consumed == key.size() && idx > maxIndex)
                    maxIndex = idx;
            }
            catch (...)
            {
            }
        }

        if (!hasLocCNTxt)
            overlays[std::to_wstring(maxIndex + 1)] = L"LocCNTxt";

        result.outputPath = result.pivotRoot / L"LocCNTxt";
        result.valid = true;
        return result;
    }

    bool WriteConfigIni(const fs::path& progRoot, const WizardConfigDraft& draft)
    {
        const auto configPath = progRoot / L"config.ini";
        std::wofstream file(configPath, std::ios::trunc);
        if (!file.is_open())
            return false;

        file << L"; Auto generated by FFXITrans setup wizard\n";
        file << L"output_path=" << draft.outputPath.wstring() << L"\n";
        file << L"babel=" << GetBabelConfigValue(draft.babelMode) << L"\n";
        file << L"noname=" << (draft.noname ? L"true" : L"false") << L"\n";
        file << L"in_situ = 0\n"
            L"en_as_ja = 1\n"
            L"ejref_tolerance = 1\n"
            L"verbose = 0\n"
            L"no_mismatch_log = 1\n";

        if (!draft.useDetectedGamePath && draft.manualGamePath.has_value() && draft.manualEnglishMode.has_value())
        {
            file << L"game_path=" << draft.manualGamePath->wstring() << L"\n";
            file << L"english_mode=" << (*draft.manualEnglishMode ? L"true" : L"false") << L"\n";
        }

        if (!draft.excludes.empty())
        {
            file << L"excludes=";
            for (size_t i = 0; i < draft.excludes.size(); ++i)
            {
                if (i > 0)
                    file << L",";
                file << xybase::string::to_wstring(draft.excludes[i]);
            }
            file << L"\n";
        }

        return true;
    }

    class SetupWizardWindow
    {
    public:
        explicit SetupWizardWindow(fs::path root)
            : progRoot(std::move(root))
        {
            detectedGamePath = DetectGamePathFromRegistry();
            draft.useDetectedGamePath = detectedGamePath.has_value();
            if (detectedGamePath.has_value())
            {
                draft.manualGamePath = detectedGamePath->gamePath;
                draft.manualEnglishMode = detectedGamePath->englishMode;
            }
            defsComments = LoadDefsComments(progRoot);
        }

        bool Run()
        {
            if (!RegisterWindowClass())
                return true;

            hwnd = CreateWindowExW(
                WS_EX_CONTROLPARENT,
                kWizardClassName,
                L"FFXITrans 配置向导",
                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                kWindowWidth,
                kWindowHeight,
                nullptr,
                nullptr,
                GetModuleHandleW(nullptr),
                this);

            if (hwnd == nullptr)
                return true;

            ShowWindow(hwnd, SW_SHOW);
            UpdateWindow(hwnd);

            MSG msg;
            while (IsWindow(hwnd) && GetMessageW(&msg, nullptr, 0, 0) > 0)
            {
                if (!IsDialogMessageW(hwnd, &msg))
                {
                    TranslateMessage(&msg);
                    DispatchMessageW(&msg);
                }
            }

            return true;
        }

    private:
        fs::path progRoot;
        std::optional<DetectedGamePath> detectedGamePath;
        std::vector<std::u8string> defsComments;
        std::vector<StepId> history;
        WizardConfigDraft draft;
        PivotConfiguration pivotPreview;
        std::wstring terminalMessage;
        HWND hwnd = nullptr;
        HWND titleLabel = nullptr;
        HWND descriptionLabel = nullptr;
        HWND statusLabel = nullptr;
        HWND backButton = nullptr;
        HWND nextButton = nullptr;
        HWND cancelButton = nullptr;
        std::vector<HWND> stepControls;
        HFONT normalFont = nullptr;
        HFONT titleFont = nullptr;
        StepId currentStep = StepId::Welcome;

        static LRESULT CALLBACK WndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
        {
            SetupWizardWindow* self = nullptr;
            if (message == WM_NCCREATE)
            {
                auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
                self = static_cast<SetupWizardWindow*>(create->lpCreateParams);
                SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
                self->hwnd = window;
            }
            else
            {
                self = reinterpret_cast<SetupWizardWindow*>(GetWindowLongPtrW(window, GWLP_USERDATA));
            }

            if (self == nullptr)
                return DefWindowProcW(window, message, wParam, lParam);

            switch (message)
            {
            case WM_CREATE:
                self->OnCreate();
                return 0;
            case WM_CTLCOLORSTATIC:
            {
                HDC dc = reinterpret_cast<HDC>(wParam);
                SetBkMode(dc, TRANSPARENT);
                SetTextColor(dc, GetSysColor(COLOR_WINDOWTEXT));
                return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_3DFACE));
            }
            case WM_COMMAND:
                self->OnCommand(LOWORD(wParam), HIWORD(wParam), reinterpret_cast<HWND>(lParam));
                return 0;
            case WM_CLOSE:
                DestroyWindow(window);
                return 0;
            case WM_DESTROY:
                self->OnDestroy();
                PostQuitMessage(0);
                return 0;
            default:
                return DefWindowProcW(window, message, wParam, lParam);
            }
        }

        bool RegisterWindowClass()
        {
            static bool registered = false;
            if (registered)
                return true;

            WNDCLASSW wc = {};
            wc.lpfnWndProc = &SetupWizardWindow::WndProc;
            wc.hInstance = GetModuleHandleW(nullptr);
            wc.lpszClassName = kWizardClassName;
            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            wc.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
            if (!RegisterClassW(&wc))
                return false;

            registered = true;
            return true;
        }

        void OnCreate()
        {
            normalFont = CreateMessageFont();
            titleFont = CreateWizardTitleFont();

            titleLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 20, 18, 700, 28, hwnd, reinterpret_cast<HMENU>(IdTitle), nullptr, nullptr);
            descriptionLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 20, 50, 700, 48, hwnd, reinterpret_cast<HMENU>(IdDescription), nullptr, nullptr);
            statusLabel = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE, 20, 448, 500, 24, hwnd, reinterpret_cast<HMENU>(IdStatus), nullptr, nullptr);
            backButton = CreateWindowExW(0, L"BUTTON", L"< 上一步", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 430, 488, 90, 28, hwnd, reinterpret_cast<HMENU>(IdBack), nullptr, nullptr);
            nextButton = CreateWindowExW(0, L"BUTTON", L"下一步 >", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON, 530, 488, 90, 28, hwnd, reinterpret_cast<HMENU>(IdNext), nullptr, nullptr);
            cancelButton = CreateWindowExW(0, L"BUTTON", L"取消", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 630, 488, 90, 28, hwnd, reinterpret_cast<HMENU>(IdCancel), nullptr, nullptr);

            SendMessageW(titleLabel, WM_SETFONT, reinterpret_cast<WPARAM>(titleFont), TRUE);
            SendMessageW(descriptionLabel, WM_SETFONT, reinterpret_cast<WPARAM>(normalFont), TRUE);
            SendMessageW(statusLabel, WM_SETFONT, reinterpret_cast<WPARAM>(normalFont), TRUE);
            SendMessageW(backButton, WM_SETFONT, reinterpret_cast<WPARAM>(normalFont), TRUE);
            SendMessageW(nextButton, WM_SETFONT, reinterpret_cast<WPARAM>(normalFont), TRUE);
            SendMessageW(cancelButton, WM_SETFONT, reinterpret_cast<WPARAM>(normalFont), TRUE);

            RenderStep();
        }

        void OnDestroy()
        {
            if (titleFont != nullptr)
                DeleteObject(titleFont);
            if (normalFont != nullptr)
                DeleteObject(normalFont);
        }

        HFONT CreateMessageFont() const
        {
            NONCLIENTMETRICSW metrics = {};
            metrics.cbSize = sizeof(metrics);
            if (!SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0))
            {
                LOGFONTW fallback = {};
                GetObjectW(GetStockObject(DEFAULT_GUI_FONT), sizeof(fallback), &fallback);
                return CreateFontIndirectW(&fallback);
            }

            return CreateFontIndirectW(&metrics.lfMessageFont);
        }

        HFONT CreateWizardTitleFont() const
        {
            NONCLIENTMETRICSW metrics = {};
            metrics.cbSize = sizeof(metrics);
            if (!SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0))
            {
                LOGFONTW fallback = {};
                GetObjectW(GetStockObject(DEFAULT_GUI_FONT), sizeof(fallback), &fallback);
                fallback.lfHeight = 22;
                fallback.lfWeight = FW_BOLD;
                return CreateFontIndirectW(&fallback);
            }

            LOGFONTW lf = metrics.lfMessageFont;
            lf.lfHeight = 22;
            lf.lfWeight = FW_BOLD;
            return CreateFontIndirectW(&lf);
        }

        void OnCommand(int id, int code, HWND control)
        {
            if (id == IdCancel)
            {
                DestroyWindow(hwnd);
                return;
            }
            if (id == IdBack)
            {
                NavigateBack();
                return;
            }
            if (id == IdNext)
            {
                NavigateNext();
                return;
            }
            if (code == BN_CLICKED)
            {
                HandleButtonClick(id, control);
                return;
            }
            if (code == LBN_SELCHANGE && id == IdList1)
            {
                SetStatus(L"");
            }
        }

        void HandleButtonClick(int id, HWND)
        {
            switch (currentStep)
            {
            case StepId::Welcome:
                if (id == IdAction1)
                    OpenReadme();
                break;
            case StepId::GamePath:
                if (id == IdBrowse1)
                    BrowseGamePath();
                else if (id == IdPrimaryRadio1 || id == IdPrimaryRadio2)
                    UpdateGamePathControlState();
                break;
            case StepId::Launcher:
                if (id == IdAction1)
                    OpenUrl(L"https://docs.ashitaxi.com/installation");
                else if (id == IdAction2)
                    OpenUrl(L"https://www.windower.net/");
                break;
            case StepId::AshitaPath:
                if (id == IdBrowse1)
                    BrowseAshitaPath();
                break;
            case StepId::PivotCheck:
                if (id == IdAction1)
                    OpenUrl(L"https://github.com/HealsCodes/XIPivot");
                else if (id == IdAction2)
                    RefreshPivotCheck();
                break;
            case StepId::PivotConfig:
                if (id == IdAction1)
                    OpenPath(draft.ashitaPath / L"config" / L"pivot");
                else if (id == IdAction2)
                    RefreshPivotPreview();
                break;
            case StepId::Excludes:
                if (id == IdAction1)
                    SelectAllExcludes(true);
                else if (id == IdAction2)
                    SelectAllExcludes(false);
                break;
            case StepId::ManualFinish:
                if (id == IdAction1)
                    OpenReadme();
                break;
            case StepId::InstallFinish:
                if (id == IdAction1)
                    OpenUrl(L"https://docs.ashitaxi.com/installation/");
                else if (id == IdAction2)
                    OpenUrl(L"https://www.windower.net/");
                break;
            default:
                break;
            }
        }

        void NavigateBack()
        {
            if (history.empty())
                return;

            currentStep = history.back();
            history.pop_back();
            RenderStep();
        }

        void NavigateNext()
        {
            SetStatus(L"");
            const auto next = ValidateAndResolveNextStep();
            if (!next.has_value())
                return;

            if (*next == StepId::Summary)
            {
                if (!PreparePivotPreview())
                    return;
            }

            if (IsFinishStep(currentStep))
            {
                DestroyWindow(hwnd);
                return;
            }

            if (*next == StepId::Summary && currentStep == StepId::Excludes)
            {
                history.push_back(currentStep);
                currentStep = *next;
                RenderStep();
                return;
            }

            if (currentStep == StepId::Summary)
            {
                if (!FinalizeConfiguration())
                    return;
                MessageBoxW(hwnd, L"配置完成，请重新运行本程序。", L"FFXITrans 配置向导", MB_OK | MB_ICONINFORMATION);
                DestroyWindow(hwnd);
                return;
            }

            history.push_back(currentStep);
            currentStep = *next;
            RenderStep();
        }

        bool IsFinishStep(StepId step) const
        {
            return step == StepId::ManualFinish || step == StepId::InstallFinish;
        }

        std::optional<StepId> ValidateAndResolveNextStep()
        {
            switch (currentStep)
            {
            case StepId::Welcome:
                draft.manualConfig = IsButtonChecked(IdPrimaryRadio1);
                if (draft.manualConfig)
                {
                    terminalMessage = L"已选择手动配置。请阅读“请读我”（README）后自行配置，然后重新运行本程序。";
                    return StepId::ManualFinish;
                }
                return StepId::GamePath;

            case StepId::GamePath:
                return ValidateGamePathStep();

            case StepId::Launcher:
                return ValidateLauncherStep();

            case StepId::AshitaPath:
                return ValidateAshitaPathStep();

            case StepId::PivotCheck:
                if (!fs::exists(draft.ashitaPath / L"polplugins" / L"pivot.dll"))
                {
                    SetStatus(L"未找到 pivot.dll。请先安装 Pivot，或返回上一步检查 Ashita 路径。", true);
                    return std::nullopt;
                }
                return StepId::PivotConfig;

            case StepId::PivotConfig:
                if (!PreparePivotPreview())
                    return std::nullopt;
                return StepId::Babel;

            case StepId::Babel:
                if (IsButtonChecked(IdPrimaryRadio1))
                    draft.babelMode = BabelMode::Off;
                else if (IsButtonChecked(IdPrimaryRadio2))
                    draft.babelMode = BabelMode::Bilingual;
                else if (IsButtonChecked(IdPrimaryRadio3))
                    draft.babelMode = BabelMode::Exotic;
                else
                    draft.babelMode = BabelMode::Tower;
                return StepId::Noname;

            case StepId::Noname:
                draft.noname = IsButtonChecked(IdPrimaryRadio1);
                return StepId::Excludes;

            case StepId::Excludes:
                CollectExcludeSelections();
                return StepId::Summary;

            case StepId::Summary:
                return StepId::Summary;

            case StepId::ManualFinish:
            case StepId::InstallFinish:
                return currentStep;
            }

            return std::nullopt;
        }

        std::optional<StepId> ValidateGamePathStep()
        {
            const bool manual = IsButtonChecked(IdPrimaryRadio2) || !detectedGamePath.has_value();
            draft.useDetectedGamePath = !manual && detectedGamePath.has_value();

            if (draft.useDetectedGamePath)
            {
                draft.manualGamePath.reset();
                draft.manualEnglishMode.reset();
                return StepId::Launcher;
            }

            const auto rawPath = Trim(ReadWindowText(GetDlgItem(hwnd, IdEdit1)));
            if (rawPath.empty())
            {
                SetStatus(L"请输入游戏安装目录。", true);
                return std::nullopt;
            }

            const fs::path gamePath = rawPath;
            if (!fs::exists(gamePath) || !fs::is_directory(gamePath))
            {
                SetStatus(L"游戏安装目录无效，请输入存在的目录。", true);
                return std::nullopt;
            }

            draft.manualGamePath = gamePath;
            draft.manualEnglishMode = IsButtonChecked(IdSecondaryRadio1);
            return StepId::Launcher;
        }

        std::optional<StepId> ValidateLauncherStep()
        {
            if (IsButtonChecked(IdPrimaryRadio1))
                draft.launcherChoice = LauncherChoice::Ashita;
            else if (IsButtonChecked(IdPrimaryRadio2))
                draft.launcherChoice = LauncherChoice::Windower;
            else
                draft.launcherChoice = LauncherChoice::NoneInstalled;

            draft.willingToInstall = IsButtonChecked(IdCheck1);

            if (draft.launcherChoice == LauncherChoice::Windower)
            {
                terminalMessage = L"已选择正在使用 Windower。该场景必须手动设定，请阅读“请读我”后自行配置。";
                return StepId::ManualFinish;
            }

            if (draft.launcherChoice == LauncherChoice::NoneInstalled)
            {
                if (!draft.willingToInstall)
                {
                    terminalMessage = L"未使用 Windower/Ashita，且不安装 Ashita。该场景必须手动设定，请阅读“请读我”后自行配置。";
                    return StepId::ManualFinish;
                }
                return StepId::InstallFinish;
            }

            return StepId::AshitaPath;
        }

        std::optional<StepId> ValidateAshitaPathStep()
        {
            const auto rawPath = Trim(ReadWindowText(GetDlgItem(hwnd, IdEdit1)));
            if (rawPath.empty())
            {
                SetStatus(L"请输入 Ashita 安装目录。", true);
                return std::nullopt;
            }

            const fs::path ashitaPath = rawPath;
            if (!fs::exists(ashitaPath) || !fs::is_directory(ashitaPath))
            {
                SetStatus(L"Ashita 安装目录无效。", true);
                return std::nullopt;
            }
            if (!ContainsRequiredAshitaFiles(ashitaPath))
            {
                SetStatus(L"该目录下缺少 Ashita.dll 或 Ashita-cli.exe。", true);
                return std::nullopt;
            }

            draft.ashitaPath = ashitaPath;
            return StepId::PivotCheck;
        }

        bool PreparePivotPreview()
        {
            pivotPreview = BuildPivotConfiguration(draft.ashitaPath);
            if (!pivotPreview.valid)
            {
                SetStatus(pivotPreview.errorMessage, true);
                return false;
            }
            draft.pivotRoot = pivotPreview.pivotRoot;
            draft.outputPath = pivotPreview.outputPath;
            return true;
        }

        bool FinalizeConfiguration()
        {
            if (!PreparePivotPreview())
                return false;
            if (!SaveIniFile(pivotPreview.pivotIniPath, pivotPreview.iniData))
            {
                SetStatus(L"写入 pivot.ini 失败，请确认 Ashita 目录可写。", true);
                return false;
            }
            if (!WriteConfigIni(progRoot, draft))
            {
                SetStatus(L"写入 config.ini 失败，请检查程序目录写入权限。", true);
                return false;
            }
            return true;
        }

        void RenderStep()
        {
            ClearStepControls();
            SetStatus(L"");

            switch (currentStep)
            {
            case StepId::Welcome:
                RenderWelcomeStep();
                break;
            case StepId::GamePath:
                RenderGamePathStep();
                break;
            case StepId::Launcher:
                RenderLauncherStep();
                break;
            case StepId::AshitaPath:
                RenderAshitaPathStep();
                break;
            case StepId::PivotCheck:
                RenderPivotCheckStep();
                break;
            case StepId::PivotConfig:
                RenderPivotConfigStep();
                break;
            case StepId::Babel:
                RenderBabelStep();
                break;
            case StepId::Noname:
                RenderNonameStep();
                break;
            case StepId::Excludes:
                RenderExcludesStep();
                break;
            case StepId::Summary:
                RenderSummaryStep();
                break;
            case StepId::ManualFinish:
                RenderManualFinishStep();
                break;
            case StepId::InstallFinish:
                RenderInstallFinishStep();
                break;
            }

            UpdateNavigationButtons();
        }

        void UpdateNavigationButtons()
        {
            EnableWindow(backButton, !history.empty());

            switch (currentStep)
            {
            case StepId::Summary:
                SetWindowTextW(nextButton, L"完成");
                break;
            case StepId::ManualFinish:
            case StepId::InstallFinish:
                SetWindowTextW(nextButton, L"关闭");
                break;
            default:
                SetWindowTextW(nextButton, L"下一步 >");
                break;
            }
        }

        void RenderWelcomeStep()
        {
            SetWindowTextW(titleLabel, L"欢迎使用 FFXITrans 配置向导");
            SetWindowTextW(descriptionLabel, L"未检测到 config.ini。该向导会逐步收集配置，并在最后生成配置文件。\r\n你可以随时使用“上一步”返回修改。\r\n\r\n首先请选择是否改为手动配置。");

            CreateRadio(IdPrimaryRadio1, L"改为手动配置（阅读请读我后自行编辑 config.ini）", 30, 130, 620, 24, draft.manualConfig, true);
            CreateRadio(IdPrimaryRadio2, L"使用向导自动配置", 30, 160, 620, 24, !draft.manualConfig, false);
            CreateButton(IdAction1, L"打开 README", 30, 210, 120, 28);
        }

        void RenderGamePathStep()
        {
            SetWindowTextW(titleLabel, L"步骤 1：游戏安装目录");
            std::wstring description = L"将检查游戏安装目录，并允许手动修改。\r\n";
            if (detectedGamePath.has_value())
                description += L"若自动检测没有问题，一般不需要手动设定。\r\n";
            else
                description += L"自动获取游戏安装信息失败，必须手动设定游戏安装目录和游戏版本。\r\n";
            SetWindowTextW(descriptionLabel, description.c_str());

            const bool allowDetected = detectedGamePath.has_value();
            const bool useDetected = allowDetected && draft.useDetectedGamePath;

            CreateRadio(IdPrimaryRadio1,
                (L"使用自动检测结果：" + (allowDetected ? detectedGamePath->gamePath.wstring() : std::wstring(L"未检测到"))).c_str(),
                30, 120, 670, 24, useDetected, true, allowDetected);
            CreateRadio(IdPrimaryRadio2, L"手动指定游戏安装目录", 30, 150, 300, 24, !useDetected, false, true);
            CreateStatic(IdStatic1, L"游戏安装目录：", 50, 190, 120, 20);
            CreateEdit(IdEdit1, draft.manualGamePath.value_or(fs::path{}).wstring(), 170, 186, 420, 24, !useDetected || !allowDetected);
            CreateButton(IdBrowse1, L"浏览...", 600, 185, 90, 26, !useDetected || !allowDetected);
            CreateStatic(IdStatic2, L"语言模式：", 50, 225, 120, 20);
            CreateRadio(IdSecondaryRadio1, L"英文版（EU/US）", 170, 223, 150, 24, draft.manualEnglishMode.value_or(false), true, !useDetected || !allowDetected);
            CreateRadio(IdSecondaryRadio2, L"日文版", 330, 223, 100, 24, !draft.manualEnglishMode.value_or(false), false, !useDetected || !allowDetected);
        }

        void RenderLauncherStep()
        {
            SetWindowTextW(titleLabel, L"步骤 2：启动器类型");
            SetWindowTextW(descriptionLabel, L"请选择你当前使用的启动器类型。若未安装，可选择是否愿意安装 Ashita。\r\n不兼容或未安装且不愿安装的情况，会在后续提示改为手动配置。\r\n\r\n可通过下方按钮打开下载页面。\r\n");

            CreateRadio(IdPrimaryRadio1, L"我已安装 Ashita", 30, 120, 250, 24, draft.launcherChoice == LauncherChoice::Ashita, true);
            CreateRadio(IdPrimaryRadio2, L"我已安装 Windower", 30, 150, 250, 24, draft.launcherChoice == LauncherChoice::Windower, false);
            CreateRadio(IdPrimaryRadio3, L"我都没有安装", 30, 180, 250, 24, draft.launcherChoice == LauncherChoice::NoneInstalled, false);
            CreateCheck(IdCheck1, L"若未安装，我愿意安装 Ashita", 50, 215, 260, 22, draft.willingToInstall);
            CreateButton(IdAction1, L"Ashita 下载", 30, 260, 120, 28);
            CreateButton(IdAction2, L"Windower 下载", 160, 260, 120, 28);
        }

        void RenderAshitaPathStep()
        {
            SetWindowTextW(titleLabel, L"步骤 3：Ashita 安装目录");
            SetWindowTextW(descriptionLabel, L"请输入或选择 Ashita 安装目录。下一步会检查该目录下是否存在 Ashita.dll 与 Ashita-cli.exe。\r\n你需要选择Ashita安装的根目录，即目录下有Ashita-cli.exe。\r\n");

            CreateStatic(IdStatic1, L"Ashita 安装目录：", 30, 130, 130, 20);
            CreateEdit(IdEdit1, draft.ashitaPath.wstring(), 165, 126, 430, 24, true);
            CreateButton(IdBrowse1, L"浏览...", 605, 125, 90, 26);
            CreateStatic(IdStatic2, BuildAshitaValidationText().c_str(), 30, 170, 660, 50);
        }

        void RenderPivotCheckStep()
        {
            SetWindowTextW(titleLabel, L"步骤 4：Pivot 检查");
            SetWindowTextW(descriptionLabel, L"将检查 Ashita 安装目录下是否存在 polplugins/pivot.dll。\r\n如果尚未安装 Pivot，可先打开下载页或安装说明，安装完成后点击“刷新检查”。\r\n");

            const bool hasPivot = fs::exists(draft.ashitaPath / L"polplugins" / L"pivot.dll");
            std::wstring status = L"Ashita 目录：\r\n" + draft.ashitaPath.wstring() + L"\r\n\r\n";
            status += hasPivot ? L"已检测到 pivot.dll，可以继续。" : L"未检测到 pivot.dll，请先安装 Pivot。";

            CreateStatic(IdStatic1, status.c_str(), 30, 130, 660, 90);
            CreateButton(IdAction1, L"打开 Pivot 页面", 30, 245, 130, 28);
            CreateButton(IdAction2, L"刷新检查", 170, 245, 100, 28);
        }

        void RenderPivotConfigStep()
        {
            SetWindowTextW(titleLabel, L"步骤 5：Pivot 与输出目录");
            SetWindowTextW(descriptionLabel, L"将检查并准备 pivot.ini 配置，并根据规则计算 pivotroot 与 output_path。\r\n若您在 Pivot 中设定了相对路径的 root_path，请改为手动配置。\r\n");

            RefreshPivotPreview();
            std::wstring content;
            if (pivotPreview.valid)
            {
                content = L"pivot.ini：\r\n" + pivotPreview.pivotIniPath.wstring()
                    + L"\r\n\r\npivotroot：\r\n" + pivotPreview.pivotRoot.wstring()
                    + L"\r\n\r\noutput_path：\r\n" + pivotPreview.outputPath.wstring();
            }
            else
            {
                content = L"无法生成 Pivot 配置预览：\r\n" + pivotPreview.errorMessage;
            }

            CreateStatic(IdStatic1, content.c_str(), 30, 130, 660, 180);
            CreateButton(IdAction1, L"打开 pivot 配置目录", 30, 325, 160, 28);
            CreateButton(IdAction2, L"重新检查", 200, 325, 100, 28);
        }

        void RenderBabelStep()
        {
            SetWindowTextW(titleLabel, L"步骤 6：巴别塔");
            SetWindowTextW(descriptionLabel, L"请选择巴别塔模式。此选项会在描述译文前附加原文，便于同时查看原文与翻译。\r\n不同模式决定附加哪一种原文。\r\n由于现在中文 Wiki 建设尚在起步阶段，使用本选项以便于查询日英攻略。\r\n");
            CreateRadio(IdPrimaryRadio1, L"关：不附加原文", 30, 130, 260, 24, draft.babelMode == BabelMode::Off, true);
            CreateRadio(IdPrimaryRadio2, L"双语：附加当前客户端原文", 30, 160, 280, 24, draft.babelMode == BabelMode::Bilingual, false);
            CreateRadio(IdPrimaryRadio3, L"异国：附加另一语言原文", 30, 190, 280, 24, draft.babelMode == BabelMode::Exotic, false);
            CreateRadio(IdPrimaryRadio4, L"三语：同时附加当前原文和另一语言原文", 30, 220, 340, 24, draft.babelMode == BabelMode::Tower, false);
            CreateStatic(IdStatic1, L"说明：\r\n- 双语：物品名为译文，在描述中，日端附加日文原文，英端附加英文原文。\r\n- 异国：日端附加英文原文，英端附加日文原文。\r\n- 三语：同时附加两种原文。", 30, 270, 660, 90);
        }

        void RenderNonameStep()
        {
            SetWindowTextW(titleLabel, L"步骤 7：不名");
            SetWindowTextW(descriptionLabel, L"请选择是否启用不名。启用后，物品名、任务名等名称字段会尽量保留原样，仅翻译说明/描述等正文部分。\r\n若你希望减少专有名词被改写，可启用此项。\r\n启用此项时，不建议使用巴别塔的双语或三语模式。\r\n");
            CreateRadio(IdPrimaryRadio1, L"启用不名", 30, 130, 250, 24, draft.noname, true);
            CreateRadio(IdPrimaryRadio2, L"不启用不名", 30, 160, 250, 24, !draft.noname, false);
            CreateStatic(IdStatic1, L"说明：\r\n- 启用：尽量不翻译名称类字段，只处理描述文本。\r\n　本选项适用于需要编写宏的场景。由于宏按名称匹配，修改名字会导致宏失效。\r\n- 不启用：名称和描述都会按正常流程翻译。", 30, 210, 660, 80);
        }

        void RenderExcludesStep()
        {
            SetWindowTextW(titleLabel, L"步骤 8：排除项");
            SetWindowTextW(descriptionLabel, L"请选择需要排除的 defs.csv 记录项。支持多选。\r\n如果不需要排除，直接保持空选择即可，这将翻译所有文件。\r\n本功能用于您不想翻译特定类型的内容时（如技能名 sys/ability 或魔法名 sys/magic）。\r\n");

            CreateListBox(IdList1, 30, 120, 500, 260);
            CreateButton(IdAction1, L"全选", 545, 120, 90, 28);
            CreateButton(IdAction2, L"全不选", 545, 155, 90, 28);

            HWND list = GetDlgItem(hwnd, IdList1);
            for (size_t i = 0; i < defsComments.size(); ++i)
            {
                SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(xybase::string::to_wstring(defsComments[i]).c_str()));
                if (std::find(draft.excludes.begin(), draft.excludes.end(), defsComments[i]) != draft.excludes.end())
                    SendMessageW(list, LB_SETSEL, TRUE, static_cast<LPARAM>(i));
            }
        }

        void RenderSummaryStep()
        {
            SetWindowTextW(titleLabel, L"步骤 9：确认并生成配置");
            SetWindowTextW(descriptionLabel, L"请确认以下配置。点击“完成”后，将写入 pivot.ini 与 config.ini，然后退出程序。\r\n");

            std::wstringstream ss;
            ss << L"游戏目录："
               << (draft.useDetectedGamePath && detectedGamePath.has_value() ? detectedGamePath->gamePath.wstring() + L"（自动检测，不写入 config.ini）" : ToDisplayPath(draft.manualGamePath.value_or(fs::path{})))
               << L"\r\n";
            if (!draft.useDetectedGamePath && draft.manualEnglishMode.has_value())
                ss << L"english_mode：" << (*draft.manualEnglishMode ? L"true" : L"false") << L"\r\n";
            ss << L"Ashita：" << draft.ashitaPath.wstring() << L"\r\n"
               << L"pivotroot：" << draft.pivotRoot.wstring() << L"\r\n"
               << L"output_path：" << draft.outputPath.wstring() << L"\r\n"
               << L"babel：" << GetBabelDisplayName(draft.babelMode) << L"（" << GetBabelConfigValue(draft.babelMode) << L"）\r\n"
               << L"noname：" << (draft.noname ? L"true" : L"false") << L"\r\n"
               << L"排除项数量：" << draft.excludes.size();

            CreateReadOnlyMultiline(IdSummary, ss.str(), 30, 120, 660, 280);
        }

        void RenderManualFinishStep()
        {
            SetWindowTextW(titleLabel, L"请改为手动配置");
            SetWindowTextW(descriptionLabel, terminalMessage.c_str());
            CreateStatic(IdStatic1, L"请点击下方按钮打开 README，并按说明手动配置。\r\n关闭本向导后，程序将退出。", 30, 130, 660, 60);
            CreateButton(IdAction1, L"打开 README", 30, 210, 120, 28);
        }

        void RenderInstallFinishStep()
        {
            SetWindowTextW(titleLabel, L"请先安装 Ashita");
            SetWindowTextW(descriptionLabel, L"当前流程无法继续，因为需要先安装 Ashita。\r\n请通过下方链接下载安装；完成后重新运行本程序。\r\n");
            CreateButton(IdAction1, L"Ashita 下载", 30, 130, 120, 28);
            CreateButton(IdAction2, L"Windower 下载", 160, 130, 120, 28);
            CreateStatic(IdStatic1, L"安装完成后重新运行本程序，即可继续向导。", 30, 180, 660, 30);
        }

        void RefreshPivotCheck()
        {
            RenderStep();
        }

        void RefreshPivotPreview()
        {
            pivotPreview = BuildPivotConfiguration(draft.ashitaPath);
            if (!pivotPreview.valid)
                SetStatus(pivotPreview.errorMessage, true);
            else
                SetStatus(L"Pivot 预览已更新。", false);
        }

        std::wstring BuildAshitaValidationText() const
        {
            if (draft.ashitaPath.empty())
                return L"尚未选择 Ashita 安装目录。";
            if (!fs::exists(draft.ashitaPath))
                return L"当前目录不存在。";
            if (ContainsRequiredAshitaFiles(draft.ashitaPath))
                return L"当前目录校验通过：已找到 Ashita.dll 和 Ashita-cli.exe。";
            return L"当前目录校验未通过：缺少 Ashita.dll 或 Ashita-cli.exe。";
        }

        void BrowseGamePath()
        {
            const auto selected = PickFolder(L"请选择 FFXI 游戏安装目录", draft.manualGamePath.value_or(fs::path{}));
            if (selected.has_value())
                SetWindowTextW(GetDlgItem(hwnd, IdEdit1), selected->wstring().c_str());
        }

        void UpdateGamePathControlState()
        {
            const bool allowDetected = detectedGamePath.has_value();
            const bool useDetected = allowDetected && IsButtonChecked(IdPrimaryRadio1);
            const bool manualEnabled = !useDetected || !allowDetected;

            if (HWND edit = GetDlgItem(hwnd, IdEdit1); edit != nullptr)
                EnableWindow(edit, manualEnabled);
            if (HWND browse = GetDlgItem(hwnd, IdBrowse1); browse != nullptr)
                EnableWindow(browse, manualEnabled);
            if (HWND english = GetDlgItem(hwnd, IdSecondaryRadio1); english != nullptr)
                EnableWindow(english, manualEnabled);
            if (HWND japanese = GetDlgItem(hwnd, IdSecondaryRadio2); japanese != nullptr)
                EnableWindow(japanese, manualEnabled);

            SetStatus(L"");
        }

        void BrowseAshitaPath()
        {
            const auto selected = PickFolder(L"请选择 Ashita 安装目录", draft.ashitaPath);
            if (selected.has_value())
            {
                SetWindowTextW(GetDlgItem(hwnd, IdEdit1), selected->wstring().c_str());
                draft.ashitaPath = *selected;
                RenderStep();
            }
        }

        void OpenReadme()
        {
            const auto localReadme = progRoot / L"请读我.txt";
            if (fs::exists(localReadme))
                OpenPath(localReadme);
        }

        void CollectExcludeSelections()
        {
            draft.excludes.clear();
            HWND list = GetDlgItem(hwnd, IdList1);
            if (list == nullptr)
                return;

            for (size_t i = 0; i < defsComments.size(); ++i)
            {
                if (SendMessageW(list, LB_GETSEL, static_cast<WPARAM>(i), 0) > 0)
                    draft.excludes.push_back(defsComments[i]);
            }
        }

        void SelectAllExcludes(bool select)
        {
            HWND list = GetDlgItem(hwnd, IdList1);
            if (list == nullptr)
                return;
            for (size_t i = 0; i < defsComments.size(); ++i)
                SendMessageW(list, LB_SETSEL, select ? TRUE : FALSE, static_cast<LPARAM>(i));
        }

        bool IsButtonChecked(int id) const
        {
            const HWND control = GetDlgItem(hwnd, id);
            return control != nullptr && SendMessageW(control, BM_GETCHECK, 0, 0) == BST_CHECKED;
        }

        HWND CreateControl(const wchar_t* className, const wchar_t* text, DWORD style, int id, int x, int y, int w, int h)
        {
            HWND control = CreateWindowExW(0, className, text, WS_CHILD | WS_VISIBLE | style, x, y, w, h, hwnd, reinterpret_cast<HMENU>(id), nullptr, nullptr);
            SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(normalFont), TRUE);
            stepControls.push_back(control);
            return control;
        }

        void CreateStatic(int id, const std::wstring& text, int x, int y, int w, int h)
        {
            CreateControl(L"STATIC", text.c_str(), 0, id, x, y, w, h);
        }

        void CreateRadio(int id, const std::wstring& text, int x, int y, int w, int h, bool checked, bool firstInGroup, bool enabled = true)
        {
            DWORD style = BS_AUTORADIOBUTTON | WS_TABSTOP;
            if (firstInGroup)
                style |= WS_GROUP;
            HWND control = CreateControl(L"BUTTON", text.c_str(), style, id, x, y, w, h);
            SendMessageW(control, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
            EnableWindow(control, enabled);
        }

        void CreateCheck(int id, const std::wstring& text, int x, int y, int w, int h, bool checked)
        {
            HWND control = CreateControl(L"BUTTON", text.c_str(), BS_AUTOCHECKBOX | WS_TABSTOP, id, x, y, w, h);
            SendMessageW(control, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
        }

        void CreateEdit(int id, const std::wstring& text, int x, int y, int w, int h, bool enabled)
        {
            HWND control = CreateControl(L"EDIT", text.c_str(), WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, id, x, y, w, h);
            EnableWindow(control, enabled);
        }

        void CreateButton(int id, const std::wstring& text, int x, int y, int w, int h, bool enabled = true)
        {
            HWND control = CreateControl(L"BUTTON", text.c_str(), WS_TABSTOP, id, x, y, w, h);
            EnableWindow(control, enabled);
        }

        void CreateListBox(int id, int x, int y, int w, int h)
        {
            CreateControl(L"LISTBOX", L"", WS_BORDER | WS_VSCROLL | LBS_NOTIFY | LBS_MULTIPLESEL | WS_TABSTOP, id, x, y, w, h);
        }

        void CreateReadOnlyMultiline(int id, const std::wstring& text, int x, int y, int w, int h)
        {
            HWND control = CreateControl(L"EDIT", text.c_str(), WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY, id, x, y, w, h);
            SendMessageW(control, EM_SETSEL, 0, 0);
        }

        void ClearStepControls()
        {
            for (HWND control : stepControls)
            {
                if (control != nullptr)
                    DestroyWindow(control);
            }
            stepControls.clear();
        }

        void SetStatus(const std::wstring& text, bool = false)
        {
            SetWindowTextW(statusLabel, text.c_str());
        }
    };
}

bool SetupWizard::RunIfConfigMissing(const std::filesystem::path& progRoot)
{
    const fs::path configPath = progRoot / L"config.ini";
    if (fs::exists(configPath))
        return false;

    const HRESULT coInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    {
        SetupWizardWindow wizard(progRoot);
        wizard.Run();
    }
    if (SUCCEEDED(coInit))
        CoUninitialize();
    return true;
}
