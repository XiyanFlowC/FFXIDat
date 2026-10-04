#pragma once

// Records of Eminence layout v10: the 0xC00 byte records still used by the de/fr
// tables of the live client. The quest entry has no uni_reward field yet, and
// both schemas are one and a half kilobytes shorter than in v30.

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>

#include "RoeFormatV30.h"

namespace roefmt
{
namespace v10
{

#pragma pack(push, 1)

struct QuestEntry
{
	uint32_t id;
	uint32_t release_date;
	uint32_t repeatable;
	uint32_t target_count;
	uint32_t emi_reward;
	uint32_t exp_reward;
	uint32_t cap_reward;
	union {
		char raw[3043];
		Record info_rec;
	} info;
	char terminator; // must be 0xFF
};

struct CategoryEntry
{
	uint32_t id;
	uint32_t count_of_children;
	struct {
		uint32_t child_id;
		uint32_t quest_flag;
		uint32_t ukn[3];
	} children[28];
	union {
		char raw[2503];
		Record info_rec;
	} info;
	char terminator; // must be 0xFF
};

#pragma pack(pop)

static_assert(sizeof(QuestEntry) == 0xC00, "v10 quest record size mismatch");
static_assert(sizeof(CategoryEntry) == 0xC00, "v10 category record size mismatch");
static_assert(offsetof(QuestEntry, info) == 28, "v10 quest text offset mismatch");
static_assert(offsetof(CategoryEntry, info) == 568, "v10 category text offset mismatch");

// One format per version; the schema axis lives in TextOffset below.
struct Format
{
	using Schema = roefmt::Schema;
	using QuestEntry = v10::QuestEntry;
	using CategoryEntry = v10::CategoryEntry;
	template <Schema K> using EntryOf = std::conditional_t<K == Schema::QUEST, QuestEntry, CategoryEntry>;

	static constexpr std::string_view id = "v10";
	static constexpr std::string_view scope = "de/fr Records of Eminence tables, oldest known layout";
	static constexpr Version version = Version::V10;

	static constexpr size_t slotSize = sizeof(QuestEntry);
	static constexpr size_t cipherSpan = slotSize;
	static constexpr size_t currencySlots = 1;
	static constexpr size_t blobLengthOffset = 0;
	static constexpr size_t blobDataOffset = 0;
	static constexpr size_t blobCapacity = 0;
	static constexpr size_t textEnd = slotSize - 1;
	static constexpr bool hasUniReward = false; // no uni_reward yet in this version

	static constexpr bool IsCurrencySchema(Schema) { return false; }
	static constexpr size_t TextOffset(Schema s)
	{
		return s == Schema::QUEST ? offsetof(QuestEntry, info) : offsetof(CategoryEntry, info);
	}
	static constexpr size_t TextCapacity(Schema s) { return textEnd - TextOffset(s); }
};

using Quest = detail::SchemaOf<Format, Schema::QUEST>;
using Category = detail::SchemaOf<Format, Schema::CATEGORY>;


static_assert(Quest::slotSize == 0xC00, "v10 quest slot size mismatch");
static_assert(Category::slotSize == 0xC00, "v10 category slot size mismatch");
static_assert(Quest::TextOffset(Schema::QUEST) == 28, "v10 quest text offset mismatch");
static_assert(Category::TextOffset(Schema::CATEGORY) == 568, "v10 category text offset mismatch");
static_assert(Quest::TextCapacity(Schema::QUEST) == 3043, "v10 quest text capacity mismatch");
static_assert(Category::TextCapacity(Schema::CATEGORY) == 2503, "v10 category text capacity mismatch");
static_assert(slotfile::SlotDatum<QuestDatum, Quest>, "v10 quest datum does not match the container protocol");
static_assert(slotfile::SlotDatum<CategoryDatum, Category>, "v10 category datum does not match the container protocol");

} // namespace v10

} // namespace roefmt
