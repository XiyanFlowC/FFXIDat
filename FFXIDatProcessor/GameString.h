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
	{"span", 1, "\x01", 1}, // ��ȷ�����ƺ��Ǹ���һ�������֣�
	{"05", 1, "\x05", 1},
	{"lf", 0, "\x07", 1},
	{"08", 0, "\x08", 1},
	{"num", 1, "\x0A", 1}, // ����һ������
	{"sel", 0, "\x0B", 1}, // ѡ���ı���ʼ���
	{"switch", 1, "\x0C", 1}, // \x0C\xXX[xxx/xxx/xxx]
	{"faith", 1, "\x11", 1}, // ��������ħ��������ͨ�ַ�����
	{"item", 4, "\x13", 1}, // ����������������
	{"key", 4, "\x16", 1}, // ��Ҫ��Ʒ��
	{"time", 1, "\x18", 1},
	{"weather", 4, "\x1A", 1}, // ������
	{"pc", 1, "\x1C", 1}, // һ����ɫ��������
	{"color", 1, "\x1F", 1}, // �ƺ�����������ʽ����ɫ��
	{"7F", 1, "\x7F", 1},
	{"A1", 0, "\xA1", 1},
	{"val", 1, "\xEF", 1}, // һ���ı�������
	

	{"lt", 0, "<", 1}, // < as my special char
	{"~EOD~", -1, "XXX", -1},
};
