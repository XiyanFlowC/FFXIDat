// The CSV view of the Records of Eminence family: one column set per schema.
//
// Column ownership: the columns of a schema are the fields of that schema's
// record struct (RoeFormatV30.h / RoeFormatV10.h / RoeFormatV20.h) plus the text
// fields whose meaning is known (Quest_Name / Name_Full / Description / Note for a
// quest, Category_Name for a category). The header and the rows list the same
// columns in the same order; no cell is a placeholder, a field a record does not
// carry stays empty (missing data), and a column a layout cannot have at all (the
// long name of a quest on ja) is left out of both the header and the rows.

#include "RoeFormats.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>

#include "CsvFile.h"
#include "xystring.h"

namespace
{
using roefmt::CategoryDatum;
using roefmt::CsvContext;
using roefmt::QuestDatum;
using roefmt::Schema;

std::u8string Number(uint64_t value)
{
	return xybase::string::to_utf8(std::to_string(value));
}

// A category record carries up to twenty-eight child entries, but only the first
// twenty of the count_of_children of them are named in the CSV cell.
constexpr uint32_t MAX_CHILDREN_IN_CELL = 20;

void QuestHeader(CsvFile &csv, size_t textCells)
{
	csv.NewCell(u8"ID");
	csv.NewCell(u8"Release_Date");
	csv.NewCell(u8"Repeatable");
	csv.NewCell(u8"Target_Count");
	csv.NewCell(u8"EMI_Reward");
	csv.NewCell(u8"EXP_Reward");
	csv.NewCell(u8"CAP_Reward");
	csv.NewCell(u8"UNI_Reward");
	csv.NewCell(u8"Quest_Name");
	// The long name follows the name, and only a layout that has such a cell gets
	// the column: ja carries three cells and no long name, so it has no column
	// here. The count of the columns follows the same table as the accessor.
	if (QuestDatum::LongNameCount(textCells) > 0)
		csv.NewCell(u8"Name_Full");
	csv.NewCell(u8"Description");
	csv.NewCell(u8"Note");
	csv.NewCell(u8"Cell_Count");
}

// Only the text cells whose meaning is known are exported - the quest name, the
// description and the note; a cell this family does not model gets no column,
// which is the convention of this repository. A field a record does not carry is
// written empty: that is missing data, not a positional placeholder.
void QuestRow(CsvFile &csv, const QuestDatum &datum)
{
	csv.NewCell(Number(datum.id));
	csv.NewCell(Number(datum.release_date));
	csv.NewCell(Number(datum.originalEntry.repeatable));
	csv.NewCell(Number(datum.originalEntry.target_count));
	csv.NewCell(Number(datum.originalEntry.emi_reward));
	csv.NewCell(Number(datum.originalEntry.exp_reward));
	csv.NewCell(Number(datum.originalEntry.cap_reward));
	csv.NewCell(Number(datum.originalEntry.uni_reward));

	std::u8string name;
	std::u8string description;
	std::u8string nameFull;
	std::u8string note;
	try { name = datum.questName(); } catch (...) {}
	try { description = datum.description(); } catch (...) {}
	try { nameFull = datum.questNameLong(); } catch (...) {}
	try { note = datum.note(); } catch (...) {}
	csv.NewCell(name);
	// Same table as the header, read off this record: a layout without a long name
	// cell (ja) writes no cell here either. An empty value means the layout has the
	// cell but this record leaves it empty, which is missing data.
	if (QuestDatum::LongNameCount(datum.originalRow.GetCellsConst().size()) > 0)
		csv.NewCell(nameFull);
	csv.NewCell(description);
	csv.NewCell(note);
	csv.NewCell(Number(datum.originalRow.GetCellsConst().size()));
}

void CategoryHeader(CsvFile &csv)
{
	csv.NewCell(u8"ID");
	csv.NewCell(u8"Child_Count");
	csv.NewCell(u8"Category_Name");
	csv.NewCell(u8"Cell_Count");
	csv.NewCell(u8"Children_Info");
}

void CategoryRow(CsvFile &csv, const CategoryDatum &datum)
{
	csv.NewCell(Number(datum.id));
	csv.NewCell(Number(datum.originalEntry.count_of_children));

	std::u8string name;
	try { name = datum.categoryName(); } catch (...) {}
	csv.NewCell(name);
	csv.NewCell(Number(datum.originalRow.GetCellsConst().size()));

	// The children are repeated sub records rather than an opaque byte block, so
	// the list is folded into one cell: the first count_of_children of them, each
	// as [child_id,quest_flag].
	std::u8string children;
	const uint32_t count = std::min<uint32_t>(datum.originalEntry.count_of_children, MAX_CHILDREN_IN_CELL);
	for (uint32_t i = 0; i < count; ++i)
	{
		if (i > 0) children += u8"; ";
		children += u8"[" + Number(datum.originalEntry.children[i].child_id) + u8"," +
			Number(datum.originalEntry.children[i].quest_flag) + u8"]";
	}
	csv.NewCell(children);
}
} // namespace

namespace roefmt
{
void WriteQuestCsvHeader(CsvFile &csv, const CsvContext &context)
{
	QuestHeader(csv, context.textCells);
	csv.NewLine();
}

void WriteQuestCsvRow(CsvFile &csv, const QuestDatum &datum)
{
	QuestRow(csv, datum);
	csv.NewLine();
}

void WriteCategoryCsvHeader(CsvFile &csv, const CsvContext &)
{
	CategoryHeader(csv);
	csv.NewLine();
}

void WriteCategoryCsvRow(CsvFile &csv, const CategoryDatum &datum)
{
	CategoryRow(csv, datum);
	csv.NewLine();
}
} // namespace roefmt
