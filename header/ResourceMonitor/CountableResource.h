#pragma once

#include <shared_mutex>
#include <typeinfo>
#include <format>

template<class T>
class CountableResource
{
public:
	static unsigned int GetRemainResource();
	static std::string PrintRemainResource();

protected:
	CountableResource();
	~CountableResource();

	static std::shared_mutex counter_mtx;
	static unsigned int resource_count;
};

template<class T>
std::shared_mutex CountableResource<T>::counter_mtx;

template<class T>
unsigned int  CountableResource<T>::resource_count = 0;

template<class T>
inline unsigned int CountableResource<T>::GetRemainResource()
{
	std::shared_lock<std::shared_mutex> s_lock(counter_mtx);
	return resource_count;
}

template<class T>
inline std::string CountableResource<T>::PrintRemainResource()
{
	std::string ret;
	ret = std::format("Remain {}: {}", typeid(T).name(), GetRemainResource());
	return ret;
}

template<class T>
inline CountableResource<T>::CountableResource()
{
	std::unique_lock<std::shared_mutex> u_lock(counter_mtx);
	resource_count++;
}

template<class T>
inline CountableResource<T>::~CountableResource()
{
	std::unique_lock<std::shared_mutex> u_lock(counter_mtx);
	resource_count--;
}


