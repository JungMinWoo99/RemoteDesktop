#pragma once

extern "C" {
#include <libavcodec/packet.h>
#include <libavutil/frame.h>
}

#include "MutexQueue/MutexQueue.h"
#include "ResourceMonitor/CountableResource.h"

#include <memory>

template <typename U>
using RequireSpecialType = typename std::enable_if<
	std::is_pointer<U>::value ||
	std::is_same<U, AVPacket*>::value ||
	std::is_same<U, AVFrame*>::value>::type;

template <class T>
class SharedAVStruct;

using SharedAVPacket = SharedAVStruct<AVPacket*>;
using SharedAVFrame = SharedAVStruct<AVFrame*>;

template <typename T>
class AVStructPool
{
	friend class SharedAVStruct<T>;
public:
	template <typename U = T, RequireSpecialType<U>* = nullptr>
	static AVStructPool<U>& getInstance() {
		std::call_once(initFlag, &AVStructPool<U>::initInstance);
		return *instance;
	}

	~AVStructPool();

	_Check_return_ std::shared_ptr<SharedAVStruct<T>> getEmptyObj()
	{
		T obj = nullptr;
		if (!empty_obj_queue.pop(obj))
			obj = CreateObj();
		SharedAVStruct<T>* shared_obj = new SharedAVStruct<T>(obj, *this);
		return std::make_shared<SharedAVStruct<T>>(*shared_obj);
	}

	size_t getPoolMemSize()
	{
		T instance;
		return sizeof(*instance) * empty_obj_queue.size();
	}

private:
	static std::unique_ptr<AVStructPool<T>> instance;
	static std::once_flag initFlag;

	AVStructPool():empty_obj_queue("AVStructPool") {};

	static void initInstance() {
		instance.reset(new AVStructPool<T>);
	}

	MutexQueue<T> empty_obj_queue;

	void ReturnObj(T& obj);
	T CreateObj();
};

//static member init
template <class T>
std::unique_ptr<AVStructPool<T>> AVStructPool<T>::instance = nullptr;
template <class T>
std::once_flag AVStructPool<T>::initFlag;

template <typename T>
class SharedAVStruct: public CountableResource<SharedAVStruct<T>>
{
	friend class AVStructPool<T>;
public:
	static int getRemainAVStruct();

	~SharedAVStruct()
	{
		remain--;
		data_src.ReturnObj(av_data);
	}

	T getPointer()
	{
		return av_data;
	}
private:
	static int remain;

	template <typename U = T, RequireSpecialType<U>* = nullptr>
	SharedAVStruct(T data, AVStructPool<T>& data_src) :data_src(data_src), av_data(data) { remain++; }

	T av_data;
	AVStructPool<T>& data_src;
};

template<class T>
int SharedAVStruct<T>::remain = 0;

template<class T>
int SharedAVStruct<T>::getRemainAVStruct()
{
	return remain;
}