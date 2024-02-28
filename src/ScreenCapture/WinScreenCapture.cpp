#include "ScreenCapture/WinScreenCapture.h"
#include <iostream>
#include <chrono>

using namespace std;

WinScreenCapture::WinScreenCapture()
	:ScreenCapture()
{
	EnumDisplayMonitors(NULL, NULL, MonitorProc, (LPARAM)&screenDC);
	if (!screenDC)
	{
		cout << "EnumDisplayMonitors fail" << endl;
		PrintError();
		exit(-1);
	}

	memDC = CreateCompatibleDC(screenDC);
	if (!memDC)
	{
		cout << "CreateCompatibleDC fail" << endl;
		PrintError();
		exit(-1);
	}

	screenHBM = CreateCompatibleBitmap(screenDC, pixel_width, pixel_height);
	if (!screenHBM)
	{
		cout << "CreateCompatibleBitmap fail" << endl;
		PrintError();
		exit(-1);
	}

	SelectObject(memDC, screenHBM);

	BITMAP bmpInfo;
	if(!GetObject(screenHBM, sizeof(BITMAP), &bmpInfo))
	{
		cout << "GetObject fail" << endl;
		PrintError();
		exit(-1);
	}

	buf_bmi.bmiHeader.biWidth = bmpInfo.bmWidth;
	buf_bmi.bmiHeader.biHeight = bmpInfo.bmHeight;
	buf_bmi.bmiHeader.biPlanes = 1;
	buf_bmi.bmiHeader.biBitCount = bmpInfo.bmPlanes * bmpInfo.bmBitsPixel;
	buf_bmi.bmiHeader.biCompression = BI_RGB;
	buf_bmi.bmiHeader.biSizeImage = 0;
	buf_bmi.bmiHeader.biXPelsPerMeter = 0;
	buf_bmi.bmiHeader.biYPelsPerMeter = 0;
	buf_bmi.bmiHeader.biClrUsed = 0;
	buf_bmi.bmiHeader.biClrImportant = 0;
	buf_bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

	pixel_width = bmpInfo.bmWidth;
	pixel_height = bmpInfo.bmHeight;
	color_bits = bmpInfo.bmBitsPixel;

	buf_byte_size = pixel_width * pixel_height * color_bits / 8;
}

shared_ptr<VideoFrameData> WinScreenCapture::CaptureCurrentScreen()
{
	static chrono::steady_clock::time_point start_tp = chrono::steady_clock::now();

	shared_ptr<VideoFrameData> pixel_data_buf = make_shared<VideoFrameData>(this->buf_byte_size);

	if (!cap_mtx.try_lock())
	{
		cout << "WinScreenCapture " << obj_id << " : too busy" << endl;
		cap_mtx.lock();
	}

	if (!BitBlt(memDC, 0, 0, pixel_width, pixel_height,
		screenDC, 0, 0, SRCCOPY))
	{
		cout << "BitBlt fail" << endl;
		PrintError();
	}
	chrono::steady_clock::time_point cap_tp = chrono::steady_clock::now();

	chrono::nanoseconds duration = cap_tp - start_tp;

	SetLastError(ERROR_SUCCESS);
	if(!GetDIBits(memDC,screenHBM,0,pixel_height,pixel_data_buf.get()->getMemPointer(), &buf_bmi, DIB_RGB_COLORS))
	{
		cout << "GetDIBits fail" << endl;
		PrintError();
	}
	pixel_data_buf.get()->setCaptureTime(duration);

	cap_mtx.unlock();

	return pixel_data_buf;
}

const BITMAPINFO& WinScreenCapture::getBMI() const
{
	return buf_bmi;
}

WinScreenCapture::~WinScreenCapture()
{
	DeleteDC(screenDC);
	DeleteDC(memDC);
	DeleteObject(screenHBM);
}

BOOL CALLBACK WinScreenCapture::MonitorProc(HMONITOR hMon, HDC hDC, LPRECT pRect, LPARAM LParam)
{
	HDC* screen = (HDC*)LParam;

	MONITORINFOEX monitorInfo;
	monitorInfo.cbSize = sizeof(MONITORINFOEX);
	GetMonitorInfo(hMon, &monitorInfo);

	if (monitorInfo.dwFlags != MONITORINFOF_PRIMARY)
		*screen = CreateDC(monitorInfo.szDevice, NULL, NULL, NULL);
	return true;
}


void WinScreenCapture::PrintError()
{
	cout << "error_code: " << GetLastError() << endl;
}
