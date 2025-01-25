#include "codepage.h"

std::string cvt_to_string(const std::wstring &str)
{
	return CodeCvt::GetInstance().CvtToString(str);
}

std::wstring cvt_to_wstring(const std::string &str)
{
	return CodeCvt::GetInstance().CvtToWString(str);
}

#pragma pack(push,1)
struct CodePageEntry
{
	uint32_t dbc;
	uint32_t wc;
};
#pragma pack(pop)

CodeCvt::~CodeCvt()
{
	xybase::string::set_string_cvt(nullptr, nullptr);
}

CodeCvt &CodeCvt::GetInstance()
{
	static CodeCvt _inst;
	return _inst;
}

bool CodeCvt::ChsOnSJisDirtyThing(xybase::StringBuilder<char> &sb, char32_t code)
{
	int a = 0, b = 0;
	if (code == U'嗨') a = uc2cp[U'口'], b = uc2cp[U'海'];
	if (code == U'呢') a = uc2cp[U'口'], b = uc2cp[U'尼'];
	if (code == U'哟') a = uc2cp[U'口'], b = uc2cp[U'约'];
	if (code == U'唉') a = uc2cp[U'口'], b = uc2cp[U'矣'];
	if (code == U'哪') a = uc2cp[U'口'], b = uc2cp[U'那'];
	if (code == U'嗨') a = uc2cp[U'口'], b = uc2cp[U'海'];

	if (a && b) {
		sb += a;
		sb += b;
		return true;
	}
	return false;
}

std::string CodeCvt::CvtToString(const std::wstring &str)
{
	auto codes = xybase::string::to_utf32(str);

	xybase::StringBuilder<char> sb;
	for (char32_t code : str)
	{
		auto dbc = uc2cp[code];
		if (dbc == 0)
		{
			dbc = uc2cp[U'〓'];
			if (dbc == 0)
			{
				dbc = uc2cp[U'?'];
			}
		}
		if (dbc > 0xFF)
		{
			sb.Append((dbc & 0xFF00) >> 8);
			sb.Append(dbc & 0xFF);
		}
		else
		{
			sb.Append(dbc);
		}
	}

	return sb.ToString();
}

std::wstring CodeCvt::CvtToWString(const std::string &str)
{
	xybase::StringBuilder<char32_t> sb;
	int current = 0;
	for (char ch : str)
	{
		if (current)
		{
			auto wc = cp2uc[(current | (ch & 0xFF)) & 0xFFFF];
			if (wc == 0)
			{
#ifdef NDEBUG
				sb.Append(U'�');
#else
				sb.Append(U"\\x");
				int cb = (current >> 8);
				cb &= 0xFF;
				if (cb < 16)
					sb.Append('0');
				sb.Append(xybase::string::itos<char32_t>(cb, 16).c_str());
				sb.Append(U"\\x");
				cb = ch;
				cb &= 0xFF;
				if (cb < 16)
					sb.Append('0');
				sb.Append(xybase::string::itos<char32_t>(cb, 16).c_str());
#endif
			}
			else
				sb.Append(wc);
			current = 0;
		}
		else
		{
			if (ch & 0x80)
			{
				current = ch << 8;
			}
			else
			{
				auto wc = cp2uc[ch];
				if (wc == 0)
				{
					wc = U'�';
				}

				sb.Append(wc);
			}
		}
	}
	return xybase::string::to_wstring(sb.ToString());
}

#include "CsvFile.h"

void CodeCvt::Init(std::filesystem::path path)
{
	uc2cp.clear();
	cp2uc.clear();

	CsvFile csv(path, std::ios::in | std::ios::binary);
	while (!csv.IsEof())
	{
		uint32_t cp = xybase::string::stoi(csv.NextCell(), 16);
		uint32_t uc = xybase::string::to_codepoint(csv.NextCell());

		if (!uc2cp.contains(uc))
			uc2cp[uc] = cp;
		if (!cp2uc.contains(cp))
			cp2uc[cp] = uc;

		csv.NextLine();
	}

	xybase::string::set_string_cvt(cvt_to_wstring, cvt_to_string);
}
