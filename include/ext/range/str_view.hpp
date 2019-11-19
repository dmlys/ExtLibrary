#pragma once
// author: Dmitry Lysachenko
// date: Tuesday 07 February 2017
// license: boost software license
//          http://www.boost.org/LICENSE_1_0.txt
//          

#include <ciso646>
#include <cassert>
#include <utility>
#include <type_traits>
#include <string_view>

#include <ext/type_traits.hpp>
#include <ext/range/range_traits.hpp>


namespace ext
{
	template <class String>
	struct str_view_traits
	{
		using char_type = ext::range_value_t<String>;
		using string_view = std::basic_string_view<char_type>;

		inline static string_view str_view(const String & str)
		{
			auto first = ext::data(str);
			auto size = boost::size(str);
			return string_view(first, size);
		}
	};

	/// similar to boost::as_literal, but tries to return std::string_view,
	/// thus working only with contiguous string ranges, string literals and std::string are those.
	/// Also explicitly deleted for temporary objects
	template <class String>
	inline auto str_view(const String & str) ->
		std::enable_if_t<ext::is_contiguous_range_v<String>, typename str_view_traits<String>::string_view>
	{
		return str_view_traits<String>::str_view(str);
	}

	template <class String>
	inline auto str_view(String & str) ->
		std::enable_if_t<ext::is_contiguous_range_v<String>, typename str_view_traits<String>::string_view>
	{
		return str_view_traits<String>::str_view(str);
	}

	// if it's a temp object - we do not want to introduce dangling strings
	template <class String>
	auto str_view(String && range) = delete;



	// boost::iterator_range overloads
	template <class CharType>
	inline auto str_view(boost::iterator_range<CharType *> rng) -> std::basic_string_view<CharType>
	{
		auto * first = rng.begin();
		auto size = rng.size();
		return std::basic_string_view(first, size);
	}

	template <class CharType>
	inline auto str_view(boost::iterator_range<const CharType *> rng) -> std::basic_string_view<CharType>
	{
		auto * first = rng.begin();
		auto size = rng.size();
		return std::basic_string_view(first, size);
	}

	// string literals overloads
	template <class CharType, std::size_t N>
	inline auto str_view(CharType (& str)[N]) -> std::basic_string_view<std::decay_t<CharType>>
	{
		assert(str[N - 1] == 0); // null terminated string
		return std::basic_string_view(str, N - 1);
	}
	
	template <class CharType>
	inline auto str_view(CharType *& str) -> std::basic_string_view<std::decay_t<CharType>>
	{
		return std::basic_string_view(str, std::char_traits<std::decay_t<CharType>>::length(str));
	}

	template <class CharType>
	inline auto str_view(CharType * const & str) -> std::basic_string_view<std::decay_t<CharType>>
	{
		return std::basic_string_view(str, std::char_traits<std::decay_t<CharType>>::length(str));
	}
}
