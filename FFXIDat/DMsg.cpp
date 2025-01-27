#include "DMsg.h"

#include <cassert>
#include <string>

#include "xystring.h"

void DMsg::Read()
{
	rows.clear();
	std::ifstream eye(path, std::ios::binary);
	DMsgHeader header;
	eye.read((char *) &header, sizeof(header));
	if (memcmp(header.magic_header, "d_msg\0\0", 8) != 0)
	{
		eye.close();
		throw std::invalid_argument("not a valid d_msg file!");
	}
	if (header.one != 1 || header.ukn1 != 0x1 || header.ukn2 != 3 || header.ukn3 != 3)
	{
		eye.close();
		throw std::invalid_argument("unknown format!");
	}

	obs = header.obs == 1;

	// block type
	if (header.blockSize)
	{
		m_blockSize = header.blockSize;
		if (header.indexSize)
			throw std::invalid_argument("weird format!!!");
		mode = Mode::Block;

		std::unique_ptr<char[]> data(new char[header.blockSize]);
		for (int i = 0; i < header.entriesCount; ++i)
		{
			eye.read(data.get(), header.blockSize);
			Xor(data.get(), header.blockSize);
			rows.push_back(Row());
			rows[i].ReadRow((Record *)data.get(), header.blockSize);
		}
	}
	// index type
	else if (header.indexSize)
	{
		mode = Mode::Variable;

		std::unique_ptr<DMsgIndex[]> indices(new DMsgIndex[header.entriesCount]);
		eye.read((char *)indices.get(), header.indexSize);
		Xor((char *)indices.get(), header.indexSize);
		if (header.indexSize < header.entriesCount * 8)
			throw std::invalid_argument("indices too short!!!");

		for (int i = 0; i < header.entriesCount; ++i)
		{
			eye.seekg(header.headerSize + header.indexSize + indices[i].offset);

			int limit = indices[i].size;
			rows.push_back(Row());
			std::unique_ptr<char[]> buffer(new char[limit]);
			eye.read(buffer.get(), limit);
			Xor(buffer.get(), limit);
			rows[i].ReadRow((Record *)buffer.get(), limit);
		}
	}
	else
	{
		throw std::invalid_argument("format error!!!");
	}
}

void DMsg::ToCsv(std::filesystem::path csvPath)
{
	CsvFile csv(csvPath, std::ios::out | std::ios::binary);
	if (mode == Mode::Block)
	{
		csv.NewCell(u8"~~TYPE=BLOCK~~");
		csv.NewCell(xybase::string::itos<char8_t>(m_blockSize));
		csv.NewLine();
	}
	else
	{
		csv.NewCell(u8"~~TYPE=LIST~~");
		csv.NewLine();
	}
	for (Row &row : rows)
	{
		for (Cell &cell : row.GetCells())
		{
			csv.NewCell(cell.ToString());
		}
		csv.NewLine();
	}
	csv.Close();
}

void DMsg::FromCsv(std::filesystem::path csvPath)
{
	rows.clear();

	CsvFile csv(csvPath, std::ios::in | std::ios::binary);

	auto header = csv.NextCell();
	csv.NextLine();
	if (header == u8"~~TYPE=BLOCK~~")
	{
		mode = Mode::Block;
		if (!csv.IsEol())
			m_blockSize = xybase::string::stoi(csv.NextCell());
		else
			m_blockSize = 0;
	}
	else if (header == u8"~~TYPE=LIST~~")
	{
		mode = Mode::Variable;
		m_blockSize = 0;
	}
	else
		csv.Rewind();

	while (!csv.IsEof())
	{
		Row r;
		while (!csv.IsEol())
		{
			Cell c;
			c.Parse(csv.NextCell());
			
			r.GetCells().push_back(std::move(c));
		}
		csv.NextLine();
		rows.push_back(std::move(r));
	}
}

void DMsg::Write()
{
	if (mode == Mode::Block)
	{
		int maxSize = 0;
		for (const Row &row : rows)
		{
			int rowSize = row.GetSize();
			if (rowSize > maxSize) maxSize = rowSize;
		}
		maxSize = (maxSize + 3) & ~3; // block size
		if (m_blockSize)
		{
			if (m_blockSize > maxSize)
				maxSize = m_blockSize;
		}

		// calculates metrics
		DMsgHeader header{
			.magic_header = {'d', '_', 'm', 's', 'g', 0, 0, 0},
			.ukn1 = 0x1,
			.obs = obs ? 1 : 0,
			.ukn2 = 3,
			.ukn3 = 3,
			.headerSize = 0x40,
			.one = 1,
			.zero = {0, 0, 0, 0}
		};
		header.blockSize = maxSize;
		header.indexSize = 0;
		header.entriesCount = rows.size();
		header.dataSize = header.blockSize * header.entriesCount;
		header.fileSize = header.headerSize + header.dataSize;

		std::ofstream pen(path, std::ios::binary | std::ios::trunc);
		pen.write((char *)&header, sizeof(header));

		for (Row &row : rows)
		{
			std::unique_ptr<char[]>buffer(new char[maxSize]);
			memset(buffer.get(), 0, maxSize);
			row.WriteRow((Record *)buffer.get(), maxSize);
			Xor(buffer.get(), maxSize);
			pen.write(buffer.get(), maxSize);
		}
	}
	else
	{
		int sumSize = 0;
		std::unique_ptr<DMsgIndex[]> indices(new DMsgIndex[rows.size()]);
		int count = 0;
		for (const Row &row : rows)
		{
			indices[count].offset = sumSize;
			indices[count++].size = row.GetSize();
			sumSize += row.GetSize();
		}

		// calculates metrics
		DMsgHeader header{
			.magic_header = {'d', '_', 'm', 's', 'g', 0, 0, 0},
			.ukn1 = 0x1,
			.obs = obs ? 1 : 0,
			.ukn2 = 3,
			.ukn3 = 3,
			.headerSize = 0x40,
			.one = 1,
			.zero = {0, 0, 0, 0}
		};
		header.blockSize = 0;
		header.entriesCount = count;
		header.indexSize = 8 * count;
		header.dataSize = sumSize;
		header.fileSize = header.headerSize + header.dataSize;

		std::ofstream pen(path, std::ios::binary | std::ios::trunc);
		pen.write((char *)&header, sizeof(header));
		Xor((char *)indices.get(), header.indexSize);
		pen.write((char *)indices.get(), header.indexSize);

		for (Row &row : rows)
		{
			std::unique_ptr<char[]>buffer(new char[row.GetSize()]);
			memset(buffer.get(), 0, row.GetSize());
			row.WriteRow((Record *)buffer.get(), row.GetSize());
			Xor(buffer.get(), row.GetSize());
			pen.write(buffer.get(), row.GetSize());
		}
	}
}

void DMsg::Row::ReadRow(Record *buffer, int limit)
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

void DMsg::Row::WriteRow(Record *buffer, int limit)
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

inline int DMsg::Cell::GetSize() const
{
	// FIXME: Remove the xybase::string::to_string or try to find some solution!
	return type ? 4 : 28 + ((xybase::string::to_string(str).size() + 1 + 3) & ~3); // str align
}
