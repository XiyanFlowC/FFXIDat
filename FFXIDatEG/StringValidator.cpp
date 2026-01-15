#include "StringValidator.h"
#include <Windows.h>
#include <filesystem>
#include <xystring.h>
#include "../FFXIDatProcessor/codepage.h"
#include "../FFXITrans/ChsToSJis.h"

bool StringValidator::s_initialized = false;
bool StringValidator::s_codeCvtAvailable = false;

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

void StringValidator::Initialize(const std::wstring& appPath)
{
	if (s_initialized)
		return;
	
	std::filesystem::path basePath(appPath);
	basePath = basePath.parent_path();
	
	// Try to load CodeCvt
	s_codeCvtAvailable = TryLoadCodeCvt(basePath);

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

	std::u8string utf8Str = xybase::string::to_utf8(wstr);
	ChsToSJis::Instance().ReplaceHanzi(
		utf8Str
	);
	
	// Use xybase::string::to_string which will call CodeCvt if set_string_cvt was called
	return xybase::string::to_string(utf8Str);
}

std::wstring StringValidator::ShiftJISToWString(const std::string& sjisStr)
{
	if (sjisStr.empty())
		return L"";
	
	// Use xybase::string::to_wstring which will call CodeCvt if set_string_cvt was called
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





