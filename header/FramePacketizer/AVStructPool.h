#pragma once

extern "C" {
#include <libavcodec/packet.h>
#include <libavutil/frame.h>
}

#include "MutexQueue/MutexQueue.h"

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

	AVStructPool() {};

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
class SharedAVStruct
{
	friend class AVStructPool<T>;
public:
	~SharedAVStruct()
	{
		data_src.ReturnObj(av_data);
	}

	T getPointer()
	{
		return av_data;
	}
private:
	template <typename U = T, RequireSpecialType<U>* = nullptr>
	SharedAVStruct(T data, AVStructPool<T>& data_src) :data_src(data_src), av_data(data) {}

	T av_data;
	AVStructPool<T>& data_src;
};

