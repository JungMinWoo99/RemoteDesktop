#include "MultiThreadFrameGetter/CaptureThread.h"
#include "MultiThreadFrameGetter/PeriodicDataCollector.h"
#include "ScreenCapture/PixFmtConverter.h"
#include "FramePacketizer/FrameEncoder.h"
#include "FramePacketizer/AVFrameManage.h"

#include <iostream>

using namespace std;

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
	FrameEncoder encoding_buf;

	capture_obj.StartCapture();
	clt_obj.StartCollect();

	auto cap_time = std::chrono::high_resolution_clock::now() + std::chrono::seconds(record_time+1);
	std::this_thread::sleep_until(cap_time);

	clt_obj.EndCollect();
	capture_obj.EndCapture();
	std::cout << periodic_buf.Size() << std::endl;	

	//video stream open
	AVFormatContext* formatContext=nullptr;
	AVCodecContext* enc_codec_context = nullptr;
	AVStream* outStream = nullptr;

	if (avformat_alloc_output_context2(&formatContext, nullptr, nullptr, "output.avi") < 0)
	{
		cout << "avformat_alloc_output_context2 fail" << endl;
	}

	outStream = avformat_new_stream(formatContext, NULL);
	if (outStream == NULL)
	{
		cout << "avformat_new_stream fail" << endl;
		exit(-2);
	}

	enc_codec_context = encoding_buf.getEncCodecContext();
	outStream->codecpar->codec_id = enc_codec_context->codec_id;
	outStream->codecpar->codec_type = enc_codec_context->codec_type;
	outStream->codecpar->width = enc_codec_context->width;
	outStream->codecpar->height = enc_codec_context->height;
	outStream->codecpar->format = enc_codec_context->pix_fmt;

	outStream->avg_frame_rate = enc_codec_context->framerate;
	outStream->r_frame_rate = enc_codec_context->framerate;
	outStream->time_base = enc_codec_context->time_base;

	if (avio_open(&formatContext->pb, "output.avi", AVIO_FLAG_WRITE) < 0)
		cout << "file open fail" << endl;


	if (avformat_write_header(formatContext, NULL) < 0)
	{
		return -1;
	}

	PixFmtConverter pix_fmt_cvt;
	shared_ptr<FrameData> next_frame;
	shared_ptr<FrameData> prev_frame = nullptr;
	auto frame = AVStructPool<AVFrame*>::getInstance().getEmptyObj();
	auto av_frame = frame.get()->getPointer();
	AllocAVFrameBuf(av_frame, encoding_buf.getEncCodecContext());
	shared_ptr<SharedAVPacket> packet;
	for (int i = 0; i < frame_per_sec * record_time; i++)
	{
		if (periodic_buf.SendFrameData(next_frame))
		{
			auto yuv_frame = pix_fmt_cvt.ConvertBGRToYUV(next_frame);
			CopyRawToAVFrame(yuv_frame, av_frame);
			encoding_buf.EncodeFrame(frame);
		}

		while (encoding_buf.SendPacket(packet))
		{
			auto recv_packet = packet.get()->getPointer();
			recv_packet->stream_index = outStream->index;
			if (av_interleaved_write_frame(formatContext, recv_packet) < 0)
				cout << "av_interleaved_write_frame fail" << endl;
		}
	}

	encoding_buf.FlushContext();

	while (encoding_buf.SendPacket(packet))
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
