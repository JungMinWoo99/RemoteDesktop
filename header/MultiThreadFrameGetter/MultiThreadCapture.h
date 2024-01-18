#pragma once

#include <chrono>
#include <thread>
#include <vector>

#include "ScreenCapture/ScreenCapture.h"
#include "ScreenCapture/ScreenDataBuffer.h"
#include "Constant/VideoConstants.h"



class MultiThreadCapture
{
public:

	MultiThreadCapture(ScreenDataBuffer& data_buf, unsigned int thread_num = DEFALUT_FRAME_RATE * TIME_STEP_DIVISION / SEC_PER_CAP_FRAME);

	void StartCapture();

	void EndCapture();

	const ScreenCapture& getCapInfo();

	ScreenDataBuffer& getDataBuf();

	~MultiThreadCapture();

private:
	std::vector< ScreenCapture*> cap_obj_vector;

	ScreenDataBuffer& data_buf;

	unsigned int thread_num;

	bool capture_continue = false;

	std::chrono::high_resolution_clock::time_point next_cap_start;

	std::thread cap_thread;

	void CaptureFunc();
};