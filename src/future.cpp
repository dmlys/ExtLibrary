// author: Dmitry Lysachenko
// date: Saturday 20 august 2016
// license: boost software license
//          http://www.boost.org/LICENSE_1_0.txt

#include <ext/future.hpp>
#include <vector>
#include <thread>

namespace ext
{
	struct future_errc_category_impl : std::error_category
	{
		const char * name() const noexcept override { return "ext::future_errc"; }
		std::string message(int val) const override;
	};

	std::string future_errc_category_impl::message(int val) const
	{
		switch (static_cast<future_errc>(val))
		{
			case future_errc::broken_promise:                return "broken_promise";
			case future_errc::future_already_retrieved:      return "future_already_retrieved";
			case future_errc::promise_already_satisfied:     return "promise_already_satisfied";
			case future_errc::no_state:                      return "no_state";
			case future_errc::cancelled:                     return "cancelled";
			default:
				return "unknown sock_errc code";
		}
	}

	const future_errc_category_impl future_errc_category_instance;
	const std::error_category & future_category() { return future_errc_category_instance; }

	future_error::future_error(const std::error_code & ec) noexcept
		: std::logic_error(ec.message()), m_err(ec) { }

	const char * future_error::what() const noexcept
	{
		return std::logic_error::what();
	}


	/************************************************************************/
	/*          shared_state_basic method implementation                    */
	/************************************************************************/
	namespace
	{
		class auto_unlocker
		{
			std::atomic_uintptr_t * m_ptr = nullptr;

		public:
			void release() noexcept { m_ptr = nullptr; }

		public:
			auto_unlocker(std::atomic_uintptr_t & ptr) : m_ptr(&ptr) {}
			~auto_unlocker() { if (m_ptr) shared_state_basic::unlock_ptr(*m_ptr); }

			auto_unlocker(const auto_unlocker &) = delete;
			auto_unlocker  & operator =(const auto_unlocker &) = delete;
		};
		
		static void backoff() noexcept
		{
			std::this_thread::yield(); // there can be better options than yield
		}
	}


	static continuation_waiter * acquire_waiter();
	static void release_waiter(continuation_waiter * ptr) noexcept;

	std::uintptr_t shared_state_basic::lock_ptr(std::atomic_uintptr_t & ptr) noexcept
	{
		// lock head
		auto fstate = ptr.load(std::memory_order_relaxed);
		if (fstate == ready) return ready;

		fstate &= ~lock_mask;

		// std::memory_order_acquire - fstate is pointer to some continuation_type,
		// and it will be checked if it's a waiter - we need acquire changes
		while (not ptr.compare_exchange_weak(fstate, fstate | lock_mask,
		                                     std::memory_order_acquire, std::memory_order_relaxed))
		{
			if (fstate == ready) return ready;
			
			backoff();
			fstate &= ~lock_mask;
		}

		return fstate;
	}

	void shared_state_basic::unlock_ptr(std::atomic_uintptr_t & ptr) noexcept
	{
		// head is locked
		assert(ptr.load(std::memory_order_relaxed) & lock_mask);
		ptr.fetch_and(~lock_mask, std::memory_order_release);
	}

	void shared_state_basic::unlock_ptr(std::atomic_uintptr_t & ptr, std::uintptr_t newval) noexcept
	{
		// head is locked
		assert(not (newval & lock_mask));
		assert(ptr.load(std::memory_order_relaxed) & lock_mask);
		ptr.exchange(newval, std::memory_order_release);
	}

	std::uintptr_t shared_state_basic::signal_future(std::atomic_uintptr_t & fstnext) noexcept
	{
		auto fstate = fstnext.load(std::memory_order_relaxed);
		fstate &= ~lock_mask;

		// compare_exchange only not locked value.
		// release data set in promise: m_val, m_exception;
		// acquire continuation list
		while (not fstnext.compare_exchange_weak(fstate, ready, std::memory_order_acq_rel, std::memory_order_relaxed))
		{
			backoff();
			fstate &= ~lock_mask;
		}

		return fstate;
	}

