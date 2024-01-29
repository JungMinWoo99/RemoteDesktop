#include "MultiThreadFrameGetter/CaptureThread.h"
#include "ScreenPrinter/WinScreenPrinter.h"
#include "ScreenCapture/DirectXScreenCapture.h"
#include "ScreenCapture/WinScreenCapture.h"
#include <iostream>

#define TEST_NUM 0

int main(void)
{
	int w, h, frame_per_sec;
	w = DEFALUT_WIDTH;
	h = DEFALUT_HEIGHT;
	frame_per_sec = DEFALUT_FRAME_RATE;

	ScreenDataBuffer screen_buf(60,"");
	DirectXScreenCapture capture_obj;
	WinScreenCapture tem;
	CaptureThread capture_thread(screen_buf, &capture_obj);
	std::shared_ptr<FrameData> buf;
	WinScreenPrinter screen_printer(w, h, tem.getBMI(), buf);

	capture_thread.StartCapture();
	
	if (TEST_NUM == 0)
	{
		screen_printer.StartPrint();
		while (true)
		{
			if (!screen_buf.SendFrameData(buf))
				continue;
		}
		screen_printer.EndPrint();
	}
	else if (TEST_NUM == 1)
	{
		int frame_num = DEFALUT_FRAME_RATE * 10;
		std::vector<std::shared_ptr<FrameData>> frame_vector;
		for (int i = 0; i < frame_num; i++)
		{
			while (!screen_buf.SendFrameData(buf));
			frame_vector.push_back(buf);
		}

		screen_printer.StartPrint();
		auto until = std::chrono::high_resolution_clock::now() + std::chrono::microseconds(1000000 / frame_per_sec);
		for (int i = 0; i < frame_num; i++)
		{
			buf = frame_vector[i];
			std::this_thread::sleep_until(until);
			until = until + std::chrono::microseconds(1000000 / frame_per_sec);
		}
		screen_printer.EndPrint();
	}
	
	capture_thread.EndCapture();

	return 0;
}
