#include "Constant/VideoConstants.h"
#include "ScreenCapture/Framedata.h"

int FrameData::remain = 0;

FrameData::FrameData(int alloc_size)
{
	remain++;
	mem = (BYTE*)malloc(alloc_size);
	mem_size = alloc_size;
	capture_time_ms = -1;
}

FrameData::FrameData(BYTE* mem_ptr, int alloc_size)
{
	remain++;
	mem = mem_ptr;
	mem_size = alloc_size;
	capture_time_ms = -1;
}

BYTE* FrameData::getMemPointer()
{
	return mem;
}

size_t FrameData::getMemSize()
{
	return mem_size;
}

void FrameData::setCaptureTime(int pts)
{
	this->capture_time_ms = pts;
}

int FrameData::getCaptureTime()
{
	return capture_time_ms;
}

int FrameData::getRemainFrame()
{
	return remain;
}

FrameData::~FrameData()
{
	remain--;

	free(mem);
}
