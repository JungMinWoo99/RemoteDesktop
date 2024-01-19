#include "MultiThreadFrameGetter/CaptureThread.h"
#include "MultiThreadFrameGetter/PeriodicDataCollector.h"
#include "FramePacketizer/PixFmtConverter.h"
#include "FramePacketizer/FrameEncoder.h"
#include "FramePacketizer/CoderThread/EncoderThread.h"
#include "FramePacketizer/AVFrameManage.h"

#include <iostream>

#define TEST_NUM 1

using namespace std;

class TestPacketProcessor : public AVPacketProcessor
{
public:
	TestPacketProcessor(AVFormatContext** formatContext, AVCodecContext** enc_codec_context, AVStream** outStream,FrameEncoder& encoding_buf)
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
		if(av_interleaved_write_frame(*formatContext, packet)<0)
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

	if(TEST_NUM == 0)
	{
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
		AVFrame* av_frame;
		AllocAVFrameBuffer(av_frame, encoding_buf.getEncCodecContext());
		AVPacket* recv_packet;
		for (int i = 0; i < frame_per_sec * record_time; i++)
		{
			if (periodic_buf.SendFrameData(next_frame))
			{
				if (next_frame == prev_frame)
					cout << "duplicated frame input: " << i << " remain frame: " << periodic_buf.Size() << endl;
				prev_frame = next_frame;
				auto yuv_frame = pix_fmt_cvt.ConvertBGRToYUV(next_frame);
				CopyRawToAVFrame(yuv_frame, av_frame);
				encoding_buf.EncodeFrame(av_frame);
			}

			while (encoding_buf.SendPacket(recv_packet))
			{
				recv_packet->stream_index = outStream->index;
				if (av_interleaved_write_frame(formatContext, recv_packet) < 0)
					cout << "av_interleaved_write_frame fail" << endl;
				av_packet_free(&recv_packet);
			}
		}

		encoding_buf.FlushContext();

		while (encoding_buf.SendPacket(recv_packet))
		{
			recv_packet->stream_index = outStream->index;
			if (av_interleaved_write_frame(formatContext, recv_packet) < 0)
				cout << "av_interleaved_write_frame fail" << endl;
			av_packet_free(&recv_packet);
		}
		av_write_trailer(formatContext);
		cout << "end encoding" << endl;

		av_frame_free(&av_frame);
	}
	else
	{
		TestPacketProcessor prc_obj(&formatContext, &enc_codec_context, &outStream, encoding_buf);
		AVPacketHandlerThread enc_thr(encoding_buf, prc_obj);
		PixFmtConverter pix_fmt_cvt;
		shared_ptr<FrameData> next_frame;
		shared_ptr<FrameData> prev_frame = nullptr;
		AVFrame* av_frame;
		AllocAVFrameBuffer(av_frame, encoding_buf.getEncCodecContext());
		AVPacket* recv_packet;
		for (int i = 0; i < frame_per_sec * record_time; i++)
		{
			if (periodic_buf.SendFrameData(next_frame))
			{
				if (next_frame == prev_frame)
					cout << "duplicated frame input: " << i << " remain frame: " << periodic_buf.Size() << endl;
				prev_frame = next_frame;
				auto yuv_frame = pix_fmt_cvt.ConvertBGRToYUV(next_frame);
				CopyRawToAVFrame(yuv_frame, av_frame);
				encoding_buf.EncodeFrame(av_frame);
			}

			while (encoding_buf.SendPacket(recv_packet))
			{
				recv_packet->stream_index = outStream->index;
				if (av_interleaved_write_frame(formatContext, recv_packet) < 0)
					cout << "av_interleaved_write_frame fail" << endl;
				av_packet_free(&recv_packet);
			}
		}
		
		enc_thr.StartHandle();
		while (!periodic_buf.Empty());
		enc_thr.EndHandle();
		encoding_buf.FlushContext();
		while (encoding_buf.SendPacket(recv_packet))
		{
			recv_packet->stream_index = outStream->index;
			if (av_interleaved_write_frame(formatContext, recv_packet) < 0)
				cout << "av_interleaved_write_frame fail" << endl;
			av_packet_free(&recv_packet);
		}
		prc_obj.EndEncoding();
	}
	
	return 0;
}
