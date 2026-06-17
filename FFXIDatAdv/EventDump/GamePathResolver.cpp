#include "GamePathResolver.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

std::string GamePathResolver::gameRoot;

void GamePathResolver::Init()
{
	if (!gameRoot.empty())
		return;

	HKEY hKey;
	const wchar_t* subKey = L"SOFTWARE\\WOW6432Node\\PlayOnline\\InstallFolder";
	const wchar_t* valueName = L"0001";
	wchar_t valueData[MAX_PATH];
	DWORD bufferSize = sizeof(valueData);
	DWORD valueType;

	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, subKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		if (RegQueryValueExW(hKey, valueName, nullptr, &valueType,
			reinterpret_cast<LPBYTE>(valueData), &bufferSize) == ERROR_SUCCESS)
		{
			if (valueType == REG_SZ)
			{
				std::wstring ws(valueData);
				gameRoot = std::string(ws.begin(), ws.end());
			}
		}
		RegCloseKey(hKey);
	}

	if (gameRoot.empty())
	{
		gameRoot = "C:\\Program Files (x86)\\PlayOnline\\SquareEnix\\FINAL FANTASY XI\\";
	}
}

std::string GamePathResolver::GetGameRoot()
{
	if (gameRoot.empty())
		Init();
	return gameRoot;
}

std::string GamePathResolver::ResolvePath(const std::string& romPath)
{
	if (gameRoot.empty())
		Init();

	// romPath format: "ROM3/0/44" or "ROM/21/44" or "ROM/25/44.DAT"
	// PathUtil: rom==1 -> {root}ROM\{cat}\{no}.DAT
	//           rom>1 -> {root}ROM{rom}\{cat}\{no}.DAT

	std::string path = romPath;
	// Remove .DAT extension if present
	if (path.size() > 4 && path.substr(path.size() - 4) == ".DAT")
		path = path.substr(0, path.size() - 4);

	// Parse ROM<vol>/<cat>/<file>
	// Find first slash after "ROM"
	size_t romStart = path.find("ROM");
	if (romStart == std::string::npos)
		return gameRoot + "\\" + path + ".DAT";

	size_t volEnd = path.find('/', romStart);
	if (volEnd == std::string::npos)
		return gameRoot + "\\" + path + ".DAT";

	std::string volStr = path.substr(romStart + 3, volEnd - romStart - 3);
	int rom = 1;
	if (!volStr.empty())
		rom = std::stoi(volStr);

	size_t catEnd = path.find('/', volEnd + 1);
	if (catEnd == std::string::npos)
		return gameRoot + "\\" + path + ".DAT";

	std::string catStr = path.substr(volEnd + 1, catEnd - volEnd - 1);
	std::string fileStr = path.substr(catEnd + 1);

	std::string result;
	if (rom == 1)
		result = gameRoot + "ROM\\" + catStr + "\\" + fileStr + ".DAT";
	else
		result = gameRoot + "ROM" + std::to_string(rom) + "\\" + catStr + "\\" + fileStr + ".DAT";

	return result;
}

bool GamePathResolver::HasGameRoot()
{
	if (gameRoot.empty())
		Init();
	return !gameRoot.empty();
}
