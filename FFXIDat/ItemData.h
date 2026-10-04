#pragma once

// ItemData: the facade of the item family.
//
// The container, the cipher and the per version layouts live in the model
// headers (SlotFile.h, ItemFormatV10.h, ItemFormatV30.h, ItemFormats.h). This
// class only keeps the historical API, forwards the encryption switch and routes
// the runtime record version to the right layout.
//
// The record version is selected here and comes from the data side annotation
// ("" for the newest known layout, "_o" for the oldest one).

#include <cstdint>
#include <string>
#include <vector>

#include "ItemFormats.h"
#include "LegacyTypeSuffix.h"
#include "SlotFile.h"

// Compatibility names of the structs that moved into their layout namespaces.
using ItemSpecType = itmfmt::SpecType;
using ItemHeader = itmfmt::v30::Header;
using ItemJobApplicability = itmfmt::v30::JobFlags;
using ItemEquipSlot = itmfmt::v30::EquipSlots;
using ItemRaceApplicability = itmfmt::v30::RaceFlags;
using ItemArmourSpec = itmfmt::v30::ArmourSpec;
using ItemPuppetSlot = itmfmt::v30::PuppetSlots;
using ItemPuppetSpec = itmfmt::v30::PuppetSpec;
using ItemNormalSpec = itmfmt::v30::NormalSpec;
using ItemUsableSpec = itmfmt::v30::UsableSpec;
using ItemWeaponSpec = itmfmt::v30::WeaponSpec;
using ItemSlipSpec = itmfmt::v30::SlipSpec;
using ItemInstinctSpec = itmfmt::v30::InstinctSpec;
using ItemCurrencySpec = itmfmt::v30::CurrencySpec;
using ItemSpecData = itmfmt::v30::SpecData;
using ItemEntry = itmfmt::v30::Entry;
using SkillType = itmfmt::v30::SkillType;


class ItemData
{
public:
	using ItemDatum = itmfmt::Datum;

	using ValidTarget = uint16_t; // bitmask for valid target types, e.g. player, NPC, etc.
	const ValidTarget VT_SELF = 0x0001,
		VT_PLAYER = 0x0002,
		VT_PARTY = 0x0004,
		VT_ALLY = 0x0008,
		VT_NPC = 0x0010,
		VT_ENEMY = 0x0020,
		VT_CORPSE = 0x0080;

	bool encryptionSuppression = false;

	void Read(std::wstring path, ItemSpecType defaultSpecType = ItemSpecType::NORMAL,
		itmfmt::Version version = itmfmt::CURRENT_VERSION);
	void Write(std::wstring path);

	// For inspection only, not designed for full fidelity round-trip
	void ToICsv(const std::wstring &path) const;

	// Record version this store was read with; it is also the version written back.
	itmfmt::Version version() const { return layoutVersion; }

	std::vector<ItemDatum> data;

private:
	itmfmt::Version layoutVersion = itmfmt::CURRENT_VERSION;
};
