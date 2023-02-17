#pragma once
#include <atomic>
#include <mutex>
#include <thread>

class checked_mutex final : public std::mutex
{
#ifndef NDEBUG
public:
	void lock()
	{
		std::mutex::lock();
		m_holder = std::this_thread::get_id();
	}

	void unlock()
	{
		m_holder = std::thread::id();
		std::mutex::unlock();
	}

	/**
	* @return true iff the mutex is locked by the caller of this method. */
	bool locked_by_caller() const
	{
		return m_holder == std::this_thread::get_id();
	}

private:
	std::atomic<std::thread::id> m_holder;
#endif // #ifndef NDEBUG
};
