// This file is a huge mess
// Find a way to refine it!
// Overhaul needed

#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <fstream>

#include "Image.h"

#pragma pack(push, 1)
struct BlockFileHeader
{
	char type[4];
	uint8_t flg1;
	uint8_t flg2;
	uint16_t ukn;
	uint32_t ukn1;
	uint32_t ukn2;
	uint32_t ukn3;
	uint32_t ukn4;
	uint32_t ukn5;
	uint32_t ukn6;
};

enum class BlockType : uint8_t
{
	BT_MENU_FORM = 0x30,
	//BT_MENU_FORM2 = 0xB0,
	//BT_IMAGE_SET2 = 0xB1,
	BT_IMAGE_SET = 0x31,
	BT_IMAGE = 0x20, //? image
	BT_END = 0x00,
};

struct BlockHeader
{
	char name[4];
	uint32_t type : 7;
	uint32_t size : 25;
	uint32_t padding[2];
};

struct MenuLayoutBlock
{
	char group[8];
	char name[8];
	uint8_t dstCount;
	uint8_t srcCount; // 16 maybe?
	uint8_t padding[14];

	struct Facet // structure not sure, only size is used to skip(
	{
		uint16_t size; // Whole structure size
		uint16_t ukn1;
		uint16_t ukn2;
		uint16_t ukn3;
		uint16_t ukn4;
		uint16_t ukn5;
		uint16_t ukn6;
		uint8_t  flags[18];
		uint32_t ukn7;
		struct ImageGroupRef {
			char group[8];
			char name[8];
		} imgGrpRef[1]; // dynamic alloc
		char pos2Str[1]; // dynamic alloc, two strings, 0h ended, 0h padded to 16
	};
};

std::string BlockNameGetCleanName(const char *str);

void BlockNamePutPaddedName(char *dst, const std::string &name);


#include <vec.h>

struct ImageClipBlock
{
	char group[8];
	char name[8];
	uint8_t refCount;
	// ref tex name list (char[16] * refCount)

	// uint8_t type = 0x74
	struct ImageClipCollection
	{
		uint16_t groupCount;
		struct ImageClip
		{
			uint8_t imageCount;
			struct ImageRef
			{
				enum class MappingType : uint8_t
				{
					MT_NORMAL = 0x0,
					MT_FLIP_HORIZONTAL = 0x1,
					MT_FLIP_VERTICAL = 0x2,

					MT_FLIP_BOTH = MT_FLIP_HORIZONTAL | MT_FLIP_VERTICAL,
				};

				xybase::Vec2<int16_t> tlPoint;
				xybase::Vec2<int16_t> trPoint;
				xybase::Vec2<int16_t> blPoint;
				xybase::Vec2<int16_t> brPoint;
				uint16_t w;
				uint16_t h;
				uint16_t x; // offset
				uint16_t y;
				MappingType type;
				RGBA tlColour;
				RGBA trColour;
				RGBA blColour;
				RGBA brColour;
				uint8_t ukn[4];
				char group[8];
				char name[8];

				ImageRef()
				{
					memset(this, 0, sizeof(ImageRef));
					memset(group, ' ', sizeof(group));
					memset(name, ' ', sizeof(name));
				}
			} imageSeq[1]; // dynamic
		} groups[1]; // dynamic
	};
};
#pragma pack(pop)


