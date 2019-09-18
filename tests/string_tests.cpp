#include <boost/test/unit_test.hpp>
#include <boost/mp11.hpp>
#include <boost/mp11/mpl.hpp>

#include <ext/strings/cow_string.hpp>
#include <ext/strings/compact_string.hpp>


using test_list = boost::mp11::mp_list<
	ext::cow_string,
	ext::compact_string
>;


BOOST_AUTO_TEST_SUITE(string_tests)

BOOST_AUTO_TEST_CASE_TEMPLATE(assign_tests, string_type, test_list)
{
	string_type source;
	string_type str;

	source.assign("some text");
	BOOST_CHECK_EQUAL(source, "some text");

	str.assign(source, 3, 3);
	BOOST_CHECK_EQUAL(str, "e t");

	str.assign(source, 5);
	BOOST_CHECK_EQUAL(str, "text");

	auto first = str.data();
	str.assign(first, first + 2);
	BOOST_CHECK_EQUAL(str, "te");

	// overlap
	str.assign("some text");
	str.assign(str.data() + 1, 4);
	BOOST_CHECK_EQUAL(str, "ome ");

	str.assign("some text");
	str.assign(str, 5);
	BOOST_CHECK_EQUAL(str, "text");
}

BOOST_AUTO_TEST_CASE_TEMPLATE(append_tests, string_type, test_list)
{
	string_type source = "some text for append tests";
	string_type str;

	str.append(source);
	BOOST_CHECK_EQUAL(str, source);

	str.clear();
	str = "123 ";
	str.append(source, 10, 3);
	BOOST_CHECK_EQUAL(str, "123 for");

	auto first = source.data();
	str.append(first, first + 4);
	BOOST_CHECK_EQUAL(str, "123 forsome");

	first = str.data();
	str.append(first + 7, first + 7 + 4);
	BOOST_CHECK_EQUAL(str, "123 forsomesome");

	first = str.data();
	str.append(first, first + 3);
	BOOST_CHECK_EQUAL(str, "123 forsomesome123");

	// overlap
	str.assign("some text 123");
	str.append(str, 4, 5);
	BOOST_CHECK_EQUAL(str, "some text 123 text");

	str.append(str.data() + 10, 8);
	BOOST_CHECK_EQUAL(str, "some text 123 text123 text");
}

