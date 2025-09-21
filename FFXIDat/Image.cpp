#include "Image.h"

#include <sstream>
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

void Image::Read(std::istream &eye)
{
	int type = eye.get();

	if (type != 0xA1 && type != 0x91)
		throw std::invalid_argument("unknown type");
	eye.seekg(-1, std::ios::cur);
	
	// 读取头部，获取基本信息
	eye.read((char *)&header, sizeof(header));

	assert(header.mipmapCount == 1); // 发现mipmapcount != 1时需做处理

	// Bitmap 读取
	if (type == 0x91)
	{
		this->type = ImageType::IT_BITMAP;
		if (header.bitCount == 8)
		{
			// 全部小端
			// 256 色调色板（A8R8G8B8） + 8 位索引
			int paletteSize = 256 * 4;
			texture.reset(new char[paletteSize + header.width * header.height]);
			eye.read(texture.get(), paletteSize + header.width * header.height);
		}
		else if (header.bitCount == 16)
		{
			// 16 位直接色(RGB565) - 5位红色，6位绿色，5位蓝色
			texture.reset(new char[header.width * header.height * 2]);
			eye.read(texture.get(), header.width * header.height * 2);
		}
		else if (header.bitCount == 24)
		{
			// 24 位直接色(R8G8B8)
			texture.reset(new char[header.width * header.height * 3]);
			eye.read(texture.get(), header.width * header.height * 3);
		}
		else if (header.bitCount == 32)
		{
			// 32 位直接色（A8R8G8B8）
			texture.reset(new char[header.width * header.height * 4]);
			eye.read(texture.get(), header.width * header.height * 4);
		}
		else
		{
			throw std::invalid_argument("unknown bitCount");
		}

		return;
	}

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

void Image::ReadFromMemory(const char* data, size_t size)
{
	std::istringstream eye(std::string(data, size), std::ios::binary);
	Read(eye);
}

void Image::Write(std::ostream &pen) const
{
	pen.write((char *)&header, sizeof(header));

	if (type != ImageType::IT_BITMAP)
	{
		pen.write((char *)&dxtHeader, sizeof(dxtHeader));
		pen.write(texture.get(), dxtHeader.textureSize);
	}
	else
	{
		if (header.bitCount == 8)
		{
			int paletteSize = 256 * 4;
			pen.write(texture.get(), paletteSize + header.width * header.height);
		}
		else if (header.bitCount == 16)
		{
			pen.write(texture.get(), header.width * header.height * 2);
		}
		else if (header.bitCount == 24)
		{
			pen.write(texture.get(), header.width * header.height * 3);
		}
		else if (header.bitCount == 32)
		{
			pen.write(texture.get(), header.width * header.height * 4);
		}
		else
		{
			throw std::invalid_argument("unknown bitCount");
		}
	}
}

void Image::WriteToMemory(char* data, size_t& size) const
{
	std::ostringstream pen(std::ios::binary);
	Write(pen);
	auto str = pen.str();
	if (str.size() > size)
		throw std::invalid_argument("buffer too small");
	size = str.size();
	memcpy(data, str.data(), str.size());
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

#pragma pack(push,1)
// RGB565位掩码结构
struct RGB565_MASKS {
    uint32_t redMask;   // 0xF800
    uint32_t greenMask; // 0x07E0
    uint32_t blueMask;  // 0x001F
};
//
//struct BITMAPFILEHEADER {
//	uint16_t bfType;      // 文件类型，必须是 'BM' (0x4D42)
//	uint32_t bfSize;      // 文件大小，以字节为单位
//	uint16_t bfReserved1; // 保留，必须设置为0
//	uint16_t bfReserved2; // 保留，必须设置为0
//	uint32_t bfOffBits;   // 从文件头到像素数据的偏移量，以字节为单位
//};
//
//struct BITMAPINFOHEADER {
//	uint32_t biSize;          // 结构体大小，必须是40字节
//	int32_t  biWidth;         // 图像宽度，以像素为单位
//	int32_t  biHeight;        // 图像高度，以像素为单位（正值表示自下而上，负值表示自上而下）
//	uint16_t biPlanes;        // 颜色平面数，必须是1
//	uint16_t biBitCount;      // 每个像素的位数（1, 4, 8, 16, 24, 或 32）
//	uint32_t biCompression;   // 压缩类型（0表示不压缩，3表示使用位掩码）
//	uint32_t biSizeImage;     // 图像数据大小，以字节为单位（可以为0，如果没有压缩）
//	int32_t  biXPelsPerMeter; // 水平分辨率，以像素/米为单位（可以为0）
//	int32_t  biYPelsPerMeter; // 垂直分辨率，以像素/米为单位（可以为0）
//	uint32_t biClrUsed;       // 使用的颜色索引数（对于调色板图像，可以为0）
//	uint32_t biClrImportant;  // 重要颜色索引数（可以为0）
//};

#pragma pack(pop)

#define BI_RGB 0
#define BI_BITFIELDS 3

std::unique_ptr<char[]> Image::GetBitmap() const
{
	if (type != ImageType::IT_BITMAP)
	{
		throw std::runtime_error("Only Bitmap format supported for Bitmap conversion");
	}
	
	size_t textureSize = 0;
	size_t paletteSize = 0;
	size_t extraSize = 0; // 用于RGB565位掩码
	
	if (header.bitCount == 8)
	{
		paletteSize = 256 * 4;
		textureSize = header.width * header.height;
	}
	else if (header.bitCount == 16)
	{
		textureSize = header.width * header.height * 2;
		extraSize = sizeof(RGB565_MASKS); // RGB565需要位掩码
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

	// 构建 Windows Bitmap 文件头并存入数据
	BITMAPFILEHEADER fileHeader = {};
	BITMAPINFOHEADER infoHeader = {};
	
	// 文件头设置
	fileHeader.bfType = 0x4D42; // "BM"
	fileHeader.bfReserved1 = 0;
	fileHeader.bfReserved2 = 0;
	
	// 信息头设置
	infoHeader.biSize = sizeof(BITMAPINFOHEADER);
	infoHeader.biWidth = header.width;
	infoHeader.biHeight = -static_cast<int32_t>(header.height); // 负值表示从上到下
	infoHeader.biPlanes = 1;
	infoHeader.biBitCount = header.bitCount;
	
	// 16位RGB565需要使用BI_BITFIELDS压缩方式
	if (header.bitCount == 16) {
		infoHeader.biCompression = BI_BITFIELDS;
	} else {
		infoHeader.biCompression = BI_RGB;
	}
	
	infoHeader.biSizeImage = textureSize;
	infoHeader.biXPelsPerMeter = 0;
	infoHeader.biYPelsPerMeter = 0;
	infoHeader.biClrUsed = (header.bitCount == 8) ? 256 : 0;
	infoHeader.biClrImportant = 0;
	
	// 计算偏移量和总大小
	size_t headerSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	if (header.bitCount == 8) {
		headerSize += paletteSize; // 调色板大小
	} else if (header.bitCount == 16) {
		headerSize += extraSize; // RGB565位掩码
	}
	
	fileHeader.bfOffBits = headerSize;
	fileHeader.bfSize = headerSize + textureSize;
	
	// 创建结果缓冲区
	auto result = std::make_unique<char[]>(fileHeader.bfSize);
	char* ptr = result.get();
	
	// 写入文件头
	memcpy(ptr, &fileHeader, sizeof(BITMAPFILEHEADER));
	ptr += sizeof(BITMAPFILEHEADER);
	
	// 写入信息头
	memcpy(ptr, &infoHeader, sizeof(BITMAPINFOHEADER));
	ptr += sizeof(BITMAPINFOHEADER);
	
	// 写入调色板或位掩码
	if (header.bitCount == 8) {
		memcpy(ptr, texture.get(), paletteSize);
		ptr += paletteSize;
		// 写入图像数据
		memcpy(ptr, texture.get() + paletteSize, textureSize);
	}
	else if (header.bitCount == 16) {
		// 写入RGB565位掩码
		RGB565_MASKS masks = {
			0xF800, // 红色掩码 (bits 15-11)
			0x07E0, // 绿色掩码 (bits 10-5)
			0x001F  // 蓝色掩码 (bits 4-0)
		};
		memcpy(ptr, &masks, sizeof(RGB565_MASKS));
		ptr += sizeof(RGB565_MASKS);
		
		// 写入图像数据
		memcpy(ptr, texture.get(), textureSize);
	}
	else {
		// 直接写入图像数据
		memcpy(ptr, texture.get(), textureSize);
	}
	
	return result;
}

void Image::ImportBitmap(const char* p_bmp)
{
	const char* ptr = p_bmp;
	
	// 读取文件头
	const BITMAPFILEHEADER* fileHeader = reinterpret_cast<const BITMAPFILEHEADER*>(ptr);
	if (fileHeader->bfType != 0x4D42) { // "BM"
		throw std::invalid_argument("Invalid BMP magic number");
	}
	ptr += sizeof(BITMAPFILEHEADER);
	
	// 读取信息头
	const BITMAPINFOHEADER* infoHeader = reinterpret_cast<const BITMAPINFOHEADER*>(ptr);
	if (infoHeader->biSize != sizeof(BITMAPINFOHEADER)) {
		throw std::invalid_argument("Unsupported BMP info header size");
	}
	ptr += sizeof(BITMAPINFOHEADER);
	
	// 检查支持的位深度
	if (infoHeader->biBitCount != 8 && infoHeader->biBitCount != 16 && 
		infoHeader->biBitCount != 24 && infoHeader->biBitCount != 32) {
		throw std::invalid_argument("Unsupported bit count");
	}
	
	// 对于16位，验证是否为RGB565格式
	if (infoHeader->biBitCount == 16) {
		if (infoHeader->biCompression != BI_BITFIELDS) {
			throw std::invalid_argument("16-bit BMP must use BI_BITFIELDS compression");
		}
		
		// 读取并验证位掩码
		const RGB565_MASKS* masks = reinterpret_cast<const RGB565_MASKS*>(ptr);
		if (masks->redMask != 0xF800 || masks->greenMask != 0x07E0 || masks->blueMask != 0x001F) {
			throw std::invalid_argument("Only RGB565 format is supported for 16-bit BMP");
		}
		ptr += sizeof(RGB565_MASKS);
	}
	
	// 填充ImageHeader
	header.type = 0x91; // Bitmap类型标记
	header.version = 0x28;
	header.width = abs(infoHeader->biWidth);
	header.height = abs(infoHeader->biHeight);
	header.mipmapCount = 1;
	header.bitCount = infoHeader->biBitCount;
	memset(header.group, 0, 8);
	memset(header.name, 0, 8);
	memset(header.ukn, 0, sizeof(header.ukn));
	
	// 计算纹理大小
	size_t textureSize = 0;
	size_t paletteSize = 0;
	
	if (header.bitCount == 8) {
		paletteSize = 256 * 4;
		textureSize = header.width * header.height;
	}
	else if (header.bitCount == 16) {
		textureSize = header.width * header.height * 2;
	}
	else if (header.bitCount == 24) {
		textureSize = header.width * header.height * 3;
	}
	else if (header.bitCount == 32) {
		textureSize = header.width * header.height * 4;
	}
	
	// 读取纹理数据
	if (header.bitCount == 8) {
		// 8位：调色板 + 图像数据
		texture.reset(new char[paletteSize + textureSize]);
		memcpy(texture.get(), ptr, paletteSize);
		
		// 复制图像数据
		const char* imageData = p_bmp + fileHeader->bfOffBits;
		memcpy(texture.get() + paletteSize, imageData, textureSize);
	}
	else {
		// 16/24/32位：直接复制图像数据
		texture.reset(new char[textureSize]);
		const char* imageData = p_bmp + fileHeader->bfOffBits;
		memcpy(texture.get(), imageData, textureSize);
	}
	
	type = ImageType::IT_BITMAP;
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
