#pragma deprecated("This header is deprecated, and its use is discouraged.")

#pragma once

#include <Windows.h>

#include "ScreenCapture/ScreenCapture.h"

class WinScreenCapture : public ScreenCapture
{
public:

	WinScreenCapture();

	std::shared_ptr<FrameData> CaptureCurrentScreen() override;

	const BITMAPINFO& getBMI() const;

	~WinScreenCapture() override;

private:

	HDC screenDC;
	HDC memDC;
	HBITMAP screenHBM;

	BITMAPINFO buf_bmi;

	static BOOL CALLBACK MonitorProc(HMONITOR hMon, HDC hDC, LPRECT pRect, LPARAM LParam);

	void PrintError();
};