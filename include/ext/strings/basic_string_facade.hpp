#pragma once
// author: Dmitry Lysachenko
// date: Saturday 13 january 2016
// license: boost software license
//          http://www.boost.org/LICENSE_1_0.txt

#include <cstring>   // for std::memcpy and others
#include <cassert>

#include <type_traits>
#include <initializer_list>
#include <tuple>     // for std::tie
#include <iterator>  // for std::reverse_iterator
#include <algorithm>
#include <utility>   // for std::declval
#include <string_view>

#include <ext/type_traits.hpp>
#include <boost/config.hpp>
#include <ext/container/container_iterator.hpp>

namespace ext
{
	/// A Policy-based string facade.
	/// Idea - Andrei Alexandrescu.
	/// http://www.drdobbs.com/generic-a-policy-based-basicstring-imple/184403784
	/// 
	/// It provides all additional operation like begin, end, compare, assign, append, find*
	/// via basic operations of storage and char_traits
	/// 
	/// @char_traits  traits class for char describing various manipulations.
	///               As in std::string
	/// 
	/// @storage    class which provides basic storage operations.
	///              * accessing storage properties: data, size, capacity, etc
	///              * manipulating storage size: reserve, resize, etc
	///     storage have to provide types:
	///       * value_type        typedef for char_t, typically char
	///       * size_type         typedef for type used for size, typically std::size_t
	///       * difference_type   typedef for pointer difference, typically std::ptrdiff_t
	///     
	///     public methods:
	///       * data()
	///       * data_end()
	///       * range()
	///       * max_size()
	///       * capacity()
	///       * size()
	///       * empty()
	///       * resize()
	///       * reserve()
	///       * shrink_to_fit()
	///       
	///       * grow_to(size_type newsize) -> value_type * // new first buffer pointer
	///       * grow_by(size_type size_increment) -> (new first buffer pointer, newsize after increment)
	///       * shrink_by(size_type size_decrement) -> (new first buffer pointer, newsize after decrement)
	///       * set_eos(value_type * endpos) // sets string terminator at pos
	/// 
	template <class storage, class char_traits>
	class basic_string_facade : public storage
	{
		typedef storage              base_type;
		typedef storage              storage_type;
		typedef basic_string_facade  self_type;
	
	public:
		using typename base_type::value_type;
		using typename base_type::size_type;
		using typename base_type::difference_type;

		typedef       value_type *    pointer;
		typedef const value_type *    const_pointer;
		typedef       value_type &    reference;
		typedef const value_type &    const_reference;
		
		//typedef pointer         iterator;
		//typedef const_pointer   const_iterator;
		//
		//typedef std::reverse_iterator<iterator>        reverse_iterator;
		//typedef std::reverse_iterator<const_iterator>  const_reverse_iterator;

		using iterator       = ext::container::container_iterator<pointer,       self_type>;
		using const_iterator = ext::container::container_iterator<const_pointer, self_type>;

		using reverse_iterator       = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		typedef char_traits traits_type;
		typedef std::basic_string_view<value_type, traits_type> string_view_type;

	public:
		static constexpr size_type npos = size_type(-1);

	private:
		const self_type * as_const() noexcept { return this; }
		static BOOST_NORETURN void throw_xpos() { throw std::out_of_range("out_of_range"); }
		auto make_pointer(size_type pos) -> std::pair<value_type *, value_type *>;
		auto make_pointer(size_type pos) const -> std::pair<const value_type *, const value_type *>;
		static bool is_inside(const value_type * ptr, size_type count,
		                      const value_type * first, const value_type * last);

		static auto make_pointer(string_view_type sv, size_type pos, size_type count)
		    -> std::pair<const value_type *, const value_type *>;

	public:
		// Workaround for boost::range. Somehow metacode that checks for member method "size" crashes msvs 2013
		// size_type size() const noexcept { return base_type::size(); }

		using base_type::data;
		using base_type::data_end;
		using base_type::range;
		using base_type::max_size;
		using base_type::capacity;
		using base_type::size;
		using base_type::empty;

		using base_type::resize;
		using base_type::reserve;
		using base_type::shrink_to_fit;

	public: // element access [done]
		reference       at(size_type pos);
		const_reference at(size_type pos) const;

		reference       operator [](size_type pos)       noexcept  { return *(data() + pos); }
		const_reference operator [](size_type pos) const noexcept  { return *(data() + pos); }
		
		reference       front()       noexcept { return *data(); }
		const_reference front() const noexcept { return *data(); }

		reference       back()       noexcept { return *(data_end() - 1); }
		const_reference back() const noexcept { return *(data_end() - 1); }

	public: // [done]
		void pop_back() noexcept { this->shrink_by(1); };
		void push_back(value_type ch)  { append(1, ch); }

		void clear()              noexcept  { resize(0); }
		size_type length() const  noexcept  { return size(); }

		self_type substr(size_type pos = 0, size_type count = npos) const;
		size_type copy(value_type * dest, size_type count, size_type pos = 0) const;

	public: // iterators [done]
		iterator begin()             noexcept   { return iterator(data()); }
		iterator end()               noexcept   { return iterator(data_end()); }
		const_iterator begin() const noexcept   { return const_iterator(data()); }
		const_iterator end()   const noexcept	{ return const_iterator(data_end()); }

		const_iterator cbegin() const noexcept  { return const_iterator(data()); }
		const_iterator cend()   const noexcept  { return const_iterator(data_end()); }

		reverse_iterator rbegin()             noexcept     { return reverse_iterator(data_end()); }
		reverse_iterator rend()               noexcept     { return reverse_iterator(data()); }
		const_reverse_iterator rbegin() const noexcept     { return const_reverse_iterator(data_end()); }
		const_reverse_iterator rend()   const noexcept     { return const_reverse_iterator(data()); }

		const_reverse_iterator crbegin() const noexcept    { return const_reverse_iterator(data_end()); }
		const_reverse_iterator crend()   const noexcept    { return const_reverse_iterator(data()); }

	public:
		operator string_view_type() const noexcept;

	public: // assign pack [done]
		self_type & assign(size_type count, value_type ch);
		self_type & assign(const self_type & str);
		self_type & assign(const self_type & str, size_type pos, size_type count = npos);
		self_type & assign(self_type && str);
		self_type & assign(const value_type * str, size_type count);
		self_type & assign(const value_type * str);
		self_type & assign(std::initializer_list<value_type> ilist);
		
		self_type & assign(const_iterator first, const_iterator last) { return assign(first.base(), last - first); }
		self_type & assign(iterator first, iterator last) { return assign(const_iterator(first), const_iterator(last)); }

		template <class InputIterator>
		auto assign(InputIterator first, InputIterator last) -> std::enable_if_t<ext::is_iterator<InputIterator>::value, self_type &>;

		template <class Type> auto assign(const Type & str) // -> self_type &
		    -> std::enable_if_t<std::is_convertible_v<const Type &, string_view_type> and not std::is_convertible_v<const Type &, const value_type *>, self_type &>;
		template <class Type> auto assign(const Type & str, size_type pos, size_type count = npos) // -> self_type &
		    -> std::enable_if_t<std::is_convertible_v<const Type &, string_view_type> and not std::is_convertible_v<const Type &, const value_type *>, self_type &>;

	public: // insert pack [done]
		self_type & insert(size_type index, size_type count, value_type ch);
		self_type & insert(size_type index, const value_type * str);
		self_type & insert(size_type index, const value_type * str, size_type count);
		self_type & insert(size_type index, const self_type & str);
		self_type & insert(size_type index, const self_type & str, size_type index_str, size_type count = npos);
		iterator insert(const_iterator pos, value_type ch);
		iterator insert(const_iterator pos, size_type count, value_type ch);
		iterator insert(const_iterator pos, std::initializer_list<value_type> ilist);
		
		iterator insert(const_iterator pos, const_iterator first, const_iterator last);
		iterator insert(const_iterator pos, iterator first, iterator last)
		{ return insert(pos, const_iterator(first), const_iterator(last)); }

		template <class InputIterator>
		iterator insert(const_iterator pos, InputIterator first, InputIterator last);

		template <class Type> auto insert(size_type pos, const Type & str) // -> self_type &
		    -> std::enable_if_t<std::is_convertible_v<const Type &, string_view_type> and not std::is_convertible_v<const Type &, const value_type *>, self_type &>;
		template <class Type> auto insert(size_type pos, const Type & str, size_type index_str, size_type count = npos) // -> self_type &
		    -> std::enable_if_t<std::is_convertible_v<const Type &, string_view_type> and not std::is_convertible_v<const Type &, const value_type *>, self_type &>;

	public: // erase pack [done]
		self_type & erase(size_type index = 0, size_type count = npos);
		iterator erase(const_iterator position);
		iterator erase(const_iterator first, const_iterator last);

	public: // append pack [done]
		self_type & append(size_type count, value_type ch);
		self_type & append(const self_type & str);
		self_type & append(const self_type & str, size_type pos, size_type count = npos);
		self_type & append(const value_type * str, size_type count);
		self_type & append(const value_type * str);
		self_type & append(std::initializer_list<value_type> ilist);

		self_type & append(const_iterator first, const_iterator last);
		self_type & append(iterator first, iterator last) { return append(const_iterator(first), const_iterator(last)); }

		template <class InputIterator, class = std::enable_if_t<ext::is_iterator<InputIterator>::value>>
		self_type & append(InputIterator first, InputIterator last);

