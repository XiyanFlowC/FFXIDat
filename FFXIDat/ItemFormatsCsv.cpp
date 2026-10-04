// The CSV view of the item family: one column set per version and schema.
//
// Column ownership: a column set lives in exactly one place here, and that place
// is the field list of the structs of that version (ItemFormatV10.h / V20.h /
// V30.h). Unknown fields are listed as well, with their positional names (Ukn4,
// Ukn12, UknAfterRelated, ...), so that a row can be compared field by field
// against the record bytes. There is no shared header and no placeholder cell:
// every column the header names has a value in every row, and the header of a
// schema is emitted right next to the row of that schema so that the two column
// lists can be read against each other.

#include "ItemFormats.h"

#include <algorithm>
#include <cstring>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "CsvFile.h"
#include "xystring.h"

namespace
{
using itmfmt::CsvContext;
using itmfmt::Datum;
using itmfmt::SpecType;

std::u8string ToUtf8(int64_t value)
{
	return xybase::string::to_utf8(std::to_string(value));
}

std::u8string ElementToText(int16_t element)
{
	if (element == -1) return u8"N/A";
	switch (element)
	{
	case 0: return u8"Fire";
	case 1: return u8"Ice";
	case 2: return u8"Wind";
	case 3: return u8"Earth";
	case 4: return u8"Thunder";
	case 5: return u8"Water";
	case 6: return u8"Light";
	case 7: return u8"Dark";
	default: return ToUtf8(element);
	}
}

std::u8string ValidTargetsToText(uint16_t targets)
{
	if (targets == 0) return u8"None";
	std::u8string result;
	if (targets & 0x01) { if (!result.empty()) result += u8" "; result += u8"Self"; }
	if (targets & 0x02) { if (!result.empty()) result += u8" "; result += u8"Player"; }
	if (targets & 0x04) { if (!result.empty()) result += u8" "; result += u8"Party"; }
	if (targets & 0x08) { if (!result.empty()) result += u8" "; result += u8"Alliance"; }
	if (targets & 0x10) { if (!result.empty()) result += u8" "; result += u8"NPC"; }
	if (targets & 0x20) { if (!result.empty()) result += u8" "; result += u8"Enemy"; }
	if (targets & 0x40) { if (!result.empty()) result += u8" "; result += u8"Unknown_0x40"; }
	if (targets & 0x80) { if (!result.empty()) result += u8" "; result += u8"Corpse"; }
	if (targets & 0x100) { if (!result.empty()) result += u8" "; result += u8"Unknown_0x100"; }
	if (targets & 0x200) { if (!result.empty()) result += u8" "; result += u8"Unknown_0x200"; }
	if (targets & 0x400) { if (!result.empty()) result += u8" "; result += u8"Unknown_0x400"; }
	if (targets & 0x800) { if (!result.empty()) result += u8" "; result += u8"Unknown_0x800"; }
	if (targets & 0x1000) { if (!result.empty()) result += u8" "; result += u8"Unknown_0x1000"; }
	if (targets & 0x2000) { if (!result.empty()) result += u8" "; result += u8"Unknown_0x2000"; }
	if (targets & 0x4000) { if (!result.empty()) result += u8" "; result += u8"Unknown_0x4000"; }
	if (targets & 0x8000) { if (!result.empty()) result += u8" "; result += u8"Unknown_0x8000"; }
	return result;
}

std::u8string BoolToU8(bool value)
{
	return value ? u8"1" : u8"0";
}

std::u8string SkillTypeToU8(uint8_t skill)
{
	using SkillType = itmfmt::v30::SkillType;
	switch (static_cast<SkillType>(skill)) {
	case SkillType::None: return u8"N/A";
	case SkillType::HandToHand: return u8"Hand-to-Hand";
	case SkillType::Dagger: return u8"Dagger";
	case SkillType::Sword: return u8"Sword";
	case SkillType::GreatSword: return u8"Great Sword";
	case SkillType::Axe: return u8"Axe";
	case SkillType::GreatAxe: return u8"Great Axe";
	case SkillType::Scythe: return u8"Scythe";
	case SkillType::Polearm: return u8"Polearm";
	case SkillType::Katana: return u8"Katana";
	case SkillType::GreatKatana: return u8"Great Katana";
	case SkillType::Club: return u8"Club";
	case SkillType::Staff: return u8"Staff";
	case SkillType::Weapon12: return u8"(Weapon 12)";
	case SkillType::Weapon11: return u8"(Weapon 11)";
	case SkillType::Weapon10: return u8"(Weapon 10)";
	case SkillType::Weapon9: return u8"(Weapon 9)";
	case SkillType::Weapon8: return u8"(Weapon 8)";
	case SkillType::Weapon7: return u8"(Weapon 7)";
	case SkillType::Weapon6: return u8"(Weapon 6)";
	case SkillType::Weapon5: return u8"(Weapon 5)";
	case SkillType::Weapon4: return u8"(Weapon 4)";
	case SkillType::AutomatonMelee: return u8"Automaton Melee";
	case SkillType::AutomatonArchery: return u8"Automaton Archery";
	case SkillType::AutomatonMagic: return u8"Automaton Magic";
	case SkillType::Archery: return u8"Archery";
	case SkillType::Marksmanship: return u8"Marksmanship";
	case SkillType::Throwing: return u8"Throwing";
	case SkillType::Guard: return u8"Guard";
	case SkillType::Evasion: return u8"Evasion";
	case SkillType::Shield: return u8"Shield";
	case SkillType::Parrying: return u8"Parrying";
	case SkillType::DivineMagic: return u8"Divine Magic";
	case SkillType::HealingMagic: return u8"Healing Magic";
	case SkillType::EnhancingMagic: return u8"Enhancing Magic";
	case SkillType::EnfeeblingMagic: return u8"Enfeebling Magic";
	case SkillType::ElementalMagic: return u8"Elemental Magic";
	case SkillType::DarkMagic: return u8"Dark Magic";
	case SkillType::SummoningMagic: return u8"Summoning Magic";
	case SkillType::Ninjutsu: return u8"Ninjutsu";
	case SkillType::Singing: return u8"Singing";
	case SkillType::StringedInstrument: return u8"Stringed Instrument";
	case SkillType::WindInstrument: return u8"Wind Instrument";
	case SkillType::BlueMagic: return u8"Blue Magic";
	case SkillType::Geomancy: return u8"Geomancy";
	case SkillType::Handbell: return u8"Handbell";
	case SkillType::Magic2: return u8"(Magic 2)";
	case SkillType::Magic1: return u8"(Magic 1)";
	case SkillType::Fishing: return u8"Fishing";
	case SkillType::Woodworking: return u8"Woodworking";
	case SkillType::Smithing: return u8"Smithing";
	case SkillType::Goldsmithing: return u8"Goldsmithing";
	case SkillType::Clothcraft: return u8"Clothcraft";
	case SkillType::Leatherworking: return u8"Leatherworking";
	case SkillType::Bonecraft: return u8"Bonecraft";
	case SkillType::Alchemy: return u8"Alchemy";
	case SkillType::Cooking: return u8"Cooking";
	case SkillType::Synergy: return u8"Synergy";
	case SkillType::Synthesis6: return u8"(Synthesis 6)";
	case SkillType::Synthesis5: return u8"(Synthesis 5)";
	case SkillType::Synthesis4: return u8"(Synthesis 4)";
	case SkillType::Synthesis3: return u8"(Synthesis 3)";
	case SkillType::Synthesis2: return u8"(Synthesis 2)";
	case SkillType::Synthesis1: return u8"(Synthesis 1)";
	default: return xybase::string::to_utf8(std::to_string(skill));
	}
}

std::u8string FlagsToDescription(const itmfmt::v30::Header &header)
{
	std::stringstream ss;
	if (header.is_ex) ss << "Ex ";
	if (header.is_rare) ss << "Rare ";
	if (header.is_alt) ss << "Alt ";
	if (header.is_wall_decoration) ss << "WallDecoration ";
	if (header.is_gm_item) ss << "GMItem ";
	if (header.is_in_mystery_box) ss << "MysteryBox ";
	if (header.is_inscribable) ss << "Inscribable ";
	if (header.is_not_listable) ss << "NotListable ";
	if (header.is_scroll) ss << "Scroll ";
	if (header.is_linkshell) ss << "Linkshell ";
	if (header.is_usable) ss << "Usable ";
	if (header.is_npc_tradeable) ss << "NPCTradeable ";
	if (header.is_equipment) ss << "Equipment ";
	if (header.is_unsellable) ss << "Unsellable ";
	if (header.is_unmailable) ss << "Unmailable ";
	return xybase::string::to_utf8(ss.str());
}

std::u8string JoinUtf8(const std::vector<const char8_t *> &parts)
{
	std::u8string result;
	for (auto part : parts)
	{
		if (!part || *part == 0) continue;
		if (!result.empty()) result += u8" ";
		result += part;
	}
	return result;
}

std::u8string EquipSlotText(const itmfmt::v30::EquipSlots &slots)
{
	std::vector<const char8_t *> parts;
	if (slots.main_hand) parts.push_back(u8"MainHand");
	if (slots.sub_hand) parts.push_back(u8"SubHand");
	if (slots.ranged) parts.push_back(u8"Ranged");
	if (slots.ammo) parts.push_back(u8"Ammo");
	if (slots.head) parts.push_back(u8"Head");
	if (slots.body) parts.push_back(u8"Body");
	if (slots.hands) parts.push_back(u8"Hands");
	if (slots.legs) parts.push_back(u8"Legs");
	if (slots.feet) parts.push_back(u8"Feet");
	if (slots.neck) parts.push_back(u8"Neck");
	if (slots.waist) parts.push_back(u8"Waist");
	if (slots.left_ear) parts.push_back(u8"LeftEar");
	if (slots.right_ear) parts.push_back(u8"RightEar");
	if (slots.left_ring) parts.push_back(u8"LeftRing");
	if (slots.right_ring) parts.push_back(u8"RightRing");
	if (slots.back) parts.push_back(u8"Back");
	return JoinUtf8(parts);
}

std::u8string RaceText(const itmfmt::v30::RaceFlags &races)
{
	std::vector<const char8_t *> parts;

	// if all
	if (races.HumeMale && races.HumeFemale && races.ElvaanMale && races.ElvaanFemale &&
		races.TaruMale && races.TaruFemale && races.Mithra && races.Galka)
	{
		return u8"All";
	}

	// if female
	if (races.ElvaanFemale && races.HumeFemale && races.TaruFemale && races.Mithra &&
		!races.Galka && !races.HumeMale && !races.ElvaanMale && !races.TaruMale)
	{
		return u8"Female";
	}

	// if male
	if (races.ElvaanMale && races.HumeMale && races.TaruMale && races.Galka &&
		!races.Mithra && !races.HumeFemale && !races.ElvaanFemale && !races.TaruFemale)
	{
		return u8"Male";
	}

	if (races.None) parts.push_back(u8"None");
	if (races.HumeMale) parts.push_back(u8"HumeMale");
	if (races.HumeFemale) parts.push_back(u8"HumeFemale");
	if (races.ElvaanMale) parts.push_back(u8"ElvaanMale");
	if (races.ElvaanFemale) parts.push_back(u8"ElvaanFemale");
	if (races.TaruMale) parts.push_back(u8"TaruMale");
	if (races.TaruFemale) parts.push_back(u8"TaruFemale");
	if (races.Mithra) parts.push_back(u8"Mithra");
	if (races.Galka) parts.push_back(u8"Galka");
	return JoinUtf8(parts);
}

std::u8string JobText(const itmfmt::v30::JobFlags &jobs)
{
	std::vector<const char8_t *> parts;
	if (jobs.pld) parts.push_back(u8"PLD");
	if (jobs.thf) parts.push_back(u8"THF");
	if (jobs.rdm) parts.push_back(u8"RDM");
	if (jobs.blm) parts.push_back(u8"BLM");
	if (jobs.whm) parts.push_back(u8"WHM");
	if (jobs.mnk) parts.push_back(u8"MNK");
	if (jobs.war) parts.push_back(u8"WAR");
	if (jobs.smn) parts.push_back(u8"SMN");
	if (jobs.drg) parts.push_back(u8"DRG");
	if (jobs.nin) parts.push_back(u8"NIN");
	if (jobs.sam) parts.push_back(u8"SAM");
	if (jobs.rng) parts.push_back(u8"RNG");
	if (jobs.brd) parts.push_back(u8"BRD");
	if (jobs.bst) parts.push_back(u8"BST");
	if (jobs.drk) parts.push_back(u8"DRK");
	if (jobs.mon) parts.push_back(u8"MON");
	if (jobs.run) parts.push_back(u8"RUN");
	if (jobs.geo) parts.push_back(u8"GEO");
	if (jobs.sch) parts.push_back(u8"SCH");
	if (jobs.dnc) parts.push_back(u8"DNC");
	if (jobs.pup) parts.push_back(u8"PUP");
	if (jobs.cor) parts.push_back(u8"COR");
	if (jobs.blu) parts.push_back(u8"BLU");
	return JoinUtf8(parts);
}

// Reinterprets the flag block or the slot word of one version as the v30 type of
// the same size, so that the text helpers above serve every version.
template <class T, class U>
T CsvMask(const U &value)
{
	static_assert(sizeof(T) == sizeof(U));
	T result;
	std::memcpy(&result, &value, sizeof(result));
	return result;
}

CsvContext ContextOf(std::span<const Datum> records)
{
	CsvContext context;
	context.schema = records.front().schema;
	context.english = std::any_of(records.begin(), records.end(), [](const Datum &datum) {
		if (!datum.hasOriginalRow) return false;
		const auto &cells = datum.originalRow.GetCellsConst();
		return cells.size() >= 5 && cells[0].GetType() == 0 && cells[1].GetType() == 1;
	});
	return context;
}

// The columns every item version shares. They come from the semantic header and
// from the text row, both of which have the same shape in every version; the per
// schema columns of the layout follow them.
//
// `extended_flags` is the one exception: it exists in the v30 header only, so a
// layout whose header has no such field (v10 and v20, hasExtendedFlags == false)
// must not carry the column.
template <class L>
void WriteCommonHeader(CsvFile &csv, const CsvContext &context)
{
	csv.NewCell(u8"ID");
	csv.NewCell(u8"Name");
	csv.NewCell(u8"Description");
	csv.NewCell(u8"Flags");
	if constexpr (L::hasExtendedFlags)
		csv.NewCell(u8"ExtendedFlags");
	csv.NewCell(u8"Stack");
	csv.NewCell(u8"Type");
	// csv.NewCell(u8"SpecType");
	csv.NewCell(u8"ResID");
	csv.NewCell(u8"Targets");
	csv.NewCell(u8"ImageLength");
	if (context.english)
	{
		csv.NewCell(u8"LogFlag");
		csv.NewCell(u8"Name_Singular");
		csv.NewCell(u8"Name_Plural");
	}
}

template <class L>
void WriteCommonRow(CsvFile &csv, const CsvContext &context, const Datum &datum)
{
	std::u8string name;
	std::u8string desc;
	std::u8string nameSg;
	std::u8string namePl;
	std::u8string logFlag;

	if (datum.hasOriginalRow)
	{
		try { name = datum.name(); } catch (...) {}
		try { desc = datum.description(); } catch (...) {}
		if (context.english)
		{
			try { logFlag = ToUtf8(datum.logFlag()); } catch (...) {}
			try { nameSg = datum.name_sg(); } catch (...) {}
			try { namePl = datum.name_pl(); } catch (...) {}
		}
	}

	csv.NewCell(ToUtf8(datum.id));
	csv.NewCell(name);
	csv.NewCell(desc);
	csv.NewCell(FlagsToDescription(datum.flags()));
	if constexpr (L::hasExtendedFlags)
		csv.NewCell(ToUtf8(datum.flags().extended_flags));
	csv.NewCell(ToUtf8(datum.stack_size()));
	csv.NewCell(ToUtf8(datum.item_type()));
	// csv.NewCell(specTypeToU8(datum.schema));
	csv.NewCell(ToUtf8(datum.resource_id()));
	csv.NewCell(ValidTargetsToText(datum.valid_targets()));
	csv.NewCell(ToUtf8(datum.originalEntry.image_length));
	if (context.english)
	{
		csv.NewCell(logFlag);
		csv.NewCell(nameSg);
		csv.NewCell(namePl);
	}
}

// ============================================================================
// v30: the columns of the newest known layout. WriteV30Header and WriteV30Row
// list the same fields in the same order, one case per schema.
// ============================================================================

void WriteV30Header(CsvFile &csv, const CsvContext &context)
{
	WriteCommonHeader<itmfmt::v30::Format>(csv, context);
	switch (context.schema)
	{
	case SpecType::WEAPON:
	{
		const char8_t *columns[] = { u8"Level", u8"Slots", u8"Races", u8"Jobs", u8"UknAfterRaces",
			u8"SuperiorLevel", u8"Ukn2", u8"DMG", u8"Delay", u8"DPS", u8"Skill", u8"Ukn12",
			u8"Ukn7", u8"Ukn9", u8"MaxCharges", u8"CastFactor", u8"UseTime", u8"ReuseTime",
			u8"Ukn20", u8"RelatedItemId", u8"iLvl", u8"Ukn22", u8"Ukn23" };
		for (const char8_t *column : columns) csv.NewCell(column);
		break;
	}
	case SpecType::ARMOUR:
	{
		const char8_t *columns[] = { u8"Level", u8"Slots", u8"Races", u8"Jobs", u8"UknAfterRaces",
			u8"SuperiorLevel", u8"ShieldSize", u8"MaxCharges", u8"CastFactor", u8"UseTime",
			u8"ReuseTime", u8"Ukn1", u8"RelatedItemId", u8"iLvl", u8"Ukn3", u8"Ukn4" };
		for (const char8_t *column : columns) csv.NewCell(column);
		break;
	}
	case SpecType::USABLE:
	{
		const char8_t *columns[] = { u8"CastFactor", u8"Ukn1", u8"Ukn2", u8"Ukn3" };
		for (const char8_t *column : columns) csv.NewCell(column);
		break;
	}
	case SpecType::NORMAL:
	{
		const char8_t *columns[] = { u8"Element", u8"Storage", u8"RelatedItemId", u8"Ukn4", u8"Ukn5" };
		for (const char8_t *column : columns) csv.NewCell(column);
		break;
	}
	case SpecType::PUPPET:
	{
		const char8_t *columns[] = { u8"Slot_Head", u8"Slot_Body", u8"Slot_Attachment", u8"Fire",
			u8"Ice", u8"Wind", u8"Earth", u8"Thunder", u8"Water", u8"Light", u8"Dark", u8"Ukn" };
		for (const char8_t *column : columns) csv.NewCell(column);
		break;
	}
	case SpecType::SLIP:
		for (size_t i = 0; i < sizeof(itmfmt::v30::SlipSpec::ukn); ++i)
			csv.NewCell(xybase::string::to_utf8(std::string("Ukn") + std::to_string(i)));
		break;
	case SpecType::INSTINCT:
		for (size_t i = 0; i < sizeof(itmfmt::v30::InstinctSpec::ukn) / sizeof(uint16_t); ++i)
			csv.NewCell(xybase::string::to_utf8(std::string("Ukn") + std::to_string(i)));
		break;
	case SpecType::CURRENCY:
		csv.NewCell(u8"Ukn");
		break;
	}
}

void WriteV30Row(CsvFile &csv, const Datum &datum)
{
	switch (datum.schema)
	{
	case SpecType::WEAPON:
	{
		const auto &spec = datum.originalEntry.spec.weapon;
		csv.NewCell(ToUtf8(spec.level));
		csv.NewCell(EquipSlotText(spec.equip_slots));
		csv.NewCell(RaceText(spec.races));
		csv.NewCell(JobText(spec.jobs));
		csv.NewCell(ToUtf8(spec.ukn_after_races));
		csv.NewCell(ToUtf8(spec.slvl));
		csv.NewCell(ToUtf8(spec.ukn2));
		csv.NewCell(ToUtf8(spec.dmg));
		csv.NewCell(ToUtf8(spec.delay));
		csv.NewCell(ToUtf8(spec.dps));
		csv.NewCell(SkillTypeToU8(spec.skill));
		csv.NewCell(ToUtf8(spec.ukn12));
		csv.NewCell(ToUtf8(spec.ukn7));
		csv.NewCell(ToUtf8(spec.ukn9));
		csv.NewCell(ToUtf8(spec.max_charges));
		csv.NewCell(ToUtf8(spec.cast_factor));
		csv.NewCell(ToUtf8(spec.use_time));
		csv.NewCell(ToUtf8(spec.reuse_time));
		csv.NewCell(ToUtf8(spec.ukn20));
		csv.NewCell(ToUtf8(spec.related_item_id));
		csv.NewCell(ToUtf8(spec.ilvl));
		csv.NewCell(ToUtf8(spec.ukn22));
		csv.NewCell(ToUtf8(spec.ukn23));
		break;
	}
	case SpecType::ARMOUR:
	{
		const auto &spec = datum.originalEntry.spec.armour;
		csv.NewCell(ToUtf8(spec.level));
		csv.NewCell(EquipSlotText(spec.equip_slots));
		csv.NewCell(RaceText(spec.equip_races));
		csv.NewCell(JobText(spec.equip_jobs));
		csv.NewCell(ToUtf8(spec.ukn_after_races));
		csv.NewCell(ToUtf8(spec.slvl));
		csv.NewCell(ToUtf8(spec.shield_size));
		csv.NewCell(ToUtf8(spec.max_charges));
		csv.NewCell(ToUtf8(spec.cast_factor));
		csv.NewCell(ToUtf8(spec.use_time));
		csv.NewCell(ToUtf8(spec.reuse_time));
		csv.NewCell(ToUtf8(spec.ukn1));
		csv.NewCell(ToUtf8(spec.related_item_id));
		csv.NewCell(ToUtf8(spec.ilvl));
		csv.NewCell(ToUtf8(spec.ukn3));
		csv.NewCell(ToUtf8(spec.ukn4));
		break;
	}
	case SpecType::USABLE:
	{
		const auto &spec = datum.originalEntry.spec.usable;
		csv.NewCell(ToUtf8(spec.cast_factor));
		csv.NewCell(ToUtf8(spec.ukn1));
		csv.NewCell(ToUtf8(spec.ukn2));
		csv.NewCell(ToUtf8(spec.ukn3));
		break;
	}
	case SpecType::NORMAL:
	{
		const auto &spec = datum.originalEntry.spec.normal;
		csv.NewCell(ElementToText(spec.element));
		csv.NewCell(ToUtf8(spec.storage));
		csv.NewCell(ToUtf8(spec.related_item_id));
		csv.NewCell(ToUtf8(spec.ukn4));
		csv.NewCell(ToUtf8(spec.ukn5));
		break;
	}
	case SpecType::PUPPET:
	{
		const auto &spec = datum.originalEntry.spec.puppet;
		csv.NewCell(BoolToU8(spec.equip_slots.head));
		csv.NewCell(BoolToU8(spec.equip_slots.body));
		csv.NewCell(BoolToU8(spec.equip_slots.attachment));
		csv.NewCell(ToUtf8(spec.fire));
		csv.NewCell(ToUtf8(spec.ice));
		csv.NewCell(ToUtf8(spec.air));
		csv.NewCell(ToUtf8(spec.earth));
		csv.NewCell(ToUtf8(spec.thunder));
		csv.NewCell(ToUtf8(spec.water));
		csv.NewCell(ToUtf8(spec.light));
		csv.NewCell(ToUtf8(spec.dark));
		csv.NewCell(ToUtf8(spec.ukn));
		break;
	}
	case SpecType::SLIP:
		for (uint8_t byte : datum.originalEntry.spec.slip.ukn)
			csv.NewCell(ToUtf8(static_cast<unsigned>(byte)));
		break;
	case SpecType::INSTINCT:
		for (uint16_t word : datum.originalEntry.spec.instinct.ukn)
			csv.NewCell(ToUtf8(static_cast<unsigned>(word)));
		break;
	case SpecType::CURRENCY:
		csv.NewCell(ToUtf8(datum.originalEntry.spec.currency.ukn));
		break;
	}
}

// ============================================================================
// v10 (de/fr) and v20 (ja/en before the 2026-09 update): their spec areas model
// fewer, differently named members than v30, so they get their own column lists.
// Every member of their spec structs is listed, in struct order, including the
// unknown ones, and the text cells come last.
// ============================================================================

// The sizes of the unknown byte arrays of a legacy spec, asked of the structs
// themselves, so that a header names exactly the bytes its rows write.
template <class L>
struct LegacyUnknownBytes
{
	static constexpr size_t normal = sizeof(decltype(std::declval<typename L::Entry>().spec.normal.ukn));
	static constexpr size_t usable = sizeof(decltype(std::declval<typename L::Entry>().spec.usable.ukn));
};

// The layout template parameter is the version, so `if constexpr` picks the
// members that version actually has, exactly like WriteLegacyRow below.
template <class L>
void WriteLegacyHeader(CsvFile &csv, const CsvContext &context)
{
	WriteCommonHeader<L>(csv, context);
	switch (context.schema)
	{
	case SpecType::WEAPON:
	{
		if constexpr (std::is_same_v<typename L::Entry, itmfmt::v10::Entry>)
		{
			// v10: no slvl and no ukn2; the word after related_item_id is the
			// v10-only ukn_after_related, named after its position.
			const char8_t *columns[] = { u8"Level", u8"Slots", u8"Races", u8"Jobs", u8"DMG",
				u8"Delay", u8"DPS", u8"Skill", u8"Ukn12", u8"Ukn7", u8"Ukn9", u8"MaxCharges",
				u8"CastFactor", u8"UseTime", u8"ReuseTime", u8"Ukn20", u8"RelatedItemId",
				u8"UknAfterRelated" };
			for (const char8_t *column : columns) csv.NewCell(column);
		}
		else
		{
			const char8_t *columns[] = { u8"Level", u8"Slots", u8"Races", u8"Jobs",
				u8"SuperiorLevel", u8"Ukn2", u8"DMG", u8"Delay", u8"DPS", u8"Skill", u8"Ukn12",
				u8"Ukn7", u8"Ukn9", u8"MaxCharges", u8"CastFactor", u8"UseTime", u8"ReuseTime",
				u8"Ukn20", u8"RelatedItemId", u8"iLvl", u8"Ukn22", u8"Ukn23" };
			for (const char8_t *column : columns) csv.NewCell(column);
		}
		break;
	}
	case SpecType::ARMOUR:
	{
		if constexpr (std::is_same_v<typename L::Entry, itmfmt::v10::Entry>)
		{
			// v10: no slvl; it has the v10-only ukn_after_use_time right after
			// use_time and ukn_after_related instead of the ilvl / ukn3 / ukn4 tail.
			const char8_t *columns[] = { u8"Level", u8"Slots", u8"Races", u8"Jobs",
				u8"ShieldSize", u8"MaxCharges", u8"CastFactor", u8"UseTime",
				u8"UknAfterUseTime", u8"ReuseTime", u8"Ukn1", u8"RelatedItemId",
				u8"UknAfterRelated" };
			for (const char8_t *column : columns) csv.NewCell(column);
		}
		else
		{
			const char8_t *columns[] = { u8"Level", u8"Slots", u8"Races", u8"Jobs",
				u8"SuperiorLevel", u8"ShieldSize", u8"MaxCharges", u8"CastFactor", u8"UseTime",
				u8"ReuseTime", u8"Ukn1", u8"RelatedItemId", u8"iLvl", u8"Ukn3", u8"Ukn4" };
			for (const char8_t *column : columns) csv.NewCell(column);
		}
		break;
	}
	case SpecType::NORMAL:
	{
		csv.NewCell(u8"Element");
		csv.NewCell(u8"Storage");
		csv.NewCell(u8"RelatedItemId");
		// The unknown prefix of this version is a byte array, so the columns are
		// expanded from it one byte at a time (v10 and v20 both hold two bytes).
		for (size_t i = 0; i < LegacyUnknownBytes<L>::normal; ++i)
			csv.NewCell(xybase::string::to_utf8(std::string("Ukn") + std::to_string(i)));
		break;
	}
	case SpecType::USABLE:
	{
		csv.NewCell(u8"CastFactor");
		// v10 models eight unknown bytes, v20 twelve; the columns are expanded from
		// the array of that version, exactly like the slip/instinct arrays of v30.
		for (size_t i = 0; i < LegacyUnknownBytes<L>::usable; ++i)
			csv.NewCell(xybase::string::to_utf8(std::string("Ukn") + std::to_string(i)));
		break;
	}
	case SpecType::PUPPET:
	{
		const char8_t *columns[] = { u8"Slot_Head", u8"Slot_Body", u8"Slot_Attachment", u8"Fire",
			u8"Ice", u8"Wind", u8"Earth", u8"Thunder", u8"Water", u8"Light", u8"Dark",
			u8"Ukn0", u8"Ukn1" };
		for (const char8_t *column : columns) csv.NewCell(column);
		break;
	}
	case SpecType::SLIP:
		for (size_t i = 0; i < 70; ++i)
			csv.NewCell(xybase::string::to_utf8(std::string("Ukn") + std::to_string(i)));
		break;
	case SpecType::INSTINCT:
		for (size_t i = 0; i < 26; ++i)
			csv.NewCell(xybase::string::to_utf8(std::string("Ukn") + std::to_string(i)));
		break;
	case SpecType::CURRENCY:
		csv.NewCell(u8"Ukn0");
		csv.NewCell(u8"Ukn1");
		break;
	}
}

// The v10 and v20 spec structs are reached through the layout that read the
// record, so `if constexpr` picks the members that version actually has; the
// value order is the column order of WriteLegacyHeader above.
template <class L>
void WriteLegacyRow(CsvFile &csv, const Datum &datum)
{
	if (datum.layoutId != L::id || datum.rawBytes.size() != L::slotSize)
		throw std::runtime_error("CSV record does not match item layout " + std::string(L::id));

	typename L::Entry entry;
	std::memcpy(&entry, datum.rawBytes.data(), sizeof(entry));

	auto number = [&](auto value) { csv.NewCell(ToUtf8(value)); };
	// The unknown arrays of this family are byte arrays except the instinct one,
	// which is an array of words; the two helpers below name their element type.
	auto unknownBytes = [&](const auto &bytes) {
		for (size_t i = 0; i < sizeof(bytes); ++i)
			number(static_cast<unsigned>(static_cast<uint8_t>(bytes[i])));
	};
	auto unknownWords = [&](const auto &words) {
		for (size_t i = 0; i < sizeof(words) / sizeof(words[0]); ++i)
			number(static_cast<unsigned>(static_cast<uint16_t>(words[i])));
	};

	switch (datum.schema)
	{
	case SpecType::WEAPON:
	{
		const auto &spec = entry.spec.weapon;
		number(spec.level);
		csv.NewCell(EquipSlotText(CsvMask<itmfmt::v30::EquipSlots>(spec.equip_slots)));
		csv.NewCell(RaceText(CsvMask<itmfmt::v30::RaceFlags>(spec.races)));
		csv.NewCell(JobText(CsvMask<itmfmt::v30::JobFlags>(spec.jobs)));
		if constexpr (requires { spec.slvl; })
		{
			number(spec.slvl);
			number(spec.ukn2);
		}
		number(spec.dmg);
		number(spec.delay);
		number(spec.dps);
		csv.NewCell(SkillTypeToU8(spec.skill));
		number(spec.ukn12);
		number(spec.ukn7);
		number(spec.ukn9);
		number(spec.max_charges);
		number(spec.cast_factor);
		number(spec.use_time);
		number(spec.reuse_time);
		number(spec.ukn20);
		number(spec.related_item_id);
		if constexpr (requires { spec.ilvl; })
		{
			number(spec.ilvl);
			number(spec.ukn22);
			number(spec.ukn23);
		}
		else number(spec.ukn_after_related);
		break;
	}
	case SpecType::ARMOUR:
	{
		const auto &spec = entry.spec.armour;
		number(spec.level);
		csv.NewCell(EquipSlotText(CsvMask<itmfmt::v30::EquipSlots>(spec.equip_slots)));
		csv.NewCell(RaceText(CsvMask<itmfmt::v30::RaceFlags>(spec.equip_races)));
		csv.NewCell(JobText(CsvMask<itmfmt::v30::JobFlags>(spec.equip_jobs)));
		if constexpr (requires { spec.slvl; }) number(spec.slvl);
		number(spec.shield_size);
		number(spec.max_charges);
		number(spec.cast_factor);
		number(spec.use_time);
		if constexpr (requires { spec.ukn_after_use_time; }) number(spec.ukn_after_use_time);
		number(spec.reuse_time);
		number(spec.ukn1);
		number(spec.related_item_id);
		if constexpr (requires { spec.ilvl; })
		{
			number(spec.ilvl);
			number(spec.ukn3);
			number(spec.ukn4);
		}
		else number(spec.ukn_after_related);
		break;
	}
	case SpecType::NORMAL:
	{
		const auto &spec = entry.spec.normal;
		csv.NewCell(ElementToText(spec.element));
		number(spec.storage);
		number(spec.related_item_id);
		// One cell per byte of the unknown array, in the column order above.
		unknownBytes(spec.ukn);
		break;
	}
	case SpecType::USABLE:
	{
		const auto &spec = entry.spec.usable;
		number(spec.cast_factor);
		unknownBytes(spec.ukn);
		break;
	}
	case SpecType::PUPPET:
	{
		const auto &spec = entry.spec.puppet;
		const auto slots = CsvMask<itmfmt::v30::PuppetSlots>(spec.equip_slots);
		csv.NewCell(BoolToU8(slots.head));
		csv.NewCell(BoolToU8(slots.body));
		csv.NewCell(BoolToU8(slots.attachment));
		number(spec.fire);
		number(spec.ice);
		number(spec.air);
		number(spec.earth);
		number(spec.thunder);
		number(spec.water);
		number(spec.light);
		number(spec.dark);
		unknownBytes(spec.ukn);
		break;
	}
	case SpecType::SLIP: unknownBytes(entry.spec.slip.ukn); break;
	case SpecType::INSTINCT: unknownWords(entry.spec.instinct.ukn); break;
	case SpecType::CURRENCY: unknownBytes(entry.spec.currency.ukn); break;
	}
}

} // namespace

