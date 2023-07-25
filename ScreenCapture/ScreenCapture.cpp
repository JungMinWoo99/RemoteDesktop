#include "ScreenCapture.hpp"
#include <iostream>

using namespace std;

auto print_error_code = []() {cout << "오류 코드: " << GetLastError() << endl; };

ScreenCapture::ScreenCapture(int pixel_width, int pixel_height)
	:pixel_width(pixel_width), pixel_height(pixel_height)
{
	screenDC = GetWindowDC(HWND_DESKTOP);
	if (!screenDC)
	{
		cout << "스크린 DC 생성 실패" << endl;
		print_error_code();
	}

	memDC = CreateCompatibleDC(screenDC);
	if (!memDC)
	{
		cout << "메모리 DC 생성 실패" << endl;
		print_error_code();
	}

	screenHBM = CreateCompatibleBitmap(screenDC, pixel_width, pixel_height);
	if (!screenHBM)
	{
		cout << "비트맵 핸들러 생성 실패" << endl;
		print_error_code();
	}

	SelectObject(memDC, screenHBM);

	BITMAP bmpInfo;
	if(!GetObject(screenHBM, sizeof(BITMAP), &bmpInfo))
	{
		cout << "비트맵 정보 생성 실패" << endl;
		print_error_code();
	}
	buf_bmi.bmiHeader.biWidth = bmpInfo.bmWidth;
	buf_bmi.bmiHeader.biHeight = bmpInfo.bmHeight; // 음수가 아닌 경우, 비트맵 이미지는 바텀-업(bottom-up) 방식입니다.
	buf_bmi.bmiHeader.biPlanes = 1;
	buf_bmi.bmiHeader.biBitCount = bmpInfo.bmPlanes * bmpInfo.bmBitsPixel;
	buf_bmi.bmiHeader.biCompression = BI_RGB;
	buf_bmi.bmiHeader.biSizeImage = 0; // 비트맵 이미지 크기가 알려지지 않은 경우 0으로 설정
	buf_bmi.bmiHeader.biXPelsPerMeter = 0;
	buf_bmi.bmiHeader.biYPelsPerMeter = 0;
	buf_bmi.bmiHeader.biClrUsed = 0;
	buf_bmi.bmiHeader.biClrImportant = 0;
	buf_bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

	color_bits = bmpInfo.bmBitsPixel;

	int buf_byte_size = pixel_width * pixel_height * color_bits / 8;
	pixel_data_buf = (BYTE*)malloc(buf_byte_size);
}

void ScreenCapture::CaptureCurrentScreen()
{
	if (!StretchBlt(memDC, 0, 0, pixel_width, pixel_height,
		screenDC, 0, 0, GetSystemMetrics(SM_CXFULLSCREEN), GetSystemMetrics(SM_CYFULLSCREEN),
		SRCCOPY))
	{
		cout << "StretchBlt 실패" << endl;
		print_error_code();
	}


	SetLastError(ERROR_SUCCESS);
	if(!GetDIBits(memDC,screenHBM,0,pixel_height,pixel_data_buf,&buf_bmi, DIB_RGB_COLORS))
	{
		cout << "GetDIBits 실패" << endl;
		print_error_code();
	}
}

int ScreenCapture::getWidth()
{
	return pixel_width;
}

int ScreenCapture::getHeight()
{
	return pixel_height;
}

const BITMAPINFO& ScreenCapture::getBMI()
{
	return buf_bmi;
}

const BYTE* ScreenCapture::getBuf()
{
	return pixel_data_buf;
}

ScreenCapture::~ScreenCapture()
{
	DeleteDC(screenDC);
	DeleteDC(memDC);
	DeleteObject(screenHBM);
	free(pixel_data_buf);
}