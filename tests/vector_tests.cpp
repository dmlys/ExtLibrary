#include <boost/test/unit_test.hpp>
#include <boost/mp11.hpp>
#include <boost/mp11/mpl.hpp>

#include <ext/container/vector.hpp>

using test_list = boost::mp11::mp_list<
	//std::vector<std::string>,
	ext::container::vector<std::string>
>;

#define CHECK_EQUAL_COLLECTIONS(c1, c2) BOOST_CHECK_EQUAL_COLLECTIONS(c1.begin(), c1.end(), c2.begin(), c2.end())


BOOST_AUTO_TEST_SUITE(vector_facade_tests)


BOOST_AUTO_TEST_SUITE(basic_tests)

BOOST_AUTO_TEST_CASE_TEMPLATE(simple, vector_type, test_list)
{
	vector_type v = {"test", "123"};

	BOOST_CHECK_EQUAL(v[0], "test");
	BOOST_CHECK_EQUAL(v[1], "123");

	BOOST_CHECK_EQUAL(v.at(0), "test");
	BOOST_CHECK_EQUAL(v.at(1), "123");

	BOOST_CHECK_THROW(v.at(2), std::out_of_range);
	BOOST_CHECK_THROW(v.at(22), std::out_of_range);
	BOOST_CHECK_THROW(v.at(SIZE_MAX), std::out_of_range);

	BOOST_CHECK_EQUAL(v.front(), "test");
	BOOST_CHECK_EQUAL(v.back(),  "123");

	BOOST_CHECK_EQUAL(*v.data(), "test");
	BOOST_CHECK_EQUAL(*v.begin(), "test");
	BOOST_CHECK_EQUAL(*v.cbegin(), "test");
	BOOST_CHECK_EQUAL(*v.rbegin(), "123");
	BOOST_CHECK_EQUAL(*v.crbegin(), "123");

	BOOST_CHECK_THROW(v.resize(SIZE_MAX), std::length_error);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(single_insert, vector_type, test_list)
{
	vector_type v = {"test", "Hello", "world"};

	v.insert(v.begin(), "test1");
	v.insert(v.begin() + 2, "test_middle");
	v.reserve(10);
	v.insert(v.begin(), "test2");
	v.insert(v.end(), "test_back");

	auto expected = {"test2", "test1", "test", "test_middle", "Hello", "world", "test_back"};
	CHECK_EQUAL_COLLECTIONS(v, expected);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(emplace_back, vector_type, test_list)
{
	vector_type v = {"initial",};

	v.emplace_back("emplace_back");
	v.push_back("push_back");
	v.reserve(10);
	v.emplace_back("emplace_back_2");
	v.push_back("push_back_2");

	auto expected = {"initial", "emplace_back", "push_back", "emplace_back_2", "push_back_2"};
	CHECK_EQUAL_COLLECTIONS(v, expected);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(range_inserts, vector_type, test_list)
{
	vector_type v;

	v.insert(v.end(), {"initial"});

	v.reserve(20);

	v.insert(v.begin(), {"first", "second", "third"});
	v.insert(v.begin() + 3, {"forth", "fifth", "sixth"});

	v.insert(v.begin() + 2, {"1", "2"});
	v.insert(v.begin() + 4, 3, "dup");

	v.shrink_to_fit();
	v.insert(v.begin() + 7, {"test"});

	auto expected = {"first", "second", "1", "2", "dup", "dup", "dup", "test", "third", "forth", "fifth", "sixth", "initial"};
	CHECK_EQUAL_COLLECTIONS(v, expected);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(assigns, vector_type, test_list)
{
	using value_type = typename vector_type::value_type;

	vector_type v;
	v.assign(5, "test");

	auto expected1 = {"test", "test", "test", "test", "test", };
	CHECK_EQUAL_COLLECTIONS(v, expected1);

	std::stringstream str("test alpha mega gamma");
	std::istream_iterator<value_type> first(str), last;
	v.assign(first, last);

	auto expected2 = {"test", "alpha", "mega", "gamma"};
	CHECK_EQUAL_COLLECTIONS(v, expected2);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(erases, vector_type, test_list)
{
	vector_type v;
	v = {"test", "Hello", "world"};
	v.insert(v.end(), 10, "dup");

	auto it = v.erase(v.begin() + 2, v.end() - 1);

	auto expected1 = {"test", "Hello", "dup"};
	CHECK_EQUAL_COLLECTIONS(v, expected1);
	BOOST_CHECK_EQUAL(*it, "dup");

	it = v.erase(v.begin() + 1);

	auto expected2 = {"test", "dup"};
	CHECK_EQUAL_COLLECTIONS(v, expected2);
	BOOST_CHECK_EQUAL(*it, "dup");
}

BOOST_AUTO_TEST_SUITE_END() // suite basic_tests


BOOST_AUTO_TEST_SUITE_END() // suite vector_facade_tests
