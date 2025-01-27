#pragma once

#include <string>
#include <map>

class ChsToSJis
{
	static struct ChsSJisMapT {
		char8_t hanzi[8];
		char8_t kanji[8];
	} replacement[];

	std::map<int, std::u8string> repMap;
public:
	std::u8string ReplaceHanzi(std::u8string in);

	ChsToSJis();

	static ChsToSJis &Instance();
};

