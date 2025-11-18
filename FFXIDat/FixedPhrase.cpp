#include "FixedPhrase.h"
#include "xystring.h"
#include "CsvFile.h"

#include <fstream>

void FixedPhrase::Read(std::wstring path)
{
	std::ifstream eye(path, std::ios::binary);
	if (!eye) throw std::runtime_error("Cannot open file " + xybase::string::to_string(path) + " for reading!");

	categories.clear();

	// read until EOF
	while (!eye.eof()) {
		fixed_phrase_category_header header;
		eye.read((char*)&header, sizeof(header));
		if (eye.gcount() != sizeof(header)) {
			if (eye.gcount() == 0) break; // End of file
			throw std::runtime_error("Failed to read full header from file " + xybase::string::to_string(path));
		}

		bool isEng = false;
		if (header.cat.b == 2)
			isEng = true;

		FixedPhraseCategory category;
		category.cat = header.cat;
		// Ensure proper null-termination for fixed-size char arrays
		category.categoryName = xybase::string::to_utf8(std::string(header.cat_name, strnlen(header.cat_name, 32)));
		category.categoryPron = xybase::string::to_utf8(std::string(header.cat_pron, strnlen(header.cat_pron, 32)));

		// Read entries
		char* buffer = new char[header.size];
		eye.read(buffer, header.size);
		if (eye.gcount() != header.size) {
			delete[] buffer;
			throw std::runtime_error("Failed to read full entries from file " + xybase::string::to_string(path));
		}

		char* ptr = buffer;
		for (int i = 0; i < header.count; ++i) {
			FixedPhraseCategory::FixedPhraseEntry entry;
			entry.cat = *((fixed_phrase_category*)ptr);
			ptr += sizeof(fixed_phrase_category);
			uint8_t textLen = *((uint8_t*)ptr++);
			entry.text = xybase::string::to_utf8(std::string(ptr));
			ptr += textLen;

			// Read pronunciation only if not English
			if (!isEng) {
				uint8_t pronLen = *((uint8_t*)ptr++);
				entry.pron = xybase::string::to_utf8(std::string(ptr));
				ptr += pronLen;
			}
			
			category.entries.push_back(entry);
		}

		delete[] buffer;
		categories.push_back(category);
	}
}

void FixedPhrase::Write(std::wstring path)
{
	std::ofstream output(path, std::ios::binary);
	if (!output) throw std::runtime_error("Cannot open file " + xybase::string::to_string(path) + " for writing!");

	for (const auto& category : categories) {
		fixed_phrase_category_header header;
		header.cat = category.cat;

		bool isEng = false;
		if (header.cat.b == 2)
			isEng = true;
		
		// Convert category name and pronunciation to fixed-size char arrays
		std::string catName = xybase::string::to_string(category.categoryName);
		std::string catPron = xybase::string::to_string(category.categoryPron);
		
		memset(header.cat_name, 0, 32);
		memset(header.cat_pron, 0, 32);
		
		strncpy_s(header.cat_name, 32, catName.c_str(), std::min(catName.length(), size_t(31)));
		strncpy_s(header.cat_pron, 32, catPron.c_str(), std::min(catPron.length(), size_t(31)));
		
		header.count = static_cast<int32_t>(category.entries.size());
		
		// Calculate size of entries data
		int32_t entriesSize = 0;
		for (const auto& entry : category.entries) {
			entriesSize += sizeof(fixed_phrase_category); // category
			entriesSize += 1; // text length byte
			entriesSize += static_cast<int32_t>(xybase::string::to_string(entry.text).length() + 1); // text
			if (!isEng)
			{
				entriesSize += 1; // pron length byte
				entriesSize += static_cast<int32_t>(xybase::string::to_string(entry.pron).length() + 1); // pron
			}
		}
		header.size = entriesSize;

		// Write header
		output.write((char*)&header, sizeof(header));

		// Write entries
		for (const auto& entry : category.entries) {
			output.write((char*)&entry.cat, sizeof(fixed_phrase_category));
			
			std::string text = xybase::string::to_string(entry.text);
			std::string pron = xybase::string::to_string(entry.pron);
			
			uint8_t textLen = static_cast<uint8_t>(text.length() + 1);
			uint8_t pronLen = static_cast<uint8_t>(pron.length() + 1);
			
			output.write((char*)&textLen, 1);
			output.write(text.c_str(), textLen);
			if (!isEng)
			{
				output.write((char*)&pronLen, 1);
				output.write(pron.c_str(), pronLen);
			}
		}
	}
}

