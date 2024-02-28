#include "Live555RtspServer/RtspServerContext.h"
#include "MultiThreadFrameGetter/CaptureThread.h"
#include "MultiThreadFrameGetter/PeriodicDataCollector.h"
#include "MemoryManage/PixFmtConverter.h"
#include "FramePacketizer/FrameEncoder.h"
#include "FramePacketizer/AVFrameManage.h"
#include "FramePacketizer/CoderThread/EncoderThread.h"
#include "TestQueueMonitorThread.h"
#include "ScreenCapture/DirectXScreenCapture.h"


#include <iostream>

using namespace std;

int main(void)
{
	TestQueueMonitorThread monitor_thread(std::chrono::seconds(10));
	monitor_thread.start();

	int w, h;
	w = DEFALUT_WIDTH;
	h = DEFALUT_HEIGHT;

	DirectXScreenCapture* cap_obj = new DirectXScreenCapture();

	//Rtsp 서버 설정
	RtspServerContext rtsp_serv;

	//프레임 버퍼 설정
	ScreenDataBuffer screen_buf(5, "Screen Frame Source Buffer");
	ScreenDataBuffer periodic_buf(10, "Screen Frame Collect Buffer");
	CaptureThread capture_obj(screen_buf, cap_obj);
	PeriodicDataCollector clt_obj(screen_buf, periodic_buf);
	FrameEncoder encoding_obj;
	PixFmtConverter pix_fmt_cvt;

	//video stream open
	EncodedPacketFramedSource::FramedSourcePacketHandler& prc_obj = rtsp_serv.GetPacketHandler();
	AVPacketHandlerThread pkt_thr(encoding_obj, prc_obj);
	FrameEncoderThread enc_thr(encoding_obj);

	//스트리밍 시작
	capture_obj.StartCapture();
	clt_obj.StartCollect();
	enc_thr.StartEncoding();
	pkt_thr.StartHandle();

	auto serv_run = [&]() {rtsp_serv.Run(); };
	thread serv_thr(serv_run);

	std::shared_ptr<VideoFrameData> frame;

	bool is_stream = true;
	while (is_stream)
	{
		while (!periodic_buf.SendFrameData(frame));
		auto yuv_data = pix_fmt_cvt.ConvertBGRToYUV(frame);

		shared_ptr<SharedAVFrame> frame_obj = AVStructPool<AVFrame*>::getInstance().getEmptyObj();
		AVFrame* av_frame = frame_obj.get()->getPointer();
		if (!AllocAVFrameBuf(av_frame, encoding_obj.getEncCodecContext()))
		{
			cout << "AllocAVFrameBuf fail" << endl;
			return -1;
		}

		CopyRawToAVFrame(yuv_data, av_frame);
		enc_thr.InputFrame(frame_obj);
	}


	serv_thr.join();
	pkt_thr.EndHandle();
	enc_thr.EndEncoding();
	clt_obj.EndCollect();
	capture_obj.EndCapture();

	monitor_thread.stop();

	return 0;
}