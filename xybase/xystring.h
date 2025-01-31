/**
* xystring.h - A platform isolation string processing util
* @author Xiyan
*/
#pragma once

#ifndef XY_XYSTRING_H__
#define XY_XYSTRING_H__

#include <string>
#include <stack>
#include <cctype>

#include "StringBuilder.h"

namespace xybase
{
	namespace string
	{
		/* Codepoint conversions. */

		/**
		 * @brief Encode a codepoint to utf-8
		 * @param codePoint Code Point.
		 * @return The utf-8 encoded string.
		*/
		std::u8string XY_API to_utf8(long codePoint);

		/**
		 * @brief Encode a codepoint to utf-16
		 * @param codepoint Code Point.
		 * @return The utf-16 encoded string.
		*/
		std::u16string XY_API to_utf16(long codepoint);

		/**
		 * @brief Encode a codepoint to utf-32
		 * @param codepoint Code Point.
		 * @return The utf-32 encoded string.
		*/
		std::u32string XY_API to_utf32(long codepoint);

		/**
		 * @brief Get the codepoint for utf-8 (only process the first character).
		 * @param str The character need to be processed.
		 * @return The codepoint of given character.
		*/
		long XY_API to_codepoint(const std::u8string &str);

		long XY_API to_codepoint(const std::u8string &str, int offset, int &leng);

		/**
		 * @brief Get the codepoint for utf-16 (only process the first character).
		 * @param str The character need to be processed.
		 * @return The codepoint of given character.
		*/
		long XY_API to_codepoint(const std::u16string &str);

		/* String type conversions. */

		/**
		 * @brief Convert locale string to ucs-4 string
		 * @param str 
		 * @return 
		*/
		std::u32string XY_API to_utf32(const std::string &str) noexcept;

		/**
		 * @brief Convert utf8 string to utf32 string
		 * @param str 
		 * @return 
		*/
		std::u32string XY_API to_utf32(const std::u8string &str) noexcept;

		/**
		 * @brief Convert utf16 string to utf32 string
		 * @param str 
		 * @return 
		*/
		std::u32string XY_API to_utf32(const std::u16string &str) noexcept;

		/**
		 * @brief Identical conversion.
		 * @param str 
		 * @return 
		*/
		std::u32string XY_API to_utf32(const std::u32string &str) noexcept;

		/**
		 * @brief Convert wstring to ucs-4 string
		 * @param str 
		 * @return 
		*/
		std::u32string XY_API to_utf32(const std::wstring &str) noexcept;

		/**
		 * @brief Convert local string to utf-16
		 * @param str 
		 * @return 
		*/
		std::u16string XY_API to_utf16(const std::string &str) noexcept;

		/**
		 * @brief Convert utf-8 to utf-16
		 * @param str utf-8 string
		 * @return utf-16 string
		*/
		std::u16string XY_API to_utf16(const std::u8string &str) noexcept;

		/**
		 * @brief Identical transform
		 * @param str 
		 * @return 
		*/
		std::u16string XY_API to_utf16(const std::u16string &str) noexcept;

		/**
		 * @brief Convert UCS-4 to Utf-16
		 * @param str 
		 * @return 
		*/
		std::u16string XY_API to_utf16(const std::u32string &str) noexcept;

		/**
		 * @brief Convert a wstring to utf16 string
		 * @param str 
		 * @return 
		*/
		std::u16string XY_API to_utf16(const std::wstring &str) noexcept;

		/**
		 * @brief Convert wstring to utf-8 string
		 * @param str 
		 * @return 
		*/
		std::u8string XY_API to_utf8(const std::wstring &str) noexcept;

		/**
		 * @brief Convert u32string to utf8
		 * @param str 
		 * @return 
		*/
		std::u8string XY_API to_utf8(const std::u32string &str) noexcept;

		/**
		 * @brief Convert utf-16 to utf-8
		 * @param str utf-16 string
		 * @return utf-8 string
		*/
		std::u8string XY_API to_utf8(const std::u16string &str) noexcept;

		/**
		 * @brief identical transform
		 * @param str 
		 * @return 
		*/
		std::u8string XY_API to_utf8(const std::u8string &str) noexcept;

		/**
		 * @brief Convert local string to utf-8 string
		 * @param str 
		 * @return 
		*/
		std::u8string XY_API to_utf8(const std::string &str) noexcept;

		/**
		 * @brief Convert local string to wide string.
		 * @param str String that to be converted.
		 * @return Converted string.
		*/
		std::wstring XY_API to_wstring(const std::string &str) noexcept;

