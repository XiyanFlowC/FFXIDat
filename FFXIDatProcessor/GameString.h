#pragma once
#include <string>
#include <map>

struct EventStringControlSeqDef;

class EventStringCodecUtil
{
	const EventStringControlSeqDef *CheckControl(const char *start);
	std::map<int, const EventStringControlSeqDef *> decDict;
	std::map<std::string, const EventStringControlSeqDef *> encDist;

public:
	EventStringCodecUtil();

	std::string Encode(const std::string &in);

	std::string Decode(const std::string &in);

	std::string Decode(const char *in);

	static EventStringCodecUtil &Instance();

};

static const struct EventStringControlSeqDef {
		const char    name[16];
		const int     parameterCount;
		const char    code[4];
		const int     step;
} gameStringControlSequenceDefinition[] = {
	{"span", 1, "\x01", 1}, // 不确定，似乎是高亮一部分文字？
	{"05", 1, "\x05", 1},
	{"lf", 0, "\x07", 1},
	{"08", 0, "\x08", 1},
	{"num", 1, "\x0A", 1}, // 插入一个变量
	{"sel", 0, "\x0B", 1}, // 选项文本开始标记
	{"switch", 1, "\x0C", 1}, // \x0C\xXX[xxx/xxx/xxx]
	{"faith", 1, "\x11", 1}, // 亲信名？魔法名？普通字符串？
	{"item", 4, "\x13", 1}, // 道具名？材料名？
	{"key", 4, "\x16", 1}, // 重要物品名
	{"time", 1, "\x18", 1},
	{"weather", 4, "\x1A", 1}, // 天气名
	{"pc", 1, "\x1C", 1}, // 一个角色名，自身？
	{"color", 1, "\x1F", 1}, // 似乎用于设置样式？颜色？
	{"7F", 1, "\x7F", 1},
	{"A1", 0, "\xA1", 1},
	{"val", 1, "\xEF", 1}, // 一个文本变量？
	

	{"lt", 0, "<", 1}, // < as my special char
	{"~EOD~", -1, "XXX", -1},
};
