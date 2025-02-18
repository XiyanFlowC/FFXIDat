#pragma once

#include <cstdint>
#include <fstream>
#include <memory>

#pragma pack(push,1)
struct ImageHeader
{
	uint8_t type;         // 0x91(Bitmap), 0xA1(DXT)
	char group[8];
	char name[8];
	uint32_t version;     // ? - always 28h?
	uint32_t width;
	uint32_t height;
	uint16_t mipmapCount; // not sure, observed 1 for now
	uint16_t bitCount;    // not sure, 4h, 8h, 20h are observed
	uint32_t ukn[6];      // observed 0 for now
};

struct DxtImageHeader
{
	char fourCC[4];
	uint32_t textureSize;
	uint32_t pitch;       // ? pitch of DDS, in bytes
};

struct RGBA
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;

	void Set(uint8_t p_r, uint8_t p_g, uint8_t p_b, uint8_t p_a)
	{
		r = p_r;
		g = p_g;
		b = p_b;
		a = p_a;
	}
};

#pragma pack(pop)

/**
 * @brief Image data process. The image file usually
 * stored in other structures, so read/write using exist
 * stream instead of create a new one.
 */
class Image
{
public:

	enum class ImageType {
		IT_BITMAP,
		IT_DXT1,
		IT_DXT2,
		IT_DXT3,
		IT_DXT4,
		IT_DXT5,
	} type;

	ImageHeader header;
	DxtImageHeader dxtHeader;
	std::unique_ptr<char[]> texture;

	int GetWidth() const;
	
	int GetHeight() const;

	void Read(std::ifstream &eye);

	void Write(std::ofstream &pen);

	std::unique_ptr<char[]> GetDds() const;

	void ImportDds(const char *p_dds);
};

