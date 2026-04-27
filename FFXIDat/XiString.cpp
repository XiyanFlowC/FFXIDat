#include "XiString.h"

#include <cassert>

void XiString::Read()
{
	entries.clear();
	std::ifstream eye(path, std::ios::in | std::ios::binary);
	XiStringHeader header;
	eye.read((char *)&header, sizeof(header));
	if (memcmp(header.magicHeader, "XISTRING", 8))
	{
		throw std::invalid_argument("invalid magic");
	}
	if (header.version != 0x20000)
	{
		throw std::invalid_argument("invalid version");
	}
	assert(header.zero[0] == header.zero[1] && header.zero[1] == header.zero[2] && header.zero[3] == header.zero[2] && header.zero[4] == header.zero[3]);
	assert(header.zero[0] == 0);
	assert(header.reserved == 0);
	id = header.id;

	if (header.entriesCount * 12 != header.indicesSize)
		throw std::invalid_argument("invalid header");

	std::unique_ptr<XiStringIndex[]> indices(new XiStringIndex[header.entriesCount]);
	eye.read((char *)indices.get(), header.indicesSize);
	std::unique_ptr<char[]> buffer(new char[header.dataSize]);
	eye.read(buffer.get(), header.dataSize);

	for (int i = 0; i < header.entriesCount; ++i)
	{
		// std::u8string str = xybase::string::to_utf8(std::string(buffer.get() + indices[i].offset, indices[i].size - 1));
		entries.push_back(StringEntry(std::string(buffer.get() + indices[i].offset, indices[i].size - 1), indices[i].flag1, indices[i].flag2, indices[i].flag3));
	}
}

void XiString::Write()
{
	std::ofstream pen(path, std::ios::out | std::ios::binary);
	XiStringHeader header;
	memset(header.zero, 0, sizeof(header.zero));
	header.reserved = 0;
	memcpy(header.magicHeader, "XISTRING", 8);
	header.version = 0x20000;
	header.id = id;
	header.entriesCount = entries.size();
	header.indicesSize = header.entriesCount * sizeof(XiStringIndex);
	header.dataSize = 0;
	for (const StringEntry &entry : entries)
	{
		header.dataSize += entry.str.size() + 1;
	}
	header.fileSize = header.dataSize + header.indicesSize + sizeof(header);

	pen.write((char *)&header, sizeof(header));
	int offset = 0;
	for (const StringEntry &entry : entries)
	{
		XiStringIndex index;
		index.offset = offset;
		index.size = entry.str.size() + 1;
		offset += index.size;
		index.flag1 = entry.flag1;
		index.flag2 = entry.flag2;
		index.flag3 = entry.flag3;
		pen.write((char *)&index, sizeof(XiStringIndex));
	}

	for (const StringEntry &entry : entries)
	{
		auto &s = entry.str;
		for (int i = 0; i < s.size(); ++i)
		{
			pen.put(s[i]);
		}
		pen.put(0);
	}
}

#include "CsvFile.h"

void XiString::ToCsv(std::filesystem::path path)
{
	CsvFile csv(path, std::ios::out | std::ios::binary);
	csv.NewCell(xybase::string::itos<char8_t>(id));
	csv.NewLine();
	for (StringEntry &entry : entries)
	{
		csv.NewCell(xybase::string::itos<char8_t>(entry.flag1));
		csv.NewCell(xybase::string::itos<char8_t>(entry.flag2));
		csv.NewCell(xybase::string::itos<char8_t>(entry.flag3));
		csv.NewCell(xybase::string::to_utf8(Decode(entry.str)));
		csv.NewLine();
	}
	csv.Close();
}

void XiString::FromCsv(std::filesystem::path path)
{
	CsvFile csv(path, std::ios::in | std::ios::binary);
	id = xybase::string::stoi<char8_t>(csv.NextCell());
	csv.NextLine();
	entries.clear();
	while (!csv.IsEof())
	{
		StringEntry e;
		e.flag1 = xybase::string::stoi(csv.NextCell());
		e.flag2 = xybase::string::stoi(csv.NextCell());
		e.flag3 = xybase::string::stoi(csv.NextCell());
		e.str = Encode(xybase::string::to_string(csv.NextCell()));
		entries.push_back(std::move(e));
		csv.NextLine();
	}
}

