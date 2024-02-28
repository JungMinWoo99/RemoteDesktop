#include "ScreenCapture/ScreenDataBuffer.h"

using namespace std;

ScreenDataBuffer::ScreenDataBuffer(unsigned int buf_size, string buf_name)
	: max_buf_size(buf_size), mem_buf(buf_name.c_str())
{
}

void ScreenDataBuffer::RecvFrameData(std::shared_ptr<VideoFrameData> frame)
{
	mem_buf.push(frame);

	while (mem_buf.size() > max_buf_size && mem_buf.pop(frame));
}

_Check_return_ bool ScreenDataBuffer::SendFrameData(std::shared_ptr<VideoFrameData>& recv)
{
	return mem_buf.pop(recv);
}

_Check_return_ bool ScreenDataBuffer::SendFrameDataBlocking(std::shared_ptr<VideoFrameData>& recv)
{
	return mem_buf.wait_and_pop_utill_not_empty(recv);
}

size_t ScreenDataBuffer::Size()
{
	return mem_buf.size();
}

ScreenDataBuffer::~ScreenDataBuffer()
{
}