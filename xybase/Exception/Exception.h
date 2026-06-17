#pragma once

#include <exception>
#include <string>

#include "../xyapi.h"

// Prevent Windows GetMessage macro from mangling this method name
#pragma push_macro("GetMessage")
#undef GetMessage

namespace xybase
{
	class Exception : public std::exception
	{
	public:
		XY_API Exception(const std::wstring &message, int err);

		XY_API virtual ~Exception();

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
