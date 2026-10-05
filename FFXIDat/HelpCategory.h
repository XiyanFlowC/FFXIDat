#pragma once

#include "HelpCategoryFormat.h"

class HelpCategory : public slotfile::SlotFile<hlcfmt::Format>
{
public:
	static std::u8string MemberIds(const hlcfmt::Datum &datum);
	void ToICsv(const std::wstring &path) const;
	void FromCsv(const std::wstring &path);
};
