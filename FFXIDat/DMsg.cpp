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
	if (header == u8"~~TYPE=BLOCK~~")
	{
		mode = Mode::Block;
		if (!csv.IsEol())
			m_blockSize = xybase::string::stoi(csv.NextCell());
		else
			m_blockSize = 0;
		csv.NextLine();
	}
	else if (header == u8"~~TYPE=LIST~~")
	{
		mode = Mode::Variable;
		m_blockSize = 0;
		csv.NextLine();
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
		header.fileSize = header.headerSize + header.dataSize + header.indexSize;

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
