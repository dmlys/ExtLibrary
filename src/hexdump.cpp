#include <boost/predef.h>
#include <ext/hexdump.hpp>
#include <cmath>

#if BOOST_COMP_MSVC
#include <intrin.h>
#pragma intrinsic(_BitScanForward64)
#endif

namespace ext::hexdump
{
	std::size_t addr_width(std::size_t count) noexcept
	{
		if (count <= 16) return 2;

		std::size_t nlast = count - 1;
		unsigned p2; // power of 2 for nlast
	#if BOOST_COMP_GNUC or BOOST_COMP_CLANG
		unsigned clz;

		if constexpr(sizeof(std::size_t) == sizeof(unsigned))
			clz = __builtin_clz(nlast);
		else if constexpr(sizeof(std::size_t) == sizeof(unsigned long))
			clz = __builtin_clzl(nlast);
		else //if constexpr(sizeof(std::size_t) == sizeof(unsigned long long))
			clz = __builtin_clzll(nlast);

		p2 = sizeof(std::size_t) * CHAR_BIT - clz;
	#elif BOOST_COMP_MSVC
		unsigned long msb_index;
		_BitScanReverse64(&msb_index, nlast);
		p2 = msb_index + 1;
	#else
		p2 = 1 + static_cast<unsigned>(std::log2(nlast));
	#endif

		return p2 / 4 + (p2 % 4 ? 1 : 0);
	}

	std::size_t buffer_estimation(std::size_t count) noexcept
	{
		return buffer_estimation(addr_width(count), count);
	}

	std::size_t buffer_estimation(std::size_t addr_width, std::size_t count) noexcept
	{
		constexpr unsigned rowsize = 16;
		auto left = count % rowsize;
		auto rows = count / rowsize + (left ? 1 : 0);

		// rowsize x $H1H2' ' + 2x' ' + rowsize x $CH + '\n'
		//auto row_width = /*offset*/offwidth + /*space sep*/2 + /*hex with space*/rowsize * 3 + /*hex sep*/ 2 + /*ascii data*/rowsize + /*newline*/1;
		auto row_width = rowsize * 4 + addr_width + 5;
		return rows * row_width;
	}
}

// first 32 chars are not printable, 127 and above - not printable either
const char ext::hexdump::ascii_tranlation_array[256] =
{
/*        0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F */
/* 0 */  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,
/* 1 */  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,
/* 2 */  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
/* 3 */  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
/* 4 */  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
/* 5 */  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
/* 6 */  96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
/* 7 */ 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126,  46,
/* 8 */  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,
/* 9 */  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,
/* A */  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,
/* B */  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,
/* C */  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,
/* D */  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,
/* E */  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,
/* F */  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,  46,
};
