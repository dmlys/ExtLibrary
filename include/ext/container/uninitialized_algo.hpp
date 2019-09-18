#pragma once
#include <memory>
#include <iterator>

namespace ext::container
{
	template <class Allocator, class InputIterator, class ForwadIterator>
	ForwadIterator uninitialized_copy(Allocator & alloc, InputIterator first, InputIterator last, ForwadIterator out)
	{
		using allocator_traits = std::allocator_traits<Allocator>;
		using value_type = typename std::iterator_traits<ForwadIterator>::value_type;
		ForwadIterator current = out;

		try
		{
			for (; first != last; (void) ++first, (void) ++current)
				allocator_traits::construct(alloc, std::addressof(*current), value_type(*first));

			return current;
		}
		catch (...)
		{
			for (; out != current; ++out)
				allocator_traits::destroy(alloc, std::addressof(*out));

			throw;
		}
	}

	template <class Allocator, class InputIterator, class Size, class ForwadIterator>
	ForwadIterator uninitialized_copy_n(Allocator & alloc, InputIterator first, Size count, ForwadIterator out)
	{
		using allocator_traits = std::allocator_traits<Allocator>;
		using value_type = typename std::iterator_traits<ForwadIterator>::value_type;
		ForwadIterator current = out;

		try
		{
			for (; count > 0; (void) ++first, (void) ++current, --count)
				allocator_traits::construct(alloc, std::addressof(*current), value_type(*first));

			return current;
		}
		catch (...)
		{
			for (; out != current; ++out)
				allocator_traits::destroy(alloc, std::addressof(*out));

			throw;
		}
	}

	template <class Allocator, class InputIterator, class ForwadIterator>
	ForwadIterator uninitialized_move(Allocator & alloc, InputIterator first, InputIterator last, ForwadIterator out)
	{
		using allocator_traits = std::allocator_traits<Allocator>;
		ForwadIterator current = out;

		try
		{
			for (; first != last; (void) ++first, (void) ++current)
				allocator_traits::construct(alloc, std::addressof(*current), std::move(*first));

			return current;
		}
		catch (...)
		{
			for (; out != current; ++out)
				allocator_traits::destroy(alloc, std::addressof(*out));

			throw;
		}
	}

	template <class Allocator, class InputIterator, class Size, class ForwadIterator>
	ForwadIterator uninitialized_move_n(Allocator & alloc, InputIterator first, Size count, ForwadIterator out)
	{
		using allocator_traits = std::allocator_traits<Allocator>;
		ForwadIterator current = out;

		try
		{
			for (; count > 0; (void) ++first, (void) ++current, --count)
				allocator_traits::construct(alloc, std::addressof(*current), std::move(*first));

			return current;
		}
		catch (...)
		{
			for (; out != current; ++out)
				allocator_traits::destroy(alloc, std::addressof(*out));

			throw;
		}
	}

	template <class Allocator, class InputIterator, class ForwadIterator>
	ForwadIterator uninitialized_move_if_noexcept(Allocator & alloc, InputIterator first, InputIterator last, ForwadIterator out)
	{
		using allocator_traits = std::allocator_traits<Allocator>;
		ForwadIterator current = out;

		try
		{
			for (; first != last; (void) ++first, (void) ++current)
				allocator_traits::construct(alloc, std::addressof(*current), std::move_if_noexcept(*first));

			return current;
		}
		catch (...)
		{
			for (; out != current; ++out)
				allocator_traits::destroy(alloc, std::addressof(*out));

			throw;
		}
	}

	template <class Allocator, class InputIterator, class Size, class ForwadIterator>
	ForwadIterator uninitialized_move_if_noexcept_n(Allocator & alloc, InputIterator first, Size count, ForwadIterator out)
	{
		using allocator_traits = std::allocator_traits<Allocator>;
		ForwadIterator current = out;

		try
		{
			for (; count > 0; (void) ++first, (void) ++current, --count)
				allocator_traits::construct(alloc, std::addressof(*current), std::move_if_noexcept(*first));

			return current;
		}
		catch (...)
		{
			for (; out != current; ++out)
				allocator_traits::destroy(alloc, std::addressof(*out));

			throw;
		}
	}

	class default_constructor
	{
	public:
		template <class Allocator, class Type>
		void operator()(Allocator & alloc, Type * ptr) const
		{
			std::allocator_traits<Allocator>::construct(alloc, ptr);
		}

		template <class Type>
		void operator()(std::allocator<Type> & alloc, Type * ptr) const
		{
			new(ptr) Type;
		}
	};

	template <class Type>
	class fill_constructor
	{
		const Type * val;

	public:
		template <class Allocator>
		void operator()(Allocator & alloc, Type * ptr) const
		{
			std::allocator_traits<Allocator>::construct(alloc, ptr, *val);
		}

		fill_constructor(const Type & val) : val(&val) {}
	};

	template <class Allocator, class Constructor, class ForwardIterator>
	ForwardIterator uninitialized_construct(Allocator & alloc, const Constructor & constructor, ForwardIterator first, ForwardIterator last)
	{
		using allocator_traits = std::allocator_traits<Allocator>;
		ForwardIterator current = first;

		try
		{
			for (; current != last; ++current)
				constructor(alloc, std::addressof(*current));

			return current;
		}
		catch (...)
		{
			for (; first != current; ++first)
				allocator_traits::destroy(std::addressof(*first));

			throw;
		}
	}

	template <class Allocator, class Constructor, class ForwardIterator, class Size>
	ForwardIterator uninitialized_construct_n(Allocator & alloc, const Constructor & constructor, ForwardIterator first, Size count)
	{
		using allocator_traits = std::allocator_traits<Allocator>;
		ForwardIterator current = first;

		try
		{
			for (; count > 0; (void) ++current, --count)
				constructor(alloc, std::addressof(*current));

			return current;
		}
		catch (...)
		{
			for (; first != current; ++first)
				allocator_traits::destroy(alloc, std::addressof(*first));

			throw;
		}
	}

	template <class Allocator, class Type, class ... Args>
	inline void construct(Allocator & alloc, Type * where, Args && ... args)
	{
		std::allocator_traits<Allocator>::construct(alloc, where, std::forward<Args>(args)...);
	}

	template <class Allocator, class Type>
	inline void destroy_at(Allocator & alloc, Type * ptr)
	{
		std::allocator_traits<Allocator>::destroy(alloc, ptr);
	}

	template <class Allocator, class InputIterator>
	void destroy(Allocator & alloc, InputIterator first, InputIterator last)
	{
		for (; first != last; ++first)
			std::allocator_traits<Allocator>::destroy(alloc, std::addressof(*first));
	}

	template <class Allocator, class InputIterator, class Size>
	void destroy_n(Allocator & alloc, InputIterator first, Size count)
	{
		for (; count > 0; (void) ++first, --count)
			std::allocator_traits<Allocator>::destroy(alloc, std::addressof(*first));
	}
}
