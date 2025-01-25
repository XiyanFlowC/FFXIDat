#pragma once

#include <cstdint>
#include <string>
#include <vector>

#pragma pack(push, 1)

struct XiStringHeader {
	char magicHeader[8];
	int32_t version; // always 0x20000
	int32_t zero[5]; // always 0?
	int32_t fileSize;
	int32_t entriesCount;
	int32_t indicesSize;
	int32_t dataSize;
	int32_t reserved; // ? always 0?
	int32_t id; // seems an id ? Xistring files have same perpose have the same id not sure
};

struct XiStringIndex {
	int32_t offset;
	uint16_t size;
	uint16_t flag1;
	uint16_t flag2;
	uint16_t flag3;// ? not sure what it is
};

#pragma pack(pop)

#include "CsvFile.h"

#include <filesystem>
#include "xystring.h"

class XiString
{
public:
	struct StringEntry {
		std::string str;
		uint16_t flag1, flag2, flag3;

		StringEntry(std::string str, uint16_t flag1, uint16_t flag2, uint16_t flag3)
			: str(str), flag1(flag1), flag2(flag2), flag3(flag3) {}

		StringEntry()
			: flag1(0), flag2(0), flag3(0) {}
	};

	using iterator = std::vector<StringEntry>::iterator;
	using const_iterator = std::vector<StringEntry>::const_iterator;

	iterator begin() { return entries.begin(); }
	iterator end() { return entries.end(); }
	const_iterator begin() const { return entries.begin(); }
	const_iterator end() const { return entries.end(); }

	XiString(std::filesystem::path path)
		: path(path), id(0) {}

	void Read();

	void Write();

	void ToCsv(std::filesystem::path path);

	void FromCsv(std::filesystem::path path);

	static std::string Encode(const std::string &in);

	static std::string Decode(const std::string &in);

	static int GetStep(const char *p);

	int id;

	std::filesystem::path path;
protected:
	std::vector<StringEntry> entries;
};

