#pragma once

#include <mutex>
#include <queue>

#define ERROR_EMPTY_QUEUE -2

template <typename T>
class MutexQueue
{
public:
	size_t size()
	{
		std::lock_guard<std::mutex> lock(que_mtx);
		return queue.size();
	}

	T& front()
	{
		std::lock_guard<std::mutex> lock(que_mtx);
		return queue.front();
	}

	T& back()
	{
		std::lock_guard<std::mutex> lock(que_mtx);
		return queue.back();
	}

	void push(const T& val)
	{
		std::lock_guard<std::mutex> lock(que_mtx);
		queue.push(val);
	}

	_Check_return_ bool pop(T& output)
	{
		std::lock_guard<std::mutex> lock(que_mtx);
		if (!queue.empty())
		{
			output = queue.front();
			queue.pop();
			return true;
		}
		else
			return false;
	}

private:
	std::queue<T> queue;

	std::mutex que_mtx;
};