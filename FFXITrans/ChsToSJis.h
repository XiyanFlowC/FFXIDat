#pragma once

#include <map>
#include <string>
#include <filesystem>

class ChsToSJis
{
	std::map<int, std::u8string> repMap;
public:
	std::u8string ReplaceHanzi(std::u8string in);

	void Init(std::filesystem::path csv);

	static ChsToSJis &Instance();
};

