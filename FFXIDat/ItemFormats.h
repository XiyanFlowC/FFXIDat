#pragma once

// The item family: version aliases and routing. One include for the call sites.
//
// A layout is a type rather than a value, so "which version does this file have"
// is answered by the annotation on the data side (the "_o" type suffix), and the
// facades dispatch on a Version. This header holds the routing table.

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

} // namespace itmfmt
