#pragma once

#include <string>

class DataManager
{
public:
	static std::string gameRootPath;

	static std::string GetPath(int rom, int cat, int no);

	static std::string GetOutPathConf(int rom, int cat, int no);

	static std::string GetOutPath(int rom, int cat, int no);
};