		self_type & operator +=(const self_type & str)                    { return append(str); }
		self_type & operator +=(value_type ch)                            { return append(1, ch); }
		self_type & operator +=(const value_type * str)                   { return append(str); }
		self_type & operator +=(std::initializer_list<value_type> ilist)  { return append(ilist); }


		template <class Type> auto append(const Type & str) // -> self_type &
		    -> std::enable_if_t<std::is_convertible_v<const Type &, string_view_type> and not std::is_convertible_v<const Type &, const value_type *>, self_type &>;
		template <class Type> auto append(const Type & str, size_type pos, size_type count = npos) // -> self_type &
		    -> std::enable_if_t<std::is_convertible_v<const Type &, string_view_type> and not std::is_convertible_v<const Type &, const value_type *>, self_type &>;
		template <class Type> auto operator +=(const Type & str) // self_type &
		    -> std::enable_if_t<std::is_convertible_v<const Type &, string_view_type> and not std::is_convertible_v<const Type &, const value_type *>, self_type &>
		{ return append(str); }

	public: // compare pack [done]
		int compare(const self_type & str) const noexcept;
		int compare(const value_type * str) const noexcept;
		int compare(size_type pos1, size_type count1, const self_type & str) const noexcept;
		int compare(size_type pos1, size_type count1, const self_type & str,
		            size_type pos2, size_type count2 = npos) const noexcept;
		int compare(size_type pos1, size_type count1, const value_type * str)                   const noexcept;
		int compare(size_type pos1, size_type count1, const value_type * str, size_type count2) const noexcept;

	public: // replace pack [done]
		self_type & replace(const_iterator first, const_iterator last, const self_type & str);
		self_type & replace(const_iterator first, const_iterator last, const value_type * str);
		self_type & replace(const_iterator first, const_iterator last, const value_type * str, size_type count2);
		self_type & replace(const_iterator first, const_iterator last, size_type count, value_type ch);
		self_type & replace(const_iterator first, const_iterator last, std::initializer_list<value_type> ilist);
		self_type & replace(const_iterator first, const_iterator last, const value_type * str_first, const value_type * str_last);
		self_type & replace(const_iterator first, const_iterator last, value_type * str_first, value_type * str_last);
		self_type & replace(size_type pos, size_type count, const self_type & str);
		self_type & replace(size_type pos, size_type count, const self_type & str, size_type pos2, size_type count2 = npos);
		self_type & replace(size_type pos, size_type count, const value_type * str);
		self_type & replace(size_type pos, size_type count, const value_type * str, size_type count2);
		self_type & replace(size_type pos, size_type count, size_type count2, value_type ch);

		self_type & replace(const_iterator first, const_iterator last, const_iterator r_first, const_iterator r_last) { return replace(first, last, r_first.base(), r_last.base()); }
		self_type & replace(const_iterator first, const_iterator last,       iterator r_first,       iterator r_last) { return replace(first, last, r_first.base(), r_last.base()); }

		template <class InputIterator>
		self_type & replace(const_iterator first, const_iterator last, InputIterator r_first, InputIterator r_last);

		template <class Type> auto replace(const_iterator first, const_iterator last, const Type & str) // -> self_type &
		    -> std::enable_if_t<std::is_convertible_v<const Type &, string_view_type> and not std::is_convertible_v<const Type &, const value_type *>, self_type &>;
		template <class Type> auto replace(size_type pos, size_type count, const Type & str) // -> self_type &
		    -> std::enable_if_t<std::is_convertible_v<const Type &, string_view_type> and not std::is_convertible_v<const Type &, const value_type *>, self_type &>;
		template <class Type> auto replace(size_type pos, size_type count, const Type & str, size_type pos2, size_type count2 = npos) // self_type &
		    -> std::enable_if_t<std::is_convertible_v<const Type &, string_view_type> and not std::is_convertible_v<const Type &, const value_type *>, self_type &>;

	public: // search methods [done]
		size_type find(const self_type & str, size_type pos = 0) const noexcept;
		size_type find(const value_type * str, size_type pos, size_type count) const noexcept;
		size_type find(const value_type * str, size_type pos = 0) const noexcept;
		size_type find(value_type ch, size_type pos = 0) const noexcept;

		size_type rfind(const self_type & str, size_type pos = npos) const noexcept;
		size_type rfind(const value_type * str, size_type pos, size_type count) const noexcept;
		size_type rfind(const value_type * str, size_type pos = npos) const noexcept;
		size_type rfind(value_type ch, size_type pos = npos) const noexcept;

		size_type find_first_of(const self_type & str, size_type pos = 0) const noexcept;
		size_type find_first_of(const value_type * str, size_type pos, size_type count) const noexcept;
		size_type find_first_of(const value_type * str, size_type pos = 0) const noexcept;
		size_type find_first_of(value_type ch, size_type pos = 0) const noexcept;

		size_type find_first_not_of(const self_type & str, size_type pos = 0) const noexcept;
		size_type find_first_not_of(const value_type * str, size_type pos, size_type count) const noexcept;
		size_type find_first_not_of(const value_type * str, size_type pos = 0) const noexcept;
		size_type find_first_not_of(value_type ch, size_type pos = 0) const noexcept;
		
		size_type find_last_of(const self_type & str, size_type pos = npos) const noexcept;
		size_type find_last_of(const value_type * str, size_type pos, size_type count) const noexcept;
		size_type find_last_of(const value_type * str, size_type pos = npos) const noexcept;
		size_type find_last_of(value_type ch, size_type pos = npos) const noexcept;
		
		size_type find_last_not_of(const self_type & str, size_type pos = npos) const noexcept;
		size_type find_last_not_of(const value_type * str, size_type pos, size_type count) const noexcept;
		size_type find_last_not_of(const value_type * str, size_type pos = npos) const noexcept;
		size_type find_last_not_of(value_type ch, size_type pos = npos) const noexcept;

	public: // ctors
		// allocators not supported yet
		basic_string_facade(size_type count, value_type ch);
		basic_string_facade(const value_type * str);
		basic_string_facade(const value_type * str, size_type count);
		basic_string_facade(const self_type & str, size_type pos) : basic_string_facade(str, pos, npos) {}
		basic_string_facade(const self_type & str, size_type pos, size_type count);
		
		basic_string_facade(const value_type * first, const value_type * last) : basic_string_facade(first, last - first) {}
		basic_string_facade(value_type * first, value_type * last)             : basic_string_facade(first, last - first) {}
		basic_string_facade(std::initializer_list<value_type> ilist)           : basic_string_facade(ilist.begin(), ilist.size()) {}

		template <class InputIt>
		basic_string_facade(InputIt first, InputIt last) { assign(first, last); }

		// both clang and msvc have troubles matching declaration and definition of those constructors 
		// if declarations is written as commented bellow. GCC is ok actually
		template <
			class Type,
			std::enable_if_t<
						std::is_convertible_v<const Type &, std::basic_string_view<typename storage::value_type, char_traits>>
				and not std::is_convertible_v<const Type &, const typename storage::value_type *>
			, int> = 0
		>
		//template <class Type, std::enable_if_t<std::is_convertible_v<const Type &, string_view_type> and not std::is_convertible_v<const Type &, const value_type *>, int> = 0>
		explicit basic_string_facade(const Type & str);

		template <
			class Type,
			std::enable_if_t<
						std::is_convertible_v<const Type &, std::basic_string_view<typename storage::value_type, char_traits>>
				and not std::is_convertible_v<const Type &, const typename storage::value_type *>
			, int> = 0
		>
		//template <class Type, std::enable_if_t<std::is_convertible_v<const Type &, string_view_type> and not std::is_convertible_v<const Type &, const value_type *>, int> = 0>
		basic_string_facade(const Type & str, size_type pos, size_type count);

	public: // operator =
		basic_string_facade & operator=(const value_type * str)                  { assign(str);   return *this; }
		basic_string_facade & operator=(value_type ch)                           { assign(1, ch); return *this; }
		basic_string_facade & operator=(std::initializer_list<value_type> ilist) { assign(ilist); return *this; }

		template <class Type> auto operator=(const Type & str)
		    -> std::enable_if_t<std::is_convertible_v<const Type &, string_view_type> and not std::is_convertible_v<const Type &, const value_type *>, self_type &>
		{ return assign(str); }

	public:
		basic_string_facade()  noexcept(std::is_nothrow_default_constructible<base_type>::value) = default;
		~basic_string_facade() noexcept(std::is_nothrow_destructible<base_type>::value) = default;

		basic_string_facade(const basic_string_facade &)              noexcept(std::is_nothrow_copy_constructible<base_type>::value) = default;
		basic_string_facade & operator =(const basic_string_facade &) noexcept(std::is_nothrow_copy_assignable<base_type>::value) = default;
		
		basic_string_facade(basic_string_facade &&)              noexcept(std::is_nothrow_move_constructible<base_type>::value) = default;
		basic_string_facade & operator =(basic_string_facade &&) noexcept(std::is_nothrow_move_assignable<base_type>::value) = default;
	};

	/************************************************************************/
	/*                   methods implementation                             */
	/************************************************************************/
	template <class storage, class char_traits>
	BOOST_FORCEINLINE
	auto basic_string_facade<storage, char_traits>::make_pointer(size_type pos)
		-> std::pair<value_type *, value_type *>
	{
		value_type * first;
		value_type * last;
		std::tie(first, last) = this->range();

		if (pos > static_cast<size_type>(last - first)) throw_xpos();
		return {first + pos, last};
	}

