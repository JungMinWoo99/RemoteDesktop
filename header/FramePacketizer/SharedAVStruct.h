#pragma once

extern "C" {
#include <libavcodec/packet.h>
#include <libavutil/frame.h>
}
#include <iostream>

template <typename U>
using RequireSpecialType = typename std::enable_if<std::is_same<U, AVPacket*>::value ||
	std::is_same<U, AVFrame*>::value>::type;

template <typename T>
class SharedAVStruct
{
	
public:
	template <typename U = T, RequireSpecialType<U>* = nullptr>
	SharedAVStruct() {}
};

using SharedAVPacket = std::shared_ptr<SharedAVStruct<AVPacket*>>;
using SharedAVFrame = std::shared_ptr<SharedAVStruct<AVFrame*>>;

template <typename U, RequireSpecialType<U>* = nullptr>
std::shared_ptr<SharedAVStruct<U>> MakeSharedAVStruct()
{
	return std::make_shared<SharedAVStruct<U>>();
}

template <>
class SharedAVStruct<AVFrame*>
{

public:
	SharedAVStruct()
	{
		data = av_frame_alloc();
		if (data == NULL)
		{
			std::cout << "av_frame_alloc fail" << std::endl;
			exit(-1);
		}
	}

	~SharedAVStruct()
	{
		av_frame_free(&data);
	}

	AVFrame*& getPointer()
	{
		return data;
	}
private:
	AVFrame* data;
};

template <>
class SharedAVStruct<AVPacket*>
{

public:
	SharedAVStruct()
	{
		data = av_packet_alloc();
		if (data == NULL)
		{
			std::cout << "av_packet_alloc fail" << std::endl;
			exit(-1);
		}
	}

	~SharedAVStruct()
	{
		av_packet_free(&data);
	}

	AVPacket*& getPointer()
	{
		return data;
	}
private:
	AVPacket* data;
};
