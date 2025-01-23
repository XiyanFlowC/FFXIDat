#include "EventString.h"

#include "xystring.h"
#include <stdio.h>
#include <cassert>
#include <stdexcept>

EventStringControlSeqDef *EventStringCodecUtil::CheckControl(const char *start)
{
	//printf("%d %d\n", start[0], start[1]);
	// std::string test1(start, 1), test2(start, 2); // HACK: 目前只有最大两项的控制序列，先这样

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
			static EventStringControlSeqDef con7f31{"-", 0, "\x7F\x31", 2}; // followed by a \x00\x07?
			// \x7F\xFX seems have only one parameter
			static EventStringControlSeqDef con7ffx{ "7F", 1, "\x7F", 1 };
			if (start[1] == 0x31)
			{
				return &con7f31;
			}
			if (start[1] > 0xF0)
			{
				return &con7ffx;
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
		decDict[p->code[0] & 0xFF] = p;
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
			size_t end = str.find('>', i);

			if (end == std::string::npos)
				throw std::invalid_argument("ctrl tag no end");

			std::string_view tag = str.substr(i + 1, end - i - 1);

			// Name parse
			auto ps = tag.find(':');
			auto name = tag.substr(0, ps);

			if (name == "-")
			{
				// It should be safe as for now I only seen it as an ending
				ret += "\x7F\x31";
				ret += '\0';
				ret += '\x07';
				return ret;
			}

			auto def = encDist.find(std::string(name));

			if (def == encDist.end())
				throw std::invalid_argument("unkown ctrl ch");

			ret += def->second->code[0];
			while (ps != std::string_view::npos) // UNDONE
			{
				// para parse
				auto pe = tag.find(':', ps + 1);
				auto p = tag.substr(ps + 1, pe - ps - 1);
				int v = xybase::string::stoi(p, 16);
				ret += v & 0xFF;
				ps = pe;
			}

			i = end;
		}
		else
		{
			ret += in[i];
		}
	}

	ret += '\0';
	ret += '\x07';
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
	while (*p && p < end)
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
				/*if ((v & 0xF0) == 0x80)
				{
					sb += '$';
					sb += xybase::string::itos(v & 0xFF, 16);
					assert((*p & 0xF0) >= 0x80);
					sb += xybase::string::itos(*p++ & 0xFF, 16);
					assert((*p & 0xF0) >= 0x80);
					sb += xybase::string::itos(*p++ & 0xFF, 16);
					assert((*p & 0xF0) >= 0x80);
					sb += xybase::string::itos(*p++ & 0xFF, 16);
				}
				else
				{*/
					sb += xybase::string::itos(v, 16);
				//}
			}

			sb += '>';

			continue;
		}

		auto def = CheckControl(p);
		if (def)
		{
			sb += '<';
			sb += def->name;
			p += def->step;
			// printf("%s:\n", def->name);
			//sb += '>';

			if (def->parameterCount)
			{
				for (int i = 0; i < def->parameterCount; ++i)
				{
					sb += ":";
					int v = (*p++ & 0xFF);
					//printf("%X\n", v);
					/*if ((v & 0xFF) != 0x82)
					{*/
						sb += xybase::string::itos(v, 16);
					//}
					//else
					//{
					//	// 似然？
					//	sb += '$';
					//	sb += xybase::string::itos(v & 0xFF, 16);
					//	assert((*p & 0xF0) >= 0x80);
					//	sb += xybase::string::itos(*p++ & 0xFF, 16);
					//	assert((*p & 0xF0) >= 0x80);
					//	sb += xybase::string::itos(*p++ & 0xFF, 16);
					//	assert((*p & 0xF0) >= 0x80);
					//	sb += xybase::string::itos(*p++ & 0xFF, 16);
					//}
				}
			}
			sb += '>';
		}
		else if (*p > 0x7F)
		{
			sb += '<';
			sb += xybase::string::itos(*p++ & 0xFF, 16);
			sb += '>';
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


