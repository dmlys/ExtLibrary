#pragma once
#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>

#include <boost/intrusive/list.hpp>
#include <ext/intrusive_ptr.hpp>
#include <ext/future.hpp>

namespace ext
{
	/// simple thread_pool implementation.
	/// Task can be submitted via submit method.
	/// For every task result of execution can be retrieved via associated future.
	/// Number of running threads can be controlled via set_nworkers/get_nworkers methods.
	/// By default thread_pool constructed with 0 workers, you must explicitly set number you want.
	/// 
	/// All methods are thread-safe
	class thread_pool
	{
		typedef boost::intrusive::list_base_hook<
			boost::intrusive::link_mode<boost::intrusive::link_mode_type::normal_link>
		> hook_type;

	private:
		/// base interface for submitted tasks,
		/// tasks are hold in intrusive linked list.
		class task_base : public hook_type
		{
		public:
			virtual ~task_base() = default;

			// note: addref, release are present in ext::packaged_once_task_impl, that can issue a conflict,
			// if signature same, but return value different - it's a error.
			// just name them differently, it's internal private class.
			virtual void task_addref()  noexcept = 0;
			virtual void task_release() noexcept = 0;
			virtual void task_abandone() noexcept = 0;
			virtual void task_execute() noexcept = 0;

		public:
			friend inline void intrusive_ptr_add_ref(task_base * ptr) noexcept { ptr->task_addref(); }
			friend inline void intrusive_ptr_release(task_base * ptr) noexcept { ptr->task_release(); }
			friend inline void intrusive_ptr_use_count(const task_base * ptr) noexcept {}
		};

		/// implementation of task_base templated by Functor
		template <class Functor, class ResultType>
		class task_impl :
			public task_base,
			public ext::packaged_once_task_impl<Functor, ResultType()>
		{
			typedef ext::packaged_once_task_impl<Functor, ResultType()> base_type;

		public:
			void task_addref()  noexcept override { base_type::addref(); }
			void task_release() noexcept override { base_type::release(); }
			void task_abandone() noexcept override { base_type::release_promise(); }
			void task_execute() noexcept override { base_type::execute(); }

		public:
			// inherit constructors
			using base_type::base_type;

		public:
			friend inline void intrusive_ptr_add_ref(task_impl * ptr) noexcept { ptr->task_addref(); }
			friend inline void intrusive_ptr_release(task_impl * ptr) noexcept { ptr->task_release(); }
			friend inline void intrusive_ptr_use_count(const task_impl * ptr) noexcept {}
		};

		class delayed_task_continuation :
			public ext::continuation_base,
			public hook_type
		{
			friend thread_pool;

			thread_pool * m_owner;
			ext::intrusive_ptr<task_base> m_task;

		public:
			void continuate(shared_state_basic * caller) noexcept override;
			void abandone() noexcept;

		public:
			delayed_task_continuation(thread_pool * owner, ext::intrusive_ptr<task_base> task)
				: m_owner(owner), m_task(std::move(task)) {}
		};
		
		/// thread worker object, also a future. When thread is finished - future becomes fulfilled
		class worker : public ext::shared_state_unexceptional<void>
		{
			typedef ext::shared_state_unexceptional<void> base_type;
			friend thread_pool;

		private:
			thread_pool * m_parent;
			std::thread m_thread;
			std::atomic_bool m_stop_request = ATOMIC_VAR_INIT(false);

		private:
			static void thread_func(ext::intrusive_ptr<worker> self);
			
		public:
			bool stop_request() noexcept;

		public:
			worker(thread_pool * parent);
		};

	private:
		typedef ext::intrusive_ptr<worker> worker_ptr;
		typedef boost::intrusive::base_hook<hook_type> item_list_option;

		typedef boost::intrusive::list<
			task_base, item_list_option,
			boost::intrusive::constant_time_size<false>
		> task_list_type;

		typedef boost::intrusive::list <
			delayed_task_continuation, item_list_option,
			boost::intrusive::constant_time_size<false>
		> delayed_task_continuation_list;

	private:
		// linked list of task
		task_list_type m_tasks;

		// delayed tasks are little tricky, for every one - we create a service continuation,
		// which when fired, adds task to task_list.
		// Those can work and fire when we are being destructed,
		// thread_pool lifetime should not linger on delayed_task - they should become abandoned.
		// Nevertheless we have lifetime problem.
		//
		// so we store those active service continuations in a list:
		//  - When continuation is fired it checks if it's taken(future internal helper flag):
		//    * if yes - thread_pool is gone, nothing to do;
		//    * if not - thread_pool is still there and we should add task to a list;
		// 
		//  - When destructing where are checking each continuation if it's taken(future internal helper flag):
		//   * if successful - service continuation is sort of cancelled and it will not access thread_pool
		//   * if not - continuation is firing right now somewhere in the middle,
		//     so destructor must wait until it finishes and then complete destruction.
		delayed_task_continuation_list m_delayed;

