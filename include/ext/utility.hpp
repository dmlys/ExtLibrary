#pragma once
#include <cassert>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <tuple>
#include <functional>
//this utility.h acts as common header, so include integer_sequence
#include <ext/type_traits.hpp>
#include <ext/integer_sequence.hpp>

namespace ext_detail_adl_helper
{
	using std::get;

	template <std::size_t Index, class Type>
	struct adl_get_type
	{
		using type = decltype(get<Index>(std::declval<Type>()));
	};

	template <std::size_t Index, class Type>
	decltype(auto) adl_get(Type && val)
	{
		return get<Index>(std::forward<Type>(val));
	}
} // namespace ext_detail_adl_helper


namespace ext
{
	// as_const, as in C++17
	// and unconst

	template <class Type> constexpr inline std::add_const_t<Type> & as_const(Type & ref) noexcept { return ref; }
	template <class Type> constexpr inline std::add_const_t<Type> * as_const(Type * ptr) noexcept { return ptr; }
	template <class Type> constexpr inline void as_const(const Type && ref) = delete;

	template <class Type> constexpr inline std::remove_const_t<Type> & unconst(const Type & ref) noexcept { return const_cast<std::remove_const_t<Type> &>(ref); }
	template <class Type> constexpr inline std::remove_const_t<Type> * unconst(const Type * ptr) noexcept { return const_cast<std::remove_const_t<Type> *>(ptr); }

	// delete this function for temporary objects
	template <class Type> constexpr inline void   unconst(const Type && ref) = delete;
	// those overloads needed because of deleted overload above
	template <class Type> constexpr inline Type & unconst(Type & ref) noexcept { return ref; }
	template <class Type> constexpr inline Type * unconst(Type * ptr) noexcept { return ptr; }


	namespace detail
	{
		template <class Type, std::size_t>
		using indexed_type = Type;
	}

	/// creates type by repeating Type into Template N times,
	/// for example: repeat_type<std::tuple, int, 3>::type is std::tuple<int, int, int>
	template <template <class...> class Template, class Type, std::size_t N, class Indices = std::make_index_sequence<N>>
	struct repeat_type;

	template <template <class...> class Template, class Type, std::size_t N, std::size_t... Indices>
	struct repeat_type<Template, Type, N, std::index_sequence<Indices...>>
	{
		using type = Template<detail::indexed_type<Type, Indices>...>;
	};

	template <template <class ...> class Template, class Type, std::size_t N>
	using repeat_type_t = typename repeat_type<Template, Type, N>::type;

	template <class Type, std::size_t N>
	using make_nth_tuple_t = repeat_type_t<std::tuple, Type, N>;


	/// test if type is a tuple like with size N, test is done via get function.
	/// if get<N - 1>(val) valid - it's a tuple.
	/// for testing use is_tuple trait, this trait is for extension/specialization.
	/// 
	/// for std::tuple, boost::tuple - it actually will not work,
	/// because of how get function is defined for them - it gives compilation error.
	/// for those you can specialize this class.
	template <class type, unsigned N>
	struct is_decayed_type_tuplelike
	{
		template <class C>
		static constexpr auto Test(int) -> decltype(ext_detail_adl_helper::adl_get<N - 1>(std::declval<C>()), ext::detail::Yes());

		template <class C>
		static constexpr auto Test(...) -> ext::detail::No;
		
		static constexpr bool value = (sizeof(Test<type>(0)) == sizeof(ext::detail::Yes));
	};

	template <class ... types, unsigned N>
	struct is_decayed_type_tuplelike<std::tuple<types...>, N>
	{
		static constexpr bool value = sizeof...(types) >= N;
	};


	/// test if type is a tuple like with size N
	template <class Type, unsigned N>
	struct is_tuple : std::integral_constant<bool, is_decayed_type_tuplelike<std::decay_t<Type>, N>::value> {};



	namespace detail
	{
		/************************************************************************/
		/*             INVOKE stuff                                             */
		/************************************************************************/
		template <class Type>
		struct is_reference_wrapper : std::false_type {};

		template <class Type>
		struct is_reference_wrapper<std::reference_wrapper<Type>> : std::true_type {};

