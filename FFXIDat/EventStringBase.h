#pragma once

#include <cstdint>

#include <vector>
#include <string>

struct EventStringBaseHeader {
	uint32_t size : 24;
	uint32_t flag : 8; // always 10h?
};

#include <filesystem>

class EventStringBase
{
public:
	using iterator = std::vector<std::u8string>::iterator;
	using const_iterator = std::vector<std::u8string>::const_iterator;

	iterator begin() { return strs.begin(); }
	iterator end() { return strs.end(); }
	const_iterator begin() const { return strs.begin(); }
	const_iterator end() const { return strs.end(); }

	EventStringBase(std::filesystem::path path)
		: path(path), flag(0x10) {}

	void Read();

	void Write();

	void ToCsv(std::filesystem::path path);

	void FromCsv(std::filesystem::path path);

	size_t Size() const { return strs.size(); }
	size_t size() const { return strs.size(); }

	void EmplaceBack(const std::u8string& str) { strs.emplace_back(str); }

	std::u8string &operator[](size_t idx) { return strs[idx]; }
	const std::u8string &operator[](size_t idx) const { return strs[idx]; }

	std::filesystem::path path;
protected:
	void Xor(char *tar, int size);

	std::vector<std::u8string> strs;

	int flag;

};

