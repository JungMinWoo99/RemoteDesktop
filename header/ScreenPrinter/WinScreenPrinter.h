#include <memory>
#include <thread>

#include "ScreenPrinter/ScreenPrinter.h"
#include "MemoryManage/Framedata.h"

class WinScreenPrinter : ScreenPrinter
{
public:
	WinScreenPrinter(int width, int height, const BITMAPINFO& bmi, const std::shared_ptr<VideoFrameData>& frame_ref);
	void StartPrint() override;
	void EndPrint() override;

private:
	void PrintFrame(std::shared_ptr<VideoFrameData> frame) override;

	void PrintFunc();
	std::thread print_thread;

	const BITMAPINFO& bmi;
	WNDCLASSEX wc = {};
	HWND _main;
	HDC mainDC;

	const std::shared_ptr<VideoFrameData>& frame_ref;
	bool is_printing;
};