std::string XiString::Encode(const std::string &in)
{
	xybase::StringBuilder sb;
	const char *p = in.c_str();

	while (*p)
	{
		if (*p == '\\')
		{
			++p;
			if (*p == 'x')
			{
				std::string hex(++p, 2);
				int val = xybase::string::stoi(hex, 16);
				sb += (char)val;
				p += 2;
			}
			else
				sb += *p++;
		}
		else if (*p == '$')
		{
			++p;
			if (*p == '$')
			{
				sb += '$';
				++p;
			}
			else if (strncmp(p, "if:", 3) == 0)
			{
				p += 3;
				std::string param1 = std::string(p, 2); p += 2;
				if (*p != ':') throw std::runtime_error("Expected ':' after first param in $if");
				++p;
				std::string param2 = std::string(p, 2); p += 2;
				if (*p != ':') throw std::runtime_error("Expected ':' after second param in $if");
				++p;
				std::string param3 = std::string(p, 2); p += 2;
				if (*p != ':') throw std::runtime_error("Expected ':' after third param in $if");
				++p;
				std::string param4 = std::string(p, 2); p += 2;
				if (*p != ';') throw std::runtime_error("Expected ';' after $if");
				++p;

				sb += "\xFA\x40\x83\x01";
				sb += Encode("\\" + std::string("x") + param1);
				sb += Encode("\\" + std::string("x") + param2);
				sb += Encode("\\" + std::string("x") + param3);
				sb += Encode("\\" + std::string("x") + param4);
			}
			else if (strncmp(p, "switch:", 7) == 0)
			{
				p += 7;
				std::vector<std::string> params;

				// Parse first parameter (1 byte = 2 hex chars)
				params.push_back(std::string(p, 2));
				p += 2;
				if (*p == ':') ++p;
				else throw std::runtime_error("Expected ':' after first param in $switch");

				// Parse remaining parameters (2 bytes each = 4 hex chars)
				while (*p != ';')
				{
					params.push_back(std::string(p, 4));
					p += 4;
					if (*p == ':') ++p;
					else if (*p != ';') throw std::runtime_error("Expected ':' or ';' after parameter in $switch");
				}
				if (*p != ';') throw std::runtime_error("Expected ';' after $switch");
				++p;

				if (params.size() < 2) throw std::runtime_error("$switch requires at least 2 parameters");

				// Calculate case count from actual parameter count
				// Format: param0 (1 byte) + base param (2 bytes) + N case params (2 bytes each)
				// So caseCount = total 2-byte params - 1 (base param) = params.size() - 1 - 1
				int caseCount = params.size() - 2;  // -1 for param0, -1 for base param

				// Encode case count as little-endian 2 bytes
				int caseCountByte1 = caseCount & 0xFF;
				int caseCountByte2 = (caseCount >> 8) & 0xFF;

				sb += "\xFA\x40\x82\x01";
				// First param: 1 byte
				sb += (char)xybase::string::stoi(params[0], 16);
				// Case count: 2 bytes (each XOR 0x80)
				sb += (char)(caseCountByte1 ^ 0x80);
				sb += (char)(caseCountByte2 ^ 0x80);
				// Remaining params: 2 bytes each (1 base + caseCount cases)
				for (int i = 1; i < params.size(); ++i)
				{
					sb += (char)xybase::string::stoi(params[i].substr(0, 2), 16);
					sb += (char)xybase::string::stoi(params[i].substr(2, 2), 16);
				}
			}
			else if (strncmp(p, "time:", 5) == 0)
			{
				p += 5;
				std::string param1 = std::string(p, 2); p += 2;
				if (*p != ':') throw std::runtime_error("Expected ':' in $time");
				++p;
				std::string param2 = std::string(p, 2); p += 2;
				if (*p != ':') throw std::runtime_error("Expected ':' in $time");
				++p;
				std::string param3 = std::string(p, 2); p += 2;
				if (*p != ':') throw std::runtime_error("Expected ':' in $time");
				++p;
				std::string param4 = std::string(p, 2); p += 2;
				if (*p != ';') throw std::runtime_error("Expected ';' after $time");
				++p;

				sb += "\xFA\x40\x81\x85";
				sb += Encode("\\" + std::string("x") + param1);
				sb += Encode("\\" + std::string("x") + param2);
				sb += Encode("\\" + std::string("x") + param3);
				sb += Encode("\\" + std::string("x") + param4);
			}
			else if (strncmp(p, "num:", 4) == 0)
			{
				p += 4;
				std::string param1 = std::string(p, 2); p += 2;
				if (*p != ';') throw std::runtime_error("Expected ';' after $num");
				++p;

				sb += "\xFA\x40\x8B\x01";
				sb += Encode("\\" + std::string("x") + param1);
			}
			else if (strncmp(p, "str:", 4) == 0)
			{
				p += 4;
				std::string param1 = std::string(p, 2); p += 2;
				if (*p != ':') throw std::runtime_error("Expected ':' in $str");
				++p;
				std::string param2 = std::string(p, 2); p += 2;
				if (*p != ';') throw std::runtime_error("Expected ';' after $str");
				++p;

				sb += "\xFA\x40\x8C\x03";
				sb += Encode("\\" + std::string("x") + param1);
				sb += Encode("\\" + std::string("x") + param2);
			}
			else if (strncmp(p, "item:", 5) == 0)
			{
				p += 5;
				std::string param1 = std::string(p, 2); p += 2;
				if (*p != ':') throw std::runtime_error("Expected ':' in $item");
				++p;
				std::string param2 = std::string(p, 2); p += 2;
				if (*p != ':') throw std::runtime_error("Expected ':' in $item");
				++p;
				std::string param3 = std::string(p, 2); p += 2;
				if (*p != ':') throw std::runtime_error("Expected ':' in $item");
				++p;
				std::string param4 = std::string(p, 2); p += 2;
				if (*p != ':') throw std::runtime_error("Expected ':' in $item");
				++p;
				std::string param5 = std::string(p, 2); p += 2;
				if (*p != ':') throw std::runtime_error("Expected ':' in $item");
				++p;
				std::string param6 = std::string(p, 2); p += 2;
				if (*p != ';') throw std::runtime_error("Expected ';' after $item");
				++p;

				sb += "\xFA\x40\x8E\x01";
				sb += Encode("\\" + std::string("x") + param1);
				sb += Encode("\\" + std::string("x") + param2);
				sb += Encode("\\" + std::string("x") + param3);
				sb += Encode("\\" + std::string("x") + param4);
				sb += Encode("\\" + std::string("x") + param5);
				sb += Encode("\\" + std::string("x") + param6);
			}
			else if (strncmp(p, "sep;", 4) == 0)
			{
				p += 4;
				sb += "\xFA\x40\x8D";
			}
			else if (strncmp(p, "ne;", 3) == 0 || strncmp(p, "eq;", 3) == 0 || 
					 strncmp(p, "le;", 3) == 0 || strncmp(p, "ge;", 3) == 0 ||
					 strncmp(p, "lt;", 3) == 0 || strncmp(p, "gt;", 3) == 0)
			{
				char op = 0;
				if (p[0] == 'n') op = 0x85;
				else if (p[0] == 'e') op = 0x86;
				else if (p[0] == 'l' && p[1] == 'e') op = 0x87;
				else if (p[0] == 'g' && p[1] == 'e') op = 0x88;
				else if (p[0] == 'l' && p[1] == 't') op = 0x89;
				else if (p[0] == 'g' && p[1] == 't') op = 0x8A;

				p += 3;

				const char *content_start = p;
				while (*p && !(*p == '$' && (strncmp(p+1, "else;", 5) == 0 || strncmp(p+1, "endif;", 6) == 0)))
					++p;

				std::string content(content_start, p - content_start);
				std::string encoded_content = Encode(content);

				sb += "\xFA\x40";
				sb += op;
				sb += (char)encoded_content.size() + 4; // the length including FA 40 and op and length byte itself
				sb += encoded_content;
			}
			else if (strncmp(p, "else;", 5) == 0)
			{
				p += 5;

				const char *content_start = p;
				while (*p && !(*p == '$' && strncmp(p+1, "endif;", 6) == 0))
					++p;

				std::string content(content_start, p - content_start);
				std::string encoded_content = Encode(content);

				sb += "\xFA\x40\x84";
				sb += (char)encoded_content.size();
				sb += encoded_content;
			}
			else if (strncmp(p, "endif;", 6) == 0)
			{
				p += 6;
			}
			else
			{
				throw std::runtime_error(std::string("Unknown control sequence: $") + std::string(p, 10));
			}
		}
		else if (*p >= 0x80)
		{
			sb += *p++;
			sb += *p++;
		}
		else
			sb += *p++;
	}

	return sb.ToString();
}

