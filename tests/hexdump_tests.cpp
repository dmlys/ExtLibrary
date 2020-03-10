#include <ext/hexdump.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/dataset.hpp>
//#include <boost/test/data/monomorphic.hpp>

#include <tuple>
#include <vector>
#include <fmt/core.h>

static auto test_data()
{
	// number, and expected widths for n-1, n, n+1.
	std::vector<std::tuple<std::size_t, unsigned, unsigned, unsigned>> test_data =
	{
	    {0x0001, 2, 2, 2},
	    {0x0002, 2, 2, 2},
	    {0x0004, 2, 2, 2},
	    {0x0008, 2, 2, 2},

	    {0x0010, 2, 2, 2},
	    {0x0020, 2, 2, 2},
	    {0x0040, 2, 2, 2},
	    {0x0080, 2, 2, 2},

	    {0x0100, 2, 2, 3},
	    {0x0200, 3, 3, 3},
	    {0x0400, 3, 3, 3},
	    {0x0800, 3, 3, 3},

	    {0x1000, 3, 3, 4},
	    {0x2000, 4, 4, 4},
	    {0x4000, 4, 4, 4},
	    {0x8000, 4, 4, 4},

	    {0x00010000, 4, 4, 5},
	    {0x00020000, 5, 5, 5},
	    {0x00040000, 5, 5, 5},
	    {0x00080000, 5, 5, 5},

	    {0x00100000, 5, 5, 6},
	    {0x00200000, 6, 6, 6},
	    {0x00400000, 6, 6, 6},
	    {0x00800000, 6, 6, 6},

	    {0x01000000, 6, 6, 7},
	    {0x02000000, 7, 7, 7},
	    {0x04000000, 7, 7, 7},
	    {0x08000000, 7, 7, 7},

	    {0x10000000, 7, 7, 8},
	    {0x20000000, 8, 8, 8},
	    {0x40000000, 8, 8, 8},
	    {0x80000000, 8, 8, 8},
	};

	return test_data;
}

BOOST_AUTO_TEST_SUITE(hexdump_tests)

BOOST_DATA_TEST_CASE(addr_width_test, boost::unit_test::data::make(test_data()), N, width_m1, width, width_p1)
{
	BOOST_CHECK_EQUAL(ext::hexdump::addr_width(N - 1), width_m1);
	BOOST_CHECK_EQUAL(ext::hexdump::addr_width(N + 0), width   );
	BOOST_CHECK_EQUAL(ext::hexdump::addr_width(N + 1), width_p1);
}

//BOOST_AUTO_TEST_CASE(addr_width_test_xxl,
//	* boost::unit_test::disabled()
//	* boost::unit_test::description("Checks all numbers, not only border one, runs very long"))
//{
//	std::size_t N, prevN;
//	unsigned width_m1, width, width_p1, prev_width;
//	auto test_data = ::test_data();
//
//	std::tie(prevN, std::ignore, std::ignore, prev_width) = test_data[0];
//
//	for (unsigned u = 1; u < test_data.size(); ++u, prev_width = width_p1, prevN = N)
//	{
//		std::tie(N, width_m1, width, width_p1) = test_data[u];
//		BOOST_TEST_MESSAGE(fmt::format("running {} iteration, N = {}", u, N));
//
//		for (std::size_t val = prevN + 1; val < N; ++val)
//			BOOST_CHECK_EQUAL(ext::hexdump::addr_width(val), prev_width);
//
//		BOOST_CHECK_EQUAL(ext::hexdump::addr_width(N - 1), width_m1);
//		BOOST_CHECK_EQUAL(ext::hexdump::addr_width(N + 0), width   );
//		BOOST_CHECK_EQUAL(ext::hexdump::addr_width(N + 1), width_p1);
//	}
//}

BOOST_AUTO_TEST_SUITE_END()
