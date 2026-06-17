#include "EntityDat.h"
#include <fstream>

static constexpr uint32_t kNameFieldSize = 0x1C;
static constexpr uint32_t kEntrySize = kNameFieldSize + 4;

std::vector<EntityEntry> EntityDat::Parse(const std::string& path)
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

std::vector<EntityEntry> EntityDat::ParseBytes(const std::vector<uint8_t>& data)
{
	std::vector<EntityEntry> result;
	size_t offset = 0;

	while (offset + kEntrySize <= data.size())
	{
		// 28-byte name field (null-terminated, latin-1)
		std::string name;
		name.reserve(kNameFieldSize);
		for (size_t i = 0; i < kNameFieldSize; ++i)
		{
			char c = static_cast<char>(data[offset + i]);
			if (c == '\0')
				break;
			name += c;
		}

		uint32_t entity_id;
		memcpy(&entity_id, data.data() + offset + kNameFieldSize, 4);

		if (entity_id != 0 && IsValidEntityId(entity_id))
		{
			// Trim whitespace
			size_t start = 0;
			while (start < name.size() && (name[start] == ' ' || name[start] == '\t'))
				++start;
			size_t end = name.size();
			while (end > start && (name[end - 1] == ' ' || name[end - 1] == '\t'))
				--end;

			EntityEntry entry;
			entry.entity_id = entity_id;
			if (end > start)
				entry.name = name.substr(start, end - start);
			result.push_back(entry);
		}

		offset += kEntrySize;
	}

	return result;
}

bool EntityDat::IsValidEntityId(uint32_t entity_id)
{
	uint32_t prefix = entity_id & 0xFFF00000;
	return prefix == 0x01000000 || prefix == 0x01100000 || prefix == 0x01300000;
}
