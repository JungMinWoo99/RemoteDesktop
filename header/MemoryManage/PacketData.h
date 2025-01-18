#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include "MemoryManage/CountableResource.h"

#include <Windows.h>

class PacketData: public CountableResource<PacketData>
{
public:
	PacketData(const AVPacket* packet);
	PacketData(size_t data_size);

	~PacketData();

	BYTE* getMemPointer();

	size_t getMemSize();
private:
	BYTE* data_ptr;
	size_t data_size;
};