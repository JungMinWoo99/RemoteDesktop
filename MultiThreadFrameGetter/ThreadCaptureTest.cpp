#include "MultiThreadFrameGetter/CaptureThread.h"
#include "ScreenPrinter/WinScreenPrinter.h"

#include <iostream>

#define TEST_NUM 0

int main(void)
{
	int w, h, frame_per_sec;
	w = DEFALUT_WIDTH;
	h = DEFALUT_HEIGHT;
	frame_per_sec = DEFALUT_FRAME_RATE;

	ScreenDataBuffer screen_buf(60);
	//MultiThreadCapture capture_obj(screen_buf);
	CaptureThread capture_obj2(screen_buf);
	std::shared_ptr<FrameData> buf;
	WinScreenPrinter screen_printer(w, h, capture_obj2.getCapInfo().getBMI(), buf);

	capture_obj2.StartCapture();
	
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
	
	capture_obj2.EndCapture();

	return 0;
}