		/// pointer to member function + object
		template <class Base, class FuncType, class Derived, class ... Args>
		auto INVOKE(FuncType Base::*pmf, Derived && ref, Args && ... args)
			noexcept(noexcept((std::forward<Derived>(ref).*pmf)(std::forward<Args>(args)...)))
			-> std::enable_if_t<
				std::is_function<FuncType>::value &&
				std::is_base_of<Base, std::decay_t<Derived>>::value,
				decltype((std::forward<Derived>(ref).*pmf)(std::forward<Args>(args)...))
			>
		{
			return (std::forward<Derived>(ref).*pmf)(std::forward<Args>(args)...);
		}

		/// pointer to member function + std::reference_wrapper of object
		template <class Base, class FuncType, class RefWrap, class ... Args>
		auto INVOKE(FuncType Base::*pmf, RefWrap && ref, Args && ... args)
			noexcept(noexcept((ref.get().*pmf)(std::forward<Args>(args)...)))
			-> std::enable_if_t<
				std::is_function<FuncType>::value &&
				is_reference_wrapper<std::decay_t<RefWrap>>::value,
				decltype((ref.get().*pmf)(std::forward<Args>(args)...))
			>
		{
			  return (ref.get().*pmf)(std::forward<Args>(args)...);
		}

		/// pointer to member function + pointer to object
		template <class Base, class FuncType, class Pointer, class ... Args>
		auto INVOKE(FuncType Base::*pmf, Pointer && ptr, Args && ... args)
			noexcept(noexcept(((*std::forward<Pointer>(ptr)).*pmf)(std::forward<Args>(args)...)))
			-> std::enable_if_t<
				std::is_function<FuncType>::value &&
				!is_reference_wrapper<std::decay_t<Pointer>>::value &&
				!std::is_base_of<Base, std::decay_t<Pointer>>::value,
				decltype(((*std::forward<Pointer>(ptr)).*pmf)(std::forward<Args>(args)...))
			>
		{
			return ((*std::forward<Pointer>(ptr)).*pmf)(std::forward<Args>(args)...);
		}

		/// pointer to member field + object
		template <class Base, class Type, class Derived>
		auto INVOKE(Type Base::*pmd, Derived && ref)
			noexcept(noexcept(std::forward<Derived>(ref).*pmd))
			-> std::enable_if_t<
				!std::is_function<Type>::value &&
				std::is_base_of<Base, std::decay_t<Derived>>::value,
				decltype(std::forward<Derived>(ref).*pmd)
			>
		{
			return std::forward<Derived>(ref).*pmd;
		}

		/// pointer to member field + std::reference_wrapper of object
		template <class Base, class Type, class RefWrap>
		auto INVOKE(Type Base::*pmd, RefWrap && ref)
			noexcept(noexcept(ref.get().*pmd))
			-> std::enable_if_t<
				!std::is_function<Type>::value && is_reference_wrapper<std::decay_t<RefWrap>>::value,
				decltype(ref.get().*pmd)
			>
		{
			return ref.get().*pmd;
		}

		/// pointer to member field + object pointer
		template <class Base, class Type, class Pointer>
		auto INVOKE(Type Base::*pmd, Pointer && ptr)
			noexcept(noexcept((*std::forward<Pointer>(ptr)).*pmd))
			-> std::enable_if_t<
				!std::is_function<Type>::value &&
				!is_reference_wrapper<std::decay_t<Pointer>>::value &&
				!std::is_base_of<Base, std::decay_t<Pointer>>::value,
				decltype((*std::forward<Pointer>(ptr)).*pmd)>
		{
			return (*std::forward<Pointer>(ptr)).*pmd;
		}

		/// functor is not a pointer to member
		template <class Functor, class ... Args>
		auto INVOKE(Functor && f, Args && ... args)
			noexcept(noexcept(std::forward<Functor>(f)(std::forward<Args>(args)...)))
			-> std::enable_if_t<
				!std::is_member_pointer<std::decay_t<Functor>>::value,
				decltype(std::forward<Functor>(f)(std::forward<Args>(args)...))
			>
		{
			return std::forward<Functor>(f)(std::forward<Args>(args)...);
		}