	bool shared_state_basic::attach_continuation(std::atomic_uintptr_t & head, continuation_type * continuation, shared_state_basic * caller) noexcept
	{
		assert(not dynamic_cast<continuation_waiter *>(continuation));
		assert(not is_continuation(continuation->m_fstnext.load(std::memory_order_relaxed)));

		auto fstate = lock_ptr(head);
		if (fstate == ready)
		{   // state became ready. just execute continuation.
			continuation->continuate(caller); // it's defined as noexcept
			return false;
		}

		auto * head_ptr = reinterpret_cast<continuation_type *>(fstate);
		if (is_continuation(fstate) && is_waiter(head_ptr))
		{
			continuation->addref();
			auto next = head_ptr->m_fstnext.load(std::memory_order_relaxed);
			continuation->m_fstnext.store(next, std::memory_order_relaxed);
			head_ptr->m_fstnext.store(reinterpret_cast<std::uintptr_t>(continuation), std::memory_order_relaxed);
		}
		else
		{
			continuation->addref();
			continuation->m_fstnext.store(fstate, std::memory_order_relaxed);
			fstate = reinterpret_cast<std::uintptr_t>(continuation);
		}

		unlock_ptr(head, fstate);
		return true;
	}

	void shared_state_basic::run_continuations(std::uintptr_t addr, shared_state_basic * caller) noexcept
	{
		if (not is_continuation(addr)) return;

		auto * ptr = reinterpret_cast<continuation_type *>(addr);
		if (is_waiter(ptr))
		{
			auto * waiter = static_cast<continuation_waiter *>(ptr);
			addr = waiter->m_fstnext.load(std::memory_order_acquire);
			waiter->continuate(caller);

			if (waiter->release() == 1)
				ext::release_waiter(waiter);

			goto loop;
		}

		do
		{
			addr = ptr->m_fstnext.load(std::memory_order_acquire);
			ptr->continuate(caller);
			ptr->release();

		loop:
			ptr = reinterpret_cast<continuation_type *>(addr);
		} while (is_continuation(addr));
	}

	auto shared_state_basic::acquire_waiter(std::atomic_uintptr_t & head) -> continuation_waiter *
	{
		auto fstate = lock_ptr(head);
		if (fstate == ready) return nullptr;

		continuation_waiter * waiter;
		auto continuation = reinterpret_cast<continuation_type *>(fstate);
		if (is_continuation(fstate) && is_waiter(continuation))
		{
			waiter = static_cast<continuation_waiter *>(continuation);
			waiter->addref();
		}
		else
		{
			auto_unlocker lock(head);
			waiter = ext::acquire_waiter();
			assert(waiter);
			lock.release();
			
			// one for wait call, one for continuation chain running after set_value call.
			// it will decrement refcount for all continuations in chain, so we must increment for them too
			waiter->addref(2);
			waiter->m_fstnext.store(fstate);
			fstate = reinterpret_cast<std::uintptr_t>(waiter);
		}

		unlock_ptr(head, fstate);
		return waiter;
	}

	void shared_state_basic::release_waiter(std::atomic_uintptr_t & head, continuation_waiter * waiter) noexcept
	{
		auto fstate = lock_ptr(head);
		if (fstate == ready)
		{
			if (waiter->release() != 1) return;

			return ext::release_waiter(waiter);
		};

		assert(is_continuation(fstate));
		assert(is_waiter(reinterpret_cast<continuation_type *>(fstate)));
		assert(waiter == reinterpret_cast<continuation_type *>(fstate));

		if (waiter->release() > 2)
			unlock_ptr(head, fstate);
		else
		{	// we are the last one using this waiter, one was reserved to set_value call,
			// but we holding pointer lock, so they will not traverse concurrently.
			// remove waiter from chain and unlock pointer.
			unlock_ptr(head, waiter->m_fstnext.load(std::memory_order_relaxed));
			waiter->release();
			
			ext::release_waiter(waiter);
		}

		return;
	}

	void shared_state_basic::set_future_ready() noexcept
	{
		auto chain = signal_future(m_fstnext);
		run_continuations(chain, this);
	}

