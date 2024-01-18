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
	void PushFrameData(std::shared_ptr<FrameData> input);

	void PopFrameData(std::shared_ptr<FrameData>& recv);
private:
	std::mutex buf_mtx;
	std::mutex pop_frame_mtx;
	std::priority_queue<std::shared_ptr<FrameData>> buf;

	//int max_buf_size = DEFALUT_FRAME_RATE * FRAME_TIME_OUT;
	int max_buf_size = DEFALUT_FRAME_RATE * 30;
};