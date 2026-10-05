#pragma once

#include "HelpFormat.h"

class HelpData : public slotfile::SlotFile<helpfmt::Format>
{
public:
	void ToICsv(const std::wstring &path) const;
	void FromCsv(const std::wstring &path);
};
