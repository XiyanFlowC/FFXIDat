#pragma once

#include <string>

class PathUtil
{
public:
	static std::wstring gameRootPath;

	static void Init();

	static std::wstring GetPath(int rom, int cat, int no);

	static std::wstring GetOutPathConf(int rom, int cat, int no);

	static std::wstring GetOutPath(int rom, int cat, int no);
};

