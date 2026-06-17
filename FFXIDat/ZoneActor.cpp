#include "ZoneActor.h"
#include <fstream>
#include <cstring>
#include <algorithm>

bool ZoneActor::Load(const std::string& path)
{
	std::ifstream fs(path, std::ios::binary | std::ios::ate);
	if (!fs)
		return false;

	std::streamsize total = fs.tellg();
	fs.seekg(0, std::ios::beg);

	std::vector<uint8_t> buf(static_cast<size_t>(total));
	if (!fs.read(reinterpret_cast<char*>(buf.data()), total))
		return false;

	entries_.clear();

	const uint8_t* rd = buf.data();
	const uint8_t* const finish = buf.data() + buf.size();

	while (rd + RECORD_SIZE <= finish)
	{
		const uint8_t* name_start = rd;

		size_t name_len = 0;
		while (name_len < NAME_BYTES && rd[name_len] != 0)
			++name_len;

		std::string name(reinterpret_cast<const char*>(name_start), name_len);

		uint32_t actor_id;
		std::memcpy(&actor_id, rd + NAME_BYTES, sizeof(actor_id));

		if (actor_id != 0)
		{
			size_t front = 0;
			while (front < name.size() && (name[front] == ' ' || name[front] == '\t'))
				++front;

			size_t back = name.size();
			while (back > front && (name[back - 1] == ' ' || name[back - 1] == '\t'))
				--back;

			ActorEntry entry;
			entry.actor_id = actor_id;
			if (back > front)
				entry.name.assign(name, front, back - front);

			entries_.push_back(std::move(entry));
		}

		rd += RECORD_SIZE;
	}

	return true;
}

std::unordered_map<uint32_t, std::string> ZoneActor::GetIdToNameMap() const
{
	std::unordered_map<uint32_t, std::string> result;
	result.reserve(entries_.size());
	for (const auto& e : entries_)
		result.emplace(e.actor_id, e.name);
	return result;
}

bool ZoneActor::SetName(uint32_t actor_id, const std::string& name)
{
	for (auto& e : entries_)
	{
		if (e.actor_id == actor_id)
		{
			e.name = name;
			return true;
		}
	}
	return false;
}

bool ZoneActor::Write(const std::string& path) const
{
	std::ofstream fs(path, std::ios::binary);
	if (!fs)
		return false;

	char field[NAME_BYTES];

	for (const auto& e : entries_)
	{
		std::memset(field, 0, sizeof(field));

		size_t copy_len = (std::min)(e.name.size(), static_cast<size_t>(NAME_BYTES));
		if (copy_len > 0)
			std::memcpy(field, e.name.data(), copy_len);

		fs.write(field, sizeof(field));
		fs.write(reinterpret_cast<const char*>(&e.actor_id), sizeof(e.actor_id));
	}

	return true;
}
