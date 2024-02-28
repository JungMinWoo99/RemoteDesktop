#include "Constant/VideoConstants.h"
#include "MemoryManage/Framedata.h"

VideoFrameData::VideoFrameData(int alloc_size)
	: captured_time()
{
	mem = (BYTE*)malloc(alloc_size);
	mem_size = alloc_size;
	pts = 0;
}

VideoFrameData::VideoFrameData(BYTE* mem_ptr, int alloc_size)
	: captured_time()
{
	mem = mem_ptr;
	mem_size = alloc_size;
	pts = 0;
}

BYTE* VideoFrameData::getMemPointer()
{
	return mem;
}

size_t VideoFrameData::getMemSize()
{
	return mem_size;
}

void VideoFrameData::setCaptureTime(std::chrono::nanoseconds captured_time)
{
	this->captured_time = captured_time;
}

std::chrono::nanoseconds VideoFrameData::getCaptureTime()
{
	return captured_time;
}

void VideoFrameData::setPts(unsigned int pts)
{
	this->pts = pts;
}

unsigned int VideoFrameData::getPts()
{
	return pts;
}

VideoFrameData::~VideoFrameData()
{
	free(mem);
}

AudioFrameData::AudioFrameData(int alloc_size)
{
	mem = (BYTE*)malloc(alloc_size);
	mem_size = alloc_size;
}

AudioFrameData::AudioFrameData(BYTE* mem_ptr, int alloc_size)
{
	mem = mem_ptr;
	mem_size = alloc_size;
}

BYTE* AudioFrameData::getMemPointer()
{
	return mem;
}

size_t AudioFrameData::getMemSize()
{
	return mem_size;
}

void AudioFrameData::setCaptureTime(std::chrono::nanoseconds captured_time)
{
	this->captured_time = captured_time;
}

std::chrono::nanoseconds AudioFrameData::getCaptureTime()
{
	return std::chrono::nanoseconds();
}

AudioFrameData::~AudioFrameData()
{
	free(mem);
}
