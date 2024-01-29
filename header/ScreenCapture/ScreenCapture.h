#pragma once

#include <Windows.h>
#include <chrono>
#include <memory>
#include <mutex>

#include "ScreenCapture/FrameData.h"
#include "Constant/VideoConstants.h"

class ScreenCapture
{
public:

	ScreenCapture(int color_bits = BYTE_PER_PIXEL, int pixel_width = DEFALUT_WIDTH, int pixel_height = DEFALUT_HEIGHT);

	virtual std::shared_ptr<FrameData> CaptureCurrentScreen() = 0;

	int getWidth() const;

	int getHeight() const;

	int getFrameDataSize() const;

	virtual ~ScreenCapture();

protected:

	std::mutex cap_mtx;

	int pixel_width;
	int pixel_height;
	int color_bits;
	int buf_byte_size;

	int obj_id;

};