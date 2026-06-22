#include "ZoneEventImage.h"
#include <fstream>
#include <cstring>
#include <algorithm>

namespace {

inline bool CanRead(const uint8_t* pos, const uint8_t* end, size_t bytes)
{
	return (pos + bytes) <= end;
}

inline uint32_t ExtractU32(const uint8_t*& p)
{
	uint32_t val;
	std::memcpy(&val, p, sizeof(val));
	p += sizeof(val);
	return val;
}

inline uint16_t ExtractU16(const uint8_t*& p)
{
	uint16_t val;
	std::memcpy(&val, p, sizeof(val));
	p += sizeof(val);
	return val;
}

constexpr uint32_t MAX_BLOCKS = 1024;
constexpr uint32_t MAX_EVENTS = 4096;

std::string HashBytesHex(const uint8_t* data, size_t size)
{
	uint64_t h = 14695981039346656037ull;
	for (size_t i = 0; i < size; ++i)
	{
		h ^= data[i];
		h *= 1099511628211ull;
	}
	char buf[32];
	snprintf(buf, sizeof(buf), "%016llX", (unsigned long long)h);
	return buf;
}

} // namespace

bool ZoneEventImage::Load(const std::string& path)
{
	std::ifstream fs(path, std::ios::binary | std::ios::ate);
	if (!fs)
		return false;

	std::streamsize total = fs.tellg();
	fs.seekg(0, std::ios::beg);

	std::vector<uint8_t> buf(static_cast<size_t>(total));
	if (!fs.read(reinterpret_cast<char*>(buf.data()), total))
		return false;

	actors_.clear();

	const uint8_t* rd = buf.data();
	const uint8_t* const finish = buf.data() + buf.size();

	if (!CanRead(rd, finish, 4))
		return false;

	uint32_t block_count = ExtractU32(rd);
	if (block_count == 0 || block_count > MAX_BLOCKS)
		return false;

	if (!CanRead(rd, finish, block_count * 4))
		return false;

	std::vector<uint32_t> block_sizes(block_count);
	for (uint32_t i = 0; i < block_count; ++i)
		block_sizes[i] = ExtractU32(rd);

	for (uint32_t bi = 0; bi < block_count; ++bi)
	{
		if (!CanRead(rd, finish, 8))
			break;

		ActorBlock actor;

		actor.actor_id = ExtractU32(rd);

		uint32_t event_count = ExtractU32(rd);
		if (event_count > MAX_EVENTS)
			break;

		size_t evt_table_bytes = event_count * 2;

		if (!CanRead(rd, finish, evt_table_bytes))
			break;
		std::vector<uint16_t> event_offsets(event_count);
		for (uint32_t i = 0; i < event_count; ++i)
			event_offsets[i] = ExtractU16(rd);

		if (!CanRead(rd, finish, evt_table_bytes))
			break;
		std::vector<uint16_t> event_ids(event_count);
		for (uint32_t i = 0; i < event_count; ++i)
			event_ids[i] = ExtractU16(rd);

		if (!CanRead(rd, finish, 4))
			break;
		uint32_t constant_count = ExtractU32(rd);

		if (!CanRead(rd, finish, constant_count * 4))
			break;
		actor.constants.resize(constant_count);
		for (uint32_t i = 0; i < constant_count; ++i)
			actor.constants[i] = ExtractU32(rd);

		if (!CanRead(rd, finish, 4))
			break;
		uint32_t code_image_size = ExtractU32(rd);

		if (!CanRead(rd, finish, code_image_size))
			break;
      actor.bytecode_hash = HashBytesHex(rd, code_image_size);
		rd += code_image_size;

		ptrdiff_t pos = rd - buf.data();
		ptrdiff_t aligned = (pos + 3) & ~static_cast<ptrdiff_t>(3);
		rd = buf.data() + aligned;

		for (uint32_t i = 0; i < event_count; ++i)
		{
			EventDescriptor desc;
			desc.event_id   = event_ids[i];
			desc.event_index = static_cast<uint16_t>(i);
			actor.events.push_back(desc);
		}

		actors_.push_back(std::move(actor));
	}

	return true;
}
