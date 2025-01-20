#include "GameString.h"

#include "xystring.h"
#include <stdio.h>
#include <cassert>
#include <stdexcept>

const EventStringControlSeqDef *EventStringCodecUtil::CheckControl(const char *start)
{
	//printf("%d %d\n", start[0], start[1]);
	// std::string test1(start, 1), test2(start, 2); // HACK: 目前只有最大两项的控制序列，先这样

	auto i = decDict.find(*start & 0xFF);
	if (i != decDict.end()) return i->second;
	//i = decDict.find(test2);
	//if (i != decDict.end()) return i->second;

	return nullptr;
}

EventStringCodecUtil::EventStringCodecUtil()
{
	const EventStringControlSeqDef *p = gameStringControlSequenceDefinition;
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
			auto def = encDist[std::string(name)];
			ret += def->code[0];
			while (ps != std::string_view::npos)
			{
				// para parse
				auto pe = tag.find(':', ps + 1);
				auto p = tag.substr(ps + 1, pe - ps - 1);
				int v = xybase::string::stoi(p);
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

	return std::string();
}

std::string EventStringCodecUtil::Decode(const std::string &in)
{
	return Decode(in.c_str());
}

std::string EventStringCodecUtil::Decode(const char *in)
{
	xybase::StringBuilder sb;

	const char *p = in;
	while (*p)
	{
		// SJIS 双字节第一字节特别处理
		// NOTE: default char is unsigned char (switch /J in MSVC
		if ((0x81 <= *p && *p <= 0x9F) || (0xE0 <= *p && *p <= 0xEA) || *p == 0xED || *p == 0xEE)
		{
			sb += *p++;
			sb += *p++;
			continue;
		}

		auto def = CheckControl(p);
		if (def)
		{
			sb += '<';
			sb += def->name;
			p += def->step;
			//sb += '>';

			if (def->parameterCount)
			{
				for (int i = 0; i < def->parameterCount; ++i)
				{
					sb += ":";
					int v = (*p++ & 0xFF);
					if (v != 130)
					{
						sb += xybase::string::itos(v);
					}
					else
					{
						// 似然？
						sb += xybase::string::itos(v);
						sb += ":";
						sb += xybase::string::itos(*p++ & 0xFF);
						sb += ":";
						sb += xybase::string::itos(*p++ & 0xFF);
						sb += ":";
						sb += xybase::string::itos(*p++ & 0xFF);
					}
					//sb += ">";
				}
			}
			sb += '>';
		}
		else
		{
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


