#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include "Record.h"
#include "Image.h"

#pragma pack(push, 1)
struct MBRecord {
	uint32_t id;
	uint16_t idx;
	char name[32];
	int16_t para1[5];
	int8_t para2[64];

	union {
		char raw[528];
		Record info_rec;
	} rec;

	char icon_data[2431];

	char terminator; // must be 0xFF
};
#pragma pack(pop)

class MonBridge
{
public:
	class MonBridgeDatum
	{
	public:
		uint32_t id;
		uint16_t idx;
		std::u8string internalName; // from name field (ASCII internal identifier)
		std::u8string displayName; // from info_rec (the display name shown in game)
		Image image;
		
		// Store the complete original entry to preserve ALL fields including unknown ones
		MBRecord originalEntry;
		
		// Convenience accessors for commonly used fields
		uint16_t& index() { return originalEntry.idx; }
		const uint16_t& index() const { return originalEntry.idx; }
		
		MonBridgeDatum() : originalEntry{}
		{
			// Initialize originalEntry with default values
			originalEntry.id = 0;
			originalEntry.idx = 0;
			memset(originalEntry.name, 0, sizeof(originalEntry.name));
			memset(originalEntry.para1, 0, sizeof(originalEntry.para1));
			memset(originalEntry.para2, 0, sizeof(originalEntry.para2));
			memset(&originalEntry.rec, 0, sizeof(originalEntry.rec));
			memset(originalEntry.icon_data, 0, sizeof(originalEntry.icon_data));
			originalEntry.terminator = 0xFF;
		}
	};

	void Read(const char* path);
	void Read(const std::wstring& path);
	void Write(const char* path);
	void Write(const std::wstring& path);
	void ToCsv(const std::wstring& path) const;
	void ToICsv(const std::wstring& path) const;

	std::vector<MonBridgeDatum> data;
};

