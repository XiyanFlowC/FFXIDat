#include "StringValidator.h"
#include <Windows.h>
#include <filesystem>
#include <xystring.h>
#include "../FFXIDatProcessor/codepage.h"
#include "../FFXITrans/ChsToSJis.h"

bool StringValidator::s_initialized = false;
bool StringValidator::s_codeCvtAvailable = false;
static std::wstring SV_MBCSTOWCS(const std::string &str);
static std::string SV_WCSTOMBCS(const std::wstring &str);

// Forward declare CodeCvt functions (will link if FFXIDatProcessor is available)
namespace {
	// Try to load CodeCvt dynamically
	bool TryLoadCodeCvt(const std::filesystem::path& basePath) {
		// Check if cp932.csv exists
		std::filesystem::path cp932Path = basePath / L"cp932.csv";
		if (!std::filesystem::exists(cp932Path))
			return false;
		
		try {
			CodeCvt::GetInstance().Init(cp932Path);
			return true;
		}
		catch (...) {
			return false;
		}
	}
}

// Our custom converters
static std::wstring SV_MBCSTOWCS(const std::string &str)
{
    // Decode using CodeCvt table mapping; preserve out-of-table bytes using a unique marker
    // Marker design: bytes 0x81 0xF0 followed by two raw bytes represent an escaped cp932 word
    // This sequence is chosen from unused vendor area; ensure round-trip by re-encoding
    std::wstring result;
    int i = 0;
    while (i < static_cast<int>(str.size())) {
        unsigned char b0 = static_cast<unsigned char>(str[i]);
        if (b0 & 0x80) {
            if (i + 1 < static_cast<int>(str.size())) {
                unsigned char b1 = static_cast<unsigned char>(str[i+1]);
                uint32_t cp = (static_cast<uint32_t>(b0) << 8) | b1;
                uint32_t uc;
                if (CodeCvt::GetInstance().TryCpToUc(cp, uc)) {
                    result += xybase::string::to_wstring(xybase::string::to_utf16(std::u32string(1, static_cast<char32_t>(uc))));
                } else {
                    // escape with marker into wstring as literal text "\xHH\xHH"
                    wchar_t esc[12];
                    swprintf_s(esc, L"\\x%02X\\x%02X", b0, b1);
                    result.append(esc);
                }
                i += 2;
            } else {
                wchar_t esc[6];
                swprintf_s(esc, L"\\x%02X", b0);
                result.append(esc);
                i += 1;
            }
        } else {
            uint32_t uc;
            if (CodeCvt::GetInstance().TryCpToUc(b0, uc)) {
                result += xybase::string::to_wstring(xybase::string::to_utf16(std::u32string(1, static_cast<char32_t>(uc))));
            } else {
                // ASCII fallback
                result += static_cast<wchar_t>(b0);
            }
            i += 1;
        }
    }
    return result;
}

static std::string SV_WCSTOMBCS(const std::wstring &str)
{
    // Pre-encode replacement using ChsToSJis to minimize failures
    std::u8string utf8Str = xybase::string::to_utf8(str);
    ChsToSJis::Instance().ReplaceHanzi(utf8Str);
    std::u32string u32 = xybase::string::to_utf32(utf8Str);

	return CodeCvt::GetInstance().CvtToString(xybase::string::to_wstring(utf8Str));
    //std::string result;
    //for (char32_t ch : u32)
    //{
    //    uint32_t cp;
    //    if (CodeCvt::GetInstance().TryUcToCp(static_cast<uint32_t>(ch), cp))
    //    {
    //        if (cp > 0xFF)
    //        {
    //            result.push_back(static_cast<char>((cp >> 8) & 0xFF));
    //            result.push_back(static_cast<char>(cp & 0xFF));
    //        }
    //        else
    //        {
    //            result.push_back(static_cast<char>(cp & 0xFF));
    //        }
    //    }
    //    else
    //    {
    //        // Out-of-table rescue: encode as literal sequence "\xHH" pairs
    //        // This ensures round-trip via our decoder
    //        char buf[6];
    //        sprintf_s(buf, "\\x%02X", static_cast<unsigned int>(ch & 0xFF));
    //        result.append(buf);
    //    }
    //}
    //return result;
}

