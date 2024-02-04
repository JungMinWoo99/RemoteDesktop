#pragma once

#include "ResourceMonitor/CountableResource.h"

#include <Windows.h>
#include <chrono>

class FrameData: public CountableResource<FrameData>
{
public:
	FrameData(int alloc_size);

	FrameData(BYTE* mem_ptr, int alloc_size);

	BYTE* getMemPointer();

	size_t getMemSize();

	void setCaptureTime(std::chrono::nanoseconds captured_time);

	std::chrono::nanoseconds getCaptureTime();

	void setPts(unsigned int pts);

	unsigned int getPts();

	~FrameData();
private:	
	BYTE* mem;
	size_t mem_size;

	std::chrono::nanoseconds captured_time;//Elapsed Time Since Program Start
	unsigned int pts;
};