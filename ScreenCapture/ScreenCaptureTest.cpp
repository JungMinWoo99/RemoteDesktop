#include "ScreenCapture.hpp"

/*
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		// 여기에 윈도우의 내용을 그리는 코드를 추가합니다.

		EndPaint(hwnd, &ps);
		return 0;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

*/
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
		CW_USEDEFAULT, CW_USEDEFAULT, 1920, 1080, NULL, NULL, GetModuleHandle(NULL), NULL);
	HDC mainDC = GetDC(_main);
	ShowWindow(_main, SW_SHOWNORMAL);
	ScreenCapture capture_obj(1920, 1080);

	capture_obj.CaptureCurrentScreen();
	SetDIBitsToDevice(mainDC, 0, 0, 1920, 1080, 0, 0, 0, 1080, capture_obj.getBuf(), &capture_obj.getBMI(), DIB_RGB_COLORS);
	UpdateWindow(_main);
	while (true);
	return 0;
}
