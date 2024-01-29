#include "MultiThreadFrameGetter/CaptureThread.h"

using namespace std;

CaptureThread::CaptureThread(ScreenDataBuffer& data_buf, ScreenCapture* capture_obj):
	data_buf(data_buf), capture_obj(capture_obj)
{
}

void CaptureThread::StartCapture()
{
	capture_continue = true;
	cap_thread = thread(&CaptureThread::CaptureFunc, this);
}

void CaptureThread::EndCapture()
{
	capture_continue = false;
	cap_thread.join();
}

const ScreenCapture* CaptureThread::getCapInfo()
{
	return capture_obj;
}

ScreenDataBuffer& CaptureThread::getDataBuf()
{
	return data_buf;
}

void CaptureThread::CaptureFunc()
{
	while (capture_continue)
	{
		auto frame = capture_obj->CaptureCurrentScreen();
		data_buf.RecvFrameData(frame);
	}
}


