#include "MultiThreadFrameGetter/CaptureThread.h"
#include "MultiThreadFrameGetter/PeriodicDataCollector.h"
#include "FramePacketizer/PixFmtConverter.h"
#include "FramePacketizer/FrameEncoder.h"
#include "FramePacketizer/AVFrameManage.h"
#include "FramePacketizer/CoderThread/EncoderThread.h"

#include <iostream>

#define STREAM_ADDR_STR "rtp://127.0.0.1:9000/RTPTest"

using namespace std;

class RTPPacketProcessor : public AVPacketProcessor
{
public:
	RTPPacketProcessor(AVFormatContext** formatContext, AVCodecContext** enc_codec_context, AVStream** outStream, FrameEncoder& encoding_buf)
		:formatContext(formatContext), enc_codec_context(enc_codec_context), outStream(outStream)
	{
		if (avformat_alloc_output_context2(formatContext, nullptr, "rtp", STREAM_ADDR_STR) < 0)
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

		(*outStream)->avg_frame_rate = (*enc_codec_context)->framerate;
		(*outStream)->r_frame_rate = (*enc_codec_context)->framerate;
		(*outStream)->time_base = (*enc_codec_context)->time_base;

		if (avio_open(&(*formatContext)->pb, STREAM_ADDR_STR, AVIO_FLAG_WRITE) < 0)
			cout << "file open fail" << endl;


		avformat_write_header(*formatContext, NULL);
	}

	~RTPPacketProcessor()
	{
		avformat_free_context(*formatContext);
	}

	void PacketProcess(AVPacket* packet) override
	{
		packet->stream_index = (*outStream)->index;
		if (av_interleaved_write_frame(*formatContext, packet) < 0)
			cout << "av_interleaved_write_frame fail" << endl;
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
	int w, h;
	w = DEFALUT_WIDTH;
	h = DEFALUT_HEIGHT;

	//프레임 버퍼 설정
	ScreenDataBuffer screen_buf(5);
	ScreenDataBuffer periodic_buf(1);
	CaptureThread capture_obj(screen_buf);
	PeriodicDataCollector clt_obj(screen_buf, periodic_buf);
	FrameEncoder encoding_obj;
	PixFmtConverter pix_fmt_cvt;

	//video stream open
	AVFormatContext* formatContext = nullptr;
	AVCodecContext* enc_codec_context = nullptr;
	AVStream* outStream = nullptr;
	RTPPacketProcessor prc_obj(&formatContext, &enc_codec_context, &outStream, encoding_obj);
	AVPacketHandlerThread pkt_thr(encoding_obj, prc_obj);
	FrameEncoderThread enc_thr(encoding_obj);

	// 전송률, 코덱 정보등 output format에 대한 자세한 정보를 보낸다.
	av_dump_format(formatContext, 0, STREAM_ADDR_STR, 1);

	char errorBuff[80];
	// rtp가 제대로 열렸는지 체크
	if (!(formatContext->oformat->flags & AVFMT_NOFILE)) {
		int ret = avio_open(&formatContext->pb, STREAM_ADDR_STR, AVIO_FLAG_WRITE);
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
	capture_obj.StartCapture();
	clt_obj.StartCollect();
	enc_thr.StartEncoding();
	pkt_thr.StartHandle();

	std::shared_ptr<FrameData> frame;
	AVFrame* av_frame;
	if (!AllocAVFrameBuf(av_frame, enc_codec_context))
	{
		cout << "AllocAVFrameBuf fail" << endl;
		return -1;
	}

	bool is_stream = true;
	while (is_stream)
	{
		while (!periodic_buf.SendFrameData(frame));
		CopyRawToAVFrame(pix_fmt_cvt.ConvertBGRToYUV(frame), av_frame);
		enc_thr.InputFrame(av_frame);
		//cout << "remain frame:" << FrameData::getRemainFrame() <<" remain packet:"<<encoding_obj.getRemainPacketNum() << endl;
	}

	pkt_thr.EndHandle();
	enc_thr.EndEncoding();
	clt_obj.EndCollect();
	capture_obj.EndCapture();

	av_frame_free(&av_frame);
	avformat_free_context(formatContext);

	return 0;
}
