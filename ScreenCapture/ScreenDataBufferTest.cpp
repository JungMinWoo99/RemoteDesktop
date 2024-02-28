#include "ScreenCapture/ScreenCapture.h"
#include "ScreenCapture/ScreenDataBuffer.h"
#include "Constant/VideoConstants.h"
#include "ScreenPrinter/WinScreenPrinter.h"

#include <thread>
#include <vector>

#define TEST_NUM 1

int main(void)
{
	int w, h, frame_per_sec;
	w = DEFALUT_WIDTH;
	h = DEFALUT_HEIGHT;
	frame_per_sec = DEFALUT_FRAME_RATE;

	ScreenCapture capture_obj(w, h);
	ScreenDataBuffer screen_buf(60);


	bool stop = false;

	auto capture_screen = [&capture_obj, &screen_buf, &stop, frame_per_sec]()
	{
		//auto until = std::chrono::high_resolution_clock::now()+std::chrono::microseconds(1000000 / frame_per_sec);
		while (!stop)
		{
			//std::this_thread::sleep_until(until);
			//until = until+ std::chrono::microseconds(1000000 / frame_per_sec);
			auto frame = capture_obj.CaptureCurrentScreen();
			screen_buf.RecvFrameData(frame);
		}
	};

	std::thread screen_cap(capture_screen);

	std::shared_ptr<VideoFrameData> buf;
	WinScreenPrinter screen_printer(w, h, capture_obj.getBMI(), buf);
	
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
		std::vector<std::shared_ptr<VideoFrameData>> frame_vector;
		for (int i = 0; i < frame_num; i++)
		{
			while(!screen_buf.SendFrameData(buf));
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
	stop = true;
	screen_cap.join();

	return 0;
}
