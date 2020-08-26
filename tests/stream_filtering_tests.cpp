#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/dataset.hpp>

#include <ext/stream_filtering/transform_width_filter.hpp>
#include <ext/stream_filtering/basexx.hpp>
#include <ext/stream_filtering/zlib.hpp>
#include <ext/stream_filtering/filtering.hpp>

#include "test_files.h"

using boost::unit_test::data::make;

namespace ext::stream_filtering
{
	static auto & operator <<(std::ostream & os, const ext::stream_filtering::processing_parameters & par)
	{
		return os << par.default_buffer_size;
	}
}


static const std::vector<ext::stream_filtering::processing_parameters> configurations =
{
	// default_buffer_size, maximum_buffer_size, full_threshold_part
	{},    // default buffer size
	{5},   // limit cases
	{4},
	{3},
	{2},
	{1},
};


BOOST_AUTO_TEST_SUITE(stream_filtering_tests)

BOOST_DATA_TEST_CASE(stream_filtering_pipe_memory_test, make(configurations), config)
{
	std::string expected, input, result;
	input = expected = "test pipe memory data";
	
	ext::stream_filtering::base64_decode_filter b64decode_filter;
	ext::stream_filtering::base64_encode_filter b64encode_filter;
	ext::stream_filtering::zlib_deflate_filter zlib_deflator;
	ext::stream_filtering::zlib_inflate_filter zlib_inflator;
	std::vector<ext::stream_filtering::filter *> filters = { &zlib_deflator, &b64encode_filter, &b64decode_filter, &zlib_inflator, };
	
	ext::stream_filtering::filter_memory(config, filters, input, result);
	BOOST_CHECK_EQUAL(expected, result);
}

BOOST_DATA_TEST_CASE(stream_filtering_pipe_stream_test, make(configurations), config)
{
	std::string expected, input, result;
	input = expected = "test pipe memory data";
	
	ext::stream_filtering::base64_decode_filter b64decode_filter;
	ext::stream_filtering::base64_encode_filter b64encode_filter;
	ext::stream_filtering::zlib_deflate_filter zlib_deflator;
	ext::stream_filtering::zlib_inflate_filter zlib_inflator;
	std::vector<ext::stream_filtering::filter *> filters = { &zlib_deflator, &b64encode_filter, &b64decode_filter, &zlib_inflator, };
	
	std::stringstream inputss(input);
	std::stringstream resultss;
	ext::stream_filtering::filter_stream(config, filters, inputss, resultss);
	result = resultss.str();
	
	BOOST_CHECK_EQUAL(expected, result);
}


BOOST_AUTO_TEST_SUITE_END()

