#pragma once
#include <boost/predef.h>

#include <codecvt>
#include <string_view>

#include <boost/range.hpp>
#include <boost/range/as_literal.hpp>

#include <ext/type_traits.hpp>
#include <ext/range/range_traits.hpp>
#include <ext/range/str_view.hpp>

#include <ext/codecvt_conv/base.hpp>
#include <ext/codecvt_conv/generic_conv.hpp>


/// Overall you should just use utf-8 held in char strings.
/// But you can be forced to work with API that uses wchar, famous example windows.
/// This class provides codecvt and helper functions to convert char <-> wchar

namespace ext::codecvt_convert::wchar_cvt
{
	// on MSVC wchar_t is utf-16, but on GCC it's utf-32 actually
	extern const std::codecvt_utf8_utf16<char16_t, 0x10FFFF, std::codecvt_mode::little_endian> u16_cvt;
	extern const std::codecvt_utf8      <char32_t, 0x10FFFF, std::codecvt_mode::little_endian> u32_cvt;
	
	
	namespace detail
	{
		template <class CharT, class Container>
		class container_wrapper
		{
			Container * m_cont;
			
		public:
			using size_type = typename Container::size_type;
			using difference_type = typename Container::difference_type;
			
			using value_type = CharT;
			using pointer = value_type *;
			using const_pointer = const value_type *;
			using reference = value_type &;
			using const_reference = const value_type &;
			
			using iterator = pointer;
			using const_iterator = const_pointer;
			
		public:
			inline size_type size() const { return m_cont->size(); }
			inline void resize(size_type sz) { m_cont->resize(sz); }

			inline value_type * data()             { return reinterpret_cast<      value_type *>(ext::data(*m_cont)); }
			inline const value_type * data() const { return reinterpret_cast<const value_type *>(ext::data(*m_cont)); }

			inline iterator begin() { return data(); }
			inline iterator end()   { return data() + m_cont->size(); }

			inline const_iterator begin() const { return data(); }
			inline const_iterator end() const   { return data() + m_cont->size(); }
			
		public:
			container_wrapper(Container & cont)
			    : m_cont(&cont) {}
		};
	}
	
	
	
	/// to_utf8  - convert some wchar/char16_t/char32_t string to utf8 char container
	/// to_utf16 - convert some utf8 string to utf16 char16_t string
	/// to_utf32 - convert some utf8 string to utf32 char32_t string
	/// to_wchar - convert some utf8 string to       wchar_t  string
	
	
	/************************************************************************/
	/*                           to_utf8                                    */
	/************************************************************************/
	template <class Container>
	inline void to_utf8(const char * utf8_str, std::size_t len, Container & cont)
	{
		static_assert(ext::is_range_of_v<Container, char>, "Not a char container");
		ext::append(cont, utf8_str, utf8_str + len);
	}
	
	template <class Container>
	inline void to_utf8(const char16_t * utf16_str, std::size_t len, Container & cont)
	{
		static_assert(ext::is_range_of_v<Container, char>, "Not a char container");
		
		using string_view = std::basic_string_view<char16_t>;
		ext::codecvt_convert::to_bytes(u16_cvt, string_view(utf16_str, len), cont);
	}
	
	template <class Container>
	inline void to_utf8(const char32_t * utf32_str, std::size_t len, Container & cont)
	{
		static_assert(ext::is_range_of_v<Container, char>, "Not a char container");
		
		using string_view = std::basic_string_view<char32_t>;
		ext::codecvt_convert::to_bytes(u32_cvt, string_view(utf32_str, len), cont);
	}
	
	template <class Container>
	inline void to_utf8(const wchar_t * wchar_str, std::size_t len, Container & cont)
	{
		static_assert(ext::is_range_of_v<Container, char>, "Not a char container");
		
		if constexpr (sizeof(wchar_t) == sizeof(char16_t))
		{
			using string_view = std::basic_string_view<char16_t>;
			ext::codecvt_convert::to_bytes(u16_cvt, string_view(reinterpret_cast<const char16_t *>(wchar_str), len), cont);
		}
		else
		{
			using string_view = std::basic_string_view<char32_t>;
			ext::codecvt_convert::to_bytes(u32_cvt, string_view(reinterpret_cast<const char32_t *>(wchar_str), len), cont);
		}
	}
	
	template <class InputRange, class Container>
	inline void to_utf8(const InputRange & input, Container & cont)
	{
		static_assert(ext::is_range_of_v<Container, char>, "Not a char container");
		using char_t = ext::range_value_t<InputRange>;
		
		auto input_view = ext::str_view(input);
		
		if      constexpr (std::is_same_v<char_t, char>)
			ext::append(cont, boost::begin(input_view), boost::end(input_view));
		else if constexpr (std::is_same_v<char_t, char16_t>)
			ext::codecvt_convert::to_bytes(u16_cvt, input_view, cont);
		else if constexpr (std::is_same_v<char_t, char32_t>)
			ext::codecvt_convert::to_bytes(u32_cvt, input_view, cont);
		else
		{
			static_assert(std::is_same_v<char_t, wchar_t>, "Unsupported input char type");
			
			auto * ptr = ext::data(input_view);
			auto size  = boost::size(input_view);
			
			if constexpr (sizeof(wchar_t) == sizeof(char16_t))
			{
				using string_view = std::basic_string_view<char16_t>;
				ext::codecvt_convert::to_bytes(u16_cvt, string_view(reinterpret_cast<const char16_t *>(ptr), size), cont);
			}
			else
			{
				using string_view = std::basic_string_view<char32_t>;
				ext::codecvt_convert::to_bytes(u32_cvt, string_view(reinterpret_cast<const char32_t *>(ptr), size), cont);
			}
		}
	}
	