	template <class storage, class char_traits>
	BOOST_FORCEINLINE
	auto basic_string_facade<storage, char_traits>::make_pointer(size_type pos) const
		-> std::pair<const value_type *, const value_type *>
	{
		const value_type * first;
		const value_type * last;
		std::tie(first, last) = this->range();

		if (pos > static_cast<size_type>(last - first)) throw_xpos();
		return {first + pos, last};
	}

	template <class storage, class char_traits>
	BOOST_FORCEINLINE
	auto basic_string_facade<storage, char_traits>::make_pointer(string_view_type sv, size_type pos, size_type count)
	    -> std::pair<const value_type *, const value_type *>
	{
		const value_type * first = sv.data();
		const value_type * last  = first + sv.size();

		if (pos > static_cast<size_type>(last - first)) throw_xpos();
		return {first + pos, last};
	}

	template <class storage, class char_traits>
	BOOST_FORCEINLINE
	bool basic_string_facade<storage, char_traits>::is_inside
	(const value_type * ptr, size_type count, const value_type * first, const value_type * last)
	{
#ifdef NDEBUG
		return first <= ptr && ptr < last;
#else // NDEBUG
		if (first <= ptr && ptr < last)
		{
			assert(ptr + count <= last);
			return true;
		}
		else
			return false;
#endif
	}

	template <class storage, class char_traits>
	inline basic_string_facade<storage, char_traits>::operator string_view_type() const noexcept
	{
		const value_type * first, * last;
		std::tie(first, last) = range();
		return string_view_type(first, last - first);
	}

	/************************************************************************/
	/*                   constructors                                       */
	/************************************************************************/
	template <class storage, class char_traits>
	basic_string_facade<storage, char_traits>::basic_string_facade(size_type count, value_type ch)
	{
		value_type * out = this->grow_to(count);
		traits_type::assign(out, count, ch);
		this->set_eos(out + count);
	}

	template <class storage, class char_traits>
	inline basic_string_facade<storage, char_traits>::basic_string_facade(const value_type * str)
		: basic_string_facade(str, traits_type::length(str))
	{

	}
	
	template <class storage, class char_traits>
	basic_string_facade<storage, char_traits>::basic_string_facade(const value_type * str, size_type count)
	{
		value_type * out = this->grow_to(count);
		traits_type::copy(out, str, count);
		this->set_eos(out + count);
	}

	template <class storage, class char_traits>
	basic_string_facade<storage, char_traits>::basic_string_facade(const self_type & str, size_type pos, size_type count /* = basic_string_facade::npos */)
	{
		const value_type * first, *last;
		std::tie(first, last) = str.make_pointer(pos);
		count = (std::min<size_type>)(last - first, count);

		value_type * out = this->grow_to(count);
		traits_type::copy(out, first, count);
		this->set_eos(out + count);
	}


	/************************************************************************/
	/*                   assign block                                       */
	/************************************************************************/
	template <class storage, class char_traits>
	basic_string_facade<storage, char_traits> &
		basic_string_facade<storage, char_traits>::assign(size_type count, value_type ch)
	{
		value_type * out = this->grow_to(count);
		traits_type::assign(out, count, ch);
		this->set_eos(out + count);
		
		return *this;
	}

	template <class storage, class char_traits>
	inline basic_string_facade<storage, char_traits> &
		basic_string_facade<storage, char_traits>::assign(const self_type & str)
	{
		return assign(str, 0, npos);
	}

	template <class storage, class char_traits>
	basic_string_facade<storage, char_traits> &
		basic_string_facade<storage, char_traits>::assign(const self_type & str, size_type pos, size_type count /* = npos */)
	{
		const value_type * first, * last;
		std::tie(first, last) = str.make_pointer(pos);
		count = (std::min<size_type>)(last - first, count);

		if (this == &str)
		{	// we can't assign more than we already have, just move it
			auto * out = data();
			traits_type::move(out, first, count);
			this->set_eos(out + count);
			this->grow_to(count);
		}
		else
		{
			value_type * out = this->grow_to(count);
			traits_type::copy(out, first, count);
			this->set_eos(out + count);
		}

		return *this;
	}

	template <class storage, class char_traits>
	inline basic_string_facade<storage, char_traits> &
		basic_string_facade<storage, char_traits>::assign(self_type && str)
	{
		base_type::operator =(std::move(str));
		return *this;
	}

	template <class storage, class char_traits>
	basic_string_facade<storage, char_traits> &
		basic_string_facade<storage, char_traits>::assign(const value_type * str, size_type count)
	{
		value_type * first, * last;
		std::tie(first, last) = this->range();

		if (is_inside(str, count, first, last))
		{	// it does not make sense if count is bigger than we already holding - just move it
			traits_type::move(first, str, count);
			this->set_eos(first + count);
			this->grow_to(count);
		}
		else
		{
			value_type * out = this->grow_to(count);
			traits_type::copy(out, str, count);
			this->set_eos(out + count);
		}

		return *this;
	}

	template <class storage, class char_traits>
	inline basic_string_facade<storage, char_traits> &
		basic_string_facade<storage, char_traits>::assign(const value_type * str)
	{
		return assign(str, traits_type::length(str));
	}

	template <class storage, class char_traits>
	inline basic_string_facade<storage, char_traits> &
		basic_string_facade<storage, char_traits>::assign(std::initializer_list<value_type> ilist)
	{
		assign(std::begin(ilist), std::end(ilist));
	}

	template <class storage, class char_traits>
	template <class InputIterator>
	auto basic_string_facade<storage, char_traits>::assign(InputIterator first, InputIterator last) ->
	    std::enable_if_t<ext::is_iterator<InputIterator>::value, self_type &>
	{
		typedef typename std::iterator_traits<InputIterator>::iterator_category cat;
		if constexpr (!std::is_convertible<cat, std::random_access_iterator_tag>::value)
		{
			clear();
			for (; first != last; ++first)
				push_back(*first);
			return *this;
		}
		else
		{
			auto count = std::distance(first, last);
			value_type * out = this->grow_to(count);
			for (; first != last; ++first, ++out)
				traits_type::assign(*out, *first);

			this->set_eos(out);
			return *this;
		}
	}

	/************************************************************************/
	/*                   append block                                       */
	/************************************************************************/
	template <class storage, class char_traits>
	basic_string_facade<storage, char_traits> &
		basic_string_facade<storage, char_traits>::append(size_type count, value_type ch)
	{
		value_type * out;
		size_type newsize;
		std::tie(out, newsize) = this->grow_by(count);
		out += newsize - count;

		traits_type::assign(out, count, ch);
		this->set_eos(out + count);
		return *this;
	}

	template <class storage, class char_traits>
	basic_string_facade<storage, char_traits> &
		basic_string_facade<storage, char_traits>::append(const self_type & str, size_type pos, size_type count /* = npos */)
	{
		const value_type * first, * last;
		std::tie(first, last) = str.make_pointer(pos);
		count = (std::min<size_type>)(last - first, count);

		value_type * out;
		size_type newsize;
		std::tie(out, newsize) = this->grow_by(count);
		
		// after grow_by first and last can be invalidated
		if (this == &str) first = out + pos;
		out += newsize - count;
		
		traits_type::copy(out, first, count);
		this->set_eos(out + count);
		return *this;
	}

	template <class storage, class char_traits>
	inline basic_string_facade<storage, char_traits> &
		basic_string_facade<storage, char_traits>::append(const self_type & str)
	{
		return append(str, 0, npos);
	}

	template <class storage, class char_traits>
	basic_string_facade<storage, char_traits> &
		basic_string_facade<storage, char_traits>::append(const value_type * str, size_type count)
	{
		const value_type * first, *last;
		std::tie(first, last) = as_const()->range();

		bool inside = is_inside(str, count, first, last);
		size_type inspos = str - first;

		value_type * out;
		size_type newsize;
		std::tie(out, newsize) = this->grow_by(count);

		if (inside) str = out + inspos;
		out += newsize - count;

		traits_type::copy(out, str, count);
		this->set_eos(out + count);
		return *this;
	}

	template <class storage, class char_traits>
	inline basic_string_facade<storage, char_traits> &
		basic_string_facade<storage, char_traits>::append(const value_type * str)
	{
		return append(str, traits_type::length(str));
	}

	template <class storage, class char_traits>
	basic_string_facade<storage, char_traits> &
		basic_string_facade<storage, char_traits>::append(const_iterator first, const_iterator last)
	{
		return append(first, last - first);
	}

	template <class storage, class char_traits>
	template <class InputIterator, class /*= std::enable_if_t<ext::is_iterator<InputIterator>::value>*/>
	basic_string_facade<storage, char_traits> &
		basic_string_facade<storage, char_traits>::append(InputIterator first, InputIterator last)
	{
		typedef typename std::iterator_traits<InputIterator>::iterator_category cat;
		if (!std::is_convertible<cat, std::random_access_iterator_tag>::value)
		{
			for (; first != last; ++first)
				push_back(*first);
			
			return *this;
		}
		else
		{
			size_type count = std::distance(first, last);
			size_type newsize;
			value_type * out;
			std::tie(out, newsize) = this->grow_by(count);
			out += newsize - count;
			
			for (; first != last; ++first, ++out)
				traits_type::assign(*out, *first);
			
			this->set_eos(out);
			return *this;
		}
	}

