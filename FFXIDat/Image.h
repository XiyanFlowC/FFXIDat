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
	Image()
		: type(ImageType::IT_BITMAP)
		, texture(nullptr)
		, header(0)
		, dxtHeader(0) {}

	Image(const Image& other)
	{
		this->type = other.type;
		this->header = other.header;
		this->dxtHeader = other.dxtHeader;
		if (other.texture)
		{
			size_t textureSize = 0;
			if (this->type != ImageType::IT_BITMAP)
			{
				textureSize = other.dxtHeader.textureSize;
			}
			else
			{
				if (header.bitCount == 8)
				{
					textureSize = 256 * 4 + header.width * header.height;
				}
				else if (header.bitCount == 16)
				{
					textureSize = header.width * header.height * 2;
				}
				else if (header.bitCount == 24)
				{
					textureSize = header.width * header.height * 3;
				}
				else if (header.bitCount == 32)
				{
					textureSize = header.width * header.height * 4;
				}
				else
				{
					throw std::invalid_argument("unknown bitCount");
				}
			}
			texture.reset(new char[textureSize]);
			std::memcpy(texture.get(), other.texture.get(), textureSize);
		}
		else
		{
			texture.reset();
		}
	}

	Image (Image&& other) noexcept
	{
		this->type = other.type;
		this->header = other.header;
		this->dxtHeader = other.dxtHeader;
		this->texture = std::move(other.texture);
	}

	~Image() = default;

	Image& operator=(const Image& other) = default;
	Image& operator=(Image&& other) = default;

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

	void Read(std::istream &eye);

	/**
	 * @brief Read image data from memory buffer
	 * @param data Pointer to the image data in memory
	 * @param size Size of the image data buffer
	 */
	void ReadFromMemory(const char* data, size_t size);

	void Write(std::ostream &pen) const;

	void WriteToMemory(char* data, size_t& size) const;

	std::unique_ptr<char[]> GetDds() const;

	std::unique_ptr<char[]> GetBitmap() const;

	void ImportDds(const char *p_dds);

	void ImportBitmap(const char* p_bmp);
};

