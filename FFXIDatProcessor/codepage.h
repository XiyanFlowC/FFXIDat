#pragma once

#include <string>
#include <map>
#include <cstdint>
#include <filesystem>
#include "xystring.h"

class CodeCvt
{
	std::map<uint32_t, uint32_t> uc2cp;
	std::map<uint32_t, uint32_t> cp2uc;
public:
	~CodeCvt();

	static CodeCvt &GetInstance();

	bool ChsOnSJisDirtyThing(xybase::StringBuilder<char> &sb, char32_t code);

	std::string CvtToString(const std::wstring &str);

	std::wstring CvtToWString(const std::string &str);

	void Init(std::filesystem::path path);
};

std::string cvt_to_string(const std::wstring &str);

std::wstring cvt_to_wstring(const std::string &str);