	template <class storage, class char_traits>
	inline basic_string_facade<storage, char_traits> &
		basic_string_facade<storage, char_traits>::append(std::initializer_list<value_type> ilist)
	{
		return append(std::begin(ilist), std::end(ilist));
	}

	/************************************************************************/
	/*                   insert  block                                      */
	/************************************************************************/
	template <class storage, class char_traits>
	basic_string_facade<storage, char_traits> &
		basic_string_facade<storage, char_traits>::insert(size_type index, size_type count, value_type ch)
	{
		size_type oldsize = size();
		if (index > oldsize) throw_xpos();
		
		value_type * out;
		size_type newsize;
		std::tie(out, newsize) = this->grow_by(count);
		out += index;

		traits_type::move(out + count, out, oldsize - index);
		this->set_eos(out - index + newsize);
		traits_type::assign(out, count, ch);
		return *this;
	}

	template <class storage, class char_traits>
	inline basic_string_facade<storage, char_traits> &
		basic_string_facade<storage, char_traits>::insert(size_type index, const value_type * str)
	{
		return insert(index, str, traits_type::length(str));
	}

	template <class storage, class char_traits>
	basic_string_facade<storage, char_traits> &
		basic_string_facade<storage, char_traits>::insert(size_type index, const value_type * str, size_type count)
	{
		const value_type * first, *last;
		std::tie(first, last) = as_const()->range();

		size_type oldsize = last - first;
		if (index > oldsize) throw_xpos();

		bool inside = is_inside(str, count, first, last);
		size_type inspos = str - first;

		value_type * out;
		size_type newsize;
		std::tie(out, newsize) = this->grow_by(count);
		
		out += index;
		traits_type::move(out + count, out, oldsize - index);
		this->set_eos(out - index + newsize);

		if (!inside)
			traits_type::copy(out, str, count);
		else
		{
			str = out - index + inspos;
			traits_type::move(out, (str <= out ? str : str + count), count);
			// it will be moved by count if right from insert point
		}
		
		return *this;
	}

	template <class storage, class char_traits>
	inline basic_string_facade<storage, char_traits> &
		basic_string_facade<storage, char_traits>::insert(size_type index, const self_type & str)
	{
		return insert(index, str, 0, npos);
	}

	template <class storage, class char_traits>
	basic_string_facade<storage, char_traits> &
		basic_string_facade<storage, char_traits>::insert(size_type index, const self_type & str, size_type index_str, size_type count /* = npos*/)
	{
		const value_type * first, * last;
		std::tie(first, last) = str.make_pointer(index_str);
		count = (std::min<size_type>)(last - first, count);

		size_type oldsize = size();
		if (index > oldsize) throw_xpos();

		value_type * out;
		size_type newsize;
		std::tie(out, newsize) = this->grow_by(count);
		
		out += index;
		traits_type::move(out + count, out, oldsize - index);
		this->set_eos(out - index + newsize);

		if (this != &str)
			traits_type::copy(out, first, count);
		else
		{
			first = out - index + index_str;
			traits_type::move(out, (first <= out ? first : first + count), count);
			// it will be moved by count if right from insert point
		}

		return *this;
	}

	template <class storage, class char_traits>
	inline typename basic_string_facade<storage, char_traits>::iterator
		basic_string_facade<storage, char_traits>::insert(const_iterator pos, value_type ch)
	{
		return insert(pos, 1, ch);
	}

	template <class storage, class char_traits>
	typename basic_string_facade<storage, char_traits>::iterator
		basic_string_facade<storage, char_traits>::insert(const_iterator pos, size_type count, value_type ch)
	{
		auto idx = pos - cbegin();
		insert(idx, count, ch);
		return begin() + idx;
	}

	template <class storage, class char_traits>
	inline typename basic_string_facade<storage, char_traits>::iterator
		basic_string_facade<storage, char_traits>::insert(const_iterator pos, std::initializer_list<value_type> ilist)
	{
		return insert(pos, ilist.begin(), ilist.size());
	}

	template <class storage, class char_traits>
	typename basic_string_facade<storage, char_traits>::iterator
		basic_string_facade<storage, char_traits>::insert(const_iterator pos, const_iterator first, const_iterator last)
	{
		size_type idx = pos - cbegin();
		insert(idx, first, last - first);
		return begin() + idx;
	}

	template <class storage, class char_traits>
	template <class InputIterator>
	typename basic_string_facade<storage, char_traits>::iterator
		basic_string_facade<storage, char_traits>::insert(const_iterator pos, InputIterator first, InputIterator last)
	{
		typedef typename std::iterator_traits<InputIterator>::iterator_category cat;
		if (!std::is_convertible<cat, std::random_access_iterator_tag>::value)
		{
			size_type idx = pos - cbegin();
			insert(idx, self_type(first, last));
			return begin() + idx;
		}
		else
		{
			size_type index = pos - cbegin();
			size_type count = std::distance(first, last);

			size_type oldsize = size();
			if (index > oldsize) throw_xpos();

			size_type newsize;
			value_type * out;
			std::tie(out, newsize) = this->grow_by(count);
			out += index;

			traits_type::move(out + count, out, oldsize - index);
			this->set_eos(out - index + newsize);

			for (auto o = out; first != last; ++first, ++o)
				traits_type::assign(*o, *first);
			
			return out;
		}
	}

	/************************************************************************/
	/*                   erase   block                                      */
	/************************************************************************/
	template <class storage, class char_traits>
	basic_string_facade<storage, char_traits> &
		basic_string_facade<storage, char_traits>::erase(size_type index /*= 0*/, size_type count /* = npos*/)
	{
		value_type * first;
		value_type * last;
		std::tie(first, last) = make_pointer(index);
		count = (std::min<size_type>)(last - first, count);
		size_type tail_count = last - first - count;

		traits_type::move(first, first + count, tail_count);
		this->set_eos(last - count);
		this->shrink_by(count);
		return *this;
	}

	template <class storage, class char_traits>
	typename basic_string_facade<storage, char_traits>::iterator
		basic_string_facade<storage, char_traits>::erase(const_iterator where)
	{
		return erase(where, where + 1);
	}

	template <class storage, class char_traits>
	typename basic_string_facade<storage, char_traits>::iterator
		basic_string_facade<storage, char_traits>::erase(const_iterator first, const_iterator last)
	{
		size_type count = last - first;
		value_type * str_last = data_end();

		traits_type::move(iterator(first), last, str_last - last);
		this->set_eos(str_last - count);

		auto res = this->shrink_by(count);
		return std::get<0>(res) + std::get<1>(res) - count;
	}

	/************************************************************************/
	/*                   replace block                                      */
	/************************************************************************/
	template <class storage, class char_traits>
	basic_string_facade<storage, char_traits> &
		basic_string_facade<storage, char_traits>::replace(const_iterator first, const_iterator last, const self_type & str)
	{
		auto it = cbegin();
		return replace(first - it, last - first, str);
	}

	template <class storage, class char_traits>
	inline basic_string_facade<storage, char_traits> &
		basic_string_facade<storage, char_traits>::replace(const_iterator first, const_iterator last, const value_type * str)
	{
		return replace(first, last, str, traits_type::length(str));
	}

	template <class storage, class char_traits>
	inline basic_string_facade<storage, char_traits> &
		basic_string_facade<storage, char_traits>::replace(const_iterator first, const_iterator last, const value_type * str, size_type len)
	{
		return replace(first, last, str, str + len);
	}

	template <class storage, class char_traits>
	basic_string_facade<storage, char_traits> &
		basic_string_facade<storage, char_traits>::replace(const_iterator first, const_iterator last, size_type count, value_type ch)
	{
		auto it = cbegin();
		return replace(first - it , last - first, count, ch);
	}

	template <class storage, class char_traits>
	inline basic_string_facade<storage, char_traits> &
		basic_string_facade<storage, char_traits>::replace(const_iterator first, const_iterator last, std::initializer_list<value_type> ilist)
	{
		return replace(first, last, ilist.begin(), ilist.end());
	}

	template <class storage, class char_traits>
	basic_string_facade<storage, char_traits> &
		basic_string_facade<storage, char_traits>::replace(
			const_iterator first, const_iterator last,
			const value_type * str_first, const value_type * str_last)
	{
		auto it = cbegin();
		return replace(first - it, last - first, str_first, str_last - str_first);
	}

	template <class storage, class char_traits>
	basic_string_facade<storage, char_traits> &
		basic_string_facade<storage, char_traits>::replace(
			const_iterator first, const_iterator last,
			value_type * str_first, value_type * str_last)
	{
		auto it = cbegin();
		return replace(first - it, last - first, str_first, str_last - str_first);
	}

	template <class storage, class char_traits>
	basic_string_facade<storage, char_traits> &
		basic_string_facade<storage, char_traits>::replace(
			size_type pos, size_type count, const self_type & str)
	{
		return replace(pos, count, str, 0, npos);
	}

