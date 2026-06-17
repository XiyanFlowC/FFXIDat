#pragma once

#include <exception>
#include <string>

#include "../xyapi.h"

#pragma push_macro("GetMessage")
#undef GetMessage

namespace xybase
{
	const int RUNTIME_EXCEPTION_LEAD = 0x8000'0000;

	class RuntimeException : public std::exception
	{
	public:
		XY_API RuntimeException(const std::wstring &message, int err);

		XY_API virtual ~RuntimeException();

		XY_API virtual const char *what() const noexcept override;

		XY_API virtual const std::wstring &GetMessage() const;

		XY_API virtual int GetErrorCode() const;

	protected:
		std::wstring message;
		int err;
		char *buf;
	};
}

#pragma pop_macro("GetMessage")
