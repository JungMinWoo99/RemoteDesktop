#include "MultiThreadCapture.h"
#include "PeriodicDataCollector.h"
#include "MultiThreadProcess.h"

#include <iostream>

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

int main(void)
{
	int w, h, frame_per_sec;
	w = DEFALUT_WIDTH;
	h = DEFALUT_HEIGHT;
	frame_per_sec = DEFALUT_FRAME_RATE;

	// 윈도우 클래스 등록
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = GetModuleHandle(NULL);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszClassName = L"MyWindowClass";
	RegisterClassEx(&wc);

	HWND _main = CreateWindowEx(0, L"MyWindowClass", L"My Window", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, w, h, NULL, NULL, GetModuleHandle(NULL), NULL);

	HDC mainDC = GetDC(_main);
	ShowWindow(_main, SW_SHOWNORMAL);

	ScreenDataBuffer screen_buf;
	ScreenDataBuffer periodic_buf;
	ScreenDataBuffer prced_data_buf(600);
	MultiThreadCapture capture_obj(screen_buf);
	PeriodicDataCollector clt_obj(screen_buf, periodic_buf);
	MultiThreadProcess prc_obj(periodic_buf, prced_data_buf, capture_obj.getCapInfo().getBMI());

	capture_obj.StartCapture();
	clt_obj.StartCollect();
	prc_obj.StartProcess();

	auto cap_time = std::chrono::high_resolution_clock::now() + std::chrono::seconds(10);
	std::this_thread::sleep_until(cap_time);

	prc_obj.EndProcess();
	clt_obj.EndCollect();
	capture_obj.EndCapture();

	std::cout << prced_data_buf.Size() << std::endl;

	std::shared_ptr<FrameData> frame;
	auto until = std::chrono::high_resolution_clock::now() + std::chrono::microseconds(1000000 / frame_per_sec);
	int prev_pts = 0;
	while (!prced_data_buf.Empty())
	{
		prced_data_buf.SendFrameData(frame);
		std::this_thread::sleep_until(until);
		until = until + std::chrono::microseconds(1000000 / frame_per_sec);
		SetDIBitsToDevice(mainDC, 0, 0, w, h, 0, 0, 0, h, frame.get()->getMemPointer(), &capture_obj.getCapInfo().getBMI(), DIB_RGB_COLORS);
		UpdateWindow(_main);
		std::cout << frame.get()->getPts() - prev_pts - 16667 << std::endl;
		prev_pts = frame.get()->getPts();
	}

	return 0;
}
