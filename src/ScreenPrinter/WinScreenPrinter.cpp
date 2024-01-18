#include "ScreenPrinter/WinScreenPrinter.h"

using namespace std;

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

WinScreenPrinter::WinScreenPrinter(int width, int height, const BITMAPINFO& bmi, const std::shared_ptr<FrameData>& frame_ref):ScreenPrinter(width,height),bmi(bmi), frame_ref(frame_ref)
{
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = GetModuleHandle(NULL);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszClassName = L"MyWindowClass";
	RegisterClassEx(&wc);

	_main = CreateWindowEx(0, L"MyWindowClass", L"My Window", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, w, h, NULL, NULL, GetModuleHandle(NULL), NULL);

	mainDC = GetDC(_main);

	is_printing = false;
}

void WinScreenPrinter::StartPrint()
{
	is_printing = true;
	ShowWindow(_main, SW_SHOWNORMAL);

	print_thread = thread(&WinScreenPrinter::PrintFunc, this);
}

void WinScreenPrinter::EndPrint()
{
	is_printing = false;
	print_thread.join();
	DestroyWindow(_main);
}

void WinScreenPrinter::PrintFrame(shared_ptr<FrameData> frame)
{
	SetDIBitsToDevice(mainDC, 0, 0, w, h, 0, 0, 0, h, frame.get()->getMemPointer(), &bmi, DIB_RGB_COLORS);
	UpdateWindow(_main);
}

void WinScreenPrinter::PrintFunc()
{
	std::shared_ptr<FrameData> prev_frame;
	while (is_printing)
	{
		if (frame_ref != prev_frame)
		{
			prev_frame = frame_ref;
			PrintFrame(prev_frame);
		}
	}
}
