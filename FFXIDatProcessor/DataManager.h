#pragma once

#include <string>

class PathUtil
{
public:
	static std::wstring gameRootPath;

	static std::wstring progRootPath;

	static void Init();

	static std::wstring GetPath(int rom, int cat, int no);

	static std::wstring GetPath(const std::wstring &path);

	static std::wstring GetOutPathConf(int rom, int cat, int no);

	static std::wstring GetOutPathConf(const std::wstring &path);

	static std::wstring GetOutPath(int rom, int cat, int no);
};

