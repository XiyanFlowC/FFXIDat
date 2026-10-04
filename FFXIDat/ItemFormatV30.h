#pragma once

// Item layout v30: the 0x1400 byte slot used by the ja/en tables of the client
// after the 2026-09 update.
//
// This layout owns its structs (Header, the eight Specs, Entry). Constants were
// measured from the live install; the static_asserts below pin the structs to
// them.
//
// The family shares the datum code (itmfmt::DatumBase in ItemDatum.h): the text
// row, the semantic header, the image and the raw slot bytes have the same shape
// in every version of the family, and the offsets come from the layout that
// parses the record. The typed view (Entry) is owned by each version, so a v30
// record is typed as the structs below and nothing else.

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "Record.h"
#include "Image.h"
#include "RecordFormat.h"
#include "SlotFile.h"
#include "ItemSpecType.h"
#include "ItemDatum.h"
#include "CsvFile.h"

namespace itmfmt
{

namespace v30
{

#pragma pack(push, 1)

struct Header
{
	uint32_t id;
	
	// flag set 1
	uint8_t is_wall_decoration : 1; // can be hung on wall
	uint8_t is_gm_item : 1;
	uint8_t is_in_mystery_box : 1; // can be yielded by mystery box
	uint8_t ukn_flg1 : 1;
	uint8_t is_alt : 1; // can be sent to another character hold by same account
	uint8_t is_inscribable : 1; // can be inscribed
	uint8_t is_not_listable : 1; // cannot be listed in auction house
	uint8_t is_scroll : 1;

	// flag set 2
	uint8_t is_linkshell : 1;
	uint8_t is_usable : 1; // can be used
	uint8_t is_npc_tradeable : 1; // can be traded with NPCs
	uint8_t is_equipment : 1; // can be equipped
	uint8_t is_unsellable : 1; // cannot be sold to vendor
	uint8_t is_unmailable : 1; // cannot be mailed
	uint8_t is_ex : 1; // cannot be traded
	uint8_t is_rare : 1;

	uint16_t extended_flags; // a new flag observed after the 2026-09 update, not present in v10/v20, uknown for now.
	uint16_t stack_size;
	uint16_t item_type;
	uint16_t resource_id;
	uint16_t valid_targets;
};


struct JobFlags
{
	uint32_t rsv1 : 1; // Bit 0
	uint32_t war : 1;  // Bit 1
	uint32_t mnk : 1;  // Bit 2
	uint32_t whm : 1;  // Bit 3
	uint32_t blm : 1;  // Bit 4
	uint32_t rdm : 1;  // Bit 5
	uint32_t thf : 1;  // Bit 6
	uint32_t pld : 1;  // Bit 7

	uint32_t drk : 1;  // Bit 8
	uint32_t bst : 1;  // Bit 9
	uint32_t brd : 1;  // Bit 10
	uint32_t rng : 1;  // Bit 11
	uint32_t sam : 1;  // Bit 12
	uint32_t nin : 1;  // Bit 13
	uint32_t drg : 1;  // Bit 14
	uint32_t smn : 1;  // Bit 15

	uint32_t blu : 1;  // Bit 16
	uint32_t cor : 1;  // Bit 17
	uint32_t pup : 1;  // Bit 18
	uint32_t dnc : 1;  // Bit 19
	uint32_t sch : 1;  // Bit 20
	uint32_t geo : 1;  // Bit 21
	uint32_t run : 1;  // Bit 22
	uint32_t mon : 1;  // Bit 23
	uint32_t rsv2 : 8; // Remaining bits
};

struct EquipSlots
{
	uint16_t main_hand : 1;
	uint16_t sub_hand : 1;
	uint16_t ranged : 1;
	uint16_t ammo : 1;
	uint16_t head : 1;
	uint16_t body : 1;
	uint16_t hands : 1;
	uint16_t legs : 1;

