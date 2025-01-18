#include "MultiThreadFrameGetter/CaptureThread.h"
#include "MultiThreadFrameGetter/PeriodicDataCollector.h"
#include "MemoryManage/PixFmtConverter.h"
#include "VideoPacketizer/VideoEncoder.h"
#include "AudioPacketizer/AudioEncoder.h"
#include "MemoryManage/AVFrameManage.h"
#include "FramePacketizer/CoderThread/EncoderThread.h"
#include "FramePacketizer/CoderThread/DecoderThread.h"
#include "TestQueueMonitorThread.h"
#include "ScreenCapture/DirectXScreenCapture.h"
#include "AudioCapture/WinAudioCapture.h"
#include "CppRtspServ/RtpMediaServ.h"

#include <iostream>

#define STREAM_ADDR_STR "rtp://59.17.75.71:27389/mystream"

using namespace std;

int main(void)
{
	TestQueueMonitorThread monitor_thread(std::chrono::seconds(10));
	monitor_thread.start();

	int w, h;
	w = DEFALUT_WIDTH;
	h = DEFALUT_HEIGHT;

	DirectXScreenCapture* video_cap_obj = new DirectXScreenCapture();
	WinAudioCapture* audio_cap_obj = new WinAudioCapture();


	//프레임 버퍼 설정
	ScreenDataBuffer screen_buf(5, "Screen Frame Source Buffer");
	ScreenDataBuffer periodic_buf(10, "Screen Frame Collect Buffer");
	CaptureThread capture_obj(screen_buf, video_cap_obj);
	PeriodicDataCollector clt_obj(screen_buf, periodic_buf);
	VideoEncoder video_encoding_obj;
	AudioEncoder audio_encoding_obj;
	ImgFmtConverter pix_fmt_cvt;
	FrameEncoderThread video_enc_thr(video_encoding_obj);
	FrameEncoderThread audio_enc_thr(audio_encoding_obj);
	

	//서버
	RtpMediaServ main_serv(STREAM_ADDR_STR);
	VideoPacketProcessor video_pkt_input_obj(main_serv);
	AudioPacketProcessor audio_pkt_input_obj(main_serv);
	AVPacketHandlerThread video_packet_handler(video_encoding_obj,video_pkt_input_obj);
	AVPacketHandlerThread audio_packet_handler(audio_encoding_obj,audio_pkt_input_obj);

	main_serv.AddStream(video_encoding_obj.getEncCodecContext());
	main_serv.AddStream(audio_encoding_obj.getEncCodecContext());

	// 전송률, 코덱 정보등 output format에 대한 자세한 정보를 보낸다.
	av_dump_format(main_serv.getFmtContext(), 0, STREAM_ADDR_STR, 1);

	char errorBuff[80];
	// rtp가 제대로 열렸는지 체크
	if (!(main_serv.getFmtContext()->oformat->flags & AVFMT_NOFILE)) {
		int ret = avio_open(&main_serv.getFmtContext()->pb, STREAM_ADDR_STR, AVIO_FLAG_WRITE);
		if (ret < 0) {
			// ERROR
			fprintf(stderr, "Could not open outfile '%s': %s", STREAM_ADDR_STR, av_make_error_string(errorBuff, 80, ret));
			return -1;
		}
		else
			cout << "rtp file open done" << endl;
	}
	else
		cout << "rtp file not found" << endl;

	//스트리밍 시작
	audio_cap_obj->StartCapture();
	capture_obj.StartCapture();
	clt_obj.StartCollect();
	video_enc_thr.StartEncoding();
	audio_enc_thr.StartEncoding();
	video_packet_handler.StartHandle();
	audio_packet_handler.StartHandle();

	std::shared_ptr<VideoFrameData> frame;

	bool is_stream = true;
	while (is_stream)
	{
		while (!periodic_buf.SendFrameData(frame));
		auto yuv_data = pix_fmt_cvt.ConvertBGRToYUV(frame);

		shared_ptr<SharedAVFrame> frame_obj = AVStructPool<AVFrame*>::getInstance().getEmptyObj();
		AVFrame* av_frame = frame_obj.get()->getPointer();
		if (!AllocAVFrameBuf(av_frame, video_encoding_obj.getEncCodecContext()))
		{
			cout << "AllocAVFrameBuf fail" << endl;
			return -1;
		}

		CopyRawToAVFrame(yuv_data, av_frame);
		video_enc_thr.InputFrame(frame_obj);
	}

	while (is_stream)
	{
		audio_enc_thr.InputFrame();
	}

	video_packet_handler.EndHandle();
	audio_packet_handler.EndHandle();
	video_enc_thr.EndEncoding();
	audio_enc_thr.EndEncoding();
	clt_obj.EndCollect();
	audio_cap_obj->EndCapture();
	capture_obj.EndCapture();
	
	monitor_thread.stop();

	return 0;
}