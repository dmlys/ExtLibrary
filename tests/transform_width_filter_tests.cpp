#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/dataset.hpp>

#include <ext/stream_filtering/transform_width_filter.hpp>
#include <ext/stream_filtering/basexx.hpp>
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


BOOST_AUTO_TEST_SUITE(transform_width_filter_tests)

BOOST_DATA_TEST_CASE(single_base64_encoder_memory, make(configurations), config)
{
	std::string expected, encoded, result;
	LoadTestFile("Base64TestFiles/DataLink32.ps1", expected, std::ios::binary);
	LoadTestFile("Base64TestFiles/DataLink32.ps1.single_encoded", encoded, std::ios::binary);
	
	ext::stream_filtering::base64_decode_filter b64decode_filter;
	std::vector<ext::stream_filtering::filter *> filters = { &b64decode_filter };
	
	ext::stream_filtering::filter_memory(config, filters, encoded, result);
	BOOST_CHECK_EQUAL(expected, result);
}

BOOST_DATA_TEST_CASE(single_base64_encoder_stream, make(configurations), params)
{
	std::string expected;
	auto source_path = test_files_location / "Base64TestFiles/DataLink32.ps1.single_encoded";
	
	LoadTestFile("Base64TestFiles/DataLink32.ps1", expected, std::ios::binary);
	std::ifstream source(source_path.string(), std::ios::binary);
	BOOST_CHECK(source.is_open());
	
	std::stringstream result;
	
	ext::stream_filtering::base64_decode_filter b64decode_filter;
	std::vector<ext::stream_filtering::filter *> filters = { &b64decode_filter };
	
	ext::stream_filtering::filter_stream(filters, source, result);
	BOOST_CHECK_EQUAL(expected, result.str());
}

BOOST_DATA_TEST_CASE(triple_base64_encoder_memory, make(configurations), config)
{
	std::string expected, encoded, result;
	LoadTestFile("Base64TestFiles/DataLink32.ps1", expected, std::ios::binary);
	LoadTestFile("Base64TestFiles/DataLink32.ps1.triple_encoded", encoded, std::ios::binary);
	
	ext::stream_filtering::base64_decode_filter b64decode_filter1, b64decode_filter2, b64decode_filter3;
	std::vector<ext::stream_filtering::filter *> filters = { &b64decode_filter1, &b64decode_filter2, &b64decode_filter3, };
	
	ext::stream_filtering::filter_memory(config, filters, encoded, result);
	BOOST_CHECK_EQUAL(expected, result);
}

BOOST_DATA_TEST_CASE(triple_base64_encoder_stream, make(configurations), params)
{
	std::string expected;
	auto source_path = test_files_location / "Base64TestFiles/DataLink32.ps1.triple_encoded";
	
	LoadTestFile("Base64TestFiles/DataLink32.ps1", expected, std::ios::binary);
	std::ifstream source(source_path.string(), std::ios::binary);
	BOOST_CHECK(source.is_open());
	
	std::stringstream result;
	
	ext::stream_filtering::base64_decode_filter b64decode_filter1, b64decode_filter2, b64decode_filter3;
	std::vector<ext::stream_filtering::filter *> filters = { &b64decode_filter1, &b64decode_filter2, &b64decode_filter3, };
	
	ext::stream_filtering::filter_stream(params, filters, source, result);
	BOOST_CHECK_EQUAL(expected, result.str());
}

BOOST_AUTO_TEST_SUITE_END()
