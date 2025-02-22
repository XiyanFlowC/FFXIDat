#include "Image.h"

#include <stdexcept>
#include <vector>
#include <cstring>
#include <cassert>
#include <d3d10.h>

int Image::GetWidth() const
{
	return header.width;
}

int Image::GetHeight() const
{
	return header.height;
}

void Image::Read(std::ifstream &eye)
{
	int type = eye.get();
	if (type != 0xA1)
		throw std::invalid_argument("unknown type");
	eye.seekg(-1, std::ios::cur);
	
	// 读取头部，获取基本信息
	eye.read((char *)&header, sizeof(header));

	assert(header.mipmapCount == 1); // 发现mipmapcount != 1时需做处理

	// DXT 读取
	eye.read((char *)&dxtHeader, sizeof(dxtHeader));
	switch (*(int32_t *)dxtHeader.fourCC)
	{
	case 'DXT5':
		this->type = ImageType::IT_DXT5;
		break;
	case 'DXT4':
		this->type = ImageType::IT_DXT4;
		break;
	case 'DXT3':
		this->type = ImageType::IT_DXT3;
		break;
	case 'DXT2':
		this->type = ImageType::IT_DXT2;
		break;
	case 'DXT1':
		this->type = ImageType::IT_DXT1;
		break;
	default:
		throw std::invalid_argument("unknown fourCc");
	}

	texture.reset(new char[dxtHeader.textureSize]);
	eye.read(texture.get(), dxtHeader.textureSize);
}

void Image::Write(std::ofstream &pen)
{
	pen.write((char *)&header, sizeof(header));

	if (type != ImageType::IT_BITMAP)
	{
		pen.write((char *)&dxtHeader, sizeof(dxtHeader));
		pen.write(texture.get(), dxtHeader.textureSize);
	}
	else
	{
		// TODO: implement ...
	}
}

// 懒得引整个头文件了
#pragma pack(push,1)
struct DDS_PIXELFORMAT 
{
	uint32_t dwSize;
	uint32_t dwFlags;
	uint32_t dwFourCC;
	uint32_t dwRGBBitCount;
	uint32_t dwRBitMask;
	uint32_t dwGBitMask;
	uint32_t dwBBitMask;
	uint32_t dwABitMask;
};

#define DDSD_CAPS 0x1
#define DDSD_HEIGHT 0x2
#define DDSD_WIDTH 0x4
#define DDSD_PITCH 0x8
#define DDSD_PIXELFORMAT 0x1000
#define DDSD_MIPMAPCOUNT 0x20000
#define DDSD_LINEARSIZE 0x80000
#define DDPF_FOURCC 0x4

struct DDS_HEADER
{
	uint32_t        dwSize;
	uint32_t        dwFlags;
	uint32_t        dwHeight;
	uint32_t        dwWidth;
	uint32_t        dwPitchOrLinearSize;
	uint32_t        dwDepth;
	uint32_t        dwMipMapCount;
	uint32_t        dwReserved1[11];
	DDS_PIXELFORMAT ddspf;
	uint32_t        dwCaps;
	uint32_t        dwCaps2;
	uint32_t        dwCaps3;
	uint32_t        dwCaps4;
	uint32_t        dwReserved2;
};
#pragma pack(pop)

const uint32_t DDS_MAGIC = 0x20534444; // "DDS "

