#pragma once

#include <mutex>
#include <condition_variable>
#include <deque>

template <typename T>
class BlockingQueue
{
private:
	std::mutex              d_mutex;
	std::condition_variable d_condition;
	std::deque<T>           d_queue;
public:
	void push(T const& value) {
		{
			std::unique_lock<std::mutex> lock(this->d_mutex);
			d_queue.push_front(value);
		}
		this->d_condition.notify_one();
	}
	T pop() {
		std::unique_lock<std::mutex> lock(this->d_mutex);
		this->d_condition.wait(lock, [=] { return !this->d_queue.empty(); });
		T rc(std::move(this->d_queue.back()));
		this->d_queue.pop_back();
		return rc;
	}
	void clear(){
		std::unique_lock<std::mutex> lock(this->d_mutex);
		this->d_queue.clear();
	}
	boost::optional<T> popMaybe(){
		std::unique_lock<std::mutex> lock(this->d_mutex);
		if (this->d_queue.size() == 0)
			return boost::none;
		T rc(std::move(this->d_queue.back()));
		this->d_queue.pop_back();
		return rc;
	}
	size_t size() {
		std::unique_lock<std::mutex> lock(this->d_mutex);
		return this->d_queue.size();
	}
};