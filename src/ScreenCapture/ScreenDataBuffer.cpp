#include "ScreenCapture/ScreenDataBuffer.h"

using namespace std;

ScreenDataBuffer::ScreenDataBuffer(unsigned int buf_size)
	: buf_size(buf_size)
{
}

void ScreenDataBuffer::RecvFrameData(std::shared_ptr<FrameData> frame)
{
	mem_buf.push(frame);

	while (mem_buf.size() > buf_size)
		mem_buf.pop();
}

_Check_return_ bool ScreenDataBuffer::SendFrameData(std::shared_ptr<FrameData>& recv)
{
	return mem_buf.pop(recv);
}

bool ScreenDataBuffer::Empty()
{
	return mem_buf.empty();
}

int ScreenDataBuffer::Size()
{
	return mem_buf.size();
}

ScreenDataBuffer::~ScreenDataBuffer()
{
}