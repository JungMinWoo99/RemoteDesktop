#pragma once

#include <string>

#include "MemoryManage/Framedata.h"
#include "Constant/VideoConstants.h"
#include "MutexQueue/MutexQueue.h"


class ScreenDataBuffer
{
public:

	ScreenDataBuffer(unsigned int buf_size, std::string buf_name);

	void RecvFrameData(std::shared_ptr<VideoFrameData> frame);

	_Check_return_ bool SendFrameData(std::shared_ptr<VideoFrameData>& recv);

	_Check_return_ bool SendFrameDataBlocking(std::shared_ptr<VideoFrameData>& recv);

	size_t Size();

	~ScreenDataBuffer();

private:

	MutexQueue<std::shared_ptr<VideoFrameData>> mem_buf;

	unsigned int max_buf_size;
};