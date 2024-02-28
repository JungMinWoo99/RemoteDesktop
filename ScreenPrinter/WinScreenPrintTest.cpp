#include "ScreenCapture/WinScreenCapture.h"
#include "Constant/VideoConstants.h"
#include "ScreenPrinter/WinScreenPrinter.h"

int main(void)
{
	int w, h;
	w = DEFALUT_WIDTH;
	h = DEFALUT_HEIGHT;
	std::shared_ptr<VideoFrameData> current_frame;
	WinScreenCapture capture_obj;
	WinScreenPrinter screen_print(w, h, capture_obj.getBMI(), current_frame);

	bool is_cap = true;
	auto cap_func = [&]() {
		while (is_cap)
		{
			current_frame = capture_obj.CaptureCurrentScreen();
		}
		};
	std::thread cap_thread(cap_func);
	screen_print.StartPrint();
	Sleep(30000);
	screen_print.EndPrint();
	is_cap = false;
	cap_thread.join();

	return 0;
}
