#pragma once

#include <queue>
#include <mutex>
#include <memory>

#include "ScreenCapture/FrameData.h"
#include "Constant/VideoConstants.h"

bool operator<(std::shared_ptr<FrameData> t1, std::shared_ptr<FrameData> t2);

class ScreenDataPriorityQueue
{
public:
	ScreenDataPriorityQueue(unsigned int max_buf_size);

	void PushFrameData(std::shared_ptr<FrameData> input);

	bool PopFrameData(std::shared_ptr<FrameData>& recv);
private:
	std::mutex buf_mtx;

	std::priority_queue<std::shared_ptr<FrameData>> buf;

	int max_buf_size;
};