	uint16_t feet : 1;
	uint16_t neck : 1;
	uint16_t waist : 1;
	uint16_t left_ear : 1;
	uint16_t right_ear : 1;
	uint16_t left_ring : 1;
	uint16_t right_ring : 1;
	uint16_t back : 1;
};

// Race applicability. It is 16 bits wide in every version: the live files use
// the nine bits below and never set anything above bit 8 (6656 weapon and 6144
// armour records checked). v30 carries two further bytes right after it, zero in
// every observed record, so they are modelled as a field of their own. RaceFlags
struct RaceFlags
{
	uint16_t None : 1;
	uint16_t HumeMale : 1;
	uint16_t HumeFemale : 1;
	uint16_t ElvaanMale : 1;
	uint16_t ElvaanFemale : 1;
	uint16_t TaruMale : 1;
	uint16_t TaruFemale : 1;
	uint16_t Mithra : 1;
	uint16_t Galka : 1;
	uint16_t Rsv : 7;
};

struct ArmourSpec
{
	int16_t level;
	EquipSlots equip_slots;
	RaceFlags equip_races;
	uint16_t ukn_after_races;
	JobFlags equip_jobs;
	uint16_t slvl;
	uint16_t shield_size;
	uint8_t max_charges;
	uint8_t cast_factor; // 使用后到效果生效的延迟时间系数，1/4秒，动画硬直时间
	uint16_t use_time;
	uint16_t reuse_time;
	uint16_t ukn1;
	uint16_t related_item_id;
	uint16_t ilvl;
	uint16_t ukn3;
	uint16_t ukn4;
	Record info_rec;
};

struct PuppetSlots
{
	uint32_t head : 1;
	uint32_t body : 1;
	uint32_t attachment : 1;
	uint32_t rsv : 29;
};

struct PuppetSpec
{
	PuppetSlots equip_slots;
	uint8_t fire : 4;
	uint8_t ice : 4;
	uint8_t air : 4;
	uint8_t earth : 4;
	uint8_t thunder : 4;
	uint8_t water : 4;
	uint8_t light : 4;
	uint8_t dark : 4;
	uint32_t ukn;
	Record info_rec;
};

struct NormalSpec
{
	int16_t element;
	int16_t storage;
	uint32_t related_item_id;
	int16_t ukn4;
	int16_t ukn5;
	Record info_rec;
};

struct UsableSpec
{
	int16_t cast_factor;
	int32_t ukn1;
	int32_t ukn2;
	uint16_t ukn3;
	Record info_rec;
};

struct WeaponSpec
{
	uint16_t level;
	EquipSlots equip_slots;
	RaceFlags races;
	uint16_t ukn_after_races;
	JobFlags jobs;
	uint16_t slvl;
	uint16_t ukn2;
	uint16_t dmg;
	uint16_t delay;
	uint16_t dps;
	uint8_t skill;
	uint8_t ukn12;
	uint16_t ukn7;
	uint16_t ukn9;
	uint8_t max_charges;
	uint8_t cast_factor; // 使用后到效果生效的延迟时间系数，1/4秒，动画硬直时间
	uint16_t use_time;
	uint16_t reuse_time;
	uint16_t ukn20;
	uint16_t related_item_id;
	uint16_t ilvl;
	uint16_t ukn22;
	uint16_t ukn23;
	Record info_rec;
};

enum class SkillType : uint8_t
{
	None = 0,
	HandToHand = 1,
	Dagger = 2,
	Sword = 3,
	GreatSword = 4,
	Axe = 5,
	GreatAxe = 6,
	Scythe = 7,
	Polearm = 8,
	Katana = 9,
	GreatKatana = 10,
	Club = 11,
	Staff = 12,
	Weapon12 = 13,
	Weapon11 = 14,
	Weapon10 = 15,
	Weapon9 = 16,
	Weapon8 = 17,
	Weapon7 = 18,
	Weapon6 = 19,
	Weapon5 = 20,
	Weapon4 = 21,
	AutomatonMelee = 22,
	AutomatonArchery = 23,
	AutomatonMagic = 24,
	Archery = 25,
	Marksmanship = 26,
	Throwing = 27,
	Guard = 28,
	Evasion = 29,
	Shield = 30,
	Parrying = 31,
	DivineMagic = 32,
	HealingMagic = 33,
	EnhancingMagic = 34,
	EnfeeblingMagic = 35,
	ElementalMagic = 36,
	DarkMagic = 37,
	SummoningMagic = 38,
	Ninjutsu = 39,
	Singing = 40,
	StringedInstrument = 41,
	WindInstrument = 42,
	BlueMagic = 43,
	Geomancy = 44,
	Handbell = 45,
	Magic2 = 46,
	Magic1 = 47,
	Fishing = 48,
	Woodworking = 49,
	Smithing = 50,
	Goldsmithing = 51,
	Clothcraft = 52,
	Leatherworking = 53,
	Bonecraft = 54,
	Alchemy = 55,
	Cooking = 56,
	Synergy = 57,
	Synthesis6 = 58,
	Synthesis5 = 59,
	Synthesis4 = 60,
	Synthesis3 = 61,
	Synthesis2 = 62,
	Synthesis1 = 63
};

struct SlipSpec
{
	uint8_t ukn[68];
	Record info_rec;
};

struct InstinctSpec
{
	uint16_t ukn[14];
	Record info_rec;
};

struct CurrencySpec
{
	uint32_t ukn;
	Record info_rec;
};

union SpecData
{
	char raw[624];
	ArmourSpec armour;
	NormalSpec normal;
	UsableSpec usable;
	PuppetSpec puppet;
	WeaponSpec weapon;
	SlipSpec slip;
	CurrencySpec currency;
	InstinctSpec instinct;
};

struct Entry
{
	Header header;
	SpecData spec;
	uint32_t image_length;
	char image_data[4475];
	uint8_t end_marker;
};

#pragma pack(pop)

static_assert(sizeof(Entry) == 0x1400);
static_assert(sizeof(Header) == 16);
static_assert(offsetof(Entry, image_length) == 640);
static_assert(sizeof(Header) + offsetof(NormalSpec, info_rec) == 28);
static_assert(sizeof(Header) + offsetof(UsableSpec, info_rec) == 28);
static_assert(sizeof(Header) + offsetof(WeaponSpec, info_rec) == 60);
static_assert(sizeof(Header) + offsetof(ArmourSpec, info_rec) == 48);
static_assert(sizeof(Header) + offsetof(PuppetSpec, info_rec) == 28);
static_assert(sizeof(Header) + offsetof(SlipSpec, info_rec) == 84);
static_assert(sizeof(Header) + offsetof(CurrencySpec, info_rec) == 20);
static_assert(sizeof(Header) + offsetof(InstinctSpec, info_rec) == 44);

static_assert(offsetof(Entry, image_data) == 644, "v30 blob data offset mismatch");
static_assert(offsetof(Entry, header.stack_size) == 8, "v30 stack size offset mismatch");
static_assert(offsetof(Entry, header.item_type) == 10, "v30 item type offset mismatch");
static_assert(offsetof(Entry, header.resource_id) == 12, "v30 resource id offset mismatch");
static_assert(offsetof(Entry, header.valid_targets) == 14, "v30 valid targets offset mismatch");
static_assert(sizeof(RaceFlags) == 2, "v30 race applicability must stay 16 bits");
static_assert(offsetof(WeaponSpec, jobs) == 8, "v30 weapon jobs offset mismatch");
static_assert(offsetof(ArmourSpec, equip_jobs) == 8, "v30 armour jobs offset mismatch");

} // namespace v30

// --------------------------------------------------------------------- datum

namespace v30
{

// One v30 record: the typed view is this version's Entry, so the spec area of a
// v30 slot is readable as the v30 spec structs.
class Datum : public DatumBase<Entry> { public: using Entry = v30::Entry; };

struct Format
{
	using Entry = v30::Entry;
	// Every version of this family owns its datum (ItemDatum.h carries the shared
	// code); this is the datum of the newest known layout.
	using Datum = v30::Datum;
	using Schema = SpecType;

