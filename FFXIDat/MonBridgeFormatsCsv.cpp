// The CSV view of the MonBridge family.
//
// Both versions of this family use the same record shape (the v30 struct plus
// the two words v10 does not have), so they share one column set. Every column
// below has a value in every row; the header and the rows are written by the two
// neighbouring functions, in the same order.

#include "MonBridgeFormats.h"

#include <string>

#include "CsvFile.h"
#include "xystring.h"

namespace
{
std::u8string Number(uint64_t value)
{
	return xybase::string::to_utf8(std::to_string(value));
}

void WriteBridgeCsvHeader(CsvFile &csv)
{
	csv.NewCell(u8"ID");
	csv.NewCell(u8"Index");
	csv.NewCell(u8"Internal_Name");
	csv.NewCell(u8"Display_Name");
}

void WriteBridgeCsvRow(CsvFile &csv, const mbfmt::Datum &datum)
{
	csv.NewCell(Number(datum.id));
	csv.NewCell(Number(datum.idx));
	csv.NewCell(datum.internalName);
	csv.NewCell(datum.displayName);
}
} // namespace

namespace mbfmt
{
namespace v30
{
CsvContext Format::GetCsvContext(std::span<const Datum>) { return CsvContext{}; }
void Format::WriteCsvHeader(CsvFile &csv, const CsvContext &)
{
	WriteBridgeCsvHeader(csv);
	csv.NewLine();
}
void Format::WriteCsvRow(CsvFile &csv, const CsvContext &, const Datum &datum)
{
	WriteBridgeCsvRow(csv, datum);
	csv.NewLine();
}
} // namespace v30

namespace v10
{
CsvContext Format::GetCsvContext(std::span<const Datum>) { return CsvContext{}; }
void Format::WriteCsvHeader(CsvFile &csv, const CsvContext &)
{
	WriteBridgeCsvHeader(csv);
	csv.NewLine();
}
void Format::WriteCsvRow(CsvFile &csv, const CsvContext &, const Datum &datum)
{
	WriteBridgeCsvRow(csv, datum);
	csv.NewLine();
}
} // namespace v10
} // namespace mbfmt
