#pragma once

#include <cstdint>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include "CsvFile.h"
#include "Record.h"

#pragma pack(push, 1)
struct DMsgHeader
{
	char magic_header[8]; // d_msg\0\0\0
	int16_t ukn1;
	int16_t obs; // xor with 0xFF or not
	int32_t ukn2; // ? 03h
	int32_t ukn3; // ? 03h
	int32_t fileSize;
	int32_t headerSize;
	int32_t indexSize; // is 0 if no index
	int32_t blockSize; // is 0 if size variable
	int32_t dataSize; // data segment size
	int32_t entriesCount; // how many entries in this file
	int32_t one; // unknown, always 1
	int32_t zero[4]; // unknown, always 0
};

struct DMsgIndex
{
	int32_t offset;
	int32_t size;
};

#pragma pack(pop)

// DMsg
class DMsg
{
public:

	using iterator = std::vector<Row>::iterator;
	using const_iterator = std::vector<Row>::const_iterator;
	
	iterator begin() { return rows.begin(); }
	iterator end() { return rows.end(); }
	const_iterator begin() const { return rows.begin(); }
	const_iterator end() const { return rows.end(); }

	void Read();

	void ToCsv(std::filesystem::path csvPath);

	void FromCsv(std::filesystem::path csvPath);

	void Write();

	DMsg(std::filesystem::path path) : path(path) {}

	Row &operator[](int index)
	{
		return rows[index];
	}

	enum class Mode
	{
		Block,
		Variable,
	};
	Mode mode = Mode::Variable;
	int m_blockSize = 0;
	bool obs = false;
	std::filesystem::path path;
protected:
	std::vector<Row> rows;

	void Xor(char *target, int size)
	{
		if (!obs) return;

		for (int i = 0; i < size; ++i)
		{
			*target++ ^= 0xFF;
		}
	}
};