	bool shared_state_basic::add_continuation(continuation_type * continuation) noexcept
	{
		return attach_continuation(m_fstnext, continuation, this);
	}

	auto shared_state_basic::acquire_waiter() -> continuation_waiter *
	{
		return acquire_waiter(m_fstnext);
	}

	void shared_state_basic::release_waiter(continuation_waiter * waiter) noexcept
	{
		return release_waiter(m_fstnext, waiter);
	}

	bool shared_state_basic::satisfy_check_promise(future_state reason)
	{
		unsigned newval, previous = m_promise_state.load(std::memory_order_relaxed);

		do {
			switch (pstatus(previous))
			{
				default:
					EXT_UNREACHABLE();

				case future_state::value:
				case future_state::exception:
				case future_state::abandonned:
					throw future_error(make_error_code(future_errc::promise_already_satisfied));

					// if we were cancelled - just ignore
				case future_state::cancellation:
					return false;

				case future_state::unsatisfied:
				case future_state::deferred:
					break;
			}

			newval = (previous & ~status_mask) | static_cast<unsigned>(reason);

		} while (not m_promise_state.compare_exchange_weak(previous, newval, std::memory_order_relaxed));
		return true;
	}

	bool shared_state_basic::satisfy_promise(future_state reason) noexcept
	{
		unsigned newval, previous = m_promise_state.load(std::memory_order_relaxed);

		do {
			switch (pstatus(previous))
			{
				default:
					EXT_UNREACHABLE();

				case future_state::value:
				case future_state::exception:
				case future_state::abandonned:
				case future_state::cancellation:
					return false;

				case future_state::unsatisfied:
				case future_state::deferred:
					break;
			}

			newval = (previous & ~status_mask) | static_cast<unsigned>(reason);

		} while (not m_promise_state.compare_exchange_weak(previous, newval, std::memory_order_relaxed));
		return true;
	}

	bool shared_state_basic::mark_uncancellable() noexcept
	{
		unsigned previous = m_promise_state.load(std::memory_order_relaxed);

		do {
			if (previous & future_uncancellable)
				return pstatus(previous) != future_state::cancellation;

			switch (pstatus(previous))
			{
				default:
					EXT_UNREACHABLE();

				case future_state::value:
				case future_state::exception:
				case future_state::abandonned:
				case future_state::cancellation:
					return false;

				case future_state::unsatisfied:
				case future_state::deferred:
					break;
			}

		} while (not m_promise_state.compare_exchange_weak(previous, previous | future_uncancellable, std::memory_order_relaxed));
		return true;
	}

	bool shared_state_basic::mark_marked() noexcept
	{
		unsigned previous = m_promise_state.load(std::memory_order_relaxed);
		unsigned newval;

		do {
			if (previous & future_marked)
				return false;

			switch (pstatus(previous))
			{
				default:
					EXT_UNREACHABLE();

				case future_state::value:
				case future_state::exception:
				case future_state::abandonned:
				case future_state::cancellation:
					return false;

				case future_state::unsatisfied:
				case future_state::deferred:
					break;
			}

			newval = previous | future_uncancellable | future_marked;
		} while (not m_promise_state.compare_exchange_weak(previous, newval, std::memory_order_relaxed));
		return true;
	}

	bool shared_state_basic::cancel() noexcept
	{
		unsigned newval, previous = m_promise_state.load(std::memory_order_relaxed);

		do {
			if (previous & future_uncancellable)
				return false;

			switch (pstatus(previous))
			{
				default:
					EXT_UNREACHABLE();

				case future_state::value:
				case future_state::exception:
				case future_state::abandonned:
				case future_state::cancellation:
					return false;

				case future_state::unsatisfied:
				case future_state::deferred:
					break;
			}

			newval = (previous & ~status_mask) | static_cast<unsigned>(future_state::cancellation);

		} while (not m_promise_state.compare_exchange_weak(previous, newval, std::memory_order_relaxed));

		set_future_ready();
		return true;
	}