	template <class storage, class char_traits>
	basic_string_facade<storage, char_traits> &
		basic_string_facade<storage, char_traits>::replace(
			size_type o_pos, size_type o_count,
			const self_type & str, size_type r_pos, size_type r_count /*= npos*/)
	{
		value_type * o_first;
		value_type * o_last;
		std::tie(o_first, o_last) = make_pointer(o_pos);

		const value_type * r_first;
		const value_type * r_last;
		std::tie(r_first, r_last) = str.make_pointer(r_pos);
		
		o_count = (std::min<size_type>)(o_last - o_first, o_count);
		r_count = (std::min<size_type>)(r_last - r_first, r_count);
		size_type tail_count = o_last - o_first - o_count;
		
		// we must handle overlap only on grow.
		// on shrink we can just move data
		if (o_count >= r_count)
		{	// shrink
			size_type diff = o_count - r_count;

			traits_type::move(o_first, r_first, r_count);
			traits_type::move(o_first + r_count, o_first + o_count, tail_count);
			this->set_eos(o_last - diff);

			this->shrink_by(diff);
			return *this;
		}
		else
		{	// grow
			size_type diff = r_count - o_count;
			std::tie(o_first, std::ignore) = this->grow_by(diff);
			
			if (this != &str)
			{	// unrelated strings, just move down and copy
				o_first += o_pos;
				traits_type::move(o_first + r_count, o_first + o_count, tail_count);
				this->set_eos(o_first + r_count + tail_count);
				traits_type::copy(o_first, r_first, r_count);
			}
			else
			{	// strings are same, we must handle overlaps
				r_first = o_first + r_pos;
				o_first += o_pos;
				
				// substring begins before hole
				if (r_first <= o_first)
				{	// remember replacement is bigger than hole (r_count > o_count) and r_first <= o_first
					// if [r_first, r_last) does not go in tail part - than no problem at all
					// if it continues after [o_first, o_last) - tail move will not touch replacement data, so it's safe
					traits_type::move(o_first + r_count, o_first + o_count, tail_count); // move_tail
					this->set_eos(o_first + r_count + tail_count); // set eos at buffer end
					traits_type::move(o_first, r_first, r_count);
				}
				// substring begins after hole
				else if (r_first >= o_first + o_count)
				{	// we move tail, and than copy replacement(forwarded by diff) into replaceable
					traits_type::move(o_first + r_count, o_first + o_count, tail_count); // move tail
					this->set_eos(o_first + r_count + tail_count); // set eos at buffer end
					traits_type::copy(o_first, r_first + diff, r_count); // replacement was moved by diff
				}
				else // substring begins in hole
				{	// remember replacement is bigger than hole.
					// first we move hole substring to beginning of hole.
					// then normal tail move, and than move tail part(diff corrected) after hole
					traits_type::move(o_first, r_first, o_count); // fill replaceable hole
					traits_type::move(o_first + r_count, o_first + o_count, tail_count); // move tail after hole
					this->set_eos(o_first + r_count + tail_count); // set eos at buffer end
					traits_type::copy(o_first + o_count, r_first + o_count + diff, diff); // fill after hole
				}
			}
			
			return *this;
		}
	}

	template <class storage, class char_traits>
	inline basic_string_facade<storage, char_traits> &
		basic_string_facade<storage, char_traits>::replace(size_type pos, size_type count, const value_type * str)
	{
		return replace(pos, count, str, traits_type::length(str));
	}
	
	template <class storage, class char_traits>
	basic_string_facade<storage, char_traits> &
		basic_string_facade<storage, char_traits>::replace(size_type pos, size_type count, const value_type * str, size_type count2)
	{
		value_type * first;
		value_type * last;
		std::tie(first, last) = make_pointer(pos);

		count = (std::min<size_type>)(last - first, count);
		size_type tail_count = last - first - count;
		
		// we must handle overlap only on grow.
		// on shrink we can just move data
		if (count >= count2)
		{	// shrink
			size_type diff = count - count2;

			traits_type::move(first, str, count2);
			traits_type::move(first + count2, first + count, tail_count);
			this->set_eos(last - diff);

			this->shrink_by(diff);
			return *this;
		}
		else
		{	// grow
			first -= pos;
			bool inside = is_inside(str, count, first, last);
			size_type inspos = str - first;

			size_type diff = count2 - count;
			std::tie(first, std::ignore) = this->grow_by(diff);
			
			if (!inside)
			{	// unrelated strings, just move down and copy
				first += pos;
				traits_type::move(first + count2, first + count, tail_count);
				this->set_eos(first + count2 + tail_count);
				traits_type::move(first, str, count2);
			}
			else
			{	// strings are same, we must handle overlaps
				str = first + inspos;
				first += pos;

				// substring begins before hole
				if (str <= first)
				{	// remember replacement is bigger than hole (count2 > count) and str <= fist
					// if [str, str + count) does not go in tail part - than no problem at all
					// if it continues after [first, last) - tail move will not touch replacement data, so it's safe
					traits_type::move(first + count2, first + count, tail_count); // move_tail
					this->set_eos(first + count2 + tail_count); // set eos at buffer end
					traits_type::move(first, str, count2);
				}
				// substring begins after hole
				else if (str >= first + count)
				{	// we move tail, and than copy replacement(forwarded by diff) into replaceable
					traits_type::move(first + count2, first + count, tail_count); // move tail
					this->set_eos(first + count2 + tail_count); // set eos at buffer end
					traits_type::copy(first, str + diff, count2); // replacement was moved by diff
				}
				else // substring begins in hole
				{	// remember replacement is bigger than hole.
					// first we move hole substring to beginning of hole.
					// then normal tail move, and than move tail part(diff corrected) after hole
					traits_type::move(first, str, count); // fill replaceable hole
					traits_type::move(first + count2, first + count, tail_count); // move tail after hole
					this->set_eos(first + count2 + tail_count); // set eos at buffer end
					traits_type::copy(first + count, str + count + diff, diff); // fill after hole
				}
			}

			return *this;
		}
	}

	template <class storage, class char_traits>
	basic_string_facade<storage, char_traits> &
		basic_string_facade<storage, char_traits>::replace(size_type o_pos, size_type o_count, size_type r_count, value_type ch)
	{
		// with this function there are cannot be overlapping by design
		value_type * o_first;
		value_type * o_last;
		std::tie(o_first, o_last) = make_pointer(o_pos);

		o_count = (std::min<size_type>)(o_count, o_last - o_first);
		size_type tail_count = o_last - o_first - o_count;
		
		if (o_count >= r_count)
		{	// shrink
			size_type diff = o_count - r_count;
			
			traits_type::assign(o_first, r_count, ch);
			traits_type::move(o_first + r_count, o_first + o_count, tail_count);
			this->set_eos(o_last - diff);

			this->shrink_by(diff);
			return *this;
		}
		else
		{	// grow
			size_type diff = r_count - o_count;
			std::tie(o_first, std::ignore) = this->grow_by(diff);
			o_first += o_pos;

			traits_type::move(o_first + r_count, o_first + o_count, tail_count);
			this->set_eos(o_first + r_count + tail_count);
			traits_type::assign(o_first, r_count, ch);
			return *this;
		}
	}

	template <class storage, class char_traits>
	template <class InputIterator>
	basic_string_facade<storage, char_traits> &
		basic_string_facade<storage, char_traits>::replace(const_iterator o_first, const_iterator o_last, InputIterator r_first, InputIterator r_last)
	{
		typedef typename std::iterator_traits<InputIterator>::iterator_category cat;
		if (!std::is_convertible<cat, std::random_access_iterator_tag>::value)
		{
			return replace(o_first, o_last, self_type(r_first, r_last));
		}

		// we handle overlapping only for overloads tacking value_type * and iterators
		// (currently out iterators are value_type *)
		// for other kind of iterators - we do not handle overloads, even if they are random_access.
		
		// o_ - our, r_ - remote
		value_type * first;
		value_type * last;
		std::tie(first, last) = this->range();

		const value_type * replace_first = o_first.base();
		const value_type * replace_last  = o_last.base();

		assert(first <= replace_first);
		assert(replace_last <= last);

		//size_type o_pos = first - o_first;
		size_type o_count = replace_last - replace_first;
		size_type r_count = std::distance(r_first, r_last);
		size_type tail_count = last - replace_last;

		if (o_count >= r_count)
		{	// shrink
			size_type diff = o_count - r_count;
			first = const_cast<value_type *>(replace_first);

			for (; r_first < r_last; ++r_first, ++first)
				traits_type::assign(*first, *r_first);

			// move trailing
			traits_type::move(first, replace_last, tail_count);
			this->set_eos(last - diff);
			this->shrink_by(diff);
			return *this;
		}
		else
		{	// grow
			size_type pos = replace_first - first;
			size_type diff = r_count - o_count;
			std::tie(first, std::ignore) = this->grow_by(diff);
			first += pos;

			traits_type::move(first + r_count, first + o_count, tail_count);
			this->set_eos(first + r_count + tail_count);

			for (; r_first < r_last; ++r_first, ++first)
				traits_type::assign(*first, *r_first);

			return *this;
		}
	}

	/************************************************************************/
	/*                 string_view support block                            */
	/************************************************************************/
	template <class storage, class char_traits>
	template <class Type>
	inline auto basic_string_facade<storage, char_traits>::assign(const Type & str)
	    -> std::enable_if_t<std::is_convertible_v<const Type &, string_view_type> and not std::is_convertible_v<const Type &, const value_type *>, self_type &>
	{
		string_view_type sv = str;
		return assign(sv.data(), sv.size());
	}

