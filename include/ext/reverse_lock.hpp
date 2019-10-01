#pragma once

namespace ext
{
	template <class Lock>
	class reverse_lock
	{
		Lock & lk;

	public:
		explicit reverse_lock(Lock & lk) : lk(lk) { lk.unlock(); }
		~reverse_lock() { lk.lock(); }

		reverse_lock(reverse_lock && ) = delete;
		reverse_lock & operator =(reverse_lock && ) = delete;
	};
}
