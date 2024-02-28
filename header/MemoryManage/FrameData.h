#pragma once

#include "ResourceMonitor/CountableResource.h"

#include <Windows.h>
#include <chrono>

class VideoFrameData: public CountableResource<VideoFrameData>
{
public:
	VideoFrameData(int alloc_size);

	VideoFrameData(BYTE* mem_ptr, int alloc_size);

	BYTE* getMemPointer();

	size_t getMemSize();

	void setCaptureTime(std::chrono::nanoseconds captured_time);

	std::chrono::nanoseconds getCaptureTime();

	void setPts(unsigned int pts);

	unsigned int getPts();

	~VideoFrameData();
private:	
	BYTE* mem;
	size_t mem_size;

	std::chrono::nanoseconds captured_time;//Elapsed Time Since Program Start
	unsigned int pts;
};

class AudioFrameData : public CountableResource<AudioFrameData>
{
public:
	AudioFrameData(int alloc_size);

	AudioFrameData(BYTE* mem_ptr, int alloc_size);

	BYTE* getMemPointer();

	size_t getMemSize();

	void setCaptureTime(std::chrono::nanoseconds captured_time);

	std::chrono::nanoseconds getCaptureTime();

	~AudioFrameData();
private:
	BYTE* mem;
	size_t mem_size;

	std::chrono::nanoseconds captured_time;//Elapsed Time Since Program Start
};