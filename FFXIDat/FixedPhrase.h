#pragma once

#include <cstdint>
#include <vector>
#include <string>

#pragma pack(push, 1)
struct fixed_phrase_category
{
	uint8_t a; // 0x02
	uint8_t b; // 0x01
	uint8_t cat; // category_idx
	uint8_t ent; // ent_idx
};

struct fixed_phrase_category_header
{
	fixed_phrase_category cat;
	char cat_name[32];
	char cat_pron[32];
	int32_t count;
	int32_t size;
};
#pragma pack(pop)

class FixedPhrase
{
public:
	class FixedPhraseCategory
	{
	public:
		class FixedPhraseEntry
		{
		public:
			fixed_phrase_category cat;

			std::u8string text;
			std::u8string pron;
		};
		fixed_phrase_category cat;

		std::u8string categoryName;
		std::u8string categoryPron;

		std::vector<FixedPhraseEntry> entries;
	};

	void Read(std::wstring path);
	void Write(std::wstring path);

	void FromCsv(std::wstring path);
	void ToCsv(std::wstring path);

	std::vector<FixedPhraseCategory> categories;
};
