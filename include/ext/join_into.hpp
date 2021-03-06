#pragma once
#include <ext/type_traits.hpp>
#include <ext/range/range_traits.hpp>
#include <ext/range/str_view.hpp>
#include <boost/range/algorithm/copy.hpp>

namespace ext
{
	/// конкатенирует строки в списке input соединяя разделителем sep и записывает результат в out
	/// алгоритм - копия boost::algorihm::join с той лишь разницей что он не создает контейнер,
	/// а пишет в заданный итератор
	/// это позволяет использовать его с объектами не являющимися владельцами строк,
	/// как например boost::iterator_range, boost::string_ref
	/// 
	/// NOTE: если нужен join_if - используйте boost::adaptors::filtered
	template <class Range, class Separator, class OutIterator>
	auto join_into(const Range & input, const Separator & sep, OutIterator out)
		-> std::enable_if_t<ext::is_iterator<OutIterator>::value, OutIterator>
	{
		auto beg = boost::begin(input);
		auto end = boost::end(input);
		if (beg == end) return out;

		{
			auto && val = *beg;
			out = boost::copy(ext::str_view(val), out);
		}

		auto sepr = ext::str_view(sep);
		for (auto it = ++beg; it != end; ++it)
		{
			auto && val = *it;
			out = boost::copy(sepr, out);
			out = boost::copy(ext::str_view(val), out);
		}

		return out;
	}

	template <class Range, class Separator, class OutRange>
	auto join_into(const Range & input, const Separator & sep, OutRange & out)
		-> std::enable_if_t<ext::is_range<OutRange>::value>
	{
		auto beg = boost::begin(input);
		auto end = boost::end(input);
		if (beg == end) return;

		auto && val = *beg;
		ext::append(out, boost::begin(val), boost::end(val));

		auto sepr = ext::str_view(sep);
		for (auto it = ++beg; it != end; ++it)
		{
			auto && val = *it;
			ext::append(out, boost::begin(sepr), boost::end(sepr));
			ext::append(out, boost::begin(val), boost::end(val));
		}
	}

	template <class Range, class Separator>
	auto join(const Range & input, const Separator & sep)
	{
		using value_type = typename boost::range_value<Range>::type;
		using char_type = typename boost::range_value<value_type>::type;
		using string = std::basic_string<char_type>;

		string out;
		ext::join_into(input, sep, out);
		return out;
	}
}
