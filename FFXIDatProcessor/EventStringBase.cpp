#include "EventStringBase.h"

#include <fstream>
#include <cassert>

#include "GameString.h"
#include"xystring.h"

void EventStringBase::Read()
{
	std::ifstream eye(path, std::ios::in | std::ios::binary);
	EventStringBaseHeader header;
	eye.read((char *)&header, 4);
	assert(header.flag == 0x10);
	flag = header.flag;

	std::unique_ptr<char[]> buf(new char[header.size]);
	eye.read(buf.get(), header.size);
	Xor(buf.get(), header.size);

	int32_t *cur = (int32_t *)buf.get();
	while (*cur < header.size)
	{
		strs.push_back(xybase::string::to_utf8(EventStringCodecUtil::Instance().Decode(buf.get() + *cur)));
		++cur;
	}
}

void EventStringBase::Write()
{
	int32_t offset = strs.size() * 4;
	std::unique_ptr<int32_t[]> indexBuffer(new int32_t[strs.size()]);
	int32_t *p = indexBuffer.get();
	std::vector<std::string> encodedString;
	for (auto &str : strs)
	{
		auto cs = EventStringCodecUtil::Instance().Encode(xybase::string::to_string(str));
		encodedString.push_back(cs);
		*p++ = offset;

		offset += cs.size() + 1;
	}

	int32_t header = offset + 4;
	header |= 0x10000000;
	std::ofstream pen(path, std::ios::out | std::ios::binary);
	pen.write((char *) & header, 4);
	Xor((char *)indexBuffer.get(), strs.size() * 4);
	pen.write((char *)indexBuffer.get(), strs.size() * 4);
	for (auto &s : encodedString)
	{
		for (auto &c : s)
		{
			pen.put(c ^ 0x80);
		}
		pen.put(0x80); // NUL terminator
	}
}

#include "CsvFile.h"

void EventStringBase::ToCsv(std::filesystem::path path)
{
	CsvFile csv(path, std::ios::out | std::ios::binary);
	for (auto &str : strs)
	{
		csv.NewCell(str);
		csv.NewLine();
	}
	csv.Close();
}

void EventStringBase::FromCsv(std::filesystem::path path)
{
	strs.clear();
	CsvFile csv(path, std::ios::in | std::ios::binary);
	while (!csv.IsEof())
	{
		strs.push_back(csv.NextCell());
		csv.NextLine();
	}
	csv.Close();
}

void EventStringBase::Xor(char *tar, int size)
{
	for (int i = 0; i < size; ++i)
	{
		tar[i] ^= 0x80;
	}
}
