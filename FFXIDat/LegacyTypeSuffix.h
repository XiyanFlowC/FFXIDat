#pragma once

#include <string>

#include "SlotFile.h"

// Definition files (defs.csv / FLIST.csv / ROM.csv) mark tables that still use
// the oldest record layout of the family with a trailing "_o" on the type code
// (for example inb_o, mbd_o, erq_o). Everything else uses the newest known
// layout of the client. The de/fr tables behind "_o" stopped being updated about
// ten years ago, so their records are the frozen, oldest version we know.
//
// The annotation is the ONLY version signal: nothing probes the file. Every
// family routes through VersionForTypeCode below.

inline bool HasLegacyTypeSuffix(const std::u8string &type)
{
	return type.size() > 2 && type.compare(type.size() - 2, 2, u8"_o") == 0;
}

inline std::u8string StripLegacyTypeSuffix(const std::u8string &type)
{
	return HasLegacyTypeSuffix(type) ? type.substr(0, type.size() - 2) : type;
}

// Same helpers for the plain char type codes used by the processor tools.
inline bool HasLegacyTypeSuffix(const std::string &type)
{
	return type.size() > 2 && type.compare(type.size() - 2, 2, "_o") == 0;
}

inline std::string BaseTypeOf(const std::string &type)
{
	return HasLegacyTypeSuffix(type) ? type.substr(0, type.size() - 2) : type;
}

inline bool IsItemTypeCode(const std::string &type)
{
	const std::string base = BaseTypeOf(type);
	return base == "iab" || base == "iwb" || base == "iub" || base == "inb" ||
		base == "ipb" || base == "isb" || base == "icb" || base == "iib";
}

// Type code -> record version, the single routing rule of all three families.
// "" (no suffix) is the newest known layout, "_o" the oldest one; a version
// without a data side suffix (the pre-update corpus) is requested explicitly.
inline constexpr slotfile::Version VersionForTypeCode(const std::string &type)
{
	if (type.ends_with("_v1")) return slotfile::Version::V10;
	if (type.ends_with("_v2")) return slotfile::Version::V20;
	if (type.ends_with("_v3")) return slotfile::Version::V30;
	return HasLegacyTypeSuffix(type) ? slotfile::Version::V10 : slotfile::Version::V30;
}

inline constexpr slotfile::Version VersionForTypeCode(const std::u8string &type)
{
	if (type.ends_with(u8"_v1")) return slotfile::Version::V10;
	if (type.ends_with(u8"_v2")) return slotfile::Version::V20;
	if (type.ends_with(u8"_v3")) return slotfile::Version::V30;
	return HasLegacyTypeSuffix(type) ? slotfile::Version::V10 : slotfile::Version::V30;
}
