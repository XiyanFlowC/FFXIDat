#pragma once

#include "SlotFile.h"

// The item family record kinds. Shared by every item layout version: which
// struct a slot holds does not depend on the record version.
namespace itmfmt
{
enum class SpecType
{
	NORMAL,
	USABLE,
	WEAPON,
	ARMOUR,
	PUPPET,
	SLIP,
	CURRENCY,
	INSTINCT,
};

// Opaque labels of the layouts this family is known to have, in the order we
// observed them. They are used for routing only (the data side spells them
// "" for the newest known layout and "_o" for the oldest one); they are never
// compared as numbers and they carry no promise about release dates.
using Version = slotfile::Version;
} // namespace itmfmt
