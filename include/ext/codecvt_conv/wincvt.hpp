#pragma once
#include <boost/predef.h>
#if BOOST_OS_WINDOWS
#include <codecvt>
#include <ext/codecvt_conv/base.hpp>
#include <ext/codecvt_conv/generic_conv.hpp>

// helpers for wchar stuff on windows

namespace ext::codecvt_convert::wincvt
{
	extern const std::codecvt_utf8_utf16<wchar_t, 0x10FFFF, std::codecvt_mode::little_endian> u8_cvt;
	
	template <class String>
	inline auto to_utf8(const String & str)
	{
		return ext::codecvt_convert::to_bytes(u8_cvt, str);
	}
	
	template <class String>
	inline auto to_utf16(const String & str)
	{
		return ext::codecvt_convert::from_bytes(u8_cvt, str);
	}
}

#endif
