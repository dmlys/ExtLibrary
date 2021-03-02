#include <ext/codecvt_conv/wchar_cvt.hpp>

namespace ext::codecvt_convert::wchar_cvt
{
	const std::codecvt_utf8_utf16<char16_t, 0x10FFFF, std::codecvt_mode::little_endian> u16_cvt;
	const std::codecvt_utf8      <char32_t, 0x10FFFF, std::codecvt_mode::little_endian> u32_cvt;
}
