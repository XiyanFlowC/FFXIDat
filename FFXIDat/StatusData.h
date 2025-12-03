#pragma once
#include <string>
#include <vector>
#include "Record.h"
#include "Image.h"

// 0/9 data ror 7, image raw

#pragma pack(push,1)
union StatusSpecData
{
	char raw[636];
	Record rec;
};

struct StatusEntry
{
	uint16_t id; // ror 7 no
	uint16_t flg;
	StatusSpecData spec; // ror 7 no
	uint32_t image_length; // raw
	char image_data[5499]; // raw, fixed size without end marker
	uint8_t end_marker; // should be 0xFF to indicate end of block
};
#pragma pack(pop)

class StatusData
{
public:
	class StatusDatum
	{
	public:
		uint32_t id;
		uint16_t flg; // Add flag field
		std::u8string description;
		Image image;
	};

	void Read(std::wstring path);
	void Write(std::wstring path);
	void ToICsv(const std::wstring& path) const;
	std::vector<StatusDatum> data;
};