std::unique_ptr<char[]> Image::GetDds() const
{
	if (type == ImageType::IT_BITMAP)
	{
		throw std::runtime_error("Bitmap format not supported for DDS conversion");
	}

	DDS_HEADER ddsHeader = {};
	DDS_PIXELFORMAT ddspf = {};

	ddspf.dwSize = 32;
	ddspf.dwFlags = DDPF_FOURCC;
	const char *fourCCStr = nullptr;
	switch (type)
	{
	case ImageType::IT_DXT1: fourCCStr = "DXT1"; break;
	case ImageType::IT_DXT2: fourCCStr = "DXT2"; break;
	case ImageType::IT_DXT3: fourCCStr = "DXT3"; break;
	case ImageType::IT_DXT4: fourCCStr = "DXT4"; break;
	case ImageType::IT_DXT5: fourCCStr = "DXT5"; break;
	default: throw std::runtime_error("Unsupported DXT type");
	}
	memcpy(&ddspf.dwFourCC, fourCCStr, 4);

	ddsHeader.dwSize = sizeof(DDS_HEADER);
	ddsHeader.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_LINEARSIZE;
	ddsHeader.dwHeight = header.height;
	ddsHeader.dwWidth = header.width;
	ddsHeader.dwPitchOrLinearSize = dxtHeader.textureSize;
	ddsHeader.dwDepth = 1;
	ddsHeader.dwMipMapCount = header.mipmapCount;
	ddsHeader.ddspf = ddspf;
	ddsHeader.dwCaps = 0x1000; // DDSCAPS_TEXTURE

	// 计算总大小
	size_t totalSize = 4 + sizeof(DDS_HEADER) + dxtHeader.textureSize;
	std::vector<char> buffer(totalSize);
	char *ptr = buffer.data();

	memcpy(ptr, &DDS_MAGIC, 4);
	ptr += 4;
	memcpy(ptr, &ddsHeader, sizeof(DDS_HEADER));
	ptr += sizeof(DDS_HEADER);
	memcpy(ptr, texture.get(), dxtHeader.textureSize);

	auto result = std::make_unique<char[]>(totalSize);
	memcpy(result.get(), buffer.data(), totalSize);
	return result;
}

void Image::ImportDds(const char *p_dds)
{
	if (memcmp(p_dds, "DDS ", 4) != 0) {
		throw std::invalid_argument("Invalid DDS magic number");
	}
	const char *ptr = p_dds + 4;

	const DDS_HEADER *ddsHeader = reinterpret_cast<const DDS_HEADER *>(ptr);
	ptr += sizeof(DDS_HEADER);

	if ((ddsHeader->ddspf.dwFlags & DDPF_FOURCC) == 0)
	{
		throw std::invalid_argument("DDS is not in FourCC format");
	}

	char fourCC[5] = { 0 };
	memcpy(fourCC, &ddsHeader->ddspf.dwFourCC, 4);
	ImageType newType;
	int unitSize = 4;
	if (strcmp(fourCC, "DXT1") == 0)
	{
		newType = ImageType::IT_DXT1;
		unitSize = 2;
	}
	else if (strcmp(fourCC, "DXT2") == 0)
	{
		newType = ImageType::IT_DXT2;
		unitSize = 4;
	}
	else if (strcmp(fourCC, "DXT3") == 0)
	{
		newType = ImageType::IT_DXT3;
		unitSize = 4;
	}
	else if (strcmp(fourCC, "DXT4") == 0)
	{
		newType = ImageType::IT_DXT4;
	}
	else if (strcmp(fourCC, "DXT5") == 0)
	{
		newType = ImageType::IT_DXT5;
	}
	else
	{
		throw std::invalid_argument("Unsupported FourCC format");
	}

	// 填充ImageHeader
	header.type = 0xA1; // DXT类型标记
	header.version = 0x28;
	header.width = ddsHeader->dwWidth;
	header.height = ddsHeader->dwHeight;
	header.mipmapCount = ddsHeader->dwMipMapCount;
	// header.bitCount = 0;

	// 填充DxtImageHeader
	fourCC[0] ^= fourCC[3];
	fourCC[1] ^= fourCC[2];
	fourCC[3] ^= fourCC[0];
	fourCC[2] ^= fourCC[1];
	fourCC[0] ^= fourCC[3];
	fourCC[1] ^= fourCC[2];

	memcpy(dxtHeader.fourCC, fourCC, 4);
	dxtHeader.textureSize = ddsHeader->dwPitchOrLinearSize;
	dxtHeader.pitch = ddsHeader->dwWidth * unitSize;//ddsHeader->dwPitchOrLinearSize;

	// 复制纹理数据
	texture.reset(new char[dxtHeader.textureSize]);
	memcpy(texture.get(), ptr, dxtHeader.textureSize);
	type = newType;
}
