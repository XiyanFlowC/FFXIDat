#pragma once

#include <filesystem>
#include <string>
#include <fstream>

// UTF-8 ����ͨ CSV ������
// ��֧���κ����������ʽ
class CsvFile
{
public:

	CsvFile(std::filesystem::path filePath, std::ios_base::openmode mode);

	~CsvFile();

	std::u8string NextCell();

	void NewCell(const std::u8string &p_str);

	void NextLine();

	void NewLine();

	bool IsEol();

	bool IsEof();

	void Close();

private:
	std::fstream m_stream;
	size_t m_size = 0;
	bool m_eolFlag = false;
	bool m_firstCellFlag = true;
};
