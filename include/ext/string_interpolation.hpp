#pragma once
#include <type_traits>
#include <iterator>
#include <utility>  // for std::pair
#include <tuple>    // for std::tie

#include <ext/range/range_traits.hpp>
#include <boost/range/as_literal.hpp>

/// функции интерполяции строк подобно unix shell оболочкам,
/// но допустимые символы обрабатываются несколько упрощенно
/// ${var} для имени допустимы любые символы
/// $var допустимы только 0-9a-zA-Z_
///  $, $0, $_, $var
/// поддерживается escape'инг \$var, $var\ cont, ${var\}}
/// 
/// пример использование:
///     std::map<string, string> dict = {{"name", "bond"}, {"date", "friday"}};
///     string res = ext::interpolate("my name is %name and this is test on %date val%", dict);
/// 
/// Dictionary - класс словаря:
///   * должен иметь как минимум key_type typedef - тип ключа,
///     некоторый строковый stl-like container совместимый с входной строкой
///   * должна быть глобальная функция dictionary_find:
///     auto res = dictionary_find(dict, key);
///     результат - некоторый тип:
///       - конвертируемый в bool, если данные по ключу найдены - true, иначе false
///       - при разыменование - возвращает некий string range того же типа что и входной текст
///         например, это может быть указатель(как простой так и smart), boost::optional
///
/// входной range любой boost::range совместимый range символов.
/// Поддерживаемые типы символов: char/char16_t/char32_t/wchar_t.
/// выходной параметр итератор или контейнер.
/// для контейнера должны быть специализированна функция ext::append.

namespace ext
{
	template <class StdMapType, class Key>
	const typename StdMapType::mapped_type * dictionary_find(const StdMapType & map, const Key & key);

	namespace interpolate_internal
	{
		/// карта допустимых символов для переменных в не расширенном формате.
		/// это 0-9 a-z A-Z _
		extern const bool key_allowed_ascii_chars[128];

		struct key_start
		{
			template <class Char>
			inline bool operator()(Char ch) const { return ch == '$'; }
		};

		struct extended_key_stop
		{
			template <class Char>
			inline bool operator()(Char ch) const { return ch == '}'; }
		};
		
		struct key_stop
		{
			template <class Char,
				typename std::enable_if<std::is_signed<Char>::value, int>::type = 0
			>
			inline bool operator()(Char ch) const
			{
				return operator()(typename std::make_unsigned<Char>::type(ch));
			}

			template <class Char,
				typename std::enable_if< ! std::is_signed<Char>::value, int>::type = 0
			>
			inline bool operator()(Char ch) const
			{
				return ch > 127 || !key_allowed_ascii_chars[ch];
			}
		};



		template <class Iterator, class OutputIterator, class StopFunc>
		std::pair<Iterator, OutputIterator> copy_escaped_until(Iterator first, Iterator last, OutputIterator out, StopFunc stop)
		{
			for (; first != last; ++first, ++out)
			{
				auto ch = *first;
				switch (ch)
				{
				// экранирующий символ, пока просто копируем следующий как есть.
				// при желании, обработку можно будет расширить
				case '\\':
					if (first == last) goto exit;
					*out = *++first;
					continue;

				default:
					if (stop(ch)) goto exit;
					*out = ch;
				}
			}

		exit: return {first, out};
		}
		
		
		template <class Iterator, class OutIterator, class Dictionary>
		OutIterator interpolate_into_iterator(Iterator first, Iterator last, OutIterator out, const Dictionary & dict)
		{
			typedef typename std::iterator_traits<Iterator>::value_type char_type;
			typedef typename Dictionary::key_type key_type;

			key_type key;
			for (;;)
			{
				// ищем ключ, копируем строку
				std::tie(first, out) = copy_escaped_until(first, last, out, key_start());
				// не нашли символ ключа -> скопировали весь шаблон
				if (first == last)
					return out;
				
				++first;
				// после символа ключа - конец строки. копируем символ и выходим
				if (first == last) {
					*out++ = '$';
					return out;
				}

				bool extended;
				ext::clear(key);
				auto keyIt = std::back_inserter(key);
				// расширенный ключ
				if (*first == '{')
				{
					extended = true;
					std::tie(first, std::ignore) = copy_escaped_until(++first, last, keyIt, extended_key_stop());
					// ключ не закрыт
					if (first == last)
					{
						*out++ = '$'; *out++ = '{';
						out = std::copy(std::begin(key), std::end(key), out);
						return out;
					}

					++first; // съедаем '}'
				} // обычный ключ
				else
				{
					extended = false;
					std::tie(first, std::ignore) = copy_escaped_until(first, last, keyIt, key_stop());
					
					// $<space> - копируем символ и идем в начало алгоритма
					if (boost::empty(key)) {
						*out++ = '$';
						continue;
					}
				}
				
				/// выполняем подстановку
				using ext::dictionary_find;
				auto val = dictionary_find(dict, key);
				if (val)
					out = std::copy(boost::const_begin(*val), boost::const_end(*val), out);
				else
				{   // ключ 'отклонено' - копируем имя ключа как есть из шаблона
					*out++ = '$';
					if (extended) *out++ = '{';
					out = std::copy(boost::const_begin(key), boost::const_end(key), out);
					if (extended) *out++ = '}';
				}
			}
		}

