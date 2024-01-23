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

	size_t Size();

	~ScreenDataBuffer();

private:

	MutexQueue<std::shared_ptr<FrameData>> mem_buf;

	unsigned int max_buf_size;
};