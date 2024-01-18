#pragma once

#include <Windows.h>

class FrameData
{
public:
	static int getRemainFrame();

	FrameData(int alloc_size);

	FrameData(BYTE* mem_ptr, int alloc_size);

	BYTE* getMemPointer();

	size_t getMemSize();

	void setCaptureTime(int pts);

	int getCaptureTime();

	~FrameData();
private:
	static int remain;
	
	BYTE* mem;
	size_t mem_size;

	int capture_time_ms;
};