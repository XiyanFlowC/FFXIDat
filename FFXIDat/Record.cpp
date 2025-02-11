#include <stdexcept>
#include <xystring.h>
#include <cassert>
#include "Record.h"

void Row::ReadRow(Record *buffer, int limit)
{
	cells.clear();
	intptr_t base = (intptr_t)buffer;
	for (int i = 0; i < buffer->cellCount; ++i)
	{
		ptrdiff_t offset = buffer->spec[i].offset;
		if (buffer->spec[i].type)
		{
			cells.push_back(Cell(*(int32_t *)base + offset));
		}
		else
		{
			RecordString *str = (RecordString *)(base + offset);
			assert(str->one == 1 && str->zero[0] == 0 && str->zero[1] == 0 && str->zero[2] == 0 && str->zero[3] == 0 && str->zero[4] == 0 && str->zero[5] == 0);

			cells.push_back(Cell(xybase::string::to_utf8(str->str)));
		}
	}
}

void Row::WriteRow(Record *buffer, int limit)
{
	ptrdiff_t offsetBase = 4 + cells.size() * 8;
	intptr_t base = (intptr_t)buffer;
	buffer->cellCount = cells.size();
	int ci = 0;
	for (const Cell &cell : cells)
	{
		buffer->spec[ci].offset = offsetBase;
		int type = buffer->spec[ci++].type = cell.GetType();

		if (type)
		{
			*(int *)(base + offsetBase) = cell.Get<int>();
		}
		else
		{
			RecordString *target = (RecordString *)(base + offsetBase);
			target->one = 1;
			memset(target->zero, 0, sizeof(target->zero));
			auto str = xybase::string::to_string(cell.Get<std::u8string>());
			if (str.size() == 0 && cell.Get<std::u8string>().size() != 0)
				throw xybase::RuntimeException(L"不可表示的宽字符在" + xybase::string::to_wstring(cell.Get<std::u8string>()), 1500);
			strcpy(target->str, str.c_str());
		}

		offsetBase += cell.GetSize();
	}

	if (offsetBase > limit) throw std::runtime_error("something went wrong.");
}

int Cell::GetSize() const
{
	// FIXME: Remove the xybase::string::to_string or try to find some solution!
	return type ? 4 : 28 + ((xybase::string::to_string(str).size() + 1 + 3) & ~3); // str align
}