	template <class InputRange, class Container = std::string>
	inline Container to_utf8(const InputRange & input)
	{
		Container cont;
		to_utf8(input, cont);
		return cont;
	}
	
	
	/************************************************************************/
	/*                           to_utf16                                   */
	/************************************************************************/
	template <class Container>
	inline void to_utf16(const char * utf8_str, std::size_t len, Container & cont)
	{
		static_assert(ext::is_range_of_v<Container, char16_t>, "Not a char16_t container");
		
		using string_view = std::basic_string_view<char>;
		ext::codecvt_convert::from_bytes(u16_cvt, string_view(utf8_str, len), cont);
	}

	template <class Container>
	inline void to_utf16(const char16_t * utf16_str, std::size_t len, Container & cont)
	{
		static_assert(ext::is_range_of_v<Container, char16_t>, "Not a char16_t container");
		ext::append(cont, utf16_str, utf16_str + len);
	}
	
	template <class InputRange, class Container>
	inline void to_utf16(const InputRange & input, Container & cont)
	{
		static_assert(ext::is_range_of_v<Container, char16_t>, "Not a char16_t container");
		
		using char_t = typename boost::range_value<InputRange>::type;
		auto input_view = ext::str_view(input);

		if constexpr (std::is_same_v<char_t, char16_t>)
			ext::append(cont, boost::begin(input_view), boost::end(input_view));
		else
		{
			static_assert(std::is_same_v<char_t, char>, "Not a char input range");
			ext::codecvt_convert::from_bytes(u16_cvt, input_view, cont);
		}
	}
	
	template <class InputRange, class Container = std::u16string>
	inline Container to_utf16(const InputRange & input)
	{
		Container cont;
		to_utf16(input, cont);
		return cont;
	}
	
	/************************************************************************/
	/*                           to_utf32                                   */
	/************************************************************************/
	template <class Container>
	inline void to_utf32(const char * utf8_str, std::size_t len, Container & cont)
	{
		static_assert(ext::is_range_of_v<Container, char32_t>, "Not a char32_t container");
		
		using string_view = std::basic_string_view<char>;
		ext::codecvt_convert::from_bytes(u32_cvt, string_view(utf8_str, len), cont);
	}

	template <class Container>
	inline void to_utf32(const char32_t * utf32_str, std::size_t len, Container & cont)
	{
		static_assert(ext::is_range_of_v<Container, char32_t>, "Not a char32_t container");
		ext::append(cont, utf32_str, utf32_str + len);
	}
	
	template <class InputRange, class Container>
	inline void to_utf32(const InputRange & input, Container & cont)
	{
		static_assert(ext::is_range_of_v<Container, char32_t>, "Not a char32_t container");
		
		using char_t = typename boost::range_value<InputRange>::type;
		auto input_view = ext::str_view(input);
		
		if constexpr (std::is_same_v<char_t, char32_t>)
			ext::append(cont, boost::begin(input_view), boost::end(input_view));
		else
		{
			static_assert(std::is_same_v<char_t, char>, "Not a char input range");
			ext::codecvt_convert::from_bytes(u32_cvt, input_view, cont);
		}
	}
	
	template <class InputRange, class Container = std::u32string>
	inline Container to_utf32(const InputRange & input)
	{
		Container cont;
		to_utf32(input, cont);
		return cont;
	}
	
	
	/************************************************************************/
	/*                           to_wchar                                   */
	/************************************************************************/
	template <class Container>
	inline void to_wchar(const char * utf8_str, std::size_t len, Container & cont)
	{
		static_assert(ext::is_range_of_v<Container, wchar_t>, "Not a wchar_t container");
		
		using string_view = std::basic_string_view<char>;
		if constexpr (sizeof(wchar_t) == sizeof(char16_t))
		{
			detail::container_wrapper<char16_t, Container> wrapper { cont };
			ext::codecvt_convert::from_bytes(u16_cvt, string_view(utf8_str, len), wrapper);
		}
		else
		{
			detail::container_wrapper<char32_t, Container> wrapper { cont };
			ext::codecvt_convert::from_bytes(u32_cvt, string_view(utf8_str, len), wrapper);
		}
	}
	
	template <class Container>
	inline void to_wchar(const wchar_t * wchar_str, std::size_t len, Container & cont)
	{
		static_assert(ext::is_range_of_v<Container, wchar_t>, "Not a wchar_t container");
		ext::append(cont, wchar_str, wchar_str + len);
	}

	template <class InputRange, class Container>
	inline void to_wchar(const InputRange & input, Container & cont)
	{
		static_assert(ext::is_range_of_v<Container, wchar_t>, "Not a wchar_t container");
		using char_t = typename boost::range_value<InputRange>::type;
		auto input_view = ext::str_view(input);
		
		if constexpr (std::is_same_v<char_t, wchar_t>)
			ext::append(cont, boost::begin(input_view), boost::end(input_view));
		else 
		{
			static_assert(std::is_same_v<char_t, char>, "Not a char input range");
			
			if constexpr (sizeof(wchar_t) == sizeof(char16_t))
			{
				detail::container_wrapper<char16_t, Container> wrapper { cont };
				ext::codecvt_convert::from_bytes(u16_cvt, input_view, wrapper);
			}
			else
			{
				detail::container_wrapper<char32_t, Container> wrapper { cont };
				ext::codecvt_convert::from_bytes(u32_cvt, input_view, wrapper);
			}
		}
	}
	
	template <class InputRange, class Container = std::wstring>
	inline Container to_wchar(const InputRange & input)
	{
		Container cont;
		to_wchar(input, cont);
		return cont;
	}
}
