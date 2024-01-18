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

	ScreenCapture(int pixel_width = DEFALUT_WIDTH, int pixel_height = DEFALUT_HEIGHT);

	std::shared_ptr<FrameData> CaptureCurrentScreen();

	int getWidth() const;

	int getHeight() const;

	int getFrameDataSize() const;

	const BITMAPINFO& getBMI() const;

	~ScreenCapture();

private:

	HDC screenDC;
	HDC memDC;
	HBITMAP screenHBM;

	BITMAPINFO buf_bmi;

	std::mutex mtx;

	int pixel_width;
	int pixel_height;
	int color_bits;
	int buf_byte_size;

	int obj_id;

	static BOOL CALLBACK MonitorProc(HMONITOR hMon, HDC hDC, LPRECT pRect, LPARAM LParam);
};