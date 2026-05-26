#include "BlockFile.h"

#include <xyutils.h>

void BlockFile::Read()
{
	std::ifstream eye(path, std::ios::in | std::ios::binary);
	eye.read((char *)&header, sizeof(header));
	type = std::string(header.type, 4);

	if (header.ukn || header.ukn1 || header.ukn2/* || header.ukn3 || header.ukn4 || header.ukn5 || header.ukn6*/)
		throw std::runtime_error("File format error: unexpected non-zero fields in header.");

	BlockHeader hdr;
	do
	{
		eye.read((char *)&hdr, sizeof(BlockHeader));

		Block *rst;
		switch ((BlockType)hdr.type)
		{
		case BlockType::BT_MENU_FORM:
			rst = new MenuBlock();
			break;
		case BlockType::BT_IMAGE:
			rst = new ImageBlock();
			break;
		case BlockType::BT_IMAGE_SET:
			rst = new ImageSetBlock();
			break;
		case BlockType::BT_END:
			rst = new EmptyBlock();
			break;
		default:
			rst = new UnknownBlock();
			break;
		}
		memcpy(&rst->blockHeader, &hdr, sizeof(BlockHeader));
		rst->Read(eye);
		blocks.push_back(rst);

		size_t pos = eye.tellg();
		if (pos == -1)
		{
			throw std::runtime_error("File format error.");
		}
		size_t pad = XY_ALIGN(pos, 16) - pos;
		while (pad--)
		{
			eye.get();
		}
	} while ((BlockType)hdr.type != BlockType::BT_END);
}

void BlockFile::Write()
{
	std::ofstream pen(path, std::ios::out | std::ios::binary);
	pen.write((char *)&header, sizeof(header));
	for (Block *block : blocks)
	{
		// FIXME: 设计写入前的计算方法而不是事后补救
		size_t startPos = pen.tellp();

		pen.write((char *)&block->blockHeader, sizeof(BlockHeader));
		block->Write(pen);

		int pos = pen.tellp();
		int pad = XY_ALIGN(pos, 16) - pos;
		while (pad--)
		{
			pen.put(0);
		}

		size_t endPos = pen.tellp();
		size_t size = endPos - startPos;
		block->blockHeader.size = size / 16;
		pen.seekp(startPos);
		pen.write((char *)&block->blockHeader, sizeof(BlockHeader));
		pen.seekp(endPos);
	}
	pen.close();
}

void BlockFile::MenuBlock::Read(std::ifstream &eye)
{
	eye.read((char *)&blockHeader, sizeof(MenuLayoutBlock));
	for (int i = 0; i < blockHeader.srcCount; ++i)
	{
		// MenuLayoutBlock::Facet 
		uint16_t size;
		eye.read((char *)&size, sizeof(uint16_t));
		eye.seekg(-2, std::ios::cur);
		std::unique_ptr<char[]> buffer(new char[size]);
		eye.read(buffer.get(), size);
		data.push_back(std::move(buffer));
	}
	for (int i = 0; i < blockHeader.dstCount; ++i)
	{
		uint16_t size;
		eye.read((char *)&size, sizeof(uint16_t));
		eye.seekg(-2, std::ios::cur);
		std::unique_ptr<char[]> buffer(new char[size]);
		eye.read(buffer.get(), size);
		data.push_back(std::move(buffer));
	}
}

void BlockFile::MenuBlock::Write(std::ofstream &pen)
{
	pen.write((char *)&blockHeader, sizeof(MenuLayoutBlock));
	for (auto &&ptr : data)
	{
		uint16_t size = *(uint16_t *)ptr.get();
		pen.write(ptr.get(), size);
	}
}

void BlockFile::ImageBlock::Read(std::ifstream &eye)
{
	image.Read(eye);
}

void BlockFile::ImageBlock::Write(std::ofstream &pen)
{
	image.Write(pen);
}

void BlockFile::ImageSetBlock::Read(std::ifstream &eye)
{
	ImageClipBlock header;
	eye.read((char *)&header, sizeof(header));

	group = std::string(header.group, 8);
	name = std::string(header.name, 8);
	char buffer[16];
	for (int i = 0; i < header.refCount; ++i)
	{
		eye.read(buffer, 16);
		refTextures.push_back(std::string(buffer, 16));
	}

	uint16_t collectionCount;
	eye.read((char *)&collectionCount, sizeof(uint16_t));
	for (int i = 0; i < collectionCount; ++i)
	{
		uint8_t clipCount;
		eye.read((char *)&clipCount, sizeof(uint8_t));
		ImageGroup curGrp;
		for (int j = 0; j < clipCount; ++j)
		{
			ImageGroup::ImageRef ref;
			eye.read((char *)&ref, sizeof(ImageGroup::ImageRef));
			curGrp.imageRefs.push_back(ref);
		}
		groups.push_back(curGrp);
	}
}

void BlockFile::ImageSetBlock::Write(std::ofstream &pen)
{
	ImageClipBlock header;
	memset(header.group, ' ', 8);
	memset(header.name, ' ', 8);
	memcpy(header.group, group.c_str(), group.size());
	memcpy(header.name, name.c_str(), name.size());
	header.refCount = refTextures.size();

	pen.write((char *)&header, sizeof(header));

	for (auto &&ref : refTextures)
	{
		pen.write(ref.c_str(), 16);
	}

	uint16_t collectionCount = groups.size();
	pen.write((char *)&collectionCount, sizeof(collectionCount));
	for (auto &&grp : groups)
	{
		uint8_t count = grp.imageRefs.size();
		pen.write((char *)&count, sizeof(count));
		for (auto &&ref : grp.imageRefs)
		{
			pen.write((char *)&ref, sizeof(ImageGroup::ImageRef));
		}
	}
}

std::string BlockNameGetCleanName(const char *str)
{
	std::string ret;
	for (int i = 0; i < 8; ++i)
	{
		if (str[i] == ' ') break;
		ret += str[i];
	}
	return ret;
}

void BlockNamePutPaddedName(char *dst, const std::string &name)
{
	memset(dst, ' ', 8);
	memcpy(dst, name.c_str(), XY_MIN(8, name.size()));
}
