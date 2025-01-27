#pragma once
#include <string>
#include <map>

struct EventStringControlSeqDef;

class EventStringCodecUtil
{
	EventStringControlSeqDef *CheckControl(const char *start);
	std::map<int, EventStringControlSeqDef *> decDict;
	std::map<std::string, EventStringControlSeqDef *> encDist;

public:
	EventStringCodecUtil();

	std::string Encode(const std::string &in);

	std::string Decode(const std::string &in);

	std::string Decode(const char *in, size_t size = (size_t)-1);

	static EventStringCodecUtil &Instance();

};

// FIXME: 重新登记控制符
static struct EventStringControlSeqDef {
		char    name[16];
		int     parameterCount;
		char    code[8];
		int     step;
} gameStringControlSequenceDefinition[] = {
	{"ins", 1, "\x01", 1}, // special proc: \x01\xXX XX is ins type? 08-> 8 bytes following others: none
	{"02", 0, "\x02", 1},
	{"03", 0, "\x03", 1},
	{"04", 0, "\x04", 1},
	{"05", 1, "\x05", 1},
	{"06", 0, "\x06", 1},
	{"lf", 0, "\x07", 1},
	{"name", 0, "\x08", 1}, // 登入角色的名字，自己的名字
	{"09", 0, "\x09", 1},
	{"num", 1, "\x0A", 1}, // 插入一个变量
	{"sel", 0, "\x0B", 1}, // 选项文本开始标记
	{"switch", 1, "\x0C", 1}, // \x0C\xXX[xxx/xxx/xxx]
	{"0D", 1, "\x0D", 1}, //
	{"0E", 1, "\x0E", 1}, //
	{"10", 1, "\x10", 1},
	{"faith", 1, "\x11", 1}, // 亲信名？魔法名？普通字符串？
	{"int", 1, "\x12", 1}, // 插入一个变量
	{"item", 1, "\x13", 1}, // 道具名？材料名
	{"14", 1, "\x14", 1}, // 
	{"15", 1, "\x15", 1}, // 
	{"key", 1, "\x16", 1}, // 重要物品名
	{"17", 1, "\x17", 1},
	{"time", 1, "\x18", 1},
	{"19", 1, "\x19", 1},
	{"weather", 1, "\x1A", 1}, // 天气名
	{"1B", 1, "\x1B", 1}, 
	{"str", 1, "\x1C", 1}, // 一个任意的字符串？
	{"1D", 0, "\x1D", 1},
	{"wanted", 1, "\x1E", 1}, // Unity 通缉令目标？
	{"color", 1, "\x1F", 1}, // 似乎用于设置样式？颜色？ 
	{"7F", 2, "\x7F", 1}, // end? sp proc
	// {"gender", 0, "\x7F\x85", 2},
	{"val", 1, "\xEF", 1}, // 一个文本变量？
	// {"FB", 0, "\xFB", 1},
	

	{"lt", 0, "<", 1}, // < as my special char
	{"~EOD~", -1, "XXX", -1},
};