	static constexpr Version version = Version::V30;
	static constexpr std::string_view id = "v30";
	static constexpr std::string_view scope = "ja/en item tables, 2026-09 update onwards";

	static constexpr size_t slotSize = sizeof(Entry);
	static constexpr size_t currencySlots = 16;
	static constexpr size_t cipherSpan = sizeof(Entry);   // whole slot is rotated
	static constexpr size_t blobLengthOffset = offsetof(Entry, image_length);
	static constexpr size_t blobDataOffset = offsetof(Entry, image_data);
	static constexpr size_t blobCapacity = sizeof(Entry::image_data);
	static constexpr size_t textEnd = offsetof(Entry, image_length);
	static constexpr SpecType DEFAULT_SCHEMA = SpecType::NORMAL;

	// Semantic header field -> slot offset. The first six bytes are shared by
	// every version, these are the offsets of the remaining ones.
	static constexpr bool hasExtendedFlags = true;
	static constexpr size_t headerExtendedFlagsOffset = offsetof(Entry, header.extended_flags);
	static constexpr size_t headerStackSizeOffset = offsetof(Entry, header.stack_size);
	static constexpr size_t headerItemTypeOffset = offsetof(Entry, header.item_type);
	static constexpr size_t headerResourceIdOffset = offsetof(Entry, header.resource_id);
	static constexpr size_t headerValidTargetsOffset = offsetof(Entry, header.valid_targets);