std::string XiString::Decode(const std::string &in)
{
	const char *p = in.c_str();
	size_t consumed = 0;
	bool hasElse = false;
	return DecodeInternal(p, in.c_str() + in.size(), consumed, &hasElse);
}

std::string XiString::DecodeInternal(const char *p, const char *end, size_t &consumed, bool *hasElse)
{
	xybase::StringBuilder sb;
	const char *start = p;

	while (p < end && *p)
	{
		if (*p == '\xFA' && p + 1 < end && *(p+1) == '\x40')
		{
			p += 2;

			switch (*p)
			{
			case 0x81: // $time:param;
				{
					++p;
					if (*p != 0x85) throw std::runtime_error("Invalid time sequence");
					++p;

					sb += "$time:";
					for (int i = 0; i < 4; ++i)
					{
						int cb = (unsigned char)*p++;
						if (cb < 16) sb += '0';
						sb += xybase::string::itos<char>(cb, 16).c_str();
						if (i < 3) sb += ':';
					}
					sb += ';';
				}
				break;
			case 0x82: // $switch:param;
				{
					++p;
					if (*p != 0x01) throw std::runtime_error("Invalid switch sequence");
					++p;

					sb += "$switch:";

					// First parameter: 1 byte
					int cb = (unsigned char)*p++;
					if (cb < 16) sb += '0';
					sb += xybase::string::itos<char>(cb, 16).c_str();

					// Case count: 2 bytes (little-endian, each byte XOR 0x80)
					int caseCountByte1 = ((unsigned char)*p++) ^ 0x80;
					int caseCountByte2 = ((unsigned char)*p++) ^ 0x80;
					int caseCount = caseCountByte1 | (caseCountByte2 << 8);

					// Note: We don't output caseCount to CSV, it will be recalculated during encoding

					// Read 1 + caseCount parameters (each 2 bytes)
					int totalParams = 1 + caseCount;
					for (int i = 0; i < totalParams; ++i)
					{
						sb += ':';
						int byte1 = (unsigned char)*p++;
						int byte2 = (unsigned char)*p++;
						if (byte1 < 16) sb += '0';
						sb += xybase::string::itos<char>(byte1, 16).c_str();
						if (byte2 < 16) sb += '0';
						sb += xybase::string::itos<char>(byte2, 16).c_str();
					}
					sb += ';';
				}
				break;
			case 0x83: // $if:param;
				{
					++p;
					if (*p != 0x01) throw std::runtime_error("Invalid if sequence");
					++p;

					sb += "$if:";
					for (int i = 0; i < 4; ++i)
					{
						int cb = (unsigned char)*p++;
						if (cb < 16) sb += '0';
						sb += xybase::string::itos<char>(cb, 16).c_str();
						if (i < 3) sb += ':';
					}
					sb += ';';/*

					bool ifHasElse = false;
					size_t ifConsumed = 0;
					std::string ifContent = DecodeInternal(p, end, ifConsumed, &ifHasElse);
					sb += ifContent;

					p += ifConsumed;

					sb += "$endif;";*/
				}
				break;
			case 0x84: // $else;
				{
					++p;
					int length = (unsigned char)*p++;

					sb += "$else;";

					size_t elseConsumed = 0;
					std::string content = DecodeInternal(p, p + length, elseConsumed, nullptr);
					sb += content;

					p += length;

					if (hasElse) *hasElse = true;
					consumed = p - start;
					return sb.ToString();
				}
				break;
			case 0x85: // ne?
			case 0x86: // eq?
			case 0x87: // le?
			case 0x88: // ge?
			case 0x89: // lt?
			case 0x8A: // gt?
				{
					char op = *p++;
					int length = (unsigned char)*p++;

					switch (op)
					{
					case 0x85: sb += "$ne;"; break;
					case 0x86: sb += "$eq;"; break;
					case 0x87: sb += "$le;"; break;
					case 0x88: sb += "$ge;"; break;
					case 0x89: sb += "$lt;"; break;
					case 0x8A: sb += "$gt;"; break;
					}

					bool opHasElse = false;
					size_t opConsumed = 0;
					std::string content = DecodeInternal(p, p + length - 4, opConsumed, &opHasElse);
					sb += content;
					p += length - 4;

					opHasElse = false;
					opConsumed = 0;
					content = DecodeInternal(p, end, opConsumed, &opHasElse);
					if (opHasElse)
					{
						// now we have the body that ONLY contains the else part, we need to mark an end for it
						sb += content;
						sb += "$endif;";
						// now, just proceed
						p += opConsumed;
					}
					else
					{
						// no else part, means DecodeInternal returns a whole content outside if block
						sb += "$endif;";
						sb += content;
						return sb.ToString(); // now, return, all content is processed, we are done
					}

				}
				break;
			case 0x8B: // $num:param;
				{
					++p;
					if (*p != 0x01) throw std::runtime_error("Invalid num sequence");
					++p;

					sb += "$num:";
					int cb = (unsigned char)*p++;
					if (cb < 16) sb += '0';
					sb += xybase::string::itos<char>(cb, 16).c_str();
					sb += ';';
				}
				break;
			case 0x8C: // $str:param;
				{
					++p;
					if (*p != 0x03) throw std::runtime_error("Invalid str sequence");
					++p;

					sb += "$str:";
					for (int i = 0; i < 2; ++i)
					{
						int cb = (unsigned char)*p++;
						if (cb < 16) sb += '0';
						sb += xybase::string::itos<char>(cb, 16).c_str();
						if (i < 1) sb += ':';
					}
					sb += ';';
				}
				break;
			case 0x8D: // $sep;
				++p;
				sb += "$sep;";
				break;
			case 0x8E: // $item:param;
				{
					++p;
					if (*p != 0x01) throw std::runtime_error("Invalid item sequence");
					++p;

					sb += "$item:";
					for (int i = 0; i < 6; ++i)
					{
						int cb = (unsigned char)*p++;
						if (cb < 16) sb += '0';
						sb += xybase::string::itos<char>(cb, 16).c_str();
						if (i < 5) sb += ':';
					}
					sb += ';';
				}
				break;
			default:
				throw std::runtime_error(std::string("Unknown control code: ") + xybase::string::itos<char>(*p, 16));
			}
		}
		else if (*p <= 0x7F)
		{
			if (*p == '\\')
				sb += "\\\\", ++p;
			else if (*p == '$')
				sb += "$$", ++p;
			else
				sb += *p++;
		}
		else
		{
			sb += *p++;
			sb += *p++;
		}
	}

	consumed = p - start;
	return sb.ToString();
}

