#include "EventBinaryDat.h"
#include <fstream>
#include <cstring>
#include <cassert>

std::vector<ActorBlock> EventBinaryDat::Parse(const std::string& path)
{
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	if (!file.is_open())
		return {};

	std::streamsize size = file.tellg();
	file.seekg(0);

	std::vector<uint8_t> data(static_cast<size_t>(size));
	if (!file.read(reinterpret_cast<char*>(data.data()), size))
		return {};

	return ParseBytes(data);
}

std::vector<ActorBlock> EventBinaryDat::ParseBytes(const std::vector<uint8_t>& data)
{
	std::vector<ActorBlock> result;
	size_t offset = 0;

	if (data.size() < 8)
		return result;

	uint32_t block_count;
	memcpy(&block_count, data.data() + offset, 4);
	offset += 4;

	if (block_count == 0 || block_count > 1024)
		return result;

	// Read block sizes
	if (offset + block_count * 4 > data.size())
		return result;
	std::vector<uint32_t> block_sizes(block_count);
	memcpy(block_sizes.data(), data.data() + offset, block_count * 4);
	offset += block_count * 4;

	for (uint32_t b = 0; b < block_count; ++b)
	{
		if (offset + 12 > data.size())
			break;

		ActorBlock block;

		// actor_number
		memcpy(&block.actor_number, data.data() + offset, 4);
		offset += 4;

		// tag_count
		uint32_t tag_count;
		memcpy(&tag_count, data.data() + offset, 4);
		offset += 4;

		if (tag_count > 4096)
			break;

		// tag_offsets (uint16 array)
		std::vector<uint16_t> tag_offsets(tag_count);
		if (offset + tag_count * 2 > data.size())
			break;
		for (uint32_t i = 0; i < tag_count; ++i)
		{
			uint16_t val;
			memcpy(&val, data.data() + offset, 2);
			tag_offsets[i] = val;
			offset += 2;
		}

		// event_exec_nums (uint16 array)
		std::vector<uint16_t> event_exec_nums(tag_count);
		if (offset + tag_count * 2 > data.size())
			break;
		for (uint32_t i = 0; i < tag_count; ++i)
		{
			uint16_t val;
			memcpy(&val, data.data() + offset, 2);
			event_exec_nums[i] = val;
			offset += 2;
		}

		// imed_count
		uint32_t imed_count;
		if (offset + 4 > data.size())
			break;
		memcpy(&imed_count, data.data() + offset, 4);
		offset += 4;

		// imed_data
		block.imed_data.resize(imed_count);
		if (imed_count > 0)
		{
			if (offset + imed_count * 4 > data.size())
				break;
			memcpy(block.imed_data.data(), data.data() + offset, imed_count * 4);
			offset += imed_count * 4;
		}

		// event_data_size
		uint32_t event_data_size;
		if (offset + 4 > data.size())
			break;
		memcpy(&event_data_size, data.data() + offset, 4);
		offset += 4;

		// event_data
		if (offset + event_data_size > data.size())
			break;

		block.event_data.resize(event_data_size);
		if (event_data_size > 0)
			memcpy(block.event_data.data(), data.data() + offset, event_data_size);
		offset += event_data_size;
		// Align to 4-byte boundary (matching Python's Aligned(4, ...))
		offset = (offset + 3) & ~3u;

		// Build events from tag_offsets and event_exec_nums
		for (uint32_t i = 0; i < tag_count; ++i)
		{
			uint32_t byte_offset = tag_offsets[i];
			if (byte_offset >= event_data_size)
				continue;

			EventEntry entry;
			entry.event_id = event_exec_nums[i];
			entry.array_index = static_cast<uint16_t>(i);
			entry.byte_offset = byte_offset;

			// Size until next event's offset or end of data
			uint32_t next_offset = event_data_size;
			if (i + 1 < tag_count && tag_offsets[i + 1] > byte_offset && tag_offsets[i + 1] <= event_data_size)
				next_offset = tag_offsets[i + 1];

			entry.byte_size = next_offset - byte_offset;

			// Use full event_data as bytecode mirror (VM can jump anywhere)
			entry.bytecode = block.event_data;

			block.events.push_back(entry);
		}

		result.push_back(block);
	}

	return result;
}
