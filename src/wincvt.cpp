#include <ext/codecvt_conv/wincvt.hpp>
#if BOOST_OS_WINDOWS
namespace ext::codecvt_convert::wincvt
{
	const std::codecvt_utf8_utf16<wchar_t, 0x10FFFF, std::codecvt_mode::little_endian> u8_cvt;
}
#endif
