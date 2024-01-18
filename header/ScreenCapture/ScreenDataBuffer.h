#pragma once

#include "ScreenCapture/FrameData.h"
#include "Constant/VideoConstants.h"
#include "MutexQueue/MutexQueue.h"


class ScreenDataBuffer
{
public:

	ScreenDataBuffer(unsigned int buf_size);

	void RecvFrameData(std::shared_ptr<FrameData> frame);

	_Check_return_ bool SendFrameData(std::shared_ptr<FrameData>& recv);

	bool Empty();

	int Size();

	~ScreenDataBuffer();

private:
	std::mutex send_frame_mtx;

	MutexQueue<std::shared_ptr<FrameData>> mem_buf;
	unsigned int buf_size;
};