		/************************************************************************/
		/*               tuple visitation                                       */
		/************************************************************************/
		template <class Tuple, class Functor, class IndexSeq>
		struct tuple_visitation_result_type_impl;

		template <class Tuple, class Functor, std::size_t ... Is>
		struct tuple_visitation_result_type_impl<Tuple, Functor, std::index_sequence<Is...>>
		{
			using type = std::common_type_t<
				std::invoke_result_t<Functor, typename ext_detail_adl_helper::adl_get_type<Is, Tuple>::type>...
			>;
		};

		template <class Tuple, class Functor>
		struct tuple_visitation_result_type
		{
			using type = typename tuple_visitation_result_type_impl<
				Tuple, Functor, std::make_index_sequence<std::tuple_size_v<std::decay_t<Tuple>>>
			>::type;
		};

		template <std::size_t I, class ReturnType, class Tuple, class Functor>
		ReturnType tupple_applier(Tuple && tuple, Functor && func)
		{
			using std::get; using std::forward;
			return forward<Functor>(func)(get<I>(forward<Tuple>(tuple)));
		}

		template <class Tuple, class Functor, std::size_t ... Is>
		decltype(auto) visit_tuple_impl(Tuple && tuple, std::size_t idx, Functor && func, std::index_sequence<Is...>)
		{
			//assert(idx < sizeof...(Is));
			if (idx >= sizeof...(Is)) throw std::out_of_range("out_of_range");

			typedef typename tuple_visitation_result_type_impl<
				Tuple &&, Functor &&,
				std::index_sequence<Is...>
			>::type common_return_type;

			using applier = common_return_type(*)(Tuple &&, Functor &&);
			static constexpr applier appliers[] = {&tupple_applier<Is, common_return_type, Tuple, Functor>...};
			return appliers[idx](std::forward<Tuple>(tuple), std::forward<Functor>(func));
		}
	}

	template <class Functor, class ... Args>
	auto invoke(Functor && f, Args && ... args)
		noexcept(noexcept(detail::INVOKE(std::forward<Functor>(f), std::forward<Args>(args)...)))
		-> decltype(detail::INVOKE(std::forward<Functor>(f), std::forward<Args>(args)...))
	{
		return detail::INVOKE(std::forward<Functor>(f), std::forward<Args>(args)...);
	}

	/// invokes f for each arguments from args, in same order as they are passed into this function
	template <class Func, class... Args>
	constexpr void invoke_for_each(Func && f, Args && ... args)
	{
		std::initializer_list<int> {
			( ext::invoke(std::forward<Func>(f), std::forward<Args>(args)), 0 )...
		};
	}

	namespace detail
	{
		template <class Functor, class Tuple, std::size_t... I>
		constexpr decltype(auto) apply_impl(Functor && f, Tuple && t, std::index_sequence<I...>)
		{
			return ext::invoke(std::forward<Functor>(f), std::get<I>(std::forward<Tuple>(t))...);
		}

		template <class Func, class Tuple, std::size_t ... I>
		constexpr void apply_for_each_impl(Func && f, Tuple && t, std::index_sequence<I...>)
		{
			ext::invoke_for_each(std::forward<Func>(f), std::get<I>(std::forward<Tuple>(t))...);
		}
	}

	template <class Functor, class Tuple>
	constexpr decltype(auto) apply(Functor && f, Tuple && t)
	{
		return detail::apply_impl(std::forward<Functor>(f), std::forward<Tuple>(t),
		                          std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value> {});
	}

	/// applies func f for each member of tuple t. see invoke_for_each
	template <class Func, class Tuple>
	constexpr void apply_for_each(Func && f, Tuple && t)
	{
		detail::apply_for_each_impl(std::forward<Func>(f), std::forward<Tuple>(t),
		                            std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value> {});
	}


	/// invokes func with element by runtime index idx from tuple ts
	/// it's somewhat like func(std::get<idx>(ts)), except index can be not compile-time constant
	template <class Functor, class ... Args>
	decltype(auto) visit(const std::tuple<Args...> & ts, std::size_t idx, Functor && func)
	{
		return detail::visit_tuple_impl(ts, idx, std::forward<Functor>(func),
		                                std::index_sequence_for<Args...> {});
	}

