#include "Constant/VideoConstants.h"
#include "ScreenCapture/Framedata.h"

FrameData::FrameData(int alloc_size)
	: captured_time()
{
	mem = (BYTE*)malloc(alloc_size);
	mem_size = alloc_size;
	pts = 0;
}

FrameData::FrameData(BYTE* mem_ptr, int alloc_size)
	: captured_time()
{
	mem = mem_ptr;
	mem_size = alloc_size;
	pts = 0;
}

BYTE* FrameData::getMemPointer()
{
	return mem;
}

size_t FrameData::getMemSize()
{
	return mem_size;
}

void FrameData::setCaptureTime(std::chrono::nanoseconds captured_time)
{
	this->captured_time = captured_time;
}

std::chrono::nanoseconds FrameData::getCaptureTime()
{
	return captured_time;
}

void FrameData::setPts(unsigned int pts)
{
	this->pts = pts;
}

unsigned int FrameData::getPts()
{
	return pts;
}

FrameData::~FrameData()
{
	free(mem);
}
