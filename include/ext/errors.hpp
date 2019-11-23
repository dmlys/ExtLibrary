#pragma once
#include <string>
#include <system_error>
#include <boost/predef.h>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>

namespace ext
{
	/// formats error message like: <ErrCat>:<ErrCode>, <msg>
	/// for example: generic:0x1, file not found
	std::string format_error(std::error_code err);
	std::string format_error(boost::system::error_code err);

	/// re-formats error message std::system_error::what(), so it contains
	/// code description in format: <ErrCat>:<ErrCode>, <msg>
	std::string format_error(std::system_error & err);
	std::string format_error(boost::system::system_error err);

	inline std::string format_errno(int err)
	{
		return format_error(std::error_code(err, std::generic_category()));
	}

	inline void throw_last_errno(const std::string & errMsg)
	{
		throw std::system_error(errno, std::generic_category(), errMsg);
	}

	inline void throw_last_errno(const char * errMsg)
	{
		throw std::system_error(errno, std::generic_category(), errMsg);
	}
}

#if BOOST_OS_WINDOWS
namespace ext
{
	/// в windows system_category реализована через FormatMessageA, как следствие
	/// текст ошибки возвращается в некой местной однобайтовой кодировке + еще и локализованный.
	///
	/// Данные категории возвращают текст в utf-8 и MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)
	/// Фактически должен быть ASCII
	std::error_category const & system_utf8_category() noexcept;
	boost::system::error_category const & boost_system_utf8_category() noexcept;

	///эквивалентно throw std::system_error(GetLastError(), std::system_category(), errMsg);
	void throw_last_system_error(const char * errMsg);
	void throw_last_system_error(const std::string & errMsg);
}
#endif