	void shared_state_basic::release_promise() noexcept
	{
		if (not satisfy_promise(future_state::abandonned)) return;

		set_future_ready();
		return;
	}

	void shared_state_basic::wait() const
	{
		auto waiter = ext::unconst(this)->acquire_waiter();
		if (not waiter) return; // became ready

		waiter->wait_ready();
		ext::unconst(this)->release_waiter(waiter);
	}

	future_status shared_state_basic::wait_for(std::chrono::steady_clock::duration timeout_duration) const
	{
		auto waiter = ext::unconst(this)->acquire_waiter();
		if (not waiter) return future_status::ready;

		auto res = waiter->wait_ready(timeout_duration) ?
		           future_status::ready :
		           future_status::timeout;

		ext::unconst(this)->release_waiter(waiter);
		return res;
	}

	future_status shared_state_basic::wait_until(std::chrono::steady_clock::time_point timeout_point) const
	{
		auto waiter = ext::unconst(this)->acquire_waiter();
		if (not waiter) return future_status::ready;

		auto res = waiter->wait_ready(timeout_point) ?
		           future_status::ready :
		           future_status::timeout;
		
		ext::unconst(this)->release_waiter(waiter);
		return res;
	}

	void unwrap_continuation::set_future_ready() noexcept
	{
		auto fstate = signal_future(m_task_next);
		run_continuations(fstate, this);
	}

	void unwrap_continuation::continuate(shared_state_basic * caller) noexcept
	{
		// we add ourself as continuation only to one shared_state at once, so there is no concurrency,
		// but continuate can run on different threads - we have to constraint memory access.
		// 
		// continuate is called after chain is acquired with std::memory_order_acquire semantics
		// so std::atomic_thread_fence(std::memory_order_acquire) is already done
		auto ptr = m_future_ptr.load(std::memory_order_relaxed);
		auto state = reinterpret_cast<ext::shared_state_basic *>(ptr);

		// either it was last future in future chain,
		// or some result was different from value(exception, abandoned, ...)
		if (not m_unwrap_count.load(std::memory_order_relaxed) or not state->has_value())
		{
			// transfer future status into us and become ready
			satisfy_promise(state->status());
			set_future_ready();
			return;
		}
		
		// NOTE: this is only method here that is not noexcept,
		// but at this point we know that it has value and it's value_type, so it's safe
		auto newstate = state->get<const ext::intrusive_ptr<shared_state_basic> &>().get();
		auto newptr = reinterpret_cast<std::uintptr_t>(newstate);
		newstate->addref();

		// set new current state, note that only continuate updates current state,
		// and it does it here, so there is no concurrency here
		lock_ptr(m_future_ptr);
		m_unwrap_count.fetch_sub(1, std::memory_order_relaxed);
		unlock_ptr(m_future_ptr, newptr);

		// if deferred - run it
		if (newstate->is_deferred())
			newstate->wait();

		m_fstnext.store(~lock_mask, std::memory_order_relaxed);
		newstate->add_continuation(this);

		// release old state
		state->release();

		// add_continuation(this) have memory_order_release semantics on this pointer
		// (unlock_ptr in attach_continuation)
		// so std::atomic_thread_fence(std::memory_order_release) is already done
	}

	bool unwrap_continuation::cancel() noexcept
	{
		if (is_ready()) return false;

		auto & future_ptr = ext::unconst(m_future_ptr);
		auto ptr = lock_ptr(future_ptr);
		// note: we increase refcount under lock,
		// so we can use newstate even after it released in continuate method
		ext::intrusive_ptr<shared_state_basic> state {reinterpret_cast<ext::shared_state_basic *>(ptr)};
		unlock_ptr(future_ptr);

		return state->cancel();
	}

	void * unwrap_continuation::get_ptr()
	{
		auto ptr = m_future_ptr.load(std::memory_order_relaxed);
		auto state = reinterpret_cast<ext::shared_state_basic *>(ptr);
		return state->get_ptr();
	}

