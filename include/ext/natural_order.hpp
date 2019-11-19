#include <cctype> // for std::isdigit
#include <locale>
#include <tuple>
#include <string_view>
#include <boost/locale/collator.hpp>

#include <ext/range/str_view.hpp>
#include <ext/algorithm/tribool_compare.hpp>

/// natural order related stuff. natural order is where number parts of string are collated as numbers and not just chars.
/// So str1txt < str12txt with natural order, while if comparing them just as strings str1txt > str12txt
namespace ext::natural_order
{
	class default_number_traits;
	class default_char_traits;
	class localized_char_traits;

	template <class NumberTraits, class StringTraits>
	class comparator;

	/// default number traits, is_digit forwarded to std::isdigit
	/// leading zeroes are ignored, numbers are compared by longest run or greatest number, see implementation
	class default_number_traits
	{
	public:
		using result_type = int;

	public:
		template <class CharType>
		bool is_digit(CharType ch) const;

		template <class CharType>
		result_type compare(const CharType * s1_first, const CharType * s1_last, const CharType * s2_first, const CharType * s2_last) const;
	};

	template <class CharType>
	inline bool default_number_traits::is_digit(CharType ch) const
	{
		using uchar_type = std::make_unsigned_t<CharType>;
		return std::isdigit(static_cast<uchar_type>(ch));
	}

	template <class CharType>
	auto default_number_traits::compare(const CharType * s1_first, const CharType * s1_last, const CharType * s2_first, const CharType * s2_last) const -> result_type
	{
		// The longest run of digits wins, but if digit numbers is same - greatest number wins.
		// We remember relation, for later, and check it if digits number equal, otherwise - longest wins
		std::size_t z1 = 0, z2 = 0;
		for (; s1_first != s2_last and *s1_first == '0'; ++s1_first, ++z1);
		for (; s2_first != s2_last and *s2_first == '0'; ++s2_first, ++z2);

		result_type result = 0;
		for (;; ++s1_first, ++s2_first)
		{
			// equal runs - return stored result
			if (s1_first == s1_last and s2_first == s2_last)
				return result != 0 ? result
					// if equal - who has more zeroes - wins(is less)
					: ext::tribool_compare(z2, z1);

			// left shorter -> less
			if (s1_first == s1_last) return -1;
			// left longer  -> greater
			if (s2_first == s2_last) return +1;

			// remember less
			if (*s1_first < *s2_first)
			{ if (result == 0) result = -1; }
			// remember greater
			else if (*s1_first > *s2_first)
			{ if (result == 0) result = +1; }
		}

		return result != 0 ? result
			// if equal - who has more zeroes - wins(is less)
			: ext::tribool_compare(z2, z1);
	}

	/// default char_traits implementation. Compares string using std::char_traits
	class default_char_traits
	{
	public:
		using result_type = int;

	public:
		template <class CharType>
		result_type compare(const CharType * s1_first, const CharType * s1_last, const CharType * s2_first, const CharType * s2_last) const;
	};

	template <class CharType>
	auto default_char_traits::compare(const CharType * s1_first, const CharType * s1_last, const CharType * s2_first, const CharType * s2_last) const -> result_type
	{
		std::size_t n1 = s1_last - s1_first;
		std::size_t n2 = s2_last - s2_first;

		std::size_t n = std::min(n1, n2);
		result_type result = std::char_traits<CharType>::compare(s1_first, s2_first, n);
		if (result != 0) return result;

		return ext::tribool_compare(n1, n2);
	}

	/// char_traits implementation using std::locale via std::collator
	class localized_char_traits
	{
	public:
		using result_type = int;

	private:
		std::locale m_loc;
		boost::locale::collator_base::level_type m_compare_level = boost::locale::collator_base::identical;