		/**
		 * @brief System provided feature call: multibyte string to wide char string.
		 * @param str Multibyte string that is needed to be converted.
		 * @return Converted string.
		*/
		std::wstring XY_API sys_mbs_to_wcs(const std::string &str) noexcept;

		/**
		 * @brief Convert to wstring (platform specified)
		 * @param str 
		 * @return 
		*/
		std::wstring XY_API to_wstring(const std::u8string &str) noexcept;

		/**
		 * @brief Convert to wstring
		 * @param str UTF-16 string
		 * @return 
		*/
		std::wstring XY_API to_wstring(const std::u16string &str) noexcept;

		/**
		 * @brief Convert to wstring
		 * @param str UCS-4 string
		 * @return 
		*/
		std::wstring XY_API to_wstring(const std::u32string &str) noexcept;

		/**
		 * @brief Identical conversion
		 * @param str 
		 * @return 
		*/
		std::wstring XY_API to_wstring(const std::wstring &str) noexcept;

		/**
		 * @brief Identical conversion
		 * @param str 
		 * @return 
		*/
		std::string XY_API to_string(const std::string &str) noexcept;

		/**
		 * @brief Convert utf8 string to local string
		 * @param str 
		 * @return 
		*/
		std::string XY_API to_string(const std::u8string &str) noexcept;

		/**
		 * @brief Convert utf16 string to local string
		 * @param str 
		 * @return 
		*/
		std::string XY_API to_string(const std::u16string &str) noexcept;

		/**
		 * @brief Convert utf32 string to local string
		 * @param str 
		 * @return 
		*/
		std::string XY_API to_string(const std::u32string &str) noexcept;

		/**
		 * @brief Convert wstring to local string
		 * @param str 
		 * @return 
		*/
		std::string XY_API to_string(const std::wstring &str) noexcept;

		/**
		 * @brief System provided feature: Convert wide char string to multibyte string.
		 * @param str The string that is needed to be converted.
		 * @return Converted string.
		*/
		std::string XY_API sys_wcs_to_mbs(const std::wstring &str) noexcept;

		/**
		 * @brief Set costum string converter (of multibyte string and wide string
		 * @param p_mbcstowcs function converts MBC string to WC string
		 * @param p_wcstombcs function converts WC string to MBC string
		*/
		void XY_API set_string_cvt(std::wstring(*p_mbcstowcs)(const std::string &), std::string(*p_wcstombcs)(const std::wstring &)) noexcept;

		template <typename ChT>
		std::basic_string<ChT> to_lower(const std::basic_string<ChT> &ori)
		{
			StringBuilder<ChT> sb;
			for (auto &&ch : ori)
			{
				if (isupper(ch))
				{
					sb += tolower(ch);
				}
				else
				{
					sb += ch;
				}
			}
			return sb.ToString();
		}

		/**
		 * @brief Replace all substring in a string with given replacement.
		 * @tparam ChT Type of char.
		 * @param original Original string.
		 * @param target The substring need to be replaced.
		 * @param replacement Replacement.
		 * @return Replaced string.
		*/
		template <typename ChT>
		std::basic_string<ChT> replace(std::basic_string<ChT> original, const std::basic_string<ChT> &target, const std::basic_string<ChT> &replacement)
		{
			size_t offset = original.find(target);
			while (offset != std::basic_string<ChT>::npos)
			{
				original.replace(offset, target.length(), replacement);
				offset = original.find(target, offset + replacement.length());
			}
			return original;
		}

		/**
		 * @brief Replace all substring in a string with given replacement in place
		 * @tparam ChT Type of char.
		 * @param original Original string.
		 * @param target The substring need to be replaced.
		 * @param replacement Replacement.
		*/
		template <typename ChT>
		void replace_in_place(std::basic_string<ChT> &original, const std::basic_string<ChT> &target, const std::basic_string<ChT> &replacement)
		{
			size_t offset = original.find(target);
			while (offset != std::basic_string<ChT>::npos)
			{
				original.replace(offset, target.length(), replacement);
				offset = original.find(target, offset + replacement.length());
			}
		}

		/**
		 * @brief Parse string to integer (up to base 36)
		 * @tparam T Type of the string unit.
		 * @param str String.
		 * @param base Base.
		 * @return Parsed integer.
		*/
		template<typename T = char>
		unsigned long long stoi(const std::basic_string_view<T> str, int base = 10)
		{
			const T *ptr = str.data();
			const T *end = ptr + str.length();
			unsigned long long ret = 0;

			while (ptr < end)
			{
				if (*ptr >= static_cast<T>('0') && *ptr <= std::min<T>(static_cast<T>('9'), static_cast<T>('0') + base - 1))
				{
					ret = ret * base + *ptr++ - static_cast<T>('0');
				}
				else if (*ptr >= static_cast<T>('a') && *ptr <= std::max<T>(static_cast<T>('a') - 1, static_cast<T>('a') + base - 11))
				{
					ret = ret * base + *ptr++ - static_cast<T>('a') + 10;
				}
				else if (*ptr >= static_cast<T>('A') && *ptr <= std::max<T>(static_cast<T>('A') - 1, static_cast<T>('A') + base - 11))
				{
					ret = ret * base + *ptr++ - static_cast<T>('A') + 10;
				}
				else break;
			}
			return ret;
		}