	unwrap_continuation::unwrap_continuation(ext::intrusive_ptr<shared_state_basic> future, unsigned unwrap_count) noexcept
		: m_future_ptr(reinterpret_cast<std::uintptr_t>(future.get())),
		  m_unwrap_count(unwrap_count)
	{
		if (future->is_deferred())
			future->wait();

		future.release()->add_continuation(this);
	}

	unwrap_continuation::~unwrap_continuation() noexcept
	{
		// required memory constraints are already should be done by this->release
		auto ptr = m_future_ptr.load(std::memory_order_relaxed);
		auto state = reinterpret_cast<ext::shared_state_basic *>(ptr);
		state->release();
	}


	void continuation_waiter_impl::continuate(shared_state_basic * caller) noexcept
	{
		{
			unique_lock lk(m_mutex);
			m_ready.store(true, std::memory_order_relaxed);
		}

		m_var.notify_all();
	}

	void continuation_waiter_impl::wait_ready() noexcept
	{
		unique_lock lk(m_mutex);
		m_var.wait(lk, [this] { return m_ready.load(std::memory_order_relaxed); });
	}

	bool continuation_waiter_impl::wait_ready(std::chrono::steady_clock::time_point timeout_point) noexcept
	{
		unique_lock lk(m_mutex);
		return m_var.wait_until(lk, timeout_point, [this] { return m_ready.load(std::memory_order_relaxed); });
	}

	bool continuation_waiter_impl::wait_ready(std::chrono::steady_clock::duration timeout_duration) noexcept
	{
		unique_lock lk(m_mutex);
		return m_var.wait_for(lk, timeout_duration, [this] { return m_ready.load(std::memory_order_relaxed); });
	}

	void continuation_waiter_impl::reset() noexcept
	{
		m_ready.store(false, std::memory_order_relaxed);
		m_fstnext.store(fsnext_init, std::memory_order_relaxed);
		m_promise_state.store(static_cast<unsigned>(future_state::unsatisfied), std::memory_order_relaxed);
	}



	class default_continuation_waiters_pool : public continuation_waiters_pool
	{
	protected:
		std::atomic_uint m_usecount = ATOMIC_VAR_INIT(0);

	public:
		void take(waiter_ptr & ptr) override;
		void putback(waiter_ptr & ptr) override;
		bool used() const noexcept override { return m_usecount.load(std::memory_order_relaxed); }
	};

	void default_continuation_waiters_pool::take(waiter_ptr & ptr)
	{
		ptr = ext::make_intrusive<ext::continuation_waiter_impl>();
		m_usecount.fetch_add(1, std::memory_order_relaxed);
	}

	void default_continuation_waiters_pool::putback(waiter_ptr & ptr)
	{
		ptr = nullptr;
		m_usecount.fetch_sub(1, std::memory_order_relaxed);
	}


	class arena_continuation_pool : public continuation_waiters_pool
	{
	protected:
		std::atomic<std::size_t> m_first_avail;
		std::atomic<std::size_t> m_last_avail;
		std::atomic<std::size_t> m_first_free;
		std::atomic<std::size_t> m_usecount = 0;
		
		std::vector<waiter_ptr> m_objects;
	
	protected:
		void init(std::size_t num);
		void free();
	
	public:
		void take(waiter_ptr & ptr) noexcept override;
		void putback(waiter_ptr & ptr) noexcept override;
		bool used() const noexcept override;
	
	public:
		arena_continuation_pool(std::size_t num) { init(num); }
		~arena_continuation_pool() { free(); }

	protected:
		arena_continuation_pool() = default;
	};
	
	void arena_continuation_pool::init(std::size_t num)
	{
		m_objects.resize(num);
		for (auto & val : m_objects)
			val = ext::make_intrusive<ext::continuation_waiter_impl>();
	
		// Strictly speaking lockfree_continuation_pool should be created and initiated before any thread are created,
		// any new threads will happen later and see anything done here, without any memory_fence. so this one is unneeded.
		std::atomic_thread_fence(std::memory_order_release);

		m_first_avail.store(0, std::memory_order_relaxed);
		m_last_avail.store(0, std::memory_order_relaxed);
		m_first_free.store(0, std::memory_order_relaxed);
	}
	
