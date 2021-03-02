#include <boost/test/unit_test.hpp>
#include <boost/mp11.hpp>
#include <boost/mp11/mpl.hpp>

#include <ext/codecvt_conv/generic_conv.hpp>
#include <ext/codecvt_conv/wchar_cvt.hpp>

// wchar_cvt stuff internally uses heavily ext::codecvt_convert::to_bytes/from_bytes,
// those already have tests, so current tests are more compile template overloads tests

using utf8_types  = boost::mp11::mp_list<char, char16_t, char32_t, wchar_t>;
using utf16_types = boost::mp11::mp_list<char, char16_t>;
using utf32_types = boost::mp11::mp_list<char, char32_t>;
using wchar_types = boost::mp11::mp_list<char, wchar_t>;

BOOST_AUTO_TEST_SUITE(wpath_cvt_tests)

BOOST_AUTO_TEST_CASE_TEMPLATE(to_utf8, char_type, utf8_types)
{
	using string_type = std::basic_string<char_type>;
	
	auto data = ext::str_view("data");
	string_type input;
	std::string result, expected;
	
	input.assign(data.begin(), data.end());
	expected.assign(data.begin(), data.end());
	
	ext::codecvt_convert::wchar_cvt::to_utf8(input.data(), input.size(), result);
	BOOST_CHECK_EQUAL(result, expected);
	
	result.clear();
	ext::codecvt_convert::wchar_cvt::to_utf8(input.data(), result);
	BOOST_CHECK_EQUAL(result, expected);
	
	result.clear();
	ext::codecvt_convert::wchar_cvt::to_utf8(input, result);
	BOOST_CHECK_EQUAL(result, expected);
	
	result = ext::codecvt_convert::wchar_cvt::to_utf8(input);
	BOOST_CHECK_EQUAL(result, expected);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(to_utf16, char_type, utf16_types)
{
	using string_type = std::basic_string<char_type>;
	
	auto data = ext::str_view("data");
	string_type input;
	std::u16string result, expected;
	
	input.assign(data.begin(), data.end());
	expected.assign(data.begin(), data.end());
	
	ext::codecvt_convert::wchar_cvt::to_utf16(input.data(), input.size(), result);
	BOOST_CHECK(result == expected);
	
	result.clear();
	ext::codecvt_convert::wchar_cvt::to_utf16(input.data(), result);
	BOOST_CHECK(result == expected);
	
	result.clear();
	ext::codecvt_convert::wchar_cvt::to_utf16(input, result);
	BOOST_CHECK(result == expected);
	
	result = ext::codecvt_convert::wchar_cvt::to_utf16(input);
	BOOST_CHECK(result == expected);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(to_utf32, char_type, utf32_types)
{
	using string_type = std::basic_string<char_type>;
	
	auto data = ext::str_view("data");
	string_type input;
	std::u32string result, expected;
	
	input.assign(data.begin(), data.end());
	expected.assign(data.begin(), data.end());
	
	ext::codecvt_convert::wchar_cvt::to_utf32(input.data(), input.size(), result);
	BOOST_CHECK(result == expected);
	
	result.clear();
	ext::codecvt_convert::wchar_cvt::to_utf32(input.data(), result);
	BOOST_CHECK(result == expected);
	
	result.clear();
	ext::codecvt_convert::wchar_cvt::to_utf32(input, result);
	BOOST_CHECK(result == expected);
	
	result = ext::codecvt_convert::wchar_cvt::to_utf32(input);
	BOOST_CHECK(result == expected);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(to_wchar, char_type, wchar_types)
{
	using string_type = std::basic_string<char_type>;
	
	auto data = ext::str_view("data");
	string_type input;
	std::wstring result, expected;
	
	input.assign(data.begin(), data.end());
	expected.assign(data.begin(), data.end());
	
	ext::codecvt_convert::wchar_cvt::to_wchar(input.data(), input.size(), result);
	BOOST_CHECK(result == expected);
	
	result.clear();
	ext::codecvt_convert::wchar_cvt::to_wchar(input.data(), result);
	BOOST_CHECK(result == expected);
	
	result.clear();
	ext::codecvt_convert::wchar_cvt::to_wchar(input, result);
	BOOST_CHECK(result == expected);
	
	result = ext::codecvt_convert::wchar_cvt::to_wchar(input);
	BOOST_CHECK(result == expected);	
}


BOOST_AUTO_TEST_SUITE_END()