namespace itmfmt
{
namespace v30
{

CsvContext Format::GetCsvContext(std::span<const Datum> records)
{
	return ContextOf(records);
}

void Format::WriteCsvHeader(CsvFile &csv, const CsvContext &context)
{
	WriteV30Header(csv, context);
	csv.NewLine();
}

void Format::WriteCsvRow(CsvFile &csv, const CsvContext &context, const Datum &datum)
{
	WriteCommonRow<Format>(csv, context, datum);
	WriteV30Row(csv, datum);
	csv.NewLine();
}

} // namespace v30

namespace v10
{

CsvContext Format::GetCsvContext(std::span<const Datum> records)
{
	return ContextOf(records);
}

void Format::WriteCsvHeader(CsvFile &csv, const CsvContext &context)
{
	WriteLegacyHeader<Format>(csv, context);
	csv.NewLine();
}

void Format::WriteCsvRow(CsvFile &csv, const CsvContext &context, const Datum &datum)
{
	WriteCommonRow<Format>(csv, context, datum);
	WriteLegacyRow<Format>(csv, datum);
	csv.NewLine();
}

} // namespace v10

namespace v20
{

CsvContext Format::GetCsvContext(std::span<const Datum> records)
{
	return ContextOf(records);
}

void Format::WriteCsvHeader(CsvFile &csv, const CsvContext &context)
{
	WriteLegacyHeader<Format>(csv, context);
	csv.NewLine();
}

void Format::WriteCsvRow(CsvFile &csv, const CsvContext &context, const Datum &datum)
{
	WriteCommonRow<Format>(csv, context, datum);
	WriteLegacyRow<Format>(csv, datum);
	csv.NewLine();
}

} // namespace v20
} // namespace itmfmt
