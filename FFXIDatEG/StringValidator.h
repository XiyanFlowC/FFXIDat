#pragma once

#include <string>

class StringValidator
{
public:
	// Initialize code page conversion (call once at startup)
	// This will initialize CodeCvt and set xybase::string callbacks
	static void Initialize(const std::wstring& appPath);
	
	// Validate if a UTF-8 string can be properly encoded to Shift-JIS
	static bool CanEncode(const std::u8string& utf8Str);
	
	// Get detailed validation error message
	static std::string GetValidationError(const std::u8string& utf8Str);
	
	// Convert UTF-8 to Shift-JIS and validate
	static bool ConvertAndValidate(const std::u8string& utf8Str, 
								   std::string& shiftJisStr);
	
	// Convert UTF-16 to Shift-JIS (uses CodeCvt if available)
	static std::string WStringToShiftJIS(const std::wstring& wstr);
	
	// Convert Shift-JIS to UTF-16 (uses CodeCvt if available)
	static std::wstring ShiftJISToWString(const std::string& sjisStr);
	
private:
	static bool s_initialized;
	static bool s_codeCvtAvailable;
};
