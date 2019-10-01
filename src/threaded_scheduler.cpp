#include <ext/threaded_scheduler.hpp>

namespace ext
{
	static constexpr auto max_timepoint()
	{
		// MSVC 2015 and some version of gcc have a bug,
		// that waiting in std::chrono::steady_clock::time_point::max()
		// does not work due to integer overflow internally.
		// 
		// Prevent this by returning time_point::max() / 2, value still will be quite a big

		return std::chrono::steady_clock::time_point {
			std::chrono::steady_clock::duration {std::chrono::steady_clock::duration::max().count() / 2}
		};
	}

	bool threaded_scheduler::entry_comparer::operator()(const task_ptr & t1, const task_ptr & t2) const noexcept
	{
		// priority_queue with std::less provides constant lookup for greatest element, we need smallest
		return t1->point > t2->point;
	}

	template <class Lock>
	inline auto threaded_scheduler::next_in(Lock & lk) const noexcept -> time_point
	{
		return m_queue.empty() ? max_timepoint() : m_queue.top()->point;
	}

	void threaded_scheduler::run_passed_events()
	{
		auto now = time_point::clock::now();
		task_ptr item;

		for (;;)
		{
			{
				std::lock_guard lk(m_mutex);
				if (m_queue.empty()) return;

				auto & top = m_queue.top();
				if (now < top->point) break;
			
				item = std::move(top);
				m_queue.pop();
			}
		
			item->task_execute();
		}
	}

	void threaded_scheduler::thread_func()
	{
		for (;;)
		{
			run_passed_events();

			std::unique_lock lk(m_mutex);
			if (m_stopped) return;

			auto wait = next_in(lk);
			m_newdata.wait_until(lk, wait);
		}
	}

	void threaded_scheduler::clear() noexcept
	{
		queue_type queue;
		{
			std::lock_guard lk(m_mutex);
			queue = std::move(m_queue);
		}

		while (!queue.empty())
		{
			queue.top()->task_abandone();
			queue.pop();
		}

		m_newdata.notify_one();
	}

	threaded_scheduler::threaded_scheduler()
	{
		m_thread = std::thread(&threaded_scheduler::thread_func, this);
	}

	threaded_scheduler::~threaded_scheduler() noexcept
	{
		{
			std::lock_guard lk(m_mutex);
			m_stopped = true;
			
			while (!m_queue.empty())
			{
				m_queue.top()->task_abandone();
				m_queue.pop();
			}
		}
		
		m_newdata.notify_one();
		m_thread.join();
	}
}
