#include "MultiThreadFrameGetter/CaptureThread.h"
#include "MultiThreadFrameGetter/PeriodicDataCollector.h"
#include "MemoryManage/PixFmtConverter.h"
#include "FramePacketizer/FrameEncoder.h"
#include "FramePacketizer/CoderThread/EncoderThread.h"
#include "FramePacketizer/AVFrameManage.h"

#include <iostream>

using namespace std;

class TestPacketProcessor : public AVPacketProcessor
{
public:
	TestPacketProcessor(AVFormatContext** formatContext, AVCodecContext** enc_codec_context, AVStream** outStream, FrameEncoder& encoding_buf)
		:formatContext(formatContext), enc_codec_context(enc_codec_context), outStream(outStream)
	{
		if (avformat_alloc_output_context2(formatContext, nullptr, nullptr, "output.avi") < 0)
		{
			cout << "avformat_alloc_output_context2 fail" << endl;
		}

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

		if (avio_open(&(*formatContext)->pb, "output.avi", AVIO_FLAG_WRITE) < 0)
			cout << "file open fail" << endl;


		avformat_write_header(*formatContext, NULL);
	}

	~TestPacketProcessor()
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
private:
	AVFormatContext** formatContext;
	AVCodecContext** enc_codec_context;
	AVStream** outStream;
};

int main(void)
{
	int w, h, frame_per_sec, record_time;
	w = DEFALUT_WIDTH;
	h = DEFALUT_HEIGHT;
	frame_per_sec = DEFALUT_FRAME_RATE;
	record_time = 5;

	ScreenDataBuffer screen_buf(300);
	ScreenDataBuffer periodic_buf(record_time * frame_per_sec);
	CaptureThread capture_obj(screen_buf);
	PeriodicDataCollector clt_obj(screen_buf, periodic_buf);
	FrameEncoder encoding_obj;

	//video stream open
	AVFormatContext* formatContext = nullptr;
	AVCodecContext* enc_codec_context = nullptr;
	AVStream* outStream = nullptr;
	TestPacketProcessor prc_obj(&formatContext, &enc_codec_context, &outStream, encoding_obj);
	
	//proc thread
	AVPacketHandlerThread frame_thr(encoding_obj, prc_obj);
	FrameEncoderThread enc_thr(encoding_obj);

	//frame data process
	ImgFmtConverter pix_fmt_cvt;
	shared_ptr<VideoFrameData> next_frame;
	shared_ptr<VideoFrameData> prev_frame = nullptr;
	shared_ptr<SharedAVPacket> packet;
	
	capture_obj.StartCapture();
	clt_obj.StartCollect();
	frame_thr.StartHandle();
	enc_thr.StartEncoding();
	cout << "all thread start" << endl;

	for (int i = 0; i < frame_per_sec * record_time; i++)
	{
		while (!periodic_buf.SendFrameData(next_frame));
		auto frame = AVStructPool<AVFrame*>::getInstance().getEmptyObj();
		auto av_frame = frame.get()->getPointer();
		AllocAVFrameBuf(av_frame, encoding_obj.getEncCodecContext());
		auto yuv_frame = pix_fmt_cvt.ConvertBGRToYUV(next_frame);
		CopyRawToAVFrame(yuv_frame, av_frame);
		enc_thr.InputFrame(frame);
	}
	cout << "capture done" << endl;

	enc_thr.EndEncoding();
	cout << "enc thread done" << endl;
	frame_thr.EndHandle();
	cout << "frm thread done" << endl;
	clt_obj.EndCollect();
	cout << "clt thread done" << endl;
	capture_obj.EndCapture();	
	cout << "cap thread done" << endl;
	
	while (encoding_obj.SendPacket(packet))
	{
		auto recv_packet = packet.get()->getPointer();
		recv_packet->stream_index = outStream->index;
		if (av_interleaved_write_frame(formatContext, recv_packet) < 0)
			cout << "av_interleaved_write_frame fail" << endl;
	}
	encoding_obj.FlushContext();
	while (encoding_obj.SendPacket(packet))
	{
		auto recv_packet = packet.get()->getPointer();
		recv_packet->stream_index = outStream->index;
		if (av_interleaved_write_frame(formatContext, recv_packet) < 0)
			cout << "av_interleaved_write_frame fail" << endl;
	}
	
	av_write_trailer(formatContext);
	cout << "end encoding" << endl;


	return 0;
}