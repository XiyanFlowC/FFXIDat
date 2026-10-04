#include "ItemData.h"

#include <stdexcept>
#include <utility>

#include "Image.h"
#include "SlotFileCsv.h"

namespace
{
// Every operation of this family is the same container call on a different
// layout, and the layout of a store follows from the alternative it holds: the
// store keeps one vector per version and exactly one of them is in use.
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
	WithLayout(version, [&](auto file) {
		file.encryptionSuppression = encryptionSuppression;
		file.Read(path, defaultSpecType);
		// The layout decides which alternative of the store the records land in:
		// they keep the typed view of the layout that parsed them.
		store = std::move(file.data);
	});

	layoutVersion = version;
}

void ItemData::Write(std::wstring path)
{
	switch (store.index())
	{
	case 0:
	{
		auto file = slotfile::SlotFile<itmfmt::v10::Format>{};
		file.encryptionSuppression = encryptionSuppression;
		file.Write(path, std::get<std::vector<itmfmt::v10::Datum>>(store));
		return;
	}
	case 1:
	{
		auto file = slotfile::SlotFile<itmfmt::v20::Format>{};
		file.encryptionSuppression = encryptionSuppression;
		file.Write(path, std::get<std::vector<itmfmt::v20::Datum>>(store));
		return;
	}
	case 2:
	{
		auto file = slotfile::SlotFile<itmfmt::v30::Format>{};
		file.encryptionSuppression = encryptionSuppression;
		file.Write(path, std::get<std::vector<itmfmt::v30::Datum>>(store));
		return;
	}
	}
	throw std::runtime_error("Unknown item record version");
}

void ItemData::ToICsv(const std::wstring &path) const
{
	switch (store.index())
	{
	case 0: slotfile::WriteICsv<itmfmt::v10::Format>(path, std::get<std::vector<itmfmt::v10::Datum>>(store)); return;
	case 1: slotfile::WriteICsv<itmfmt::v20::Format>(path, std::get<std::vector<itmfmt::v20::Datum>>(store)); return;
	case 2: slotfile::WriteICsv<itmfmt::v30::Format>(path, std::get<std::vector<itmfmt::v30::Datum>>(store)); return;
	}
	throw std::runtime_error("Unknown item record version");
}
