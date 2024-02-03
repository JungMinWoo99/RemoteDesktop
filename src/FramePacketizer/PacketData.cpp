#include "FramePacketizer/PacketData.h"

PacketData::PacketData(const AVPacket* packet)
	:data_size(packet->size)
{
	data_ptr = (BYTE*)malloc(data_size);
	memcpy(data_ptr, packet->data, data_size);
}

PacketData::PacketData(size_t data_size)
	: data_size(data_size)
{
	data_ptr = (BYTE*)malloc(data_size);
}

PacketData::~PacketData()
{
	free(data_ptr);
}

BYTE* PacketData::getMemPointer()
{
	return data_ptr;
}

size_t PacketData::getMemSize()
{
	return data_size;
}
