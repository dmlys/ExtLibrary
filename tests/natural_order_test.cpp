#include <ext/natural_order.hpp>
#include <boost/locale/generator.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(natural_order_tests)

BOOST_AUTO_TEST_CASE(simple_tests)
{
	BOOST_CHECK_EQUAL(ext::natural_comparator.compare("test10", "test1"), +1);
	BOOST_CHECK_EQUAL(ext::natural_comparator.compare("test1" , "test1"),  0);
	BOOST_CHECK_EQUAL(ext::natural_comparator.compare("test01", "test1"), -1);
	BOOST_CHECK_EQUAL(ext::natural_comparator.compare("test01", "test002"), -1);

	std::vector<std::string> input, expected;
	input = {"test01", "test002", "test1", "test2"};
	expected = {"test01", "test1", "test002", "test2"};

	auto actual = input;
	std::sort(actual.begin(), actual.end(), ext::natural_comparator.less());
	BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(locale_tests)
{
	std::vector<std::string> input, expected;
	input = {"test01", "test002", "test1", "test2"};
	expected = {"test01", "test1", "test002", "test2"};

	auto actual = input;
	auto sorter = ext::natural_order::locale_comparator(std::locale()).less();
	std::sort(actual.begin(), actual.end(), sorter);
	BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(locale_icase_tests)
{
	std::vector<std::string> input, expected;
	input = {"Test01", "tEst002", "TEST1", "TeST2"};
	expected = {"Test01", "TEST1", "tEst002", "TeST2"};

	boost::locale::generator gen;
	std::locale loc = gen("");
	auto sorter = ext::natural_order::locale_comparator(boost::locale::collator_base::primary, loc).less();

	auto actual = input;
	std::sort(actual.begin(), actual.end(), sorter);
	BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_SUITE_END()
