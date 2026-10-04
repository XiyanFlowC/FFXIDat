#pragma once

// Records of Eminence layout v20: the 0xC00 byte records of the ja/en tables
// before the 2026-09 update, still present in the pre-update corpus
// (LocCNTxtOld: ROM/307/15 quest, ROM/307/23 category).
//
// Measured text offsets: quest 32 - i.e. the quest entry already carries the
// whole reward block including uni_reward, one and a half kilobytes before the
// current layout does - and category 568, exactly like v10. The category record
// therefore reuses the v10 layout (the alias is asserted below); the quest
// record is its own type because its prefix is 4 bytes longer than v10's.

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>

#include "RoeFormatV10.h"
#include "RoeFormatV30.h"

namespace roefmt
{
namespace v20
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
	uint32_t uni_reward;
	union {
		char raw[3039];
		Record info_rec;
	} info;
	char terminator; // must be 0xFF
};

#pragma pack(pop)

static_assert(sizeof(QuestEntry) == 0xC00, "v20 quest record size mismatch");
static_assert(offsetof(QuestEntry, info) == 32, "v20 quest text offset mismatch");

// One format per version; the schema axis lives in TextOffset below. The category
// record is the v10 one, the quest record is this version's own (it already
// carries uni_reward).
struct Format
{
	using Schema = roefmt::Schema;
	using QuestEntry = v20::QuestEntry;
	using CategoryEntry = v10::CategoryEntry;
	template <Schema K> using EntryOf = std::conditional_t<K == Schema::QUEST, QuestEntry, CategoryEntry>;

	using Version = slotfile::Version;

	static constexpr std::string_view id = "v20";
	static constexpr std::string_view scope = "ja/en Records of Eminence tables before the 2026-10 update";
	static constexpr Version version = Version::V20;
	
	static constexpr size_t slotSize = sizeof(QuestEntry);
	static constexpr size_t cipherSpan = slotSize;
	static constexpr size_t currencySlots = 1;
	static constexpr size_t blobLengthOffset = 0;
	static constexpr size_t blobDataOffset = 0;
	static constexpr size_t blobCapacity = 0;
	static constexpr size_t textEnd = slotSize - 1;
	static constexpr bool hasUniReward = true;

	static constexpr bool IsCurrencySchema(Schema) { return false; }
	static constexpr size_t TextOffset(Schema s)
	{
		return s == Schema::QUEST ? offsetof(QuestEntry, info) : offsetof(CategoryEntry, info);
	}
	static constexpr size_t TextCapacity(Schema s) { return textEnd - TextOffset(s); }
};

using Quest = detail::SchemaOf<Format, Schema::QUEST>;
using Category = detail::SchemaOf<Format, Schema::CATEGORY>;


static_assert(Quest::slotSize == 0xC00, "v20 quest slot size mismatch");
static_assert(Quest::TextOffset(Schema::QUEST) == 32, "v20 quest text offset mismatch");
static_assert(Quest::TextCapacity(Schema::QUEST) == 3039, "v20 quest text capacity mismatch");
static_assert(Quest::hasUniReward, "v20 quest entry already has uni_reward");
static_assert(slotfile::SlotDatum<QuestDatum, Quest>, "v20 quest datum does not match the container protocol");

} // namespace v20
} // namespace roefmt
