#include "ScreenCapture/ScreenCapture.h"
#include <iostream>
#include <chrono>

using namespace std;

auto print_error_code = []() {cout << "error_code: " << GetLastError() << endl; };

ScreenCapture::ScreenCapture(int pixel_width, int pixel_height)
	:pixel_width(pixel_width), pixel_height(pixel_height)
{
	static int obj_id_count = 0;

	obj_id = obj_id_count++;

	EnumDisplayMonitors(NULL, NULL, MonitorProc, (LPARAM)&screenDC);
	if (!screenDC)
	{
		cout << "EnumDisplayMonitors fail" << endl;
		print_error_code();
		exit(-1);
	}

	memDC = CreateCompatibleDC(screenDC);
	if (!memDC)
	{
		cout << "CreateCompatibleDC fail" << endl;
		print_error_code();
		exit(-1);
	}

	screenHBM = CreateCompatibleBitmap(screenDC, pixel_width, pixel_height);
	if (!screenHBM)
	{
		cout << "CreateCompatibleBitmap fail" << endl;
		print_error_code();
		exit(-1);
	}

	SelectObject(memDC, screenHBM);

	BITMAP bmpInfo;
	if(!GetObject(screenHBM, sizeof(BITMAP), &bmpInfo))
	{
		cout << "GetObject fail" << endl;
		print_error_code();
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

	color_bits = bmpInfo.bmBitsPixel;

	buf_byte_size = pixel_width * pixel_height * color_bits / 8;
}

shared_ptr<FrameData> ScreenCapture::CaptureCurrentScreen()
{
	static chrono::high_resolution_clock::time_point start_tp = chrono::high_resolution_clock::now();

	shared_ptr<FrameData> pixel_data_buf = make_shared<FrameData>(this->buf_byte_size);

	if (!mtx.try_lock())
	{
		cout << "ScreenCapture " << obj_id << " : too busy" << endl;
		mtx.lock();
	}

	chrono::high_resolution_clock::time_point cap_tp = chrono::high_resolution_clock::now();
	if (!BitBlt(memDC, 0, 0, pixel_width, pixel_height,
		screenDC, 0, 0, SRCCOPY))
	{
		cout << "BitBlt fail" << endl;
		print_error_code();
	}

	auto duration = chrono::duration_cast<chrono::microseconds>(cap_tp - start_tp);

	SetLastError(ERROR_SUCCESS);
	if(!GetDIBits(memDC,screenHBM,0,pixel_height,pixel_data_buf.get()->getMemPointer(), &buf_bmi, DIB_RGB_COLORS))
	{
		cout << "GetDIBits fail" << endl;
		print_error_code();
	}
	pixel_data_buf.get()->setCaptureTime(duration.count());

	mtx.unlock();

	return pixel_data_buf;
}

int ScreenCapture::getWidth() const
{
	return pixel_width;
}

int ScreenCapture::getHeight() const
{
	return pixel_height;
}

int ScreenCapture::getFrameDataSize() const
{
	return buf_byte_size;
}

const BITMAPINFO& ScreenCapture::getBMI() const
{
	return buf_bmi;
}

ScreenCapture::~ScreenCapture()
{
	DeleteDC(screenDC);
	DeleteDC(memDC);
	DeleteObject(screenHBM);
}

BOOL CALLBACK ScreenCapture::MonitorProc(HMONITOR hMon, HDC hDC, LPRECT pRect, LPARAM LParam)
{
	HDC* screen = (HDC*)LParam;

	MONITORINFOEX monitorInfo;
	monitorInfo.cbSize = sizeof(MONITORINFOEX);
	GetMonitorInfo(hMon, &monitorInfo);

	if (monitorInfo.dwFlags != MONITORINFOF_PRIMARY)
		*screen = CreateDC(monitorInfo.szDevice, NULL, NULL, NULL);
	return true;
}