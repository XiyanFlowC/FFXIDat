#pragma once

// The item family: version aliases, routing and the version aware facade.
//
// A layout is a type rather than a value, so "which version does this file have"
// is answered by the annotation on the data side (the "_o" type suffix), and the
// facades dispatch on a Version. This header holds the routing table and the two
// type erasing views the call sites use: AnyDatum (one record of whatever
// version the store holds) and AnyData (the store itself, one vector per
// version).

#include <cstddef>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "ItemFormatV10.h"
#include "ItemFormatV20.h"
#include "ItemFormatV30.h"

namespace itmfmt
{

// The newest known layout of this family (an alias, not a format name). Not every
// language is on it: the de/fr tables are still v10.
namespace Current = v30;

// Layout of a record version label. The labels are opaque: they are compared for
// equality only, never as an order.
inline constexpr Version CURRENT_VERSION = Version::V30;
inline constexpr Version OLDEST_VERSION = Version::V10;

// The routing table of this family: which data side annotation selects which
// version, with the constants a sniffer needs to recognise the file.
//
// v20 deliberately has no row: the live client has no file with that layout any
// more, so nothing on the data side can point at it. The pre-update corpus
// (LocCNTxtOld) is read by asking for Version::V20 explicitly.
struct VersionRoute
{
	Version version;
	std::string_view suffix;      // data side type suffix ("" = newest known)
	std::string_view id;          // Format::id of the layout
	size_t slotSize;
	size_t currencySlots;
};

inline constexpr VersionRoute VERSION_ROUTES[] = {
	{ CURRENT_VERSION, "", v30::Format::id, v30::Format::slotSize, v30::Format::currencySlots },
	{ Version::V30, "_v3", v30::Format::id, v30::Format::slotSize, v30::Format::currencySlots },
	{ Version::V20, "_v2", v20::Format::id, v20::Format::slotSize, v20::Format::currencySlots },
	{ Version::V10, "_v1", v10::Format::id, v10::Format::slotSize, v10::Format::currencySlots },
	{ Version::V10, "_o", v10::Format::id, v10::Format::slotSize, v10::Format::currencySlots },
};

// --------------------------------------------------------------- typed views

// The store of one item file: one vector of records per version, of which
// exactly one is in use. The active alternative is the one the file was read
// with, so `data` still holds records whose typed view is the Entry of the
// layout that parsed them.
using DatumStore = std::variant<std::vector<v10::Datum>, std::vector<v20::Datum>,
	std::vector<v30::Datum>>;

// A pointer to the record of the active alternative. A pointer rather than a
// reference so that the handle stays copyable and assignable, which is what
// `auto datum = store[i];` needs.
using DatumPointer = std::variant<v10::Datum *, v20::Datum *, v30::Datum *>;

inline DatumPointer DatumPointerAt(DatumStore &store, size_t index)
{
	return std::visit([index](auto &records) -> DatumPointer { return &records[index]; }, store);
}

inline DatumPointer DatumPointerAt(const DatumStore &store, size_t index)
{
	return std::visit([index](const auto &records) -> DatumPointer {
		return const_cast<std::remove_const_t<std::remove_reference_t<decltype(records[0])>> *>(&records[index]);
	}, store);
}

// One item record of whatever version a store holds: the version aware handle
// the call sites use. Every text, header and image accessor is forwarded to the
// record of the active version, so the same code serves v10, v20 and v30; a
// consumer that needs the spec area selects the version with `visit` and gets
// the typed view of that version.
class AnyDatum
{
public:
	explicit AnyDatum(DatumPointer datum) : active(datum) {}

	uint32_t id() const
	{
		return std::visit([](auto *datum) -> uint32_t { return datum->id; }, active);
	}

	const Image &image() const
	{
		return std::visit([](auto *datum) -> const Image & { return datum->image; }, active);
	}

	SpecType schema() const
	{
		return std::visit([](auto *datum) { return datum->schema; }, active);
	}

	// The tail of the slot: the length of the image blob and the end marker. Both
	// have the same name and the same place in every version.
	uint32_t image_length() const
	{
		return std::visit([](auto *datum) { return datum->originalEntry.image_length; }, active);
	}

	uint8_t end_marker() const
	{
		return std::visit([](auto *datum) { return static_cast<uint8_t>(datum->originalEntry.end_marker); }, active);
	}

	// The image blob as the record stores it, valid for `image_length()` bytes.
	const char *image_data() const
	{
		return std::visit([](auto *datum) -> const char * { return datum->originalEntry.image_data; }, active);
	}

	std::string_view layoutId() const
	{
		return std::visit([](auto *datum) { return datum->layoutId; }, active);
	}

	const std::vector<char> &raw() const
	{
		return std::visit([](auto *datum) -> const std::vector<char> & { return datum->raw(); }, active);
	}

	Row &row() { return std::visit([](auto *datum) -> Row & { return datum->row(); }, active); }
	const Row &row() const { return std::visit([](auto *datum) -> const Row & { return datum->row(); }, active); }

	bool hasOriginalRow() const
	{
		return std::visit([](auto *datum) { return datum->hasOriginalRow; }, active);
	}

	RecordFormat recordFormat() const
	{
		return std::visit([](auto *datum) { return datum->recordFormat; }, active);
	}