inline constexpr ImageClipBlock::ImageClipCollection::ImageClip::ImageRef::MappingType operator | (ImageClipBlock::ImageClipCollection::ImageClip::ImageRef::MappingType & lhs, ImageClipBlock::ImageClipCollection::ImageClip::ImageRef::MappingType & rhs)
{
	return static_cast<ImageClipBlock::ImageClipCollection::ImageClip::ImageRef::MappingType>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

inline constexpr ImageClipBlock::ImageClipCollection::ImageClip::ImageRef::MappingType operator & (ImageClipBlock::ImageClipCollection::ImageClip::ImageRef::MappingType &lhs, ImageClipBlock::ImageClipCollection::ImageClip::ImageRef::MappingType &rhs)
{
	return static_cast<ImageClipBlock::ImageClipCollection::ImageClip::ImageRef::MappingType>(static_cast<int>(lhs) & static_cast<int>(rhs));
}

class BlockFile
{
public:
	class Block
	{
	public:
		BlockHeader blockHeader;

		Block() : rawData(nullptr) {}

		char *GetRawData() { return rawData; }
		const char *GetRawData() const { return rawData; }

		virtual void Read(std::ifstream &eye) = 0;

		virtual void Write(std::ofstream &pen) = 0;

		// 通用克隆接口，便于在外部安全复制块
		virtual Block *Clone() const = 0;

		virtual ~Block() { if (rawData) delete[] rawData; }

	protected:

		char *rawData; // should be char [], by new[]
	};

	// 没有体的块
	class EmptyBlock : public Block
	{
	public:
		EmptyBlock() {}
		virtual void Read(std::ifstream &eye) {}
		virtual void Write(std::ofstream &pen) {}
		virtual Block *Clone() const override { return new EmptyBlock(*this); }
	};

	class UnknownBlock : public Block
	{
	public:
		virtual void Read(std::ifstream &eye)
		{
			size_t size = blockHeader.size * 16 - 16; // block header size === 16
			rawData = new char[size];
			eye.read(rawData, size);
		}

		virtual void Write(std::ofstream &pen)
		{
			size_t size = blockHeader.size * 16 - 16;
			pen.write(rawData, size);
		}

		virtual Block *Clone() const override
		{
			UnknownBlock *blk = new UnknownBlock(*this);
			if (rawData)
			{
				size_t size = blockHeader.size * 16 - 16;
				blk->rawData = new char[size];
				memcpy(blk->rawData, rawData, size);
			}
			return blk;
		}
	};

	// 保存了一个图像的块
	class ImageBlock : public Block
	{
	public:
		Image image;

		virtual void Read(std::ifstream &eye) override;
		virtual void Write(std::ofstream &pen) override;
		virtual Block *Clone() const override { return new ImageBlock(*this); }
	};

	// 图像集（拼图）的块
	class ImageSetBlock : public Block
	{
	public:
		class ImageGroup {
		public:
			using ImageRef = ImageClipBlock::ImageClipCollection::ImageClip::ImageRef;

			std::vector<ImageRef> imageRefs;
		};

		std::string group;
		std::string name;

		std::vector<std::string> refTextures;

		std::vector<ImageGroup> groups;
		virtual void Read(std::ifstream &eye) override;
		virtual void Write(std::ofstream &pen) override;
		virtual Block *Clone() const override { return new ImageSetBlock(*this); }
	};

	// 似乎定义了菜单列表的块
	class MenuBlock : public Block
	{
	public:
		// 不好解析，先作为黑箱处理
		std::vector<std::unique_ptr<char[]>> data;
		virtual void Read(std::ifstream &eye) override;
		virtual void Write(std::ofstream &pen) override;
		virtual Block *Clone() const override
		{
			MenuBlock *blk = new MenuBlock();
			blk->Block::blockHeader = this->Block::blockHeader;
			blk->blockHeader = this->blockHeader;
			for (const auto &ptr : data)
			{
				if (!ptr) continue;
				uint16_t size = *(uint16_t *)ptr.get();
				std::unique_ptr<char[]> buf(new char[size]);
				memcpy(buf.get(), ptr.get(), size);
				blk->data.push_back(std::move(buf));
			}
			return blk;
		}
	private:
		MenuLayoutBlock blockHeader;
	};

	BlockFileHeader header;
	std::string type;

	std::vector<Block *> blocks;

	BlockFile(std::filesystem::path path)
		: path(path) {}

	~BlockFile()
	{
		for (Block *ptr : blocks)
		{
			delete ptr;
		}
	}

	void Read();

	void Write();

	std::filesystem::path path;
};