// UNDONE: 很可能是错的，但\x01的概念无法分析
// 很可能是类似XIV的机制，但是没有直接定义的长度标志。
// 等待样本分析
struct XiStringControlSequenceStepDefinition {
	char code[4];
	char type[12];
	int step;
} xiStringControlSequenceStepDefinitions[] = {
	// 80 ???
	{"", "", -1},
	{"\x81\x85", "", 2 + 3},
	{"\x82\x01", "", 2 + 11}, // if?
	{"\x83\x01", "", 2 + 4},
	{"\x84\x02", "", 2 + 0},
	{"\x84\x05", "", 2 + 0},
	{"\x84\x0A", "", 2 + 0},
	{"\x84\x10", "", 2 + 0},
	{"\x85\x11", "", 2 + 0},
	{"\x85\x1D", "", 2 + 0},
	{"\x85\x2A", "", 2 + 0},
	{"\x86\x08", "", 2 + 0},
	{"\x86\x0D", "", 2 + 0},
	{"\x89\x0A", "", 2 + 0},
	{"\x89\x06", "", 2 + 0},
	{"\x8B\x01", "", 2 + 1},
	{"\x8C\x03", "", 2 + 2},
	// 8D sep?
	{"\x8E\x01", "", 2 + 6},
	{"", "", -1}
};

