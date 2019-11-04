#pragma once
#include <queue>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <ext/intrusive_ptr.hpp>
#include <ext/future.hpp>

namespace ext
{
	/// scheduler implementation via background thread with priority_queue.
	/// Task can be submitted via submit method.
	/// For every task result of execution can be retrieved via associated future.
	/// 
	/// All methods are thread-safe
	class threaded_scheduler
	{
	public:
		typedef std::chrono::steady_clock::time_point time_point;
		typedef std::chrono::steady_clock::duration   duration;

	private:
		class task_base
		{
		public:
			time_point point;

		public:
			virtual ~task_base() = default;

			// note: addref, release are present in ext::packaged_once_task_impl, that can issue a conflict,
			// if signature same, but return value different - it's a error.
			// just name them differently, it's internal private class.
			virtual void task_addref()   noexcept = 0;
			virtual void task_release()  noexcept = 0;
			virtual void task_abandone() noexcept = 0;
			virtual void task_execute()  noexcept = 0;

		public:
			friend inline void intrusive_ptr_add_ref(task_base * ptr) noexcept { ptr->task_addref(); }
			friend inline void intrusive_ptr_release(task_base * ptr) noexcept { ptr->task_release(); }
			friend inline void intrusive_ptr_use_count(const task_base * ptr) noexcept {}
		};

		template <class Functor, class ResultType>
		class task_impl :
			public task_base,
			public ext::packaged_once_task_impl<Functor, ResultType()>
		{
			typedef ext::packaged_once_task_impl<Functor, ResultType()> base_type;

		public:
			void task_addref()   noexcept override { base_type::addref(); }
			void task_release()  noexcept override { base_type::release(); }
			void task_abandone() noexcept override { base_type::release_promise(); }
			void task_execute()  noexcept override { base_type::execute(); }

		public:
			task_impl(time_point tp, Functor func)
				: base_type(std::move(func)) { task_base::point = tp; }

		public:
			friend inline void intrusive_ptr_add_ref(task_impl * ptr) noexcept { ptr->addref(); }
			friend inline void intrusive_ptr_release(task_impl * ptr) noexcept { ptr->release(); }
			friend inline void intrusive_ptr_use_count(const task_impl * ptr) noexcept {}
		};
		
		typedef ext::intrusive_ptr<task_base> task_ptr;

		class entry_comparer
		{
		public:
			typedef bool result_type;
			bool operator()(const task_ptr & t1, const task_ptr & t2) const noexcept;
		};

	private:
		typedef std::priority_queue<
			task_ptr,
			std::deque<task_ptr>,
			entry_comparer
		> queue_type;

	private:
		queue_type m_queue;
		std::thread m_thread;
		bool m_stopped = false;

		mutable std::mutex m_mutex;
		mutable std::condition_variable m_newdata;

	private:
		void thread_func();
		void run_passed_events();

		template <class Lock>
		time_point next_in(Lock & lk) const noexcept;

	public:
		template <class Functor, class ... Args>
		auto submit(time_point tp, Functor && func, Args && ... args) ->
			ext::future<std::invoke_result_t<std::decay_t<Functor>, std::decay_t<Args>...>>;

		template <class Functor, class ... Args>
		auto submit(duration  rel, Functor && func, Args && ... args) ->
			ext::future<std::invoke_result_t<std::decay_t<Functor>, std::decay_t<Args>...>>;
		
		void clear() noexcept;

	public:
		threaded_scheduler();
		~threaded_scheduler() noexcept;

		threaded_scheduler(threaded_scheduler &&) = delete;
		threaded_scheduler & operator =(threaded_scheduler &&) = delete;

		threaded_scheduler(const threaded_scheduler & ) = delete;
		threaded_scheduler & operator =(const threaded_scheduler &) = delete;
	};

	template <class Functor, class ... Args>
	auto threaded_scheduler::submit(time_point tp, Functor && func, Args && ... args) ->
		ext::future<std::invoke_result_t<std::decay_t<Functor>, std::decay_t<Args>...>>
	{
		auto closure = [func = std::forward<Functor>(func),
		                args_tuple = std::make_tuple(std::forward<Args>(args)...)]() mutable
		{
			return ext::apply(std::move(func), std::move(args_tuple));
		};
		
		using result_type = std::invoke_result_t<std::decay_t<Functor>, std::decay_t<Args>...>;
		using functor_type = decltype(closure);
		using task_type = task_impl<functor_type, result_type>;
		using future_type = ext::future<result_type>;

		auto task = ext::make_intrusive<task_type>(tp, std::move(closure));
		future_type fut {task};

		{
			std::lock_guard lk(m_mutex);
			m_queue.push(std::move(task));
		}
		
		m_newdata.notify_one();
		return fut;
	}

	template <class Functor, class ... Args>
	inline auto threaded_scheduler::submit(duration rel, Functor && func, Args && ... args) ->
		ext::future<std::invoke_result_t<std::decay_t<Functor>, std::decay_t<Args>...>>
	{
		return submit(rel + time_point::clock::now(), std::forward<Functor>(func), std::forward<Args>(args)...);
	}
}