		std::tuple<
			result_type (localized_char_traits::*)(const char     * s1_first, const char     * s1_last, const char     * s2_first, const char     * s2_last) const,
			result_type (localized_char_traits::*)(const wchar_t  * s1_first, const wchar_t  * s1_last, const wchar_t  * s2_first, const wchar_t  * s2_last) const,
			result_type (localized_char_traits::*)(const char16_t * s1_first, const char16_t * s1_last, const char16_t * s2_first, const char16_t * s2_last) const,
			result_type (localized_char_traits::*)(const char32_t * s1_first, const char32_t * s1_last, const char32_t * s2_first, const char32_t * s2_last) const
		> m_methods;

	private:
		template <class CharType> result_type   std_compare(const CharType * s1_first, const CharType * s1_last, const CharType * s2_first, const CharType * s2_last) const;
		template <class CharType> result_type boost_compare(const CharType * s1_first, const CharType * s1_last, const CharType * s2_first, const CharType * s2_last) const;

	public:
		template <class CharType>
		result_type compare(const CharType * s1_first, const CharType * s1_last, const CharType * s2_first, const CharType * s2_last) const;

	public:
		localized_char_traits(const std::locale & loc = std::locale());
		localized_char_traits(boost::locale::collator_base::level_type level, const std::locale & loc = std::locale());
	};

	localized_char_traits::localized_char_traits(const std::locale & loc)
	    : m_loc(loc)
	{
		m_methods = std::make_tuple(
			&localized_char_traits::std_compare<char>,
			&localized_char_traits::std_compare<wchar_t>,
	        &localized_char_traits::std_compare<char16_t>,
            &localized_char_traits::std_compare<char32_t>
		);
	}

	localized_char_traits::localized_char_traits(boost::locale::collator_base::level_type level, const std::locale & loc)
	    : m_loc(loc), m_compare_level(level)
	{
		if (m_compare_level == boost::locale::collator_base::identical)
		{
			m_methods = std::make_tuple(
				&localized_char_traits::std_compare<char>,
				&localized_char_traits::std_compare<wchar_t>,
		        &localized_char_traits::std_compare<char16_t>,
	            &localized_char_traits::std_compare<char32_t>
			);
		}
		else
		{
			m_methods = std::make_tuple(
				&localized_char_traits::boost_compare<char>,
				&localized_char_traits::boost_compare<wchar_t>,
		        &localized_char_traits::boost_compare<char16_t>,
	            &localized_char_traits::boost_compare<char32_t>
			);
		}
	}

	template <class CharType>
	auto localized_char_traits::std_compare(const CharType * s1_first, const CharType * s1_last, const CharType * s2_first, const CharType * s2_last) const -> result_type
	{
		using std_collator_facet = std::collate<CharType>;
		const auto & facet = std::use_facet<std_collator_facet>(m_loc);
		return facet.compare(s1_first, s1_last, s2_first, s2_last);
	}

	template <class CharType>
	auto localized_char_traits::boost_compare(const CharType * s1_first, const CharType * s1_last, const CharType * s2_first, const CharType * s2_last) const -> result_type
	{
		using boost_collator_facet = boost::locale::collator<CharType>;
		const auto & facet = std::use_facet<boost_collator_facet>(m_loc);
		return facet.compare(m_compare_level, s1_first, s1_last, s2_first, s2_last);
	}

	template <class CharType>
	inline auto localized_char_traits::compare(const CharType * s1_first, const CharType * s1_last, const CharType * s2_first, const CharType * s2_last) const -> result_type
	{
		using method = result_type (localized_char_traits::*)(const CharType * s1_first, const CharType * s1_last, const CharType * s2_first, const CharType * s2_last) const;
		return (this->*std::get<method>(m_methods))(s1_first, s1_last, s2_first, s2_last);
		//return std::invoke(std::get<method>(m_methods), this, s1_first, s1_last, s2_first, s2_last);
	}


