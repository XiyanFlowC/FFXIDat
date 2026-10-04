#pragma once

// Item layout v20: the 0xC00 byte slot of the ja/en tables before the 2026-09
// update. The live client does not use it any more, but the pre-update corpus
// (LocCNTxtOld) still holds these files, and they are the source of the
// translations that were made before the update.
//
// Measured from the pre-update corpus: the same header and the same blob as v10,
// with a different spec prefix for usable / weapon / armour. Only the text
// position is confirmed for this version, so its spec area stays opaque exactly
// like v10's; the offsets are pinned by the static_asserts below.

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "ItemFormatV10.h"
#include "ItemFormatV30.h"

namespace itmfmt
{
	namespace v20
	{

		// The de/fr and the pre-update ja/en records share one header shape, so v10 owns
		// it (14 bytes: id, the two flag bytes, then the four 16 bit fields).
		using Header = v10::Header;

#pragma pack(push, 1)

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

		struct PuppetSlots
		{
			uint32_t head : 1;
			uint32_t body : 1;
			uint32_t attachment : 1;
			uint32_t rsv : 29;
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

		// Field map measured by cross version value matching against v30 (every located
		// field agrees for all shared ids); a field the reference never fills cannot be
		// located that way and keeps its positional name.
		// The unknown bytes of the normal and the usable spec stay arrays: the struct
		// states the byte layout, and the CSV writer (ItemFormatsCsv.cpp) expands them
		// into one Ukn0..Ukn(N-1) column per byte.
		struct NormalSpec
		{
			int16_t element; int16_t storage; uint32_t related_item_id;
			uint8_t ukn[2];
			Record info_rec;
		};
		struct UsableSpec
		{
			int16_t cast_factor;
			uint8_t ukn[12];
			Record info_rec;
		};
		struct WeaponSpec
		{
			uint16_t level;
			uint16_t equip_slots;
			uint16_t races;      // race mask, 16 bits wide in every version
			uint32_t jobs;
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
			uint8_t cast_factor;
			uint16_t use_time;
			uint16_t reuse_time;
			uint16_t ukn20;
			uint16_t related_item_id;
			uint16_t ilvl;
			uint16_t ukn22;
			uint16_t ukn23;
			Record info_rec;
		};
		struct ArmourSpec
		{
			int16_t level;
			uint16_t equip_slots;
			uint16_t equip_races;
			uint32_t equip_jobs;
			uint16_t slvl;
			uint16_t shield_size;
			uint8_t max_charges;
			uint8_t cast_factor;
			uint16_t use_time;
			uint16_t reuse_time;
			uint16_t ukn1;
			uint16_t related_item_id;
			uint16_t ilvl;
			uint16_t ukn3;
			uint16_t ukn4;
			Record info_rec;
		};
		struct PuppetSpec 
		{
			uint32_t equip_slots;
			uint8_t fire : 4;
			uint8_t ice : 4;
			uint8_t air : 4;
			uint8_t earth : 4;
			uint8_t thunder : 4;
			uint8_t water : 4;
			uint8_t light : 4;
			uint8_t dark : 4;
			uint8_t ukn[2];
			Record info_rec; 
		};
		struct SlipSpec { uint8_t ukn[70]; Record info_rec; };
		struct CurrencySpec { uint8_t ukn[2];  Record info_rec; };
		struct InstinctSpec { uint8_t ukn[26]; Record info_rec; };

		union SpecData
		{
			char raw[626];
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
			char image_data[2427];
			uint8_t end_marker;
		};

#pragma pack(pop)

		static_assert(sizeof(Header) == 14, "v20 header size mismatch");
		static_assert(sizeof(Entry) == 0xC00, "v20 slot size mismatch");
		static_assert(offsetof(Entry, image_length) == 640, "v20 blob length offset mismatch");
		static_assert(offsetof(Entry, image_data) == 644, "v20 blob data offset mismatch");

		struct Format
		{
			using Entry = v20::Entry;
			using Datum = itmfmt::Datum;
			using Schema = SpecType;

			static constexpr Version version = Version::V20;
			static constexpr std::string_view id = "v20";
			static constexpr std::string_view scope = "ja/en item tables before the 2026-09 update";

			static constexpr size_t slotSize = sizeof(Entry);
			static constexpr size_t currencySlots = 16;
			static constexpr size_t cipherSpan = sizeof(Entry);
			static constexpr size_t blobLengthOffset = offsetof(Entry, image_length);
			static constexpr size_t blobDataOffset = offsetof(Entry, image_data);
			static constexpr size_t blobCapacity = sizeof(Entry::image_data);
			static constexpr size_t textEnd = offsetof(Entry, image_length);
			static constexpr SpecType DEFAULT_SCHEMA = SpecType::NORMAL;

			static constexpr bool hasExtendedFlags = false;
			static constexpr size_t headerExtendedFlagsOffset = 0; // absent; never read
			static constexpr size_t headerStackSizeOffset = offsetof(Entry, header.stack_size);
			static constexpr size_t headerItemTypeOffset = offsetof(Entry, header.item_type);
			static constexpr size_t headerResourceIdOffset = offsetof(Entry, header.resource_id);
			static constexpr size_t headerValidTargetsOffset = offsetof(Entry, header.valid_targets);

			static constexpr bool IsCurrencySchema(SpecType schema) { return schema == SpecType::CURRENCY; }

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

			static CsvContext GetCsvContext(std::span<const Datum> records);
			static void WriteCsvHeader(CsvFile& csv, const CsvContext& context);
			static void WriteCsvRow(CsvFile& csv, const CsvContext& context, const Datum& datum);
		};

		// Text offsets measured from the pre-update corpus (LocCNTxtOld).
		static_assert(Format::TextOffset(SpecType::NORMAL) == 24, "v20 normal text offset mismatch");
		static_assert(Format::TextOffset(SpecType::USABLE) == 28, "v20 usable text offset mismatch");
		static_assert(Format::TextOffset(SpecType::WEAPON) == 56, "v20 weapon text offset mismatch");
		static_assert(Format::TextOffset(SpecType::ARMOUR) == 44, "v20 armour text offset mismatch");
		static_assert(Format::TextOffset(SpecType::PUPPET) == 24, "v20 puppet text offset mismatch");
		static_assert(Format::TextOffset(SpecType::SLIP) == 84, "v20 slip text offset mismatch");
		static_assert(Format::TextOffset(SpecType::CURRENCY) == 16, "v20 currency text offset mismatch");
		static_assert(Format::TextOffset(SpecType::INSTINCT) == 40, "v20 instinct text offset mismatch");

		static_assert(slotfile::SlotDatum<itmfmt::Datum, Format>, "v20 datum does not match the container protocol");

	} // namespace v20
} // namespace itmfmt