	template <class storage, class char_traits>
	template <class Type>
	auto basic_string_facade<storage, char_traits>::assign(const Type & str, size_type pos, size_type count /* = npos */)
	    -> std::enable_if_t<std::is_convertible_v<const Type &, string_view_type> and not std::is_convertible_v<const Type &, const value_type *>, self_type &>
	{
		string_view_type sv = str;
		const value_type * first, * last;
		std::tie(first, last) = make_pointer(sv, pos, count);
		count = (std::min<size_type>)(last - first, count);
		return assign(first, count);
	}

	template <class storage, class char_traits>
	template <class Type>
	inline auto basic_string_facade<storage, char_traits>::insert(size_type pos, const Type & str)
	    -> std::enable_if_t<std::is_convertible_v<const Type &, string_view_type> and not std::is_convertible_v<const Type &, const value_type *>, self_type &>
	{
		string_view_type sv = str;
		return insert(pos, sv.data(), sv.size());
	}

	template <class storage, class char_traits>
	template <class Type>
	auto basic_string_facade<storage, char_traits>::insert(size_type pos, const Type & str, size_type index_str, size_type count /* = npos */)
	    -> std::enable_if_t<std::is_convertible_v<const Type &, string_view_type> and not std::is_convertible_v<const Type &, const value_type *>, self_type &>
	{
		string_view_type sv = str;
		const value_type * first, * last;
		std::tie(first, last) = make_pointer(sv, index_str, count);
		count = (std::min<size_type>)(last - first, count);
		return insert(pos, first, count);
	}

	template <class storage, class char_traits>
	template <class Type>
	inline auto basic_string_facade<storage, char_traits>::append(const Type & str)
	    -> std::enable_if_t<std::is_convertible_v<const Type &, string_view_type> and not std::is_convertible_v<const Type &, const value_type *>, self_type &>
	{
		string_view_type sv = str;
		return append(sv.data(), sv.size());
	}

	template <class storage, class char_traits>
	template <class Type>
	auto basic_string_facade<storage, char_traits>::append(const Type & str, size_type pos, size_type count /* = npos */)
	    -> std::enable_if_t<std::is_convertible_v<const Type &, string_view_type> and not std::is_convertible_v<const Type &, const value_type *>, self_type &>
	{
		string_view_type sv = str;
		const value_type * first, * last;
		std::tie(first, last) = make_pointer(sv, pos, count);
		count = (std::min<size_type>)(last - first, count);
		return append(first, count);
	}

	template <class storage, class char_traits>
	template <class Type>
	inline auto basic_string_facade<storage, char_traits>::replace(const_iterator first, const_iterator last, const Type & str)
	    -> std::enable_if_t<std::is_convertible_v<const Type &, string_view_type> and not std::is_convertible_v<const Type &, const value_type *>, self_type &>
	{
		string_view_type sv = str;
		return replace(first, last, str.data(), str.size());
	}

	template <class storage, class char_traits>
	template <class Type>
	inline auto basic_string_facade<storage, char_traits>::replace(size_type pos, size_type count, const Type & str)
	    -> std::enable_if_t<std::is_convertible_v<const Type &, string_view_type> and not std::is_convertible_v<const Type &, const value_type *>, self_type &>
	{
		string_view_type sv = str;
		return replace(pos, count, sv.data(), sv.size());
	}

	template <class storage, class char_traits>
	template <class Type>
	auto basic_string_facade<storage, char_traits>::replace(size_type pos, size_type count, const Type & str, size_type pos2, size_type count2 /* = npos */)
	    -> std::enable_if_t<std::is_convertible_v<const Type &, string_view_type> and not std::is_convertible_v<const Type &, const value_type *>, self_type &>
	{
		string_view_type sv = str;
		const value_type * first, * last;
		std::tie(first, last) = make_pointer(sv, pos2, count2);
		count2 = (std::min<size_type>)(last - first, count2);
		return replace(pos, count, first, count2);
	}

	template <class storage, class char_traits>
	template <
	    class Type,
	    std::enable_if_t<
	                std::is_convertible_v<const Type &, std::basic_string_view<typename storage::value_type, char_traits>>
	        and not std::is_convertible_v<const Type &, const typename storage::value_type *>
	    , int
	    > /* = 0*/
	>
	basic_string_facade<storage, char_traits>::basic_string_facade(const Type & str)
	{
		string_view_type sv = str;
		auto count = sv.size();
		auto first = sv.data();

		value_type * out = this->grow_to(count);
		traits_type::copy(out, first, count);
		this->set_eos(out + count);
	}

	template <class storage, class char_traits>
	template <
	    class Type,
	    std::enable_if_t<
	                std::is_convertible_v<const Type &, std::basic_string_view<typename storage::value_type, char_traits>>
	        and not std::is_convertible_v<const Type &, const typename storage::value_type *>
	    , int
	    > /* = 0*/
	>
	basic_string_facade<storage, char_traits>::basic_string_facade(const Type & str, size_type pos, size_type count)
	{
		string_view_type sv = str;
		const value_type * first, *last;
		std::tie(first, last) = str.make_pointer(pos);
		count = (std::min<size_type>)(last - first, count);

		value_type * out = this->grow_to(count);
		traits_type::copy(out, first, count);
		this->set_eos(out + count);
	}

	/************************************************************************/
	/*                   compare block                                      */
	/************************************************************************/
	template <class storage, class char_traits>
	inline int basic_string_facade<storage, char_traits>::compare(const self_type & str) const noexcept
	{
		return compare(0, npos, str);
	}

	template <class storage, class char_traits>
	inline int basic_string_facade<storage, char_traits>::compare(size_type pos1, size_type count1, const self_type & str) const noexcept
	{
		return compare(pos1, count1, str, 0, npos);
	}

	template <class storage, class char_traits>
	int basic_string_facade<storage, char_traits>::compare
	    (size_type pos1, size_type count1,
	     const self_type & str, size_type pos2, size_type count2 /* = npos */) const noexcept
	{
		const value_type
			* first1,
			* last1,
			* first2,
			* last2;

		std::tie(first1, last1) = make_pointer(pos1);
		std::tie(first2, last2) = str.make_pointer(pos2);

		size_type sz1 = (std::min<size_type>)(last1 - first1, count1);
		size_type sz2 = (std::min<size_type>)(last2 - first2, count2);
		int answer = traits_type::compare(first1, first2, (std::min)(sz1, sz2));

		return
			answer != 0 ? answer :
		    sz1 == sz2 ?  0 :       // answer == 0, sizes deternine result
		    sz1 <  sz2 ? -1 : +1;
	}

	template <class storage, class char_traits>
	inline int basic_string_facade<storage, char_traits>::compare(const value_type * str) const noexcept
	{
		return compare(0, npos, str);
	}

	template <class storage, class char_traits>
	inline int basic_string_facade<storage, char_traits>::compare(size_type pos1, size_type count1, const value_type * str) const noexcept
	{
		return compare(0, npos, str, 0, traits_type::length(str));
	}

	template <class storage, class char_traits>
	int basic_string_facade<storage, char_traits>::compare
	    (size_type pos1, size_type count1,
		 const value_type * str, size_type count2) const noexcept
	{
		const value_type
			* first1,
			* last1;

		std::tie(first1, last1) = make_pointer(pos1);

		size_type sz1 = (std::min<size_type>)(last1 - first1, count1);
		size_type answer = traits_type::compare(first1, str, (std::min)(sz1, count2));

		return
			answer != 0 ? answer :
		    sz1 == count2 ? 0 :        // answer == 0, sizes determine result
			sz1 < count2 ? -1 : +1;
	}

	/************************************************************************/
	/*                   misc block                                         */
	/************************************************************************/
	template <class storage, class char_traits>
	auto basic_string_facade<storage, char_traits>::at(size_type pos) const -> const_reference
	{
		const value_type * first;
		std::tie(first, std::ignore) = make_pointer(pos);
		return *first;
	}

	template <class storage, class char_traits>
	auto basic_string_facade<storage, char_traits>::at(size_type pos) -> reference
	{
		const value_type * first;
		std::tie(first, std::ignore) = make_pointer(pos);
		return *first;
	}
	
	template <class storage, class char_traits>
	basic_string_facade<storage, char_traits>
		basic_string_facade<storage, char_traits>::substr(size_type pos /* = 0 */, size_type count /* = npos */) const
	{
		const value_type * first;
		const value_type * last;
		std::tie(first, last) = make_pointer(pos);

		size_type sz = (std::min<size_type>)(last - first, count);
		return self_type(first, sz);
	}

	template <class storage, class char_traits>
	auto basic_string_facade<storage, char_traits>::copy(value_type * dest, size_type count, size_type pos /* = 0 */) const -> size_type
	{
		const value_type * first;
		const value_type * last;
		std::tie(first, last) = make_pointer(pos);

		size_type sz = (std::min<size_type>)(last - first, count);
		traits_type::copy(dest, first, sz);
		return sz;
	}

	/************************************************************************/
	/*                   find block                                         */
	/************************************************************************/
	template <class storage, class char_traits>
	auto basic_string_facade<storage, char_traits>::find(const self_type & str, size_type pos /* = 0 */) const noexcept -> size_type
	{
		const value_type * first;
		const value_type * last;
		std::tie(first, last) = str.range();

		return find(first, pos, last - first);
	}

