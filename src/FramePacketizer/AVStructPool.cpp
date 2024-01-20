#include "FramePacketizer/AVStructPool.h"
#include <iostream>
template <>
AVStructPool<AVFrame*>::~AVStructPool()
{
	AVFrame* obj;

	while (empty_obj_queue.pop(obj))
		av_frame_free(&obj);
}

template <>
AVStructPool<AVPacket*>::~AVStructPool()
{
	AVPacket* obj;

	while (empty_obj_queue.pop(obj))
		av_packet_free(&obj);
}

template<>
void AVStructPool<AVFrame*>::ReturnObj(AVFrame*& obj)
{
	av_frame_unref(obj);
	empty_obj_queue.push(obj);
	obj = nullptr;
}

template <>
void AVStructPool<AVPacket*>::ReturnObj(AVPacket*& obj)
{
	av_packet_unref(obj);
	empty_obj_queue.push(obj);
	obj = nullptr;
}

template<>
AVFrame* AVStructPool<AVFrame*>::CreateObj()
{
	AVFrame* data = av_frame_alloc();
	if (data == NULL)
	{
		std::cout << "av_frame_alloc fail" << std::endl;
		exit(-1);
	}
	return data;
}

template <>
AVPacket* AVStructPool<AVPacket*>::CreateObj()
{
	AVPacket* data = av_packet_alloc();
	if (data == NULL)
	{
		std::cout << "av_packet_alloc fail" << std::endl;
		exit(-1);
	}
	return data;
}