BOOST_AUTO_TEST_CASE_TEMPLATE(concat_tests, string_type, test_list)
{
	string_type str = "first";
	string_type res = "prefix " +
		str +
		" " +
		"second";
	BOOST_CHECK_EQUAL(res, "prefix first second");

	res = "prefix " + string_type("first") + " second";
	BOOST_CHECK_EQUAL(res, "prefix first second");

	res.clear();
	res.shrink_to_fit();
	str = "some string test";
	res = "some string testsome string test";

	str = str + str;
	BOOST_CHECK_EQUAL(str, res);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(insert_tests, string_type, test_list)
{
	string_type str = "some text";
	string_type toins = "instxt";

	str.insert(4, " " + toins);
	BOOST_CHECK_EQUAL(str, "some instxt text");

	str.insert(str.size(), str, 4, toins.size() + 1);
	BOOST_CHECK_EQUAL(str, "some instxt text instxt");
}

BOOST_AUTO_TEST_CASE_TEMPLATE(replace_tests, string_type, test_list)
{
	string_type str = "this is bad something";
	std::string vbad = "bad";        // std::string for different iterator type,
	std::string vgood = "verygood";  // so proper overload will be choosen

	auto pos = str.find(vbad.c_str());

	auto it = str.begin();
	str.replace(it + pos, it + pos + vbad.size(), vgood.begin(), vgood.end());
	BOOST_CHECK_EQUAL(str, "this is verygood something");

	it = str.begin();
	str.replace(it + pos, it + pos + vgood.size(), vbad.begin(), vbad.end());
	BOOST_CHECK_EQUAL(str, "this is bad something");

	str = "this is bad something";
	str.shrink_to_fit();

	str.replace(pos, vbad.size(), vgood.c_str());
	BOOST_CHECK_EQUAL(str, "this is verygood something");

	str.replace(pos, vgood.size(), vbad.c_str());
	BOOST_CHECK_EQUAL(str, "this is bad something");

	/************************************************************************/
	/*             replace overlap self_type overload tests                 */
	/************************************************************************/
	// overlap, substring begins before hole
	str = "this is bad something"; // -> "bad something is bad something"
	str.replace(0, 4, str, 8);
	BOOST_CHECK_EQUAL(str, "bad something is bad something");

	// overlap, substring begins after hole
	str = "some txt";
	str.replace(4, 1, str, 0);
	BOOST_CHECK_EQUAL(str, "somesome txttxt");

	// overlap, substring begins inside hole
	str = "abcdef";
	str.replace(0, 2, str, 1, 3);
	BOOST_CHECK_EQUAL(str, "bcdcdef");

	// overlap before and over hole
	str = "abcdef";
	str.replace(0, 4, str, 0, 5);
	BOOST_CHECK_EQUAL(str, "abcdeef");

	str = "123";
	str.replace(str.size(), 0, str);
	BOOST_CHECK_EQUAL(str, "123123");

	/************************************************************************/
	/*             replace overlap char *    overload test                  */
	/************************************************************************/
	// overlap, shrink test
	str = "bad something is bad something";
	auto f1 = str.data() + str.rfind("bad something");
	auto l1 = f1 + std::strlen("bad something");
	str.replace(str.begin(), str.end(), f1, l1);
	BOOST_CHECK_EQUAL(str, "bad something");


	// overlap, substring begins before hole
	str = "this is bad something"; // -> "bad something is bad something"
	str.replace(str.begin() + 0, str.begin() + 4, str.begin() + 8, str.end());
	BOOST_CHECK_EQUAL(str, "bad something is bad something");

	// overlap, substring begins after hole
	str = "some txt";
	str.replace(str.begin() + 4, str.begin() + 4 + 1, str.begin() + 0, str.end());
	BOOST_CHECK_EQUAL(str, "somesome txttxt");

	// overlap, substring begins inside hole
	str = "abcdef";
	str.replace(str.begin() + 0, str.begin() + 2, str.begin() + 1, str.begin() + 1 + 3);
	BOOST_CHECK_EQUAL(str, "bcdcdef");

	// overlap before and over hole
	str = "abcdef";
	str.replace(str.begin() + 0, str.begin() + 4, str.begin() + 0, str.begin() + 5);
	BOOST_CHECK_EQUAL(str, "abcdeef");
}

BOOST_AUTO_TEST_CASE_TEMPLATE(erase_tests, string_type, test_list)
{
	string_type str = "some text";

	str.erase(0, 5);
	BOOST_CHECK_EQUAL(str, "text");
}

BOOST_AUTO_TEST_CASE_TEMPLATE(string_view_tests, string_type, test_list)
{
	using namespace std::literals;

	string_type str("test"sv);
	BOOST_CHECK_EQUAL(str, "test");

	str += " world"sv;
	BOOST_CHECK_EQUAL(str, "test world");

	str.replace(0, 5, ""sv);
	BOOST_CHECK_EQUAL(str, "world");

	str.replace(0, str.size(), "world hello"sv, 6, str.npos);
	BOOST_CHECK_EQUAL(str, "hello");
}

BOOST_AUTO_TEST_CASE_TEMPLATE(find_tests, string_type, test_list)
{
	string_type s = "some";

	size_t pos;
	pos = s.find('s');
	BOOST_CHECK_EQUAL(pos, 0);

	pos = s.find("some");
	BOOST_CHECK_EQUAL(pos, 0);

	pos = s.find(s);
	BOOST_CHECK_EQUAL(pos, 0);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(rfind_tests, string_type, test_list)
{
	string_type s = "some";

	size_t pos;
	pos = s.rfind('s');
	BOOST_CHECK_EQUAL(pos, 0);

	pos = s.rfind("some");
	BOOST_CHECK_EQUAL(pos, 0);

	pos = s.rfind(s);
	BOOST_CHECK_EQUAL(pos, 0);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(find_first_of_tests, string_type, test_list)
{
	string_type s = "some";
	size_t pos;

	pos = s.find_first_of("om");
	BOOST_CHECK_EQUAL(pos, 1);

	pos = s.find_first_of("s", 0);
	BOOST_CHECK_EQUAL(pos, 0);

	pos = s.find_first_of("e", 3);
	BOOST_CHECK_EQUAL(pos, 3);

	pos = s.find_first_of(string_type("123"));
	BOOST_CHECK_EQUAL(pos, string_type::npos);

	pos = s.find_first_of(string_type("me"));
	BOOST_CHECK_EQUAL(pos, 2);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(find_last_of_tests, string_type, test_list)
{
	string_type s = "some";
	size_t pos;

	pos = s.find_last_of("om");
	BOOST_CHECK_EQUAL(pos, 2);

	pos = s.find_last_of("s", 0);
	BOOST_CHECK_EQUAL(pos, string_type::npos);

	pos = s.find_last_of("e", 3);
	BOOST_CHECK_EQUAL(pos, string_type::npos);

	pos = s.find_last_of(string_type("123"));
	BOOST_CHECK_EQUAL(pos, string_type::npos);

	pos = s.find_last_of(string_type("me"));
	BOOST_CHECK_EQUAL(pos, 3);
}


BOOST_AUTO_TEST_SUITE_END()