void StringValidator::Initialize(const std::wstring& appPath)
{
	if (s_initialized)
		return;
	
	std::filesystem::path basePath(appPath);
	basePath = basePath.parent_path();
	
    // Try to load CodeCvt (it will register its own callbacks)
    s_codeCvtAvailable = TryLoadCodeCvt(basePath);

    // Re-register our converters AFTER CodeCvt so our callbacks stay in control
    xybase::string::set_string_cvt(SV_MBCSTOWCS, SV_WCSTOMBCS);

	ChsToSJis::Instance().Init(basePath / L"chs2sjis.csv");
	
	s_initialized = true;
}

// UTF-8 to UTF-16 using Windows API
static std::wstring UTF8ToWString(const std::string& utf8Str)
{
	if (utf8Str.empty())
		return L"";
	
	int size = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, nullptr, 0);
	if (size <= 0)
		return L"";
	
	std::wstring result(size - 1, 0);
	MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, &result[0], size);
	
	return result;
}

// Use xybase::string for conversion (will use CodeCvt if initialized)
std::string StringValidator::WStringToShiftJIS(const std::wstring& wstr)
{
	if (wstr.empty())
		return "";

    // Convert to UTF-8 first
    std::u8string utf8Str = xybase::string::to_utf8(wstr);
    // Pre-encode replacement to reduce failures
    ChsToSJis::Instance().ReplaceHanzi(utf8Str);

    // Back to wstring for precise page mapping when CodeCvt is present
    std::wstring replacedW = xybase::string::to_wstring(utf8Str);

    if (s_codeCvtAvailable)
    {
        return CodeCvt::GetInstance().CvtToString(replacedW);
    }

    // Fallback to system conversion
    return xybase::string::sys_wcs_to_mbs(replacedW);
}

std::wstring StringValidator::ShiftJISToWString(const std::string& sjisStr)
{
	if (sjisStr.empty())
		return L"";
	
    // Use our registered callback (SV_MBCSTOWCS) via xybase
    return xybase::string::to_wstring(sjisStr);
}

bool StringValidator::CanEncode(const std::u8string& utf8Str)
{
	try
	{
		// Convert UTF-8 to UTF-16
		std::string utf8Bytes(reinterpret_cast<const char*>(utf8Str.c_str()));
		std::wstring wstr = UTF8ToWString(utf8Bytes);
		
		// Try to convert to Shift-JIS using xybase (will use CodeCvt if available)
		std::string sjis = WStringToShiftJIS(wstr);
		
		// Try to convert back to verify round-trip
		std::wstring backConverted = ShiftJISToWString(sjis);
		
		// If the strings match after round-trip, encoding is valid
		return wstr == backConverted;
	}
	catch (const std::exception&)
	{
		return false;
	}
}

std::string StringValidator::GetValidationError(const std::u8string& utf8Str)
{
	if (CanEncode(utf8Str))
		return "";
	
	try
	{
		std::string utf8Bytes(reinterpret_cast<const char*>(utf8Str.c_str()));
		std::wstring wstr = UTF8ToWString(utf8Bytes);
		
		// Find the first character that cannot be encoded
		for (size_t i = 0; i < wstr.length(); ++i)
		{
			std::wstring singleChar(1, wstr[i]);
			std::string sjis = WStringToShiftJIS(singleChar);
			std::wstring backConverted = ShiftJISToWString(sjis);
			
			if (singleChar != backConverted)
			{
				return "Character at position " + std::to_string(i) + 
					   " cannot be encoded to Shift-JIS";
			}
		}
	}
	catch (const std::exception& e)
	{
		return std::string("Encoding error: ") + e.what();
	}
	
	return "Unknown encoding error";
}

bool StringValidator::ConvertAndValidate(const std::u8string& utf8Str, 
										 std::string& shiftJisStr)
{
	if (!CanEncode(utf8Str))
		return false;
	
	try
	{
		std::string utf8Bytes(reinterpret_cast<const char*>(utf8Str.c_str()));
		std::wstring wstr = UTF8ToWString(utf8Bytes);
		
		shiftJisStr = WStringToShiftJIS(wstr);
		return true;
	}
	catch (const std::exception&)
	{
		return false;
	}
}