		template <class Iterator, class Containter, class Dictionary>
		void interpolate_into_container(Iterator first, Iterator last, Containter & cont, const Dictionary & dict)
		{
			typedef typename std::iterator_traits<Iterator>::value_type char_type;
			typedef typename Dictionary::key_type key_type;

			key_type key;
			auto outit = std::back_inserter(cont);
			for (;;)
			{
				// ищем ключ, копируем строку
				std::tie(first, outit) = copy_escaped_until(first, last, outit, key_start());
				// не нашли символ ключа -> скопировали весь шаблон
				if (first == last)
					return;

				++first;
				// после символа ключа - конец строки. копируем символ и выходим
				if (first == last) {
					*outit++ = '$';
					return;
				}

				bool extended;
				ext::clear(key);
				auto keyIt = std::back_inserter(key);
				// расширенный ключ
				if (*first == '{')
				{
					extended = true;
					std::tie(first, std::ignore) = copy_escaped_until(++first, last, keyIt, extended_key_stop());
					// ключ не закрыт
					if (first == last)
					{
						*outit++ = '$'; *outit++ = '{';
						outit = std::copy(std::begin(key), std::end(key), outit);
						return;
					}

					++first; // съедаем '}'
				} // обычный ключ
				else
				{
					extended = false;
					std::tie(first, std::ignore) = copy_escaped_until(first, last, keyIt, key_stop());

					// $<space> - копируем символ и идем в начало алгоритма
					if (boost::empty(key)) {
						*outit++ = '$';
						continue;
					}
				}
				
				/// выполняем подстановку
				using ext::dictionary_find;
				auto val = dictionary_find(dict, key);
				if (val)
					ext::append(cont, boost::const_begin(*val), boost::const_end(*val));
				else
				{   // ключ 'отклонен' - копируем имя ключа как есть из шаблона
					*outit++ = '$';
					if (extended) *outit++ = '{';
					ext::append(cont, boost::const_begin(key), boost::const_end(key));
					if (extended) *outit++ = '}';
				}
			}
		}
	} // namespace InterpolateInternal

	/// поиск в карте словаре по умолчанию,
	/// может быть специализированно/перегружено, в том числе в родном namespace'е класса -
	/// тем самым предоставляя поиск в словаре для interpolate функций
	template <class StdMapType, class Key>
	const typename StdMapType::mapped_type * dictionary_find(const StdMapType & map, const Key & key)
	{
		auto it = map.find(key);
		return it != map.end() ? &it->second : nullptr;
	}

	template <
		class Range, class Dictionary, class OutIter,
		typename std::enable_if<!ext::is_range<OutIter>::value, int>::type = 0
	>
	inline OutIter interpolate(const Range & text, const Dictionary & dict, OutIter dest)
	{
		static_assert(std::is_same<char, typename boost::range_value<Range>::type>::value, "only char currently supported");
		auto textlit = boost::as_literal(text);
		return interpolate_internal::interpolate_into_iterator
			(boost::const_begin(textlit), boost::const_end(textlit), dest, dict);
	}

	template <
		class Range, class Dictionary, class Container,
		typename std::enable_if<ext::is_range<Container>::value, int>::type = 0
	>
	inline void interpolate(const Range & text, const Dictionary & dict, Container & outc)
	{
		static_assert(std::is_same<char, typename boost::range_value<Range>::type>::value, "only char currently supported");
		auto textlit = boost::as_literal(text);
		interpolate_internal::interpolate_into_container
			(boost::const_begin(textlit), boost::const_end(textlit), outc, dict);
	}

	template <class Range, class Dictionary>
	auto interpolate(const Range & text, const Dictionary & dict) ->
	                 std::basic_string<typename boost::range_value<Range>::type>
	{
		std::basic_string<typename boost::range_value<Range>::type> res;
		interpolate(text, dict, res);
		return res;
	}

	template <
		class CharT, class CharTraits, class Allocator,
		class Dictionary
	>
	auto interpolate(const std::basic_string<CharT, CharTraits, Allocator> & text,
	                 const Dictionary & dict) -> std::basic_string<CharT, CharTraits, Allocator>
	{
		std::basic_string<CharT, CharTraits, Allocator> res;
		interpolate(text, dict, res);
		return res;
	}
}
