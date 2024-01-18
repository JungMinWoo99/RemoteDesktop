#pragma once

#include <thread>

#include "ScreenCapture/ScreenCapture.h"
#include "ScreenCapture/ScreenDataBuffer.h"

class CaptureThread 
{
public:
	CaptureThread(ScreenDataBuffer& data_buf);

	void StartCapture();

	void EndCapture();

	const ScreenCapture& getCapInfo();

	ScreenDataBuffer& getDataBuf();

private:
	ScreenCapture capture_obj;

	ScreenDataBuffer& data_buf;

	bool capture_continue = false;

	std::thread cap_thread;

	void CaptureFunc();
};