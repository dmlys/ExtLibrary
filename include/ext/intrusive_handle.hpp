#pragma once
#include <cassert>
#include <utility> // for std::exchange
#include <boost/operators.hpp>
#include <ext/noaddref.hpp>

namespace ext
{
	/// similar to intrusive_ptr, but manges object pointed by numeric handle.
	template <class HandleType, class HandleTraits, class TagType = void>
	class intrusive_handle :
		boost::totally_ordered<intrusive_handle<HandleType, HandleTraits, TagType>, HandleType,
		  boost::totally_ordered1<intrusive_handle<HandleType, HandleTraits, TagType>,
		    HandleTraits> >
	{
		using self_type = intrusive_handle;

	public:
		using handle_traits = HandleTraits;
		using handle_type   = HandleType;
		using tag_type      = TagType;

	private:
		handle_type m_handle = defval();

	private:
		static handle_type defval() noexcept { return handle_traits::defval(static_cast<handle_type>(0)); }

	public:
		auto use_count() const noexcept { return handle_traits::use_count(m_handle); }
		auto addref()          noexcept { return handle_traits::addref(m_handle);    }
		auto subref()          noexcept { return handle_traits::subref(m_handle);    }

		// releases the ownership of the managed object if any.
		// get() returns defval after the call. Semanticly same as unique_ptr::release
		auto release() noexcept { return std::exchange(m_handle, defval()); }

		// reset returns whatever intrusive_ptr_release returns(including void)
		void reset(handle_type handle)                noexcept;
		void reset(handle_type handle, noaddref_type) noexcept;
		void reset(handle_type handle, bool add_ref)  noexcept;
		void reset()                                  noexcept { return reset(defval()); }

		handle_type get() const noexcept { return m_handle; }

		explicit operator handle_type() const noexcept { return m_handle; }
		explicit operator bool() const noexcept { return m_handle != defval(); }

	public:
		bool operator  <(const intrusive_handle & ptr) const noexcept { return m_handle  < ptr.m_handle; }
		bool operator ==(const intrusive_handle & ptr) const noexcept { return m_handle == ptr.m_handle; }

		bool operator  <(handle_type handle) const noexcept { return m_handle  < handle; }
		bool operator ==(handle_type handle) const noexcept { return m_handle == handle; }

	public:
		explicit intrusive_handle(handle_type handle)                noexcept : m_handle(handle) { addref(); }
		explicit intrusive_handle(handle_type handle, noaddref_type) noexcept : m_handle(handle) { }
		explicit intrusive_handle(handle_type handle, bool add_ref)  noexcept : m_handle(handle) { if (add_ref) addref(); }

		intrusive_handle()  noexcept = default;
		~intrusive_handle() noexcept { subref(); }

		intrusive_handle(const self_type & op) noexcept : m_handle(op.m_handle) { addref(); }
		intrusive_handle(self_type && op)      noexcept : m_handle(op.release()) {}

		intrusive_handle & operator =(const self_type & op) noexcept { if (this != &op) reset(op.m_handle);             return *this; }
		intrusive_handle & operator =(self_type && op)      noexcept { if (this != &op) reset(op.release(), noaddref);  return *this; }

		friend void swap(intrusive_handle & p1, intrusive_handle & p2) noexcept { std::swap(p1.m_handle, p2.m_handle); }
	};

	template <class HandleType, class HandleTraits, class TagType>
	void intrusive_handle<HandleType, HandleTraits, TagType>::reset(handle_type handle) noexcept
	{
		subref();
		m_handle = handle;
		addref();
	}

	template <class HandleType, class HandleTraits, class TagType>
	void intrusive_handle<HandleType, HandleTraits, TagType>::reset(handle_type handle, noaddref_type) noexcept
	{
		subref();
		m_handle = handle;
	}

	template <class HandleType, class HandleTraits, class TagType>
	void intrusive_handle<HandleType, HandleTraits, TagType>::reset(handle_type handle, bool add_ref) noexcept
	{
		subref();
		m_handle = handle;
		if (add_ref) addref();
	}

}