	/// invokes func with element by runtime index idx from tuple ts
	/// it's somewhat like func(std::get<idx>(ts)), except index can be not compile-time constant
	template <class Functor, class ... Args>
	decltype(auto) visit(std::tuple<Args...> & ts, std::size_t idx, Functor && func)
	{
		return detail::visit_tuple_impl(ts, idx, std::forward<Functor>(func),
		                                std::index_sequence_for<Args...> {});
	}

	/// invokes func with element by runtime index idx from tuple ts
	/// it's somewhat like func(std::get<idx>(ts)), except index can be not compile-time constant
	template <class Functor, class ... Args>
	decltype(auto) visit(std::tuple<Args...> && ts, std::size_t idx, Functor && func)
	{
		return detail::visit_tuple_impl(std::move(ts), idx, std::forward<Functor>(func),
		                                std::index_sequence_for<Args...> {});
	}

	/// helper method, to invoke expressions on variadic packs
	/// for example: aux_pass(++std::get<Indexes>(tuple)...);
	/// order of evaluation is undefined
	template <class ... Types>
	inline void aux_pass(Types && ...) {}


	/// extracts element from tuple by idx, like get function,
	/// except this is functor -> can be easier passed to functions.
	/// for example: std::transform(vec.begin(), vec.end(), vecint.begin(), get_functor<0>())
	template <std::size_t Idx>
	struct get_functor
	{
		template <class Type>
		decltype(auto) operator()(Type && val) const
		{
			return ext_detail_adl_helper::adl_get<Idx>(std::forward<Type>(val));
		}
	};

	// like std::pair: first/second
	typedef get_functor<0> first_el;
	typedef get_functor<1> second_el;


	/// находит элемент в карте по ключу и возвращает ссылку на него,
	/// если элемента нет - создает его с помощью параметров args
	/// полезно когда карта использует типы не default constructible
	template <class Map, class Key, class Arg>
	typename Map::mapped_type & accuqire_from_map(Map & map, const Key & key, Arg && arg)
	{
		auto it = map.find(key);
		if (it != map.end())
			return it->second;
		else
		{
			auto res = map.emplace(key, std::forward<Arg>(arg));
			return res.first->second;
		}
	}

	/// находит элемент в карте по ключу и возвращает ссылку на него,
	/// если элемента нет - создает его с помощью параметров args
	/// полезно когда карта использует типы не default constructible
	template <class Map, class Key, class ... Args>
	typename Map::mapped_type & accuqire_from_map(Map & map, const Key & key, Args && ... args)
	{
		auto it = map.find(key);
		if (it != map.end())
			return it->second;
		else
		{
			auto res = map.emplace(std::piecewise_construct,
			                       std::forward_as_tuple(key),
			                       std::forward_as_tuple(args...));
			
			return res.first->second;
		}
	}

	/// находит элемент в карте по ключу и возвращает ссылку на него,
	/// если элемента нет - создает его и инициализирует с помощью параметров args
	/// тип должен быть default constructable и быть способным инициализироваться выражением {..., ...}
	/// например простая структура:
	/// struct test {
	///    string s; int i; vector<int> vi;
	/// };
	template <class Map, class Key, class ... Args>
	typename Map::mapped_type & accuqire_init_from_map(Map & map, const Key & key, Args && ... args)
	{
		auto it = map.find(key);
		if (it != map.end())
			return it->second;
		else
		{
			auto & val = map[key];
			return val = {std::forward<Args>(args)...};
		}
	}

	template <class Map, class Key>
	typename Map::mapped_type * find_ptr(Map & map, const Key & key)
	{
		auto it = map.find(key);
		return it != map.end() ? &it->second : nullptr;
	}

	template <class Map, class Key>
	const typename Map::mapped_type * find_ptr(const Map & map, const Key & key)
	{
		auto it = map.find(key);
		return it != map.end() ? &it->second : nullptr;
	}
}
