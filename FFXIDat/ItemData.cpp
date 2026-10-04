#include "ItemData.h"

#include <stdexcept>
#include <utility>

#include "Image.h"
#include "SlotFileCsv.h"

namespace
{
// Every operation of this family is the same container call on a different layout.
template <class Fn>
void WithLayout(itmfmt::Version version, Fn &&fn)
{
	switch (version)
	{
	case itmfmt::Version::V10: fn(slotfile::SlotFile<itmfmt::v10::Format>{}); return;
	case itmfmt::Version::V20: fn(slotfile::SlotFile<itmfmt::v20::Format>{}); return;
	case itmfmt::Version::V30: fn(slotfile::SlotFile<itmfmt::v30::Format>{}); return;
	}
	throw std::runtime_error("Unknown item record version");
}
} // namespace

void ItemData::Read(std::wstring path, ItemSpecType defaultSpecType, itmfmt::Version version)
{
	std::vector<ItemDatum> records;
	WithLayout(version, [&](auto file) {
		file.encryptionSuppression = encryptionSuppression;
		file.Read(path, defaultSpecType);
		records = std::move(file.data);
	});

	data = std::move(records);
	layoutVersion = version;
}

void ItemData::Write(std::wstring path)
{
	WithLayout(layoutVersion, [&](auto file) {
		file.encryptionSuppression = encryptionSuppression;
		file.Write(path, data);
	});
}

void ItemData::ToICsv(const std::wstring &path) const
{
	WithLayout(layoutVersion, [&](auto file) {
		slotfile::WriteICsv<decltype(file)::LayoutType>(path, data);
	});
}
