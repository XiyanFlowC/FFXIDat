#include "EventString.h"

#include "xystring.h"
#include <stdio.h>
#include <cassert>
#include <stdexcept>

EventStringControlSeqDef *EventStringCodecUtil::CheckControl(const char *start, const char *end) // FIXME: 使用结构体统一控制
{
	//printf("%d %d\n", start[0], start[1]);
	// std::string test1(start, 1), test2(start, 2); // HACK: 目前只有最大两项的控制序列，先这样
	if (*start == 0)
	{
		if (start + 1 >= end)
			return nullptr;
		if (start[1] != 0x20 && start[1] != 0x07) {
			return nullptr;
		}
		static EventStringControlSeqDef end{ "-", 0, "\x00", 1 };
		int p = 1;
		end.parameterCount = 0;
		while (start[p++] != '\x07')
			end.parameterCount += 1;
		return &end;
	}

	auto i = decDict.find(*start & 0xFF);
	if (i != decDict.end())
	{
		if (*start == 0x7F)
		{
			// Followed by the next sentence, separated by a 00(should be a parameter of 7F?) and 07(line feed)
			//             Supposed to be a parameter of 7F as any else?
			//             |
			// XX XX 7F 31 00 07 YY YY
			// |                 |
			// |                 Next string pointer points
			// This string pointer points
			// Perhaps the Square use this as a fall-through?
			// I stop the extration here, for repeatition extraction is awful
			// Have no idea how this works.
			// This special usage does not seen in English files, why?
			// static EventStringControlSeqDef con7f31{"-", 0, "\x7F\x31\x00", 3}; // followed by a string ended with \x07?
			// \x7F\xFX seems have only one parameter
			static EventStringControlSeqDef con7f1v{ "7F", 1, "\x7F", 1 };
			static EventStringControlSeqDef gender{ "gender", 0, "\x7F\x85", 2 };
			/*if (start[1] == 0x31 && start[2] == 0)
			{
				int p = 3;
				con7f31.parameterCount = 0;
				while (start[p++] != '\x07')
					con7f31.parameterCount += 1;
				return &con7f31;
			}*/
			if (start[1] == 0x85)
			{
				return &gender;
			}
			if (start[1] > 0xF0 || start[1] == 0x31)
			{
				return &con7f1v;
			}

			return i->second;
		}

		return i->second;
	}
	//i = decDict.find(test2);
	//if (i != decDict.end()) return i->second;

	return nullptr;
}

EventStringCodecUtil::EventStringCodecUtil()
{
	EventStringControlSeqDef *p = gameStringControlSequenceDefinition;
	while (p->parameterCount != -1)
	{
		encDist[p->name] = p;
		decDict[p->code[0] & 0xFF] = p; // FIXME: 通用化
		++p;
	}
}

std::string EventStringCodecUtil::Encode(const std::string &in)
{
	// NOTE: 需要直接对string 写入，字符串内存在0值！！！
	std::string ret;

	std::string_view str(in);

	for (size_t i = 0; i < in.size(); ++i)
	{
		// 控制序列处理
		if (str[i] == '<')
		{
			bool endFlag = false;
			size_t end = str.find('>', i);

			if (end == std::string::npos)
				throw std::invalid_argument("ctrl tag no end");

			std::string_view tag = str.substr(i + 1, end - i - 1);

			// Name parse
			auto ps = tag.find(':');
			auto name = tag.substr(0, ps);

			if (name == "-") // a \0 .* \7 seq
			{
				// It should be safe as for now I only seen it as an ending
				// ret += "\x7F\x31";
				ret += '\0';
				// ret += '\x07';
				// return ret;
				endFlag = true;
			}
			else if (name == "gender") // FIXME: 使用结构体统一控制
			{
				ret += "\x7F\x85";
				i = end;
				continue;
			}
			else
			{
				auto def = encDist.find(std::string(name));

				if (def == encDist.end())
				{
					ret += xybase::string::stoi(name, 16);
				}
				else
					ret += def->second->code[0]; // FIXME: 通用化
			}

			while (ps != std::string_view::npos)
			{
				// para parse
				auto pe = tag.find(':', ps + 1);
				auto p = tag.substr(ps + 1, pe - ps - 1);
				int v = xybase::string::stoi(p, 16);
				ret += v & 0xFF;
				ps = pe;
			}

			if (endFlag)
			{
				ret += '\x07';
				return ret;
			}

			i = end;
		}
		else
		{
			ret += in[i];
		}
	}

	ret += '\x00';
	return ret;
}

std::string EventStringCodecUtil::Decode(const std::string &in)
{
	return Decode(in.c_str(), in.size());
}

std::string EventStringCodecUtil::Decode(const char *in, size_t limit)
{
	xybase::StringBuilder sb;
	const char *end = limit == (size_t)-1 ? (const char *) -1 : in + limit;

	const char *p = in;
	while (/**p &&*/ p < end)
	{
		// SJIS 双字节第一字节特别处理
		// NOTE: default char is unsigned char (switch /J in MSVC
		if ((0x81 <= *p && *p <= 0x9F) || (0xE0 <= *p && *p <= 0xEA) || *p == 0xED || *p == 0xEE)
		{
			sb += *p++;
			sb += *p++;
			continue;
		}

		// ins, special proc
		if (*p == 0x01)
		{
			sb += "<ins";
			const char *end = p + 2 + p[1];
			p += 1;

			while (p < end)
			{
				sb += ':';
				int v = (*p++ & 0xFF);
				sb += xybase::string::itos(v, 16);
			}

			sb += '>';

			continue;
		}

		auto def = CheckControl(p, end);
		if (def)
		{
			bool endFlag = false;

			sb += '<';
			sb += def->name;
			endFlag = def->name[0] == '-';
			p += def->step;
			// printf("%s:\n", def->name);
			//sb += '>';

			if (def->parameterCount)
			{
				for (int i = 0; i < def->parameterCount; ++i)
				{
					sb += ":";
					int v = (*p++ & 0xFF);
					sb += xybase::string::itos(v, 16);
				}
			}
			sb += '>';

			if (endFlag)
				return sb.ToString();
		}
		else if (*p > 0x7F)
		{
			sb += '<';
			sb += xybase::string::itos(*p++ & 0xFF, 16);
			sb += '>';
		}
		else if (*p == '\0')
		{
			break;
		}
		else
		{
			assert(*p >= '\x20' && *p <= '\x7E');
			sb += *p++;
		}
	}

	return sb.ToString();
}

EventStringCodecUtil &EventStringCodecUtil::Instance()
{
	static EventStringCodecUtil _inst;
	return _inst;
}