	uint16_t &stack_size() { return std::visit([](auto *datum) -> uint16_t & { return datum->stack_size(); }, active); }
	const uint16_t &stack_size() const { return std::visit([](auto *datum) -> const uint16_t & { return datum->stack_size(); }, active); }
	uint16_t &item_type() { return std::visit([](auto *datum) -> uint16_t & { return datum->item_type(); }, active); }
	const uint16_t &item_type() const { return std::visit([](auto *datum) -> const uint16_t & { return datum->item_type(); }, active); }
	uint16_t &resource_id() { return std::visit([](auto *datum) -> uint16_t & { return datum->resource_id(); }, active); }
	const uint16_t &resource_id() const { return std::visit([](auto *datum) -> const uint16_t & { return datum->resource_id(); }, active); }
	uint16_t &valid_targets() { return std::visit([](auto *datum) -> uint16_t & { return datum->valid_targets(); }, active); }
	const uint16_t &valid_targets() const { return std::visit([](auto *datum) -> const uint16_t & { return datum->valid_targets(); }, active); }

	std::u8string name() const { return std::visit([](auto *datum) { return datum->name(); }, active); }
	bool setName(const std::u8string &value) { return std::visit([&](auto *datum) { return datum->setName(value); }, active); }
	std::u8string name_sg() const { return std::visit([](auto *datum) { return datum->name_sg(); }, active); }
	bool setName_sg(const std::u8string &value) { return std::visit([&](auto *datum) { return datum->setName_sg(value); }, active); }
	std::u8string name_pl() const { return std::visit([](auto *datum) { return datum->name_pl(); }, active); }
	bool setName_pl(const std::u8string &value) { return std::visit([&](auto *datum) { return datum->setName_pl(value); }, active); }
	std::u8string description() const { return std::visit([](auto *datum) { return datum->description(); }, active); }
	bool setDescription(const std::u8string &value) { return std::visit([&](auto *datum) { return datum->setDescription(value); }, active); }
	int logFlag() const { return std::visit([](auto *datum) { return datum->logFlag(); }, active); }
	bool setLogFlag(int value) { return std::visit([&](auto *datum) { return datum->setLogFlag(value); }, active); }

	size_t cellCount() const { return std::visit([](auto *datum) { return datum->cellCount(); }, active); }
	const Cell &cell(size_t index) const { return std::visit([index](auto *datum) -> const Cell & { return datum->cell(index); }, active); }
	void detectFormat() { std::visit([](auto *datum) { datum->detectFormat(); }, active); }

	// Run F on the record of the active version; the argument is a reference to
	// that version's datum, so the record and its spec area are typed. The const
	// overload exists for callers that hold the handle by const reference; the
	// record it reaches is the same one (the handle is a pointer variant).
	template <class F>
	auto visit(F &&f)
	{
		return std::visit([&](auto *datum) { return f(*datum); }, active);
	}

	template <class F>
	auto visit(F &&f) const
	{
		return std::visit([&](auto *datum) { return f(*datum); }, active);
	}

private:
	DatumPointer active;
};

// The store behind a version aware range of records. `ItemData::data` is a
// member of this type rather than a function, so `items.data.size()`,
// `items.data[0]` and `for (auto datum : items.data)` keep working; the records
// themselves are the ones the active version owns.
class AnyData
{
public:
	explicit AnyData(DatumStore &store) : store(store) {}

	size_t size() const { return std::visit([](const auto &records) { return records.size(); }, store); }
	bool empty() const { return size() == 0; }

	AnyDatum operator[](size_t index) { return AnyDatum(DatumPointerAt(store, index)); }
	AnyDatum operator[](size_t index) const { return AnyDatum(DatumPointerAt(store, index)); }
	AnyDatum front() { return AnyDatum(DatumPointerAt(store, 0)); }
	AnyDatum front() const { return AnyDatum(DatumPointerAt(store, 0)); }

	// Iteration is over the records of the active version. The iterator hands out
	// the version aware handle, so a range-for keeps working unchanged; a loop
	// that needs the typed view of one version uses `visit` on each handle.
	class Iterator
	{
	public:
		Iterator(DatumStore &store, size_t index) : store(store), index(index) {}

		AnyDatum operator*() const { return AnyDatum(DatumPointerAt(store, index)); }

		// The handle is a view of the record, so the iterator keeps the one it
		// hands out and points at it.
		AnyDatum *operator->() { current = operator*(); return &current; }

		Iterator &operator++() { ++index; return *this; }
		bool operator!=(const Iterator &other) const { return index != other.index; }
		bool operator==(const Iterator &other) const { return index == other.index; }

	private:
		DatumStore &store;
		size_t index = 0;
		AnyDatum current{ DatumPointer{} };
	};

	Iterator begin() { return Iterator(store, 0); }
	Iterator end() { return Iterator(store, size()); }

	// A const store is iterated the same way: the handle is a view of the records
	// it points at, exactly like a const reference to a vector element (the
	// records themselves stay the ones the active version owns).
	Iterator begin() const { return Iterator(const_cast<DatumStore &>(store), 0); }
	Iterator end() const { return Iterator(const_cast<DatumStore &>(store), size()); }

	void push_back(v10::Datum datum) { std::get<std::vector<v10::Datum>>(store).push_back(std::move(datum)); }
	void push_back(v20::Datum datum) { std::get<std::vector<v20::Datum>>(store).push_back(std::move(datum)); }
	void push_back(v30::Datum datum) { std::get<std::vector<v30::Datum>>(store).push_back(std::move(datum)); }

private:
	DatumStore &store;
};

} // namespace itmfmt
