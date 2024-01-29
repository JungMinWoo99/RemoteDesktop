#include "ScreenCapture/DirectXScreenCapture.h"
#include "ScreenCapture/WinScreenCapture.h"
#include "Constant/VideoConstants.h"
#include <thread>

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
	WinScreenCapture capture_obj;
	DirectXScreenCapture capture_obj2;

	while (true)
	{
		//std::this_thread::sleep_for(std::chrono::microseconds(1000000 / frame_per_sec));
		auto buf = capture_obj2.CaptureCurrentScreen();
		SetDIBitsToDevice(mainDC, 0, 0, w, h, 0, 0, 0, h, buf.get()->getMemPointer(), &capture_obj.getBMI(), DIB_RGB_COLORS);
		UpdateWindow(_main);
	}

	return 0;
}
