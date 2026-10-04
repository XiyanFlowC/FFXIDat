#pragma once

// MonBridge: the facade of the MonBridge family.
//
// The container and the per version layouts live in the model headers
// (SlotFile.h, MonBridgeFormat*.h); this class keeps the historical API and
// routes the runtime record version to the right layout. The version comes from
// the data side annotation ("_o" for the de/fr tables).

#include <cstdint>
#include <string>
#include <vector>

#include "Image.h"
#include "MonBridgeFormats.h"
#include "Record.h"

// Compatibility names of the structs that moved into their layout namespaces.
using MBRecord = mbfmt::v30::Entry;
using MBRecordLegacy = mbfmt::v10::Entry;

class MonBridge
{
public:
	using MonBridgeDatum = mbfmt::Datum;

	// Record version of the file: the newest known layout by default, the oldest
	// one (0xC00) for the de/fr tables. v20 uses the same record as v10, so both
	// select that layout.
	void Read(const char* path, slotfile::Version version = slotfile::Version::V30);
	void Read(const std::wstring& path, slotfile::Version version = slotfile::Version::V30);
	void Write(const char* path);
	void Write(const std::wstring& path);
	void ToICsv(const std::wstring& path) const;

	// Record version of the data currently held.
	slotfile::Version version() const { return layoutVersion; }

	std::vector<MonBridgeDatum> data;

private:
	slotfile::Version layoutVersion = slotfile::Version::V30;
};