void FixedPhrase::FromCsv(std::wstring path)
{
	CsvFile csv(path, std::ios::in | std::ios::binary);
	categories.clear();

	// Check if header exists and skip it
	if (!csv.IsEof()) {
		std::u8string firstCell = csv.NextCell();
		// If the first cell looks like "Category_A" or contains letters, it's likely a header
		bool isHeader = firstCell.find(u8"Category") != std::u8string::npos || 
		                firstCell.find(u8"Text") != std::u8string::npos ||
		                !std::isdigit(static_cast<unsigned char>(firstCell[0]));
		
		if (isHeader) {
			// Skip the entire header line
			csv.NextLine();
		} else {
			// Rewind to process the first line as data
			csv.Rewind();
		}
	}

	FixedPhraseCategory* currentCategory = nullptr;

	while (!csv.IsEof()) {
		// Read all fields for current row
		std::u8string catA = csv.NextCell();
		if (catA.empty()) {
			csv.NextLine();
			continue;
		}

		std::u8string catB = csv.IsEol() ? u8"" : csv.NextCell();
		std::u8string catCat = csv.IsEol() ? u8"" : csv.NextCell();
		std::u8string catEnt = csv.IsEol() ? u8"" : csv.NextCell();
		std::u8string text = csv.IsEol() ? u8"" : csv.NextCell();
		std::u8string pron = csv.IsEol() ? u8"" : csv.NextCell();
		/*std::u8string categoryName = csv.IsEol() ? u8"" : csv.NextCell();
		std::u8string categoryPron = csv.IsEol() ? u8"" : csv.NextCell();*/

		csv.NextLine();

		// Validate we have minimum required fields
		if (catA.empty() || catB.empty() || catCat.empty() || catEnt.empty()) {
			continue;
		}

		// Parse category info
		fixed_phrase_category cat;
		try {
			cat.a = static_cast<uint8_t>(xybase::string::stoi(catA));
			cat.b = static_cast<uint8_t>(xybase::string::stoi(catB));
			cat.cat = static_cast<uint8_t>(xybase::string::stoi(catCat));
			cat.ent = static_cast<uint8_t>(xybase::string::stoi(catEnt));
		} catch (const std::exception&) {
			continue; // Skip invalid rows
		}

		// Check if this is a category header (ent_idx == 0)
		if (cat.ent == 0) {
			// This is a category definition
			categories.emplace_back();
			currentCategory = &categories.back();
			currentCategory->cat = cat;
			currentCategory->categoryName = text;
			currentCategory->categoryPron = pron;
		} else {
			// This is an entry within a category
			if (!currentCategory || currentCategory->cat.cat != cat.cat) {
				// If we don't have a current category or category index doesn't match,
				// create a new category with default values
				categories.emplace_back();
				currentCategory = &categories.back();
				currentCategory->cat = cat;
				currentCategory->cat.ent = 0; // Set category ent to 0
				currentCategory->categoryName = text;
				currentCategory->categoryPron = pron;
			}

			// Create entry
			FixedPhraseCategory::FixedPhraseEntry entry;
			entry.cat = cat;
			entry.text = text;
			entry.pron = pron;

			currentCategory->entries.push_back(entry);
		}
	}
}

void FixedPhrase::ToCsv(std::wstring path)
{
	CsvFile csv(path, std::ios::out | std::ios::binary);

	// Write header
	csv.NewCell(u8"Category_A");
	csv.NewCell(u8"Category_B");
	csv.NewCell(u8"Category_Index");
	csv.NewCell(u8"Entry_Index");
	csv.NewCell(u8"Text");
	csv.NewCell(u8"Pronunciation");
	csv.NewCell(u8"Type");
	csv.NewCell(u8"Notes");
	csv.NewLine();

	for (const auto& category : categories) {
		// First, write the category header (ent_idx = 0)
		csv.NewCell(xybase::string::to_utf8(std::to_string(category.cat.a)));
		csv.NewCell(xybase::string::to_utf8(std::to_string(category.cat.b)));
		csv.NewCell(xybase::string::to_utf8(std::to_string(category.cat.cat)));
		csv.NewCell(u8"0"); // ent_idx = 0 for category
		csv.NewCell(category.categoryName);
		csv.NewCell(category.categoryPron);
		csv.NewCell(u8"CATEGORY");
		csv.NewCell(u8"Category definition");
		csv.NewLine();

		// Then write all entries in this category
		for (const auto& entry : category.entries) {
			csv.NewCell(xybase::string::to_utf8(std::to_string(entry.cat.a)));
			csv.NewCell(xybase::string::to_utf8(std::to_string(entry.cat.b)));
			csv.NewCell(xybase::string::to_utf8(std::to_string(entry.cat.cat)));
			csv.NewCell(xybase::string::to_utf8(std::to_string(entry.cat.ent)));
			csv.NewCell(entry.text);
			csv.NewCell(entry.pron);
			csv.NewCell(u8"ENTRY");
			csv.NewCell(u8""); // No notes for entries
			csv.NewLine();
		}

		// Add a blank line between categories for better readability
		csv.NewCell(u8"");
		csv.NewCell(u8"");
		csv.NewCell(u8"");
		csv.NewCell(u8"");
		csv.NewCell(u8"");
		csv.NewCell(u8"");
		csv.NewCell(u8"");
		csv.NewCell(u8"");
		csv.NewLine();
	}
}
