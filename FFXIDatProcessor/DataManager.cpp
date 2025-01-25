#include "DataManager.h"

#include <format>
#include <filesystem>

#define WIN32_MEAN_AND_LEAN
#include <Windows.h>

std::wstring PathUtil::gameRootPath = L"C:\\Program Files (x86)\\PlayOnline\\SquareEnix\\FINAL FANTASY XI\\"; // HACK:FIXME: read from regtable?

std::wstring PathUtil::progRootPath = L"."; // if failed

std::wstring PathUtil::GetPath(int rom, int cat, int no)
{
    if (rom == 1)
        return std::format(L"{}ROM\\{}\\{}.DAT", gameRootPath, cat, no);
    return std::format(L"{}ROM{}\\{}\\{}.DAT", gameRootPath, rom, cat, no);
}

void PathUtil::Init()
{
    HKEY hKey;
    const wchar_t *subKey = L"SOFTWARE\\WOW6432Node\\PlayOnline\\InstallFolder";
    const wchar_t *valueName = L"0001";
    wchar_t valueData[MAX_PATH];
    DWORD bufferSize = sizeof(valueData);

    DWORD valueType;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, subKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        // 查询键值
        if (RegQueryValueExW(hKey, valueName, nullptr, &valueType, reinterpret_cast<LPBYTE>(valueData), &bufferSize) == ERROR_SUCCESS) {
            // 检查值的类型是否为字符串
            if (valueType == REG_SZ) {
                gameRootPath = valueData;
            }
        }
        RegCloseKey(hKey);
    }

    wchar_t module_path[MAX_PATH];
    if (GetModuleFileNameW(NULL, module_path, MAX_PATH))
    {
        std::filesystem::path exePath = module_path;
        progRootPath = exePath.parent_path();
    }
}

std::wstring PathUtil::GetOutPathConf(int rom, int cat, int no)
{
    auto path = (rom == 1) ? std::format(L"output\\ROM\\{}", cat) : std::format(L"output\\ROM{}\\{}", rom, cat);
    if (!std::filesystem::exists(path))
        std::filesystem::create_directories(path);
    return std::format(L"{}\\{}.DAT", path, no);
}

std::wstring PathUtil::GetOutPathConf(const std::wstring &path)
{
    std::filesystem::path p("./output");
    p = p / path;
    if (std::filesystem::exists(p.parent_path())) return p.wstring();
    std::filesystem::create_directories(p.parent_path());
    return p.wstring();
}

std::wstring PathUtil::GetPath(const std::wstring &path)
{
    return std::format(L"{}\\{}", gameRootPath, path);
}

std::wstring PathUtil::GetOutPath(int rom, int cat, int no)
{
    auto path = (rom == 1) ? std::format(L"output\\ROM\\{}", cat) : std::format(L"output\\ROM{}\\{}", rom, cat);
    return std::format(L"{}\\{}.DAT", path, no);
}