	/// Comparator that can compare strings according to natural order.
	/// It is implemented via 2 traits classes:
	///  * NumberTraits - number related stuff:
	///    + is char a digit
	///    + compare numbers
	///  * CharTraits - compare string parts
	///
	/// it provides compare method returning +1/0/-1
	/// as well as relation predicates: less, greater, etc
	template <class NumberTraits, class CharTraits>
	class comparator
	{
	public:
		using number_traits = NumberTraits;
		using char_traits = CharTraits;
		using result_type = int;

	private:
		number_traits m_number_traits;
		char_traits m_char_traits;

	public:
		template <class StrViewCharType, class StrViewCharTraits>
		result_type compare(std::basic_string_view<StrViewCharType, StrViewCharTraits> s1, std::basic_string_view<StrViewCharType, StrViewCharTraits> s2) const;

		template <class String1, class String2>
		result_type compare(const String1 & s1, const String2 & s2) const { return compare(ext::str_view(s1), ext::str_view(s2)); }

	public:
		auto less()          const { return [*this](const auto & ...args) { return compare(args...) <  0; }; }
		auto less_equal()    const { return [*this](const auto & ...args) { return compare(args...) <= 0; }; }
		auto greater()       const { return [*this](const auto & ...args) { return compare(args...) >  0; }; }
		auto greater_equal() const { return [*this](const auto & ...args) { return compare(args...) >= 0; }; }
		auto equal_to()      const { return [*this](const auto & ...args) { return compare(args...) == 0; }; }
		auto not_equal_to()  const { return [*this](const auto & ...args) { return compare(args...) != 0; }; }

	public:
		comparator() = default;
		constexpr comparator(number_traits number_traits, char_traits char_traits)
		    : m_number_traits(std::move(number_traits)), m_char_traits(std::move(char_traits)) {}
	};

	template <class NumberTraits, class CharTraits>
	template <class StrViewCharType, class StrViewCharTraits>
	auto comparator<NumberTraits, CharTraits>::compare(std::basic_string_view<StrViewCharType, StrViewCharTraits> s1, std::basic_string_view<StrViewCharType, StrViewCharTraits> s2) const -> result_type
	{
		using string_type_view = std::basic_string_view<StrViewCharType, StrViewCharTraits>;
		using char_type = typename string_type_view::value_type;

		const char_type * s1_first = s1.data();
		const char_type * s1_last  = s1_first + s1.size();

		const char_type * s2_first = s2.data();
		const char_type * s2_last  = s2_first + s2.size();

		decltype(s1_first) s1_dfirst, s1_dlast;
		decltype(s2_first) s2_dfirst, s2_dlast;

		result_type result;

		while (s1_first != s1_last and s2_first != s2_last)
		{
			s1_dfirst = s1_first;
			s2_dfirst = s2_first;

			while (s1_dfirst != s1_last and not m_number_traits.is_digit(*s1_dfirst))
				++s1_dfirst;

			while (s2_dfirst != s2_last and not m_number_traits.is_digit(*s2_dfirst))
				++s2_dfirst;

			result = m_char_traits.compare(s1_first, s1_dfirst, s2_first, s2_dfirst);
			if (result != 0) return result;

			s1_dlast = s1_dfirst;
			s2_dlast = s2_dfirst;

			while (s1_dlast != s1_last and m_number_traits.is_digit(*s1_dlast))
				++s1_dlast;

			while (s2_dlast != s2_last and m_number_traits.is_digit(*s2_dlast))
				++s2_dlast;

			result = m_number_traits.compare(s1_dfirst, s1_dlast, s2_dfirst, s2_dlast);
			if (result != 0) return result;

			s1_first = s1_dlast;
			s2_first = s2_dlast;
		}

		return 0;
	}

	auto locale_comparator(                                                std::locale loc = std::locale()) { return comparator(default_number_traits(), localized_char_traits(loc)); }
	auto locale_comparator(boost::locale::collator_base::level_type level, std::locale loc = std::locale()) { return comparator(default_number_traits(), localized_char_traits(level, loc)); }
	
	constexpr comparator<default_number_traits, default_char_traits> natural_comparator {};
}

namespace ext
{
	using natural_order::natural_comparator;
}
