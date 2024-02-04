#pragma once

#include  <shared_mutex>
#include <queue>
#include <vector>
#include <iostream>
#include <fstream>

class MutexQueueMonitor;

class MonitorableQueue
{
public:
	virtual size_t size() = 0;
};

template <typename T>
class MutexQueue : public MonitorableQueue
{
public:

	MutexQueue(std::string queue_name)
	{
		MutexQueueMonitor::getMonitor().push(std::make_pair<MonitorableQueue*, std::string>(this, queue_name.c_str()));
	}

	size_t size() override
	{
		std::shared_lock<std::shared_mutex> s_lock(que_mtx);
		return queue.size();
	}

	T& front()
	{
		std::shared_lock<std::shared_mutex> s_lock(que_mtx);
		return queue.front();
	}

	T& back()
	{
		std::shared_lock<std::shared_mutex> s_lock(que_mtx);
		return queue.back();
	}

	void push(const T& val)
	{
		std::unique_lock<std::shared_mutex> u_lock(que_mtx);
		queue.push(val);
	}

	_Check_return_ bool pop(T& output)
	{
		std::unique_lock<std::shared_mutex> u_lock(que_mtx);
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
	MutexQueue();

	std::queue<T> queue;

	std::shared_mutex que_mtx;
};

class MutexQueueMonitor
{
public:
	static MutexQueueMonitor& getMonitor()
	{
		std::call_once(initFlag, &MutexQueueMonitor::initInstance);
		return *instance;
	}

	void push(std::pair< MonitorableQueue*, std::string> new_queue)
	{
		queue_info_list.push_back(new_queue);
	}

	void PrintQueueList() const
	{
		static int print_count = 0;

		log_stream << ++print_count << "th queue status report" << std::endl;
		for (std::pair< MonitorableQueue*, std::string> queue_info : queue_info_list)
		{
			log_stream << queue_info.second << " size: " << queue_info.first->size() << std::endl;
		}
	}

private:
	static std::unique_ptr<MutexQueueMonitor> instance;
	static std::once_flag initFlag;

	static std::ofstream log_stream;

	static void initInstance() 
	{
		instance.reset(new MutexQueueMonitor());
	}

	MutexQueueMonitor() {};

	std::vector<std::pair< MonitorableQueue*, std::string>> queue_info_list;
};