		template<typename T = char>
		unsigned long long stoi(const std::basic_string<T> str, int base = 10)
		{
			return stoi<T>(std::basic_string_view<T>(str), base);
		}

		/**
		 * @brief Parse integer
		 * @tparam T 
		 * @param str 
		 * @param base 
		 * @return 
		 */
		template<typename T = char>
		long long pint(const std::basic_string<T> &str, int base = 10)
		{
			long long ret = str[0] == '-' ? -1 : 1;
			return ret * stoi<T>(str[0] == '-' ? str.substr(1) : str);
		}

		template<typename T = char>
		double pflt(const std::basic_string<T> &str, int base = 10)
		{
			double res = 0.0;
			int isNeg = 0, flag = 0;
			double fact = 0.1;
			for (int ch : str) {
				if (!isdigit(ch))
				{
					if (ch == '-')
					{
						isNeg ^= 1;
						continue;
					}
					else if (ch == '.')
					{
						flag = 1;
						continue;
					}
					else throw xybase::InvalidParameterException(L"value", L"not a valid real number.", 37701);
				}

				if (flag)
				{
					res = res + fact * (ch - '0');
					fact *= 0.1;
				}
				else
					res = res * 10 + ch - '0';
			}

			return isNeg ? -res : res;
		}

		template<typename T = char>
		std::basic_string<T> itos(unsigned long long value, int base = 10)
		{
			static const char *pre = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
			if (value == 0) return std::basic_string<T>{'0'};

			std::stack<T> stk;
			while (value)
			{
				stk.push(static_cast<T>(pre[value % base]));
				value /= base;
			}

			StringBuilder<T> sb;
			while (!stk.empty())
			{
				sb.Append(stk.top());
				stk.pop();
			}
			return sb.ToString();
		}

		/**
		 * @brief 【模板用】将原始编码的指定字符串转换为类型指定的编码。
		 * @tparam Ch 目标类型
		 * @tparam SrcCh 原始字符串类型
		 * @param str 要转换的字符串
		 * @return 转完毕的字符串
		*/
		template <typename Ch, typename SrcCh>
		std::basic_string<Ch> to_enc(const std::basic_string<SrcCh> &str)
		{
			if constexpr (std::is_same_v<Ch, wchar_t>) return xybase::string::to_wstring(str);
			else if constexpr (std::is_same_v<Ch, char>) return xybase::string::to_string(str);
			else if constexpr (std::is_same_v<Ch, char8_t>) return xybase::string::to_utf8(str);
			else if constexpr (std::is_same_v<Ch, char16_t>) return xybase::string::to_utf16(str);
			else if constexpr (std::is_same_v<Ch, char32_t>) return xybase::string::to_utf32(str);
			else abort();
		}

		template <typename Ch>
		std::basic_string<Ch> escape(const std::basic_string<Ch> &str)
		{
			std::basic_string<Ch> result;
			for (const auto &ch : str)
			{
				switch (ch)
				{
				case '\\':
					result += static_cast<Ch>('\\');
					result += static_cast<Ch>('\\');
					break;
				case '\r':
					result += static_cast<Ch>('\\');
					result += static_cast<Ch>('r');
					break;
				case '\n':
					result += static_cast<Ch>('\\');
					result += static_cast<Ch>('n');
					break;
				default:
					result += ch;
					break;
				}
			}
			return result;
		}

		template <typename Ch>
		std::basic_string<Ch> unescape(const std::basic_string<Ch> &str)
		{
			std::basic_string<Ch> result;
			auto it = str.begin();
			while (it != str.end())
			{
				if (*it == static_cast<Ch>('\\') && std::next(it) != str.end())
				{
					++it; // Skip the backslash
					switch (*it)
					{
					case '\\':
						result += static_cast<Ch>('\\');
						break;
					case 'r':
						result += static_cast<Ch>('\r');
						break;
					case 'n':
						result += static_cast<Ch>('\n');
						break;
					default:
						result += *it; // Non-standard escape, add as-is
						break;
					}
				}
				else
				{
					result += *it;
				}
				++it;
			}
			return result;
		}
	}
}

#endif // !XY_XYSTRING_H__
