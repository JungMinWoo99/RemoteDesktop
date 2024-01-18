#pragma once

#include "ScreenToMat.h"
#include "ScreenDataBuffer.h"
#include "VideoConstants.h"

class MultiThreadProcess
{
public:
	MultiThreadProcess(ScreenDataBuffer& input_buf, ScreenDataBuffer& output_buf, const BITMAPINFO& bmi, unsigned int thread_num = DEFALUT_FRAME_RATE / SEC_PER_PRC_FRAME);

	void StartProcess();

	void EndProcess();

	~MultiThreadProcess();

private:
	ScreenDataBuffer& input_buf;
	ScreenDataBuffer& output_buf;

	std::vector<ScreenToMat*> prc_obj_vector;

	unsigned int thread_num;

	bool process_continue = false;
	
	std::thread prc_thread;

	void ProcessFunc();
};