#pragma once

// The MonBridge family: version aliases and routing.

#include "MonBridgeFormatV10.h"
#include "MonBridgeFormatV30.h"

namespace mbfmt
{

// The newest known layout of this family. An alias, not a format name.
namespace Current = v30;

inline constexpr Version CURRENT_VERSION = Version::V30;
inline constexpr Version OLDEST_VERSION = Version::V10;

} // namespace mbfmt
