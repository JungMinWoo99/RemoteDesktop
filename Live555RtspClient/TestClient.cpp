#include "Live555RtspClient/RtspClinetContext.h"
#include "MultiThreadFrameGetter/CaptureThread.h"
#include "MultiThreadFrameGetter/PeriodicDataCollector.h"
#include "MemoryManage/PixFmtConverter.h"
#include "FramePacketizer/FrameDecoder.h"
#include "MemoryManage/AVFrameManage.h"
#include "FramePacketizer/CoderThread/DecoderThread.h"
#include "ScreenPrinter/WinScreenPrinter.h"
#include "ScreenCapture/WinScreenCapture.h"

#include <iostream>

using namespace std;

class RTPFrameProcessor : public AVFrameProcessor
{
public:
	RTPFrameProcessor(const BITMAPINFO& bmi)
		:s_printer(DEFALUT_WIDTH, DEFALUT_HEIGHT, bmi, frame_ref)
	{
		yuv_frame_data = make_shared<VideoFrameData>(DEFALUT_WIDTH * DEFALUT_HEIGHT * 4 / 2 * 3 / 4);
		s_printer.StartPrint();
	}

	~RTPFrameProcessor()
	{
	}

	void FrameProcess(AVFrame* frame) override
	{
		//std::this_thread::sleep_for(std::chrono::microseconds(1000000 / DEFALUT_FRAME_RATE));
		CopyAVFrameToRaw(frame, yuv_frame_data);
		frame_ref = cnv.ConvertYUVToBGR(yuv_frame_data);
	}

	void EndEncoding()
	{
		s_printer.EndPrint();
	}
private:
	WinScreenPrinter s_printer;
	shared_ptr<VideoFrameData> yuv_frame_data;
	ImgFmtConverter cnv;
	shared_ptr<VideoFrameData> frame_ref;
};


int main() {
	//디코드 및 출력
	
	WinScreenCapture capture_obj;
	FrameDecoder decoding_obj;
	RTPFrameProcessor prc_obj(capture_obj.getBMI());
	AVFrameHandlerThread frm_thr(decoding_obj, prc_obj);
	PacketDecoderThread dec_thr(decoding_obj);
	RtspClinetContext clnt_obj(dec_thr);

	clnt_obj.openURL("RemoteDesktop", "rtsp://59.17.74.89/stream1");
	

	//Decoding thread set
	dec_thr.StartDecoding();
	frm_thr.StartHandle();

	clnt_obj.Run();
	
	frm_thr.EndHandle();
	dec_thr.EndDecoding();

	return 0;
}