#include "CsvFile.h"


CsvFile::CsvFile(std::filesystem::path filePath, std::ios_base::openmode mode)
	: m_stream(filePath, mode)
{
	if (!m_stream.is_open())
		throw std::runtime_error("Open file failed > " + filePath.string());

	if (mode & std::ios_base::in)
	{
		m_stream.seekg(0, std::ios_base::end);
		m_size = m_stream.tellg();
		m_stream.seekg(0);

		char buf[3];
		m_stream.read(buf, 3);
		// 不为 BOM，Rewind
		if (memcmp(buf, "\xEF\xBB\xBF", 3)) m_stream.seekg(0);
	}
	if (mode & std::ios::out)
	{
		// 让 Excel 之类的开心
		m_stream.write("\xEF\xBB\xBF", 3);
	}
}

CsvFile::~CsvFile()
{
}

std::u8string CsvFile::NextCell()
{
	if (m_eolFlag)
		throw std::runtime_error("No more cell!");

	std::u8string ret;

	int x = m_stream.tellg();

	char8_t ch = m_stream.get();
	if (ch == '\"')
	{
		ch = m_stream.get();
		while ((1))
		{
			if (ch == '\"')
			{
				ch = m_stream.get();
				if (ch == '\"')
					ret += '\"';
				else if (ch == ',')
					break;
				else if (ch == '\n')
					break;
				else if (ch == '\r')
					break;
				else
				{
					throw std::runtime_error("bad csv format.");
				}
			}
			else ret += ch;

			ch = m_stream.get();
		}
	}
	else while (ch != ',' && ch != '\n' && ch != '\r')
	{
		ret += ch;

		ch = m_stream.get();
	}
	if (ch == '\r')
	{
		ch = m_stream.get();
		if (ch != '\n')
			throw std::runtime_error("bad eol.");
	}

	m_eolFlag = ch == '\n';

	return ret;
}

void CsvFile::NewCell(const std::u8string &p_str)
{
	if (m_firstCellFlag)
		m_firstCellFlag = false;
	else
		m_stream.put((uint8_t)',');

	if (p_str.find_first_of(u8"\n\r\",") != std::u8string::npos)
	{
		m_stream.put((uint8_t)'"');
		for (auto &&ch : p_str)
		{
			if (ch == '"') m_stream.write("\"\"", 2);
			else m_stream.put((uint8_t) ch);
		}
		m_stream.put((uint8_t)'"');
	}
	else
	{
		m_stream.write((char *)p_str.c_str(), p_str.size());
	}
}

bool CsvFile::IsEol()
{
	return m_eolFlag;
}

void CsvFile::NextLine()
{
	if (!m_eolFlag)
	{
		int ch = m_stream.get();
		while (ch != '\r' && ch != '\n') ch = m_stream.get();
		if (ch == '\r')
		{
			ch = m_stream.get();
			if (ch != '\n')
			{
				m_stream.seekg(-1, std::ios::cur);
			}
		}
	}
	m_eolFlag = false;
}

void CsvFile::NewLine()
{
	m_firstCellFlag = true;
	m_stream.put((uint8_t)'\r');
	m_stream.put((uint8_t)'\n');
}

bool CsvFile::IsEof()
{
	if (m_size) return m_stream.tellg() >= m_size;
	return m_stream.eof();
}

void CsvFile::Close()
{
	m_stream.close();
}

void CsvFile::Rewind()
{
	if (m_size) m_stream.seekg(0);
	else m_stream.seekp(0);
}
