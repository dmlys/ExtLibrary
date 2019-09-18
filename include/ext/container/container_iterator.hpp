#pragma once
#include <type_traits>
#include <iterator>

namespace ext::container
{
	template <class Iterator, class Container>
	class container_iterator
	{
		using self_type = container_iterator;

	private:
		Iterator m_it;

	public:
		using iterator_type = Iterator;
		using iterator_category = typename std::iterator_traits<iterator_type>::iterator_category;
		using value_type = typename std::iterator_traits<iterator_type>::value_type;
		using difference_type = typename std::iterator_traits<iterator_type>::difference_type;
		using reference = typename std::iterator_traits<iterator_type>::reference;
		using pointer = typename std::iterator_traits<iterator_type>::pointer;

	public:
		const iterator_type & base() const noexcept { return m_it; }

		reference operator  *() const noexcept { return *m_it; }
		pointer   operator ->() const noexcept { return  m_it; }

		self_type & operator ++()    noexcept { ++m_it; return *this; }
		self_type   operator ++(int) noexcept { return self_type(m_it++); }

		self_type & operator --()    noexcept { --m_it; return *this; }
		self_type   operator --(int) noexcept { return self_type(m_it--); }

		reference operator[](difference_type n) const noexcept { return m_it[n]; }

		self_type & operator +=(difference_type n) noexcept { m_it += n; return *this; }
		self_type & operator -=(difference_type n) noexcept { m_it -= n; return *this; }

		self_type   operator  +(difference_type n) noexcept { return self_type(m_it + n); }
		self_type   operator  -(difference_type n) noexcept { return self_type(m_it - n); }

	public:
		container_iterator() noexcept = default;
		~container_iterator() noexcept = default;

	public:
		explicit container_iterator(iterator_type it) noexcept : m_it(it) {}

		template <class OtherIterator>
		container_iterator(const container_iterator<OtherIterator, std::enable_if_t<std::is_convertible_v<OtherIterator, Iterator>, Container>> & other)
		    : m_it(other.base()) {}
	};


	template <class IteratorL, class IteratorR, class Container>
	inline auto operator -(const container_iterator<IteratorL, Container> & lhs, const container_iterator<IteratorR, Container> & rhs) noexcept
	{
		return lhs.base() - rhs.base();
	}

	template <class Iterator, class Container>
	inline auto operator +(typename container_iterator<Iterator, Container>::difference_type lhs, const container_iterator<Iterator, Container> & rhs) noexcept
	{
		return container_iterator<Iterator, Container>(rhs.base() + lhs);
	}

	template <class IteratorL, class IteratorR, class Container>
	inline bool operator ==(const container_iterator<IteratorL, Container> & lhs, const container_iterator<IteratorR, Container> & rhs) noexcept
	{
		return lhs.base() == rhs.base();
	}

	template <class IteratorL, class IteratorR, class Container>
	inline bool operator !=(const container_iterator<IteratorL, Container> & lhs, const container_iterator<IteratorR, Container> & rhs) noexcept
	{
		return lhs.base() != rhs.base();
	}

	template <class IteratorL, class IteratorR, class Container>
	inline bool operator <(const container_iterator<IteratorL, Container> & lhs, const container_iterator<IteratorR, Container> & rhs) noexcept
	{
		return lhs.base() < rhs.base();
	}

	template <class IteratorL, class IteratorR, class Container>
	inline bool operator <=(const container_iterator<IteratorL, Container> & lhs, const container_iterator<IteratorR, Container> & rhs) noexcept
	{
		return lhs.base() <= rhs.base();
	}

	template <class IteratorL, class IteratorR, class Container>
	inline bool operator >(const container_iterator<IteratorL, Container> & lhs, const container_iterator<IteratorR, Container> & rhs) noexcept
	{
		return lhs.base() > rhs.base();
	}

	template <class IteratorL, class IteratorR, class Container>
	inline bool operator >=(const container_iterator<IteratorL, Container> & lhs, const container_iterator<IteratorR, Container> & rhs) noexcept
	{
		return lhs.base() >= rhs.base();
	}
}
