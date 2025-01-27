#include "XiString.h"

#include <cassert>

void XiString::Read()
{
	entries.clear();
	std::ifstream eye(path, std::ios::in | std::ios::binary);
	XiStringHeader header;
	eye.read((char *)&header, sizeof(header));
	if (memcmp(header.magicHeader, "XISTRING", 8))
	{
		throw std::invalid_argument("invalid magic");
	}
	if (header.version != 0x20000)
	{
		throw std::invalid_argument("invalid version");
	}
	assert(header.zero[0] == header.zero[1] && header.zero[1] == header.zero[2] && header.zero[3] == header.zero[2] && header.zero[4] == header.zero[3]);
	assert(header.zero[0] == 0);
	assert(header.reserved == 0);
	id = header.id;

	if (header.entriesCount * 12 != header.indicesSize)
		throw std::invalid_argument("invalid header");

	std::unique_ptr<XiStringIndex[]> indices(new XiStringIndex[header.entriesCount]);
	eye.read((char *)indices.get(), header.indicesSize);
	std::unique_ptr<char[]> buffer(new char[header.dataSize]);
	eye.read(buffer.get(), header.dataSize);

	for (int i = 0; i < header.entriesCount; ++i)
	{
		// std::u8string str = xybase::string::to_utf8(std::string(buffer.get() + indices[i].offset, indices[i].size - 1));
		entries.push_back(StringEntry(std::string(buffer.get() + indices[i].offset, indices[i].size - 1), indices[i].flag1, indices[i].flag2, indices[i].flag3));
	}
}

void XiString::Write()
{
	std::ofstream pen(path, std::ios::out | std::ios::binary);
	XiStringHeader header;
	memset(header.zero, 0, sizeof(header.zero));
	header.reserved = 0;
	memcpy(header.magicHeader, "XISTRING", 8);
	header.version = 0x20000;
	header.id = id;
	header.entriesCount = entries.size();
	header.indicesSize = header.entriesCount * sizeof(XiStringIndex);
	header.dataSize = 0;
	for (const StringEntry &entry : entries)
	{
		header.dataSize += entry.str.size() + 1;
	}
	header.fileSize = header.dataSize + header.indicesSize + sizeof(header);

	pen.write((char *)&header, sizeof(header));
	int offset = 0;
	for (const StringEntry &entry : entries)
	{
		XiStringIndex index;
		index.offset = offset;
		index.size = entry.str.size() + 1;
		offset += index.size;
		index.flag1 = entry.flag1;
		index.flag2 = entry.flag2;
		index.flag3 = entry.flag3;
		pen.write((char *)&index, sizeof(XiStringIndex));
	}

	for (const StringEntry &entry : entries)
	{
		auto &s = entry.str;
		for (int i = 0; i < s.size(); ++i)
		{
			pen.put(s[i]);
		}
		pen.put(0);
	}
}

#include "CsvFile.h"

void XiString::ToCsv(std::filesystem::path path)
{
	CsvFile csv(path, std::ios::out | std::ios::binary);
	csv.NewCell(xybase::string::itos<char8_t>(id));
	csv.NewLine();
	for (StringEntry &entry : entries)
	{
		csv.NewCell(xybase::string::itos<char8_t>(entry.flag1));
		csv.NewCell(xybase::string::itos<char8_t>(entry.flag2));
		csv.NewCell(xybase::string::itos<char8_t>(entry.flag3));
		csv.NewCell(xybase::string::to_utf8(Decode(entry.str)));
		csv.NewLine();
	}
	csv.Close();
}

void XiString::FromCsv(std::filesystem::path path)
{
	CsvFile csv(path, std::ios::in | std::ios::binary);
	id = xybase::string::stoi<char8_t>(csv.NextCell());
	csv.NextLine();
	entries.clear();
	while (!csv.IsEof())
	{
		StringEntry e;
		e.flag1 = xybase::string::stoi(csv.NextCell());
		e.flag2 = xybase::string::stoi(csv.NextCell());
		e.flag3 = xybase::string::stoi(csv.NextCell());
		e.str = Encode(xybase::string::to_string(csv.NextCell()));
		entries.push_back(std::move(e));
		csv.NextLine();
	}
}

std::string XiString::Encode(const std::string &in)
{
	xybase::StringBuilder sb;
	const char *p = in.c_str();

	while (*p)
	{
		if (*p == '\\')
		{
			++p;
			if (*p == 'x')
			{
				std::string hex(++p, 2);

				int val = xybase::string::stoi(hex, 16);
				sb += (char)val;

				p += 2;
			}
			else
				sb += *p++;
		}
		else if (*p >= 0x80)
		{
			sb += *p++;
			sb += *p++;
		}
		else
			sb += *p++;
	}

	return sb.ToString();
}

std::string XiString::Decode(const std::string &in)
{
	xybase::StringBuilder sb;

	const char *p = in.c_str();
	while (*p)
	{
		if (*p <= 0x7F)
		{
			if (*p == '\\')
				sb += "\\\\", ++p;
			else
				sb += *p++;
		}
		else
		{
			int step = GetStep(p);
			if (step)
			{
				while (step--)
				{
					sb.Append("\\x");
					int cb = *p++;
					cb &= 0xFF;
					if (cb < 16)
						sb.Append('0');
					sb.Append(xybase::string::itos<char>(cb, 16).c_str());
				}
			}
			else
			{
				sb += *p++;
				sb += *p++;
			}
		}
	}

	return sb.ToString();
}

// UNDONE: 很可能是错的，但\x01的概念无法分析
// 很可能是类似XIV的机制，但是没有直接定义的长度标志。
// 等待样本分析
struct XiStringControlSequenceStepDefinition {
	char code[4];
	char name[12];
	int step;
} xiStringControlSequenceStepDefinitions[] = {
	// 80 ???
	{"\x81\x85", "", 2 + 3},
	{"\x82\x01", "", 2 + 11}, // if?
	{"\x83\x01", "", 2 + 4},
	{"\x84\x02", "", 2 + 0},
	{"\x84\x05", "", 2 + 0},
	{"\x84\x0A", "", 2 + 0},
	{"\x84\x10", "", 2 + 0},
	{"\x85\x11", "", 2 + 0},
	{"\x85\x1D", "", 2 + 0},
	{"\x85\x2A", "", 2 + 0},
	{"\x86\x08", "", 2 + 0},
	{"\x86\x0D", "", 2 + 0},
	{"\x89\x0A", "", 2 + 0},
	{"\x89\x06", "", 2 + 0},
	{"\x8B\x01", "", 2 + 1},
	{"\x8C\x03", "", 2 + 2},
	// 8D sep?
	{"\x8E\x01", "", 2 + 6},
	{"", "", -1}
};

int XiString::GetStep(const char *p)
{
	if (p[0] != 0xFA || p[1] != 0x40) return 0;
	p += 2;

	if (p[0] == 0x8D)
	{
		return 1;
	}

	auto *d = xiStringControlSequenceStepDefinitions;
	while (d->step != -1)
	{
		if (d->code[0] == p[0] && d->code[1] == p[1])
		{
			return 2/*start marker*/ + d->step/*step*/;
		}

		++d;
	}

	switch (p[0])
	{
	case 0x81:
		return 5;
	case 0x82:
		return 13;
	case 0x83:
		return 6;
	case 0x84:
	case 0x85:
	case 0x86:
	case 0x89:
		return 2;
	case 0x8B:
		return 3;
	case 0x8C:
		return 4;
	case 0x8E:
		return 8;
	}

	printf("%02X%02X\n", p[0], p[1]);
	throw std::runtime_error("unknown ctrl char.");
}
