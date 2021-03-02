#pragma once
#include <ext/type_traits.hpp>

#include <boost/predef.h>
#include <boost/range.hpp>
// as_literal includes boost/range/detail/str_types.hpp
// and adds support for boost::range_value for "char *" like ranges
// boost/range.hpp does not include this header, but overral it's usefull
#include <boost/range/as_literal.hpp>

namespace ext
{
	/// функция обеспечивает обобщенный интерфейс data, как std::vector.data, std::array.data
	/// наподобие boost::range::begin
	/// здесь можно описывать метод data для контейнеров не имеющих его, но обладающих всем что бы его иметь
	template <class Range>
	inline auto data(Range & rng) -> decltype(rng.data())
	{
		return rng.data();
	}

	template <class Range>
	inline auto data(const Range & rng) -> decltype(rng.data())
	{
		return rng.data();
	}

	template <class Type>
	inline Type * data(Type * arr)
	{
		return arr;
	}

	template <class Type>
	inline Type * data(const boost::iterator_range<Type *> & rng)
	{
		return rng.begin();
	}

	template <class Type>
	inline Type * data(const std::pair<Type *, Type *> & rng)
	{
		return rng.first;
	}

#if BOOST_LIB_STD_DINKUMWARE
	// msvc не использует cow для строк, можно сделать const_cast
	template <class CharT, class traits, class allocator>
	inline CharT * data(std::basic_string<CharT, traits, allocator> & str)
	{
		return const_cast<CharT *>(str.data());
	}
#else
	template <class CharT, class traits, class allocator>
	inline CharT * data(std::basic_string<CharT, traits, allocator> & str)
	{
#if __cplusplus >= 201703L
		return str.data();
#else
		return str.empty() ? nullptr : &str[0];
#endif // __cplusplus >= 201703L
	}
#endif // BOOST_LIB_STD_DINKUMWARE

	/// generic assign method, by default calls assign member function
	/// can be specialized/overloaded for containers/ranges which do not provide assign memeber method, like boost::iterator_range
	template <class Container, class Iterator>
	inline void assign(Container & rng, Iterator first, Iterator last)
	{
		rng.assign(first, last);
	}

	template <class RngIterator, class Iterator>
	inline void assign(boost::iterator_range<RngIterator> & rng, Iterator first, Iterator last)
	{
		rng = {first, last};
	}

	template <class RngIterator, class Iterator>
	inline void assign(boost::sub_range<RngIterator> & rng, Iterator first, Iterator last)
	{
		rng = {first, last};
	}

	template <class PairIterator, class Iterator>
	inline void assign(std::pair<PairIterator, PairIterator> & p, Iterator first, Iterator last)
	{
		p = {first, last};
	}

	/// generic append/range_push_back method, by default calls cont.insert(cont.end(), first, last)
	/// can be specialized/overloaded for containers/ranges which provides different method
	template <class Container, class Iterator>
	inline void append(Container & cont, Iterator first, Iterator last)
	{
		cont.insert(cont.end(), first, last);
	}

	template <class CharT, class traits, class allocator, class Iterator>
	inline void append(std::basic_string<CharT, traits, allocator> & str, Iterator first, Iterator last)
	{
		str.append(first, last);
	}

	/// generic clear method
	template <class Container>
	inline void clear(Container & cont)
	{
		cont.clear();
	}


	/************************************************************************/
	/*        range type traits                                             */
	/************************************************************************/
	namespace range_detail
	{
		template <class Type>
		struct has_resize_method_test
		{
			template<class C>
			static auto Test(int) -> decltype(std::declval<C>().resize(0), ext::detail::Yes());
			
			template<class C>
			static ext::detail::No Test(...);

			typedef typename std::decay<Type>::type DecayedType;
			static const bool value = (sizeof(Test<DecayedType>(0)) == sizeof(ext::detail::Yes));
		};

		template <typename Type>
		struct is_contiguous_range_test
		{
			template <class C>
			static auto Test(int) -> decltype(ext::data(std::declval<C>()));
			
			template <class C>
			static ext::detail::No Test(...);

			typedef typename std::decay<Type>::type DecayedType;
			static const bool value = std::is_pointer_v<decltype(Test<DecayedType>(0))>;
		};

		/// скорее всего теста boost::has_range_iterator<Type>::type должно быть достаточно
		template <class Type, class = typename boost::has_range_iterator<Type>::type>
		struct is_begin_expression_valid_test
		{
			static const bool value = false;
		};