	template <class storage, class char_traits>
	auto basic_string_facade<storage, char_traits>::find(const value_type * str, size_type pos, size_type count) const noexcept -> size_type
	{
		if (pos < 0) return pos;

		const value_type * first;
		const value_type * last;
		std::tie(first, last) = this->range();
		auto start = first + pos;

		// empty strings always matches if inside this string
		if (count == 0 && start <= last) return pos;
		
		// off outside this string or count bigger than size - off -- nothing to search
		if (start >= last || count > static_cast<size_type>(last - start))
			return npos;
		
		--count;
		value_type ch = *str++;
		for (;;)
		{
			start = traits_type::find(start, last - start, ch);
			if (start == nullptr) return npos;

			if (traits_type::compare(++start, str, count) == 0)
				return start - first - 1;
		}
	}

	template <class storage, class char_traits>
	inline auto basic_string_facade<storage, char_traits>::find(const value_type * str, size_type pos /* = 0 */) const noexcept -> size_type
	{
		return find(str, pos, traits_type::length(str));
	}

	template <class storage, class char_traits>
	auto basic_string_facade<storage, char_traits>::find(value_type ch, size_type pos /* = 0 */) const noexcept -> size_type
	{
		if (pos < 0) return pos;

		const value_type * first;
		const value_type * last;
		std::tie(first, last) = this->range();
		auto start = first + pos;

		if (first >= last) return npos;
		start = traits_type::find(start, start - last, ch);
		return start ? start - first : npos;
	}

	/************************************************************************/
	/*                 rfind block                                          */
	/************************************************************************/
	template <class storage, class char_traits>
	auto basic_string_facade<storage, char_traits>::rfind(const self_type & str, size_type pos /* = npos */) const noexcept -> size_type
	{
		const value_type * first;
		const value_type * last;
		std::tie(first, last) = str.range();

		return rfind(first, pos, last - first);
	}

	template <class storage, class char_traits>
	auto basic_string_facade<storage, char_traits>::rfind(const value_type * str, size_type pos, size_type count) const noexcept -> size_type
	{
		if (pos < 0) return pos;

		const value_type * first;
		const value_type * last;
		std::tie(first, last) = this->range();

		// if empty search return pos, unless pos == npos, then return size
		if (count == 0) return pos == npos ? last - first : pos;

		auto start = static_cast<size_type>(last - first) >= pos ? first + pos : last;
		// search is bigger than this string
		if (count > static_cast<size_type>(start - first)) return npos;
		
		start -= count;
		value_type ch = *str++;
		--count;
		
		for (; start >= first; --start)
		{
			bool matched = traits_type::eq(*start, ch) && traits_type::compare(start + 1, str, count) == 0;
			if (matched) return start - first;
		}

		return npos;
	}

	template <class storage, class char_traits>
	inline auto basic_string_facade<storage, char_traits>::rfind(const value_type * str, size_type pos /* = npos */) const noexcept -> size_type
	{
		return rfind(str, pos, traits_type::length(str));
	}

	template <class storage, class char_traits>
	inline auto basic_string_facade<storage, char_traits>::rfind(value_type ch, size_type pos /* = npos */) const noexcept -> size_type
	{
		return rfind(&ch, pos, 1);
	}

	/************************************************************************/
	/*                   find_first_of                                      */
	/************************************************************************/
	template <class storage, class char_traits>
	auto basic_string_facade<storage, char_traits>::find_first_of(const self_type & str, size_type pos /* = 0 */) const noexcept -> size_type
	{
		const value_type * first;
		const value_type * last;
		std::tie(first, last) = str.range();
		
		return find_first_of(first, pos, last - first);
	}

	template <class storage, class char_traits>
	auto basic_string_facade<storage, char_traits>::find_first_of(const value_type * str, size_type pos, size_type count) const noexcept -> size_type
	{
		if (pos < 0) return pos;

		const value_type * first;
		const value_type * last;
		std::tie(first, last) = this->range();
		auto start = first + pos <= last ? first + pos : last;

		for (; start < last; ++start)
		{
			if (traits_type::find(str, count, *start))
				return start - first;
		}

		return npos;
	}

	template <class storage, class char_traits>
	inline auto basic_string_facade<storage, char_traits>::find_first_of(const value_type * str, size_type pos /* = 0 */) const noexcept -> size_type
	{
		return find_first_of(str, pos, traits_type::length(str));
	}

	template <class storage, class char_traits>
	inline auto basic_string_facade<storage, char_traits>::find_first_of(value_type ch, size_type pos /* = 0 */) const noexcept -> size_type
	{
		return find_first_of(&ch, pos, 1);
	}

	/************************************************************************/
	/*                   find_first_not_of                                  */
	/************************************************************************/
	template <class storage, class char_traits>
	auto basic_string_facade<storage, char_traits>::find_first_not_of(const self_type & str, size_type pos /* = 0 */) const noexcept -> size_type
	{
		const value_type * first;
		const value_type * last;
		std::tie(first, last) = str.range();

		return find_first_not_of(first, pos, last - first);
	}

	template <class storage, class char_traits>
	auto basic_string_facade<storage, char_traits>::find_first_not_of(const value_type * str, size_type pos, size_type count) const noexcept -> size_type
	{
		if (pos < 0) return pos;

		const value_type * first;
		const value_type * last;
		std::tie(first, last) = this->range();
		auto cur = first + pos <= last ? first + pos : last;

		for (; cur < last; ++cur)
		{
			if (!traits_type::find(str, count, *cur))
				return cur - first;
		}
		
		return npos;
	}

	template <class storage, class char_traits>
	inline auto basic_string_facade<storage, char_traits>::find_first_not_of(const value_type * str, size_type pos /* = 0 */) const noexcept -> size_type
	{
		return find_first_not_of(str, pos, traits_type::length(str));
	}

	template <class storage, class char_traits>
	inline auto basic_string_facade<storage, char_traits>::find_first_not_of(value_type ch, size_type pos /* = 0 */) const noexcept -> size_type
	{
		return find_first_not_of(&ch, pos, 1);
	}

	/************************************************************************/
	/*                  find_last_of                                        */
	/************************************************************************/
	template <class storage, class char_traits>
	auto basic_string_facade<storage, char_traits>::find_last_of(const self_type & str, size_type pos /* = npos */) const noexcept -> size_type
	{
		const value_type * first;
		const value_type * last;
		std::tie(first, last) = str.range();
		
		return find_last_of(first, pos, last - first);
	}

	template <class storage, class char_traits>
	auto basic_string_facade<storage, char_traits>::find_last_of(const value_type * str, size_type pos, size_type count) const noexcept -> size_type
	{
		if (pos < 0) return pos;

		const value_type * first;
		const value_type * last;
		std::tie(first, last) = this->range();
		auto cur = static_cast<size_type>(last - first) >= pos ? first + pos : last;
		
		// assume strings will not start at null address
		for (--cur; cur >= first; --cur)
		{
			if (traits_type::find(str, count, *cur))
				return cur - first;
		}

		return npos;
	}

	template <class storage, class char_traits>
	inline auto basic_string_facade<storage, char_traits>::find_last_of(const value_type * str, size_type pos /* = npos */) const noexcept -> size_type
	{
		return find_last_of(str, pos, traits_type::length(str));
	}

	template <class storage, class char_traits>
	inline auto basic_string_facade<storage, char_traits>::find_last_of(value_type ch, size_type pos /* = npos */) const noexcept -> size_type
	{
		return find_last_of(&ch, pos, 1);
	}

	/************************************************************************/
	/*                  find_last_not_of                                    */
	/************************************************************************/
	template <class storage, class char_traits>
	auto basic_string_facade<storage, char_traits>::find_last_not_of(const self_type & str, size_type pos /* = npos */) const noexcept -> size_type
	{
		const value_type * first;
		const value_type * last;
		std::tie(first, last) = str.range();

		return find_last_not_of(first, pos, last - first);
	}

	template <class storage, class char_traits>
	auto basic_string_facade<storage, char_traits>::find_last_not_of(const value_type * str, size_type pos, size_type count) const noexcept -> size_type
	{
		if (pos < 0) return pos;

		const value_type * first;
		const value_type * last;
		std::tie(first, last) = this->range();
		auto cur = static_cast<size_type>(last - first) >= pos ? first + pos : last;

		// assume strings will not start at null address
		for (--cur; cur >= first; --cur)
		{
			if (!traits_type::find(str, count, *cur))
				return cur - first;
		}

		return npos;
	}

	template <class storage, class char_traits>
	inline auto basic_string_facade<storage, char_traits>::find_last_not_of(const value_type * str, size_type pos /* = npos */) const noexcept -> size_type
	{
		return find_last_not_of(str, pos, traits_type::length(str));
	}

	template <class storage, class char_traits>
	inline auto basic_string_facade<storage, char_traits>::find_last_not_of(value_type ch, size_type pos /* = npos */) const noexcept -> size_type
	{
		return find_last_not_of(&ch, pos, 1);
	}

	/************************************************************************/
	/*                   non-member compare                                 */
	/************************************************************************/
	template <class storage, class char_traits>
	inline bool operator ==(const basic_string_facade<storage, char_traits> & lhs,
	                        const basic_string_facade<storage, char_traits> & rhs) noexcept
	{
		return lhs.compare(rhs) == 0;
	}

	template <class storage, class char_traits>
	inline bool operator !=(const basic_string_facade<storage, char_traits> & lhs,
	                        const basic_string_facade<storage, char_traits> & rhs) noexcept
	{
		return lhs.compare(rhs) != 0;
	}
	
	template <class storage, class char_traits>
	inline bool operator <(const basic_string_facade<storage, char_traits> & lhs,
	                       const basic_string_facade<storage, char_traits> & rhs) noexcept
	{
		return lhs.compare(rhs) < 0;
	}
	
