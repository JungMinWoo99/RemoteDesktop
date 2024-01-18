#include <memory>
#include "ScreenCapture/FrameData.h"

class ScreenPrinter abstract
{
public:
	virtual void StartPrint() abstract;
	virtual void EndPrint() abstract;

protected:
	ScreenPrinter(int w, int h) :w(w), h(h) {};
	virtual void PrintFrame(std::shared_ptr<FrameData> frame) abstract;
	int w, h;
};