	static constexpr bool IsCurrencySchema(SpecType schema) { return schema == SpecType::CURRENCY; }

	// text record offsets, verified against the live files
	static constexpr size_t TextOffset(SpecType schema)
	{
		switch (schema)
		{
		case SpecType::NORMAL: return offsetof(Entry, spec.normal.info_rec);
		case SpecType::USABLE: return offsetof(Entry, spec.usable.info_rec);
		case SpecType::WEAPON: return offsetof(Entry, spec.weapon.info_rec);
		case SpecType::ARMOUR: return offsetof(Entry, spec.armour.info_rec);
		case SpecType::PUPPET: return offsetof(Entry, spec.puppet.info_rec);
		case SpecType::SLIP: return offsetof(Entry, spec.slip.info_rec);
		case SpecType::CURRENCY: return offsetof(Entry, spec.currency.info_rec);
		case SpecType::INSTINCT: return offsetof(Entry, spec.instinct.info_rec);
		}
		return offsetof(Entry, spec.normal.info_rec);
	}

	static constexpr size_t TextCapacity(SpecType schema) { return textEnd - TextOffset(schema); }

	// CSV view. Defined in ItemFormatsCsv.cpp: the column set is per family and
	// version, and neither the shared container nor the adapter knows about it.
	static CsvContext GetCsvContext(std::span<const Datum> records);
	static void WriteCsvHeader(CsvFile& csv, const CsvContext& context);
	static void WriteCsvRow(CsvFile &csv, const CsvContext &context, const Datum &datum);
};

static_assert(Format::TextOffset(SpecType::NORMAL) == 28, "normal text offset mismatch");
static_assert(Format::TextOffset(SpecType::USABLE) == 28, "usable text offset mismatch");
static_assert(Format::TextOffset(SpecType::WEAPON) == 60, "weapon text offset mismatch");
static_assert(Format::TextOffset(SpecType::ARMOUR) == 48, "armour text offset mismatch");
static_assert(Format::TextOffset(SpecType::PUPPET) == 28, "puppet text offset mismatch");
static_assert(Format::TextOffset(SpecType::SLIP) == 84, "slip text offset mismatch");
static_assert(Format::TextOffset(SpecType::CURRENCY) == 20, "currency text offset mismatch");
static_assert(Format::TextOffset(SpecType::INSTINCT) == 44, "instinct text offset mismatch");

static_assert(slotfile::SlotDatum<Datum, Format>, "v30 datum does not match the container protocol");

} // namespace v30
} // namespace itmfmt