		// how many delayed_continuations were not "taken/cancelled" at destruction,
		// and how many we must wait - it's sort of a semaphore.
		std::size_t m_delayed_count = 0;

		// vector of worker objects, it also holds workers that are stopping.
		// vector is always partitioned by working/stopping:
		//   [0, m_pending)    - working threads
		//   [m_pending, last) - stopping threads
		std::vector<worker_ptr> m_workers;
		std::size_t m_pending = 0;

		mutable std::mutex m_mutex;
		mutable std::condition_variable m_event;

	private:
		static bool is_finished(const worker_ptr & wptr) noexcept { return wptr->is_ready(); }
		static bool join_worker(worker_ptr & wptr);
		void thread_func(std::atomic_bool & stop_request);

	public: // execution control
		/// returns current number of workers
		unsigned get_nworkers() const;
		/// sets number of workers:
		/// if n == $current - does nothing and returns ready future
		/// if n >  $current - creates more workers and returns ready future
		/// if n <  $current - stops $current - n workers and returns future,
		///                   which become ready when all stopped workers are completely stopped.
		ext::future<void> set_nworkers(unsigned n);

		/// stops all workers, returns future which become ready when all workers,
		/// that were created in these thread pool at any time,
		/// are completely stopped.
		ext::future<void> stop() { return set_nworkers(0); }

	public: // job control
		/// submits task for execution, returns future representing result of execution.
		template <class Functor, class ... Args>
		auto submit(Functor && func, Args && ... args) ->
		    ext::future<std::invoke_result_t<std::decay_t<Functor>, std::decay_t<Args>...>>;
		
		template <class Future, class Functor, class ... Args>
		auto submit(Future future, Functor && func, Args && ... args) ->
			ext::future<std::invoke_result_t<std::decay_t<Functor>, std::enable_if_t<is_future_type_v<Future>, Future>, std::decay_t<Args>...>>;

		/// clears all not already executed tasks.
		/// Associated futures status become abandoned
		void clear() noexcept;

	public:
		// 0 means 0, no workers at all, you must explicitly set number you want
		thread_pool(unsigned nworkers = 0);
		~thread_pool() noexcept;

		thread_pool(thread_pool &&) = delete;
		thread_pool & operator =(thread_pool &&) = delete;

		thread_pool(const thread_pool &) = delete;
		thread_pool & operator =(const thread_pool &) = delete;
	};

	template <class Functor, class ... Args>
	auto thread_pool::submit(Functor && func, Args && ... args) ->
		ext::future<std::invoke_result_t<std::decay_t<Functor>, std::decay_t<Args>...>>
	{
		using result_type = std::invoke_result_t<std::decay_t<Functor>, std::decay_t<Args>...>;
		
		auto closure = [func = std::forward<Functor>(func),
		                args_tuple = std::make_tuple(std::forward<Args>(args)...)]() mutable -> result_type
		{
			return ext::apply(std::move(func), std::move(args_tuple));
		};

		using functor_type = decltype(closure);
		using task_type = task_impl<functor_type, result_type>;
		using future_type = ext::future<result_type>;
		
		auto task = ext::make_intrusive<task_type>(std::move(closure));
		future_type fut {task};

		{
			std::lock_guard lk(m_mutex);
			m_tasks.push_back(*task.release());
		}

		m_event.notify_one();
		return fut;
	}

	template <class Future, class Functor, class ... Args>
	auto thread_pool::submit(Future future, Functor && func, Args && ... args) ->
		ext::future<std::invoke_result_t<std::decay_t<Functor>, std::enable_if_t<is_future_type_v<Future>, Future>, std::decay_t<Args>...>>
	{
		static_assert(ext::is_future_type_v<Future>);

		auto handle = future.handle();
		auto closure = [func = std::forward<Functor>(func),
		                args_tuple = std::make_tuple(std::move(future), std::forward<Args>(args)...)]() mutable
		{
			return ext::apply(std::move(func), std::move(args_tuple));
		};

		using functor_type = decltype(closure);
		using result_type = std::invoke_result_t<functor_type>;
		using task_type = task_impl<functor_type, result_type>;
		using future_type = ext::future<result_type>;

		auto task = ext::make_intrusive<task_type>(std::move(closure));
		future_type fut {task};

		if (handle->is_deferred())
		{	// make it ready
			handle->wait();

			{
				std::lock_guard lk(m_mutex);
				m_tasks.push_back(*task.release());
			}

			m_event.notify_one();
		}
		else
		{
			auto cont = ext::make_intrusive<delayed_task_continuation>(this, std::move(task));

			{
				std::lock_guard lk(m_mutex);
				m_delayed.push_back((cont.addref(), *cont.get()));
			}

			handle->add_continuation(cont.get());
		}
		
		return fut;
	}
}