	template <class storage, class char_traits>
	inline bool operator <=(const basic_string_facade<storage, char_traits> & lhs,
	                        const basic_string_facade<storage, char_traits> & rhs) noexcept
	{
		return lhs.compare(rhs) <= 0;
	}
	
	template <class storage, class char_traits>
	inline bool operator >(const basic_string_facade<storage, char_traits> & lhs,
	                       const basic_string_facade<storage, char_traits> & rhs) noexcept
	{
		return lhs.compare(rhs) > 0;
	}
	
	template <class storage, class char_traits>
	inline bool operator >=(const basic_string_facade<storage, char_traits> & lhs,
	                        const basic_string_facade<storage, char_traits> & rhs) noexcept
	{
		return lhs.compare(rhs) >= 0;
	}


	template <class storage, class char_traits>
	inline bool operator ==(const typename basic_string_facade<storage, char_traits>::value_type * lhs,
	                        const basic_string_facade<storage, char_traits> & rhs) noexcept
	{
		return rhs.compare(lhs) == 0;
	}
	
	template <class storage, class char_traits>
	inline bool operator ==(const basic_string_facade<storage, char_traits> & lhs,
	                        const typename basic_string_facade<storage, char_traits>::value_type * rhs) noexcept
	{
		return lhs.compare(rhs) == 0;
	}
	
	template <class storage, class char_traits>
	inline bool operator !=(const typename basic_string_facade<storage, char_traits>::value_type * lhs,
	                        const basic_string_facade<storage, char_traits> & rhs) noexcept
	{
		return rhs.compare(lhs) != 0;
	}
	
	template <class storage, class char_traits>
	inline bool operator !=(const basic_string_facade<storage, char_traits> & lhs,
	                        const typename basic_string_facade<storage, char_traits>::value_type * rhs) noexcept
	{
		return lhs.compare(rhs) != 0;
	}
	
	template <class storage, class char_traits>
	inline bool operator <(const typename basic_string_facade<storage, char_traits>::value_type * lhs,
	                       const basic_string_facade<storage, char_traits> & rhs) noexcept
	{
		return rhs.compare(lhs) > 0;
	}
	
	template <class storage, class char_traits>
	inline bool operator <(const basic_string_facade<storage, char_traits> & lhs,
	                       const typename basic_string_facade<storage, char_traits>::value_type * rhs) noexcept
	{
		return lhs.compare(rhs) < 0;
	}
	
	template <class storage, class char_traits>
	inline bool operator <=(const typename basic_string_facade<storage, char_traits>::value_type * lhs,
	                        const basic_string_facade<storage, char_traits> & rhs) noexcept
	{
		return rhs.compare(lhs) >= 0;
	}
	
	template <class storage, class char_traits>
	inline bool operator <=(const basic_string_facade<storage, char_traits> & lhs,
	                        const typename basic_string_facade<storage, char_traits>::value_type * rhs) noexcept
	{
		return lhs.compare(rhs) <= 0;
	}
	
	template <class storage, class char_traits>
	inline bool operator >(const typename basic_string_facade<storage, char_traits>::value_type * lhs,
	                       const basic_string_facade<storage, char_traits> & rhs) noexcept
	{
		return rhs.compare(lhs) < 0;
	}
	
	template <class storage, class char_traits>
	inline bool operator >(const basic_string_facade<storage, char_traits> & lhs,
	                       const typename basic_string_facade<storage, char_traits>::value_type * rhs) noexcept
	{
		return lhs.compare(rhs) > 0;
	}
	
	template <class storage, class char_traits>
	inline bool operator >=(const typename basic_string_facade<storage, char_traits>::value_type * lhs,
	                        const basic_string_facade<storage, char_traits> & rhs) noexcept
	{
		return rhs.compare(lhs) <= 0;
	}
	
	template <class storage, class char_traits>
	inline bool operator >=(const basic_string_facade<storage, char_traits> & lhs,
	                        const typename basic_string_facade<storage, char_traits>::value_type * rhs) noexcept
	{
		return lhs.compare(rhs) >= 0;
	}


	/************************************************************************/
	/*                 non member operator +                                */
	/************************************************************************/
	template <class storage, class char_traits>
	inline
	basic_string_facade<storage, char_traits> operator +(basic_string_facade<storage, char_traits> && lhs,
	                                                     const basic_string_facade<storage, char_traits> & rhs)
	{
		return std::move(lhs.append(rhs));
	}

	template <class storage, class char_traits>
	inline
	basic_string_facade<storage, char_traits> operator +(const basic_string_facade<storage, char_traits> & lhs,
	                                                     basic_string_facade<storage, char_traits> && rhs)
	{
		return std::move(rhs.insert(0, lhs));
	}

	template <class storage, class char_traits>
	inline
	basic_string_facade<storage, char_traits> operator +(basic_string_facade<storage, char_traits> && lhs,
	                                                     basic_string_facade<storage, char_traits> && rhs)
	{
		return std::move(lhs.append(rhs));
	}

	template <class storage, class char_traits>
	inline
	basic_string_facade<storage, char_traits> operator +(const typename basic_string_facade<storage, char_traits>::value_type * lhs,
	                                                     basic_string_facade<storage, char_traits> && rhs)
	{
		return std::move(rhs.insert(0, lhs));
	}
	
	template <class storage, class char_traits>
	inline
	basic_string_facade<storage, char_traits> operator +(typename basic_string_facade<storage, char_traits>::value_type lhs,
	                                                     basic_string_facade<storage, char_traits> && rhs)
	{
		return std::move(rhs.insert(0, 1, lhs));
	}

	template <class storage, class char_traits>
	inline
	basic_string_facade<storage, char_traits> operator +(basic_string_facade<storage, char_traits> && lhs,
	                                                     const typename basic_string_facade<storage, char_traits>::value_type * rhs)
	{
		return std::move(lhs.append(rhs));
	}

	template <class storage, class char_traits>
	inline
	basic_string_facade<storage, char_traits> operator +(basic_string_facade<storage, char_traits> && lhs,
	                                                     typename basic_string_facade<storage, char_traits>::value_type rhs)
	{
		return std::move(lhs.append(1, rhs));
	}



	template <class storage, class char_traits>
	inline
	basic_string_facade<storage, char_traits> operator +(const basic_string_facade<storage, char_traits> & lhs,
	                                                     const basic_string_facade<storage, char_traits> & rhs)
	{
		typedef basic_string_facade<storage, char_traits> this_type;
		return this_type(lhs) + rhs;
	}
	
	template <class storage, class char_traits>
	inline
	basic_string_facade<storage, char_traits> operator +(const typename basic_string_facade<storage, char_traits>::value_type * lhs,
	                                                     const basic_string_facade<storage, char_traits> & rhs)
	{
		typedef basic_string_facade<storage, char_traits> this_type;
		return this_type(lhs) + rhs;
	}
	
	template <class storage, class char_traits>
	inline
	basic_string_facade<storage, char_traits> operator +(typename basic_string_facade<storage, char_traits>::value_type lhs,
	                                                     const basic_string_facade<storage, char_traits> & rhs)
	{
		typedef basic_string_facade<storage, char_traits> this_type;
		return this_type(1, lhs) + rhs;
	}
	
	template <class storage, class char_traits>
	inline
	basic_string_facade<storage, char_traits> operator +(const basic_string_facade<storage, char_traits> & lhs,
	                                                     const typename basic_string_facade<storage, char_traits>::value_type * rhs)
	{
		typedef basic_string_facade<storage, char_traits> this_type;
		return this_type(lhs) + rhs;
	}
	
	template <class storage, class char_traits>
	inline
	basic_string_facade<storage, char_traits> operator +(const basic_string_facade<storage, char_traits> & lhs,
	                                                     typename basic_string_facade<storage, char_traits>::value_type rhs)
	{
		typedef basic_string_facade<storage, char_traits> this_type;
		return this_type(lhs) + rhs;
	}
	


	template <class storage, class char_traits>
	inline
	basic_string_facade<storage, char_traits> operator +(basic_string_facade<storage, char_traits> && lhs,
	                                                     std::basic_string_view<typename basic_string_facade<storage, char_traits>::value_type, char_traits> rhs)
	{
		return std::move(lhs.append(rhs));
	}

	template <class storage, class char_traits>
	inline
	basic_string_facade<storage, char_traits> operator +(const basic_string_facade<storage, char_traits> & lhs,
	                                                     std::basic_string_view<typename basic_string_facade<storage, char_traits>::value_type, char_traits> rhs)
	{
		typedef basic_string_facade<storage, char_traits> this_type;
		return this_type(lhs) + rhs;
	}

	template <class storage, class char_traits>
	inline
	basic_string_facade<storage, char_traits> operator +(std::basic_string_view<typename basic_string_facade<storage, char_traits>::value_type, char_traits> lhs,
	                                                     basic_string_facade<storage, char_traits> && rhs)
	{
		return std::move(rhs.insert(0, lhs));
	}

	template <class storage, class char_traits>
	inline
	basic_string_facade<storage, char_traits> operator +(std::basic_string_view<typename basic_string_facade<storage, char_traits>::value_type, char_traits> lhs,
	                                                     const basic_string_facade<storage, char_traits> & rhs)
	{
		typedef basic_string_facade<storage, char_traits> this_type;
		return lhs + this_type(rhs);
	}

} // namespace ext
