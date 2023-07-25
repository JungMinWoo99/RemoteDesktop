#include <Windows.h>

class ScreenCapture
{
public:
	ScreenCapture(int pixel_width, int pixel_height);
	void CaptureCurrentScreen();
	int getWidth();
	int getHeight();
	const BITMAPINFO& getBMI();
	const BYTE* getBuf();
	~ScreenCapture();
private:

	HDC screenDC;
	HDC memDC;
	HBITMAP screenHBM;

	BITMAPINFO buf_bmi;
	BYTE* pixel_data_buf;

	int pixel_width;
	int pixel_height;
	int color_bits;
};