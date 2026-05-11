#pragma once

#include <string>
#include <map>
#include <set>
#include <cstdint>
#include <filesystem>
#include "xystring.h"

class CodeCvt
{
	std::map<uint32_t, uint32_t> uc2cp;
	std::map<uint32_t, uint32_t> cp2uc;
 std::set<uint8_t> leadBytes;
public:
	~CodeCvt();

	static CodeCvt &GetInstance();

	bool ChsOnSJisDirtyThing(xybase::StringBuilder<char> &sb, char32_t code);

	std::string CvtToString(const std::wstring &str);

	std::wstring CvtToWString(const std::string &str);

	void Init(std::filesystem::path path);

    // Mapping helpers for custom converters
    bool TryUcToCp(uint32_t uc, uint32_t &cp) const;
    bool TryCpToUc(uint32_t cp, uint32_t &uc) const;
};

std::string cvt_to_string(const std::wstring &str);

std::wstring cvt_to_wstring(const std::string &str);
