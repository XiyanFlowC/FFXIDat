#pragma once

// The Records of Eminence family: version aliases and routing.

#include "RoeFormatV10.h"
#include "RoeFormatV20.h"
#include "RoeFormatV30.h"

namespace roefmt
{

// The newest known layout of this family. An alias, not a format name.
namespace Current = v30;

inline constexpr Version CURRENT_VERSION = Version::V30;
inline constexpr Version OLDEST_VERSION = Version::V10;

// v20 has no data side suffix: the live client has no file with that layout any
// more, so nothing can point at it. The pre-update corpus (LocCNTxtOld) is read
// by asking for Version::V20 explicitly.

} // namespace roefmt