int XiString::GetStep(const char *p)
{
	if (p[0] != 0xFA || p[1] != 0x40) return 0;
	p += 2;

	/*if (p[0] == 0x8D)
	{
		return 3;
	}*/

	//auto *d = xiStringControlSequenceStepDefinitions;
	//while (d->step != -1)
	//{
	//	if (d->code[0] == p[0] && d->code[1] == p[1])
	//	{
	//		return 2/*start marker*/ + d->step/*step*/;
	//	}

	//	++d;
	//}

	switch (p[0])
	{
	case 0x81: // $time:param;
		return 2 + 1 + 4; // time insertion
		// e.g. <FA 40 81 85 81 01 81>
	case 0x82: // $switch:param;
		if (p[1] != 0x01) break;
		return 2 + 1 + 1 + 1 + 2 + 2 + 2 * ((p[3] ^ 0x80)); // 2(FA 40) + 3(82 01 81) + 2(caseCount) + 2(base) + 2*caseCount(cases)
		// e.g. <FA 40 82 01 81 83 80 A0 80 8C 80 92 80 98 80>
		//       |     |  |  |  |     |     |     |     +-- case 2 (98 80 ^ 0x80 = 18 00)
		//       |     |  |  |  |     |     |     +-- case 1 (92 80 ^ 0x80 = 12 00)
		//       |     |  |  |  |     |     +-- case 0 (8C 80 ^ 0x80 = 0C 00)
		//       |     |  |  |  |     +-- base param (A0 80 ^ 0x80 = 20 00)
		//       |     |  |  |  +-- case count byte 1 (83 ^ 0x80 = 03)
		//       |     |  |  +-- param0 (0x81)
		//       |     |  +-- subtype (0x01)
		//       |     +-- switch opcode (0x82)
		//       +-- control sequence marker
	case 0x83: // $if:param;
		if (p[1] != 0x01) break; 
		return 2 + 1 + 4; // if condition starter. followed by one variable(and two entities for true/false cases)
		// e.g. <FA 40 83 01 81 03 80 80> 
	case 0x84: // $else;
		return 2 + 1 + 1 + p[1]; // else for if condition? followed by a string length (p[1]) and string
		// e.g. <FA 40 84 05 XX XX XX XX XX> or <FA 40 84 02 XX XX>
	
	// NOTE: 87 88 and 8A seem to be related to if conditions by their code near NE, EQ and LT, but their exact meaning is unclear.
	// A reverse engineering of the game client would be needed to determine their exact behavior. For now, we can only guess based on observed patterns in the data. And if thye occur in the future, we can analyze them based on their context and content.
	case 0x85: // ne
	case 0x86: // eq
	case 0x87: // guessed, not observed, but seems to be le? or gt?
	case 0x88: // guessed, not observed, but seems to be ge? or ge?
	case 0x89: // lt
	case 0x8A: // guessed, not observed, but seems to be gt? or le?
		return p[1]; // size indicator of if mode (lt/gt/etc?) case? followed by a total length (step) indicator (p[1]) and content
		// e.g. <FA 40 8A 06 XX XX> or <FA 40 86 0A XX XX XX XX XX XX XX XX>
		// It is possible that another FA 40 in 85-8A section:
		// e.g. <FA 40 89 0A FA 40 8C 03 80 80>
	case 0x8B: // $num:param;
		if (p[1] != 0x01) break;
		return 2 + 1 + 2; // number insertion
		// e.g. <FA 40 8B 01 81>
	case 0x8C: // $str:param;
		if (p[1] != 0x03) break;
		//return 2 + 1 + p[1]; // string insertion?
		return 2 + 1 + 2;
		// e.g. <FA 40 8C 03 80 80> only 03 observed, is it really a length indicator?
	case 0x8D: // $sep;
		return 2 + 1; // seperator (for switch cases)
		// i.e. <FA 40 8D>
	case 0x8E: // $item:param;
		return 2 + 1 + 1 + 6; // item name insertion?
		// e.g. <FA 40 8E 01 XX XX XX XX XX XX>
	default:
		break;
	}

	printf("%02X%02X\n", p[0], p[1]);
	throw std::runtime_error("unknown ctrl char.");
}
