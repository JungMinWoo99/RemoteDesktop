#include "MultiThreadFrameGetter/CaptureThread.h"
#include "MultiThreadFrameGetter/PeriodicDataCollector.h"
#include "MemoryManage/PixFmtConverter.h"
#include "VideoPacketizer/VideoEncoder.h"
#include "AudioPacketizer/AudioEncoder.h"
#include "MemoryManage/AVFrameManage.h"
#include "FramePacketizer/CoderThread/EncoderThread.h"
#include "TestQueueMonitorThread.h"
#include "ScreenCapture/DirectXScreenCapture.h"

#include <iostream>

#define STREAM_ADDR_STR "rtsp://59.17.75.71:8554/mystream"

using namespace std;

class RTPPacketProcessor : public AVPacketProcessor
{
public:
	RTPPacketProcessor(AVFormatContext** formatContext, AVCodecContext** enc_codec_context, AVStream** outStream, FrameEncoder& encoding_buf)
		:formatContext(formatContext), enc_codec_context(enc_codec_context), outStream(outStream)
	{
		avformat_network_init();
		if (avformat_alloc_output_context2(formatContext, nullptr, "rtsp", STREAM_ADDR_STR) < 0)
		{
			cout << "avformat_alloc_output_context2 fail" << endl;
			exit(-2);
		}
		else
			cout << "formatContext alloc done" << endl;

		*outStream = avformat_new_stream(*formatContext, NULL);
		if (outStream == NULL)
		{
			cout << "avformat_new_stream fail" << endl;
			exit(-2);
		}

		*enc_codec_context = encoding_buf.getEncCodecContext();
		(*outStream)->codecpar->codec_id = (*enc_codec_context)->codec_id;
		(*outStream)->codecpar->codec_type = (*enc_codec_context)->codec_type;
		(*outStream)->codecpar->width = (*enc_codec_context)->width;
		(*outStream)->codecpar->height = (*enc_codec_context)->height;
		(*outStream)->codecpar->format = (*enc_codec_context)->pix_fmt;
		(*outStream)->codecpar->bit_rate = 9000;

		(*outStream)->avg_frame_rate = (*enc_codec_context)->framerate;
		(*outStream)->r_frame_rate = (*enc_codec_context)->framerate;
		(*outStream)->time_base = (*enc_codec_context)->time_base;

		avformat_write_header(*formatContext, NULL);
	}

	~RTPPacketProcessor()
	{
		avformat_free_context(*formatContext);
	}

	void PacketProcess(AVPacket* packet) override
	{
		packet->stream_index = (*outStream)->index;
		int ret = av_interleaved_write_frame(*formatContext, packet);
		if (ret < 0) {
			char errorStr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
			av_make_error_string(errorStr, AV_ERROR_MAX_STRING_SIZE, ret);
			cout << "av_interleaved_write_frame fail: " << errorStr << endl;
			exit(ret);
		}
	}

	void EndEncoding()
	{
		av_write_trailer(*formatContext);
		cout << "end encoding" << endl;
	}
private:
	AVFormatContext** formatContext;
	AVCodecContext** enc_codec_context;
	AVStream** outStream;
};

int main(void)
{
	TestQueueMonitorThread monitor_thread(std::chrono::seconds(10));
	monitor_thread.start();

	int w, h;
	w = DEFALUT_WIDTH;
	h = DEFALUT_HEIGHT;

	DirectXScreenCapture* cap_obj = new DirectXScreenCapture();

	//프레임 버퍼 설정
	ScreenDataBuffer screen_buf(5,"Screen Frame Source Buffer");
	ScreenDataBuffer periodic_buf(10, "Screen Frame Collect Buffer");
	CaptureThread capture_obj(screen_buf, cap_obj);
	PeriodicDataCollector clt_obj(screen_buf, periodic_buf);
	VideoEncoder encoding_obj;
	ImgFmtConverter pix_fmt_cvt;

	//video stream open
	AVFormatContext* formatContext = nullptr;
	AVCodecContext* enc_codec_context = nullptr;
	AVStream* outStream = nullptr;
	RTPPacketProcessor prc_obj(&formatContext, &enc_codec_context, &outStream, encoding_obj);
	AVPacketHandlerThread pkt_thr(encoding_obj, prc_obj);
	FrameEncoderThread enc_thr(encoding_obj);

	// 전송률, 코덱 정보등 output format에 대한 자세한 정보를 보낸다.
	av_dump_format(formatContext, 0, STREAM_ADDR_STR, 1);

	//스트리밍 시작
	capture_obj.StartCapture();
	clt_obj.StartCollect();
	enc_thr.StartEncoding();
	pkt_thr.StartHandle();

	std::shared_ptr<VideoFrameData> frame;

	bool is_stream = true;
	while (is_stream)
	{
		while (!periodic_buf.SendFrameData(frame));
		auto yuv_data = pix_fmt_cvt.ConvertBGRToYUV(frame);

		shared_ptr<SharedAVFrame> frame_obj = AVStructPool<AVFrame*>::getInstance().getEmptyObj();
		AVFrame* av_frame = frame_obj.get()->getPointer();
		if (!AllocAVFrameBuf(av_frame, enc_codec_context))
		{
			cout << "AllocAVFrameBuf fail" << endl;
			return -1;
		}
		
		CopyRawToAVFrame(yuv_data, av_frame);
		enc_thr.InputFrame(frame_obj);
	}

	pkt_thr.EndHandle();
	enc_thr.EndEncoding();
	clt_obj.EndCollect();
	capture_obj.EndCapture();

	avformat_free_context(formatContext);

	monitor_thread.stop();

	return 0;
}