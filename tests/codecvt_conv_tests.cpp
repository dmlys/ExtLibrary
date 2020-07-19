#include <boost/predef.h>
#if BOOST_OS_WINDOWS and not BOOST_PLAT_MINGW

#include <locale>
#include <codecvt>
#include <fstream>

#include <boost/test/unit_test.hpp>
#include <boost/range.hpp>
#include <boost/range/istream_range.hpp>
#include <boost/range/as_array.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext.hpp>
#include <boost/locale.hpp>

#include <ext/codecvt_conv.hpp>
#include <ext/filesystem_utils.hpp>
#include "test_files.h"

namespace
{
	struct CodeCvtByName : std::codecvt_byname<wchar_t, char, std::mbstate_t>
	{
		CodeCvtByName(const char * name) : std::codecvt_byname<wchar_t, char, std::mbstate_t>(name) {}
	};

	struct CodecvtFixture
	{
		std::codecvt_utf8<wchar_t> utf8;
		CodeCvtByName cp1251;
		CodeCvtByName cp1252;

		std::string martin_ru_utf8;
		std::string martin_ru_cp1251;
		std::wstring martin_ru;

		CodecvtFixture()
			: cp1251("ru_RU.cp1251"),
			  cp1252("en_US.cp1252")
		{
			ReadFiles();
		}

		void ReadFiles()
		{
			//std::cout << std::filesystem::current_path() << std::endl;
			LoadTestFile("CodecvtConvTestFiles/Martin_ru_utf8.txt", martin_ru_utf8, std::ios::binary);
			LoadTestFile("CodecvtConvTestFiles/Martin_ru_cp1251.txt", martin_ru_cp1251, std::ios::binary);

			auto file = test_files_location / "CodecvtConvTestFiles/Martin_ru_utf16LE.txt";
			std::wifstream ifs(file.c_str(), std::ios::binary);
			ifs >> std::noskipws;
			auto cvt = new std::codecvt_utf16<wchar_t, 0x10FFFF, std::codecvt_mode(/*std::little_endian &*/ std::consume_header)>;
			ifs.imbue(std::locale(std::locale(), cvt));
			std::copy(std::istream_iterator<wchar_t, wchar_t>(ifs), std::istream_iterator<wchar_t, wchar_t>(),
			          std::back_inserter(martin_ru));
		}
	};
}


BOOST_FIXTURE_TEST_SUITE( CvtSuite, CodecvtFixture )

BOOST_AUTO_TEST_CASE( BasicTest )
{
	using namespace std;
	wstring wstr = L"Test Русский Текст Test";
	wstring wstrRestored;
	string str;
	BOOST_CHECK_NO_THROW((ext::codecvt_convert::to_bytes(cp1251, wstr, str)));
	BOOST_CHECK(str == "Test Русский Текст Test");
	BOOST_CHECK_NO_THROW((ext::codecvt_convert::from_bytes(cp1251, str, wstrRestored)));
	BOOST_CHECK(wstr == wstrRestored);
}

BOOST_AUTO_TEST_CASE(ExtremeTest)
{
	//тестируем экстремально маленькие длины
	using namespace std;
	using namespace ext::codecvt_convert;
	wstring wstr = L"\x0420"; //russian 'Р'
	string str;
	str.resize(1);

	BOOST_CHECK_NO_THROW(to_bytes(utf8, wstr, str));
	BOOST_CHECK(str.size() == 2);
	
	char buffer[1] = {' '};
	auto output = boost::as_array(buffer);
	//conversion should zero convert, because utf-8 "Р" is 2 bytes long, no space for it
	BOOST_CHECK_NO_THROW(to_bytes(utf8, wstr, output));
	BOOST_CHECK(buffer[0] == 0);

}

BOOST_AUTO_TEST_CASE( FileTestUtf8 )
{
	using namespace std;
	string str;
	wstring wstr;
	str.resize(10);
	wstr.resize(10);


	ext::codecvt_convert::from_bytes(utf8, martin_ru_utf8, wstr);
	ext::codecvt_convert::to_bytes(utf8, martin_ru, str);
	BOOST_CHECK(wstr == martin_ru);
	BOOST_CHECK(str == martin_ru_utf8);

	string back;
	wstring wback;
	back.resize(10);

	ext::codecvt_convert::from_bytes(utf8, str, wback);
	ext::codecvt_convert::to_bytes(utf8, wstr, back);
	BOOST_CHECK(back == martin_ru_utf8);
	BOOST_CHECK(wback == martin_ru);
}

BOOST_AUTO_TEST_CASE( FileTestCp1251 )
{
	using namespace std;
	string str;
	wstring wstr;
	str.resize(10);
	wstr.resize(10);


	ext::codecvt_convert::from_bytes(cp1251, martin_ru_cp1251, wstr);
	ext::codecvt_convert::to_bytes(cp1251, martin_ru, str);
	BOOST_CHECK(wstr == martin_ru);
	BOOST_CHECK(str == martin_ru_cp1251);

	string back;
	wstring wback;
	back.resize(10);

	ext::codecvt_convert::from_bytes(cp1251, str, wback);
	ext::codecvt_convert::to_bytes(cp1251, wstr, back);
	BOOST_CHECK(back == martin_ru_cp1251);
	BOOST_CHECK(wback == martin_ru);
}

BOOST_AUTO_TEST_CASE( PartialTest )
{
	using namespace std;
	using namespace ext::codecvt_convert;

	wstring input = L"Проверка";
	string str = boost::locale::conv::utf_to_utf<char>(input);
	//not full char
	str.pop_back();
	codecvt_utf8<wchar_t> u8cvt;
	wstring wstr;
	BOOST_CHECK_THROW(wstr = from_bytes(u8cvt, str), conversion_failure);
}

BOOST_AUTO_TEST_SUITE_END()

#endif // BOOST_OS_WINDOWS