	void arena_continuation_pool::free()
	{
		// all objects are in the pool
		assert(m_first_avail == m_first_free);
		m_objects.clear();
	}

	void arena_continuation_pool::take(waiter_ptr & ptr) noexcept
	{
		std::size_t first, last, new_first;
		std::size_t sz = m_objects.size();
		do {
			first = m_first_avail.load(std::memory_order_relaxed);
			last = m_last_avail.load(std::memory_order_relaxed);
			new_first = (first + 1) % sz;
	
			if (new_first == last)
			{
				//throw std::runtime_error("ext::future: waiter pool exhausted, see init_future_library");
				ptr = ext::make_intrusive<ext::continuation_waiter_impl>();
				m_usecount.fetch_add(1, std::memory_order_relaxed);
				return;
			}
	
		} while (not m_first_avail.compare_exchange_weak(first, new_first, std::memory_order_acquire, std::memory_order_relaxed));
		
		// successfully taken element, now move it
		ptr = std::move(m_objects[first]);
	}

	bool arena_continuation_pool::used() const noexcept
	{
		return m_last_avail.load(std::memory_order_relaxed) != m_first_free.load(std::memory_order_relaxed) or m_usecount.load(std::memory_order_relaxed);;
	}

	void arena_continuation_pool::putback(waiter_ptr & ptr) noexcept
	{
		std::size_t first_avail, first_free, new_free;
		std::size_t sz = m_objects.size();
	
		// acquire free cell, where to put ptr
		do {
			first_avail = m_first_avail.load(std::memory_order_relaxed);
			first_free = m_first_free.load(std::memory_order_relaxed);
			new_free = (first_free + 1) % sz;
	
			// pool is full, actually this should not happen
			if (first_free == first_avail)
			{
				ptr.release();
				m_usecount.fetch_sub(1, std::memory_order_relaxed);
				return;
			}
		
		} while(not m_first_free.compare_exchange_weak(first_free, new_free, std::memory_order_relaxed));
		
		m_objects[first_free] = std::move(ptr);
		
		// There can be multiple concurrent calls to putback, m_last_avail should be increased in order,
		// but writing can be out of order - later call, can finish writing to cell earlier.
		// Force order, by incrementing m_last_avail, only when m_last_avail is index of our cell.
		// This is somewhat ugly, and produce waiting, for now i think it would suffice.
		while (not m_last_avail.compare_exchange_weak(first_free, new_free, std::memory_order_release))
			backoff();
	}

	// global object pool of continuation_waiters.
	static default_continuation_waiters_pool g_default_pool;
	static continuation_waiters_pool * g_pool = &g_default_pool;

	static continuation_waiter * acquire_waiter()
	{
		continuation_waiters_pool::waiter_ptr ptr;
		g_pool->take(ptr);

		if (not ptr)
			throw std::runtime_error("ext::future: waiter pool exhausted, see init_future_library");

		assert(ptr->use_count() == 1);
		return ptr.release();
	}

	static void release_waiter(continuation_waiter * ptr) noexcept
	{
		assert(ptr->use_count() == 1);
		continuation_waiters_pool::waiter_ptr wptr {ptr, ext::noaddref};
		wptr->reset();
		g_pool->putback(wptr);
	}

	bool init_future_library(std::unique_ptr<continuation_waiters_pool> pool)
	{
		if (g_pool->used()) return false;

		if (g_pool != &g_default_pool)
			delete g_pool;

		g_pool = pool.release();
		return true;
	}

	bool init_future_library(unsigned waiter_slots)
	{
		if (g_pool->used()) return false;

		if (g_pool != &g_default_pool)
			delete g_pool;

		// waiter_slots == 0 - use default_continuation_waiters_pool: allocate waiters new/delete
		if (waiter_slots == 0)
		{
			g_pool = &g_default_pool;
			return true;
		}
		
		g_pool = new arena_continuation_pool(waiter_slots);
		return true;
	}

	void free_future_library()
	{
		if (g_pool != &g_default_pool)
		{
			delete g_pool;
			g_pool = &g_default_pool;
		}
	}
}
