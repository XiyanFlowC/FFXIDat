#include "MonBridge.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <utility>

#include "SlotFileCsv.h"
#include "xystring.h"

// The string conversion of this family lives here so that the layout headers do
// not have to depend on the project helpers.
namespace mbfmt
{
std::u8string InternalNameFromBytes(const char* bytes, size_t size)
{
	return xybase::string::to_utf8(std::string(bytes, strnlen(bytes, size)));
}

std::string InternalNameToBytes(const std::u8string& name)
{
	return xybase::string::to_string(name);
}
} // namespace mbfmt

void MonBridge::Read(const char* path, slotfile::Version version)
{
	Read(xybase::string::sys_mbs_to_wcs(std::string(path)), version);
}

void MonBridge::Read(const std::wstring& path, slotfile::Version version)
{
	std::vector<MonBridgeDatum> records;
	switch (version)
	{
	case slotfile::Version::V10:
	case slotfile::Version::V20: // v20 uses the same record as v10
	{
		slotfile::SlotFile<mbfmt::v10::Format> file;
		file.Read(path);
		records = std::move(file.data);
		break;
	}
	case slotfile::Version::V30:
	default:
	{
		slotfile::SlotFile<mbfmt::v30::Format> file;
		file.Read(path);
		records = std::move(file.data);
		break;
	}
	}

	data = std::move(records);
	layoutVersion = version;
}

void MonBridge::Write(const char* path)
{
	Write(xybase::string::sys_mbs_to_wcs(std::string(path)));
}

void MonBridge::Write(const std::wstring& path)
{
	if (layoutVersion != slotfile::Version::V30)
		slotfile::SlotFile<mbfmt::v10::Format>().Write(path, data);
	else
		slotfile::SlotFile<mbfmt::v30::Format>().Write(path, data);
}

void MonBridge::ToICsv(const std::wstring& path) const
{
	// The CSV column set of both versions lives in MonBridgeFormatsCsv.cpp; this
	// only routes the version, exactly like Read and Write above.
	if (layoutVersion != slotfile::Version::V30)
		slotfile::WriteICsv<mbfmt::v10::Format>(path, data);
	else
		slotfile::WriteICsv<mbfmt::v30::Format>(path, data);
}
