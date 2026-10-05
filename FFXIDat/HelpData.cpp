#include "HelpData.h"

#include <map>
#include <set>
#include <vector>
#include <xystring.h>

#include "CsvFile.h"

namespace
{
std::vector<std::u8string_view> Columns(const std::vector<helpfmt::Datum> &records)
{
	const size_t count = records.empty() ? 2 : records.front().row().GetCellsConst().size();
	if (count != 2 && count != 5)
		throw std::runtime_error("Invalid help CSV text layout");
	for (const auto &datum : records)
		if (datum.row().GetCellsConst().size() != count)
			throw std::runtime_error("Mixed help CSV text layouts");
	if (count == 5)
		return { u8"id", u8"index", u8"unknown", u8"reserved", u8"title",
			u8"text_integer", u8"name_2", u8"name_3", u8"description" };
	return { u8"id", u8"index", u8"unknown", u8"reserved", u8"title", u8"description" };
}

std::u8string Number(int64_t value)
{
	return xybase::string::to_utf8(std::to_string(value));
}
}

void HelpData::ToICsv(const std::wstring &path) const
{
	const auto columns = Columns(data);
	CsvFile csv(path, std::ios::out | std::ios::binary);
	for (auto column : columns)
		csv.NewCell(std::u8string(column));
	csv.NewLine();
	for (const auto &datum : data)
	{
		csv.NewCell(Number(datum.id));
		csv.NewCell(Number(datum.index));
		csv.NewCell(Number(datum.unknown));
		csv.NewCell(Number(datum.reserved));
		for (const auto &cell : datum.row().GetCellsConst())
			csv.NewCell(cell.GetType() == 0 ? cell.Get<std::u8string>() : Number(cell.Get<int>()));
		csv.NewLine();
	}
	csv.Close();
}

void HelpData::FromCsv(const std::wstring &path)
{
	const auto columns = Columns(data);
	CsvFile csv(path, std::ios::in | std::ios::binary);
	auto readRow = [&]() {
		std::vector<std::u8string> row(columns.size());
		for (auto &cell : row)
		{
			if (csv.IsEof() || csv.IsEol())
				throw std::runtime_error("Incomplete help CSV row");
			cell = csv.NextCell();
		}
		if (!csv.IsEol() && !csv.IsEof())
			throw std::runtime_error("Unexpected help CSV column");
		csv.NextLine();
		return row;
	};
	const auto header = readRow();
	for (size_t i = 0; i < columns.size(); ++i)
		if (header[i] != columns[i])
			throw std::runtime_error("Invalid help CSV header");

	auto records = data;
	std::map<std::u8string, size_t> byId;
	for (size_t i = 0; i < records.size(); ++i)
		if (!byId.emplace(Number(records[i].id), i).second)
			throw std::runtime_error("Duplicate help record ID");
	std::set<size_t> seen;
	while (!csv.IsEof())
	{
		const auto row = readRow();
		const auto it = byId.find(row[0]);
		if (it == byId.end() || !seen.insert(it->second).second)
			throw std::runtime_error("Unknown or duplicate help CSV ID");
		auto &datum = records[it->second];
		if (row[1] != Number(datum.index) || row[2] != Number(datum.unknown) ||
			row[3] != Number(datum.reserved))
			throw std::runtime_error("Help CSV metadata must match the original DAT");
		auto &cells = datum.row().GetCells();
		for (size_t i = 0; i < cells.size(); ++i)
		{
			if (cells[i].GetType() == 0)
				cells[i].Set(row[i + 4]);
			else if (row[i + 4] != Number(cells[i].Get<int>()))
				throw std::runtime_error("Help CSV text integer must match the original DAT");
		}
	}
	if (seen.size() != records.size())
		throw std::runtime_error("Help CSV must contain every original record");
	Serialize(records);
	data = std::move(records);
}