		template <class Type>
		struct is_begin_expression_valid_test<Type, boost::mpl::true_>
		{
			template <class C, class = decltype(boost::begin(std::declval<C>()))>
			static ext::detail::Yes Test(int);

			template <class C>
			static ext::detail::No Test(...);

			typedef typename std::remove_cv<typename std::remove_reference<Type>::type>::type DecayedType;
			static const bool value = sizeof(Test<DecayedType>(0)) == sizeof(ext::detail::Yes);
		};

		/// скорее всего теста boost::has_range_iterator<Type>::type должно быть достаточно
		template <class Type, class = typename boost::has_range_iterator<Type>::type>
		struct is_end_expression_valid_test
		{
			static const bool value = false;
		};

		template <class Type>
		struct is_end_expression_valid_test<Type, boost::mpl::true_>
		{
			template <class C, class = decltype(boost::end(std::declval<C>()))>
			static ext::detail::Yes Test(int);

			template <class C>
			static ext::detail::No Test(...);

			typedef typename std::remove_cv<typename std::remove_reference<Type>::type>::type DecayedType;
			static const bool value = sizeof(Test<DecayedType>(0)) == sizeof(ext::detail::Yes);
		};
	}

	/// checks is there is resize method on type Type
	template <class Type>
	struct has_resize_method :
		std::integral_constant<bool,
			ext::range_detail::has_resize_method_test<Type>::value
		>
	{};

	template <class Type>
	constexpr auto has_resize_method_v = has_resize_method<Type>::value;


	/// determines if container is contiguous
	/// if expression: ext::data(declval<Type>()) is valid - it is contiguous container
	/// examples of such containers is: vector, string, array. But not std::deque
	template <class Type>
	struct is_contiguous_range :
		std::integral_constant<bool,
			ext::range_detail::is_contiguous_range_test<Type>::value
		>
	{};

	template <class Type>
	constexpr auto is_contiguous_range_v = is_contiguous_range<Type>::value;

	template <class Type>
	struct is_begin_expression_valid :
		std::integral_constant<bool,
			ext::range_detail::is_begin_expression_valid_test<Type>::value
		>
	{};

	template <class Type>
	struct is_end_expression_valid :
		std::integral_constant<bool,
			ext::range_detail::is_end_expression_valid_test<Type>::value
		>
	{};

	template <class Type>
	struct is_range :
		std::integral_constant<bool,
			is_begin_expression_valid<Type>::value &&
			is_end_expression_valid<Type>::value
		>
	{};

	template <class Type>
	constexpr auto is_range_v = is_range<Type>::value;


	template <class Type>
	struct is_container : std::conjunction<is_range<Type>, has_resize_method<Type>> {};

	template <class Type>
	constexpr auto is_container_v = is_container<Type>::value;

	template <class Type>
	struct is_contiguous_container : std::conjunction<is_container<Type>, is_contiguous_range<Type>> {};

	template <class Type>
	constexpr auto is_contiguous_container_v = is_contiguous_container<Type>::value;



	template <class Range, class Default = void, bool = ext::is_range<Range>::value>
	struct range_value
	{
		typedef Default type;
	};

	template <class Range, class Default>
	struct range_value<Range, Default, true>
	{
		typedef typename boost::range_value<Range>::type type;
	};

	template <class Range, class Default = void>
	using range_value_t = typename range_value<Range, Default>::type;


	template <class Range, class Type>
	struct is_range_of :
		std::is_same<range_value_t<Range>, Type> {};

	template <class Range, class Type>
	constexpr auto is_range_of_v = is_range_of<Range, Type>::value;



	/// some if_*_of shortcuts

	template <class Range, class Type>
	struct is_contiguous_range_of :
	    std::conjunction<ext::is_contiguous_range<Range>, ext::is_range_of<Range, Type>> {};

	template <class Range, class Type>
	constexpr auto is_contiguous_range_of_v = is_contiguous_range_of<Range, Type>::value;

	template <class Container, class Type>
	struct is_container_of :
	    std::conjunction<ext::is_container<Container>, ext::is_range_of<Container, Type>> {};

	template <class Container, class Type>
	constexpr auto is_container_of_v = is_container_of<Container, Type>::value;

	template <class Container, class Type>
	struct is_contiguous_container_of :
	        std::conjunction<ext::is_contiguous_container<Container>, ext::is_range_of<Container, Type>> {};

	template <class Container, class Type>
	constexpr auto is_contiguous_container_of_v = is_contiguous_container_of<Container, Type>::value;
}
