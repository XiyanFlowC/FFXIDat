#pragma once

#include <stdexcept>
#include <cstdint>
#include <vector>
#include <string>

struct RecordString
{
	int32_t one;
	int32_t zero[6];
	char str[1];
};

struct RecordSpec
{
	int32_t offset;
	int32_t type; // 0 for string, 1 for int
};

struct Record
{
	int32_t cellCount;
	RecordSpec spec[1]; // dynamic alloc
};

class Cell
{
public:
	Cell(int value = 0) : type(1), value(value) {}

	Cell(const std::u8string &str) : str(str), type(0), value(0) {}

	std::u8string ToString() const
	{
		return type ? (char8_t *)std::to_string(value).c_str() : std::u8string(u8"'") + str;
	}

	void Parse(const std::u8string &p_str)
	{
		type = 0;
		if (p_str.starts_with('\''))
		{
			str = p_str.substr(1);
		}
		else if (p_str[0] > '9' || p_str[0] < '0')
		{
			str = p_str;
		}
		else
		{
			try
			{
				value = std::stoi((char *)p_str.c_str());
				type = 1;
			}
			catch (std::invalid_argument &ex)
			{
				value = 0;
				type = 0;
				str = p_str;
			}
		}
	}

	template<class T>
	T Get() const;

	template<>
	int Get() const
	{
		if (type) return value;
		else throw std::runtime_error("Not a number!");
	}

	template<>
	std::u8string Get() const
	{
		if (type) throw std::runtime_error("Not a string!");
		return str;
	}

	template<>
	const char *Get() const
	{
		if (type) return (const char *)value;
		return (const char *)str.c_str();
	}

	void Set(int p_value)
	{
		value = p_value;
		type = 1;
	}

	void Set(const std::u8string &p_str)
	{
		str = p_str;
		type = 0;
	}

	int GetType() const
	{
		return type;
	}

	int GetSize() const;

protected:
	std::u8string str;
	int value;
	int type; // 0 str, 1 int
};

class Row
{
public:
	using iterator = std::vector<Cell>::iterator;
	using const_iterator = std::vector<Cell>::const_iterator;

	iterator begin() { return cells.begin(); }
	iterator end() { return cells.end(); }
	const_iterator begin() const { return cells.begin(); }
	const_iterator end() const { return cells.end(); }

	void ReadRow(Record *buffer, int limit);

	/*void ReadRow(std::ifstream &eye, int limit)
	{
		std::unique_ptr<char[]> buffer(new char[limit]);
		eye.read(buffer.get(), limit);
		Xor(buffer.get(), limit);
		ReadRow((Record *)buffer.get(), limit);
	}*/

	void WriteRow(Record *stream, int limit);

	int GetSize() const
	{
		int ret = 4 + cells.size() * 8;
		for (const Cell &cell : cells)
		{
			ret += cell.GetSize();
		}
		return ret;
	}

	Cell &operator[](int index)
	{
		return cells[index];
	}

	std::vector<Cell> &GetCells()
	{
		return cells;
	}
	
	const std::vector<Cell> &GetCellsConst() const
	{
		return cells;
	}
protected:
	// int cellCount;
	std::vector<Cell> cells;
};
