#pragma once

#include "ScreenCapture/ScreenDataBuffer.h"
#include <thread>

class PeriodicDataCollector
{
public:
	PeriodicDataCollector(ScreenDataBuffer& input_buf, ScreenDataBuffer& output_buf, unsigned int collect_rate = DEFALUT_FRAME_RATE);

	void StartCollect();

	void EndCollect();

	ScreenDataBuffer& getOutputBuf();
private:
	ScreenDataBuffer& input_buf;
	ScreenDataBuffer& output_buf;

	std::thread clt_thread;

	unsigned int collect_rate;

	bool collect_continue = false;

	void CollectFunc();
};