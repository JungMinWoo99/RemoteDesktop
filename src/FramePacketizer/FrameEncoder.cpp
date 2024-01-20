#include "FramePacketizer/FrameEncoder.h"


#include <iostream>

#define DEFALUT_SEGMENT_SEC 1
#define DEFALUT_TIME_BASE_UNIT 1000
#define DEFALUT_PIX_FMT AV_PIX_FMT_YUV420P
#define GOP_SIZE 30

using namespace std;

FrameEncoder::FrameEncoder(int w, int h, int frame_rate, AVCodecID coedec_id)
	:frame_rate(frame_rate)
{
	int ret;

	enc_codec = avcodec_find_encoder(coedec_id);
	if (enc_codec == NULL)
	{
		cout << "find_encoder fail" << endl;
		exit(-1);
	}

	enc_context = avcodec_alloc_context3(enc_codec);
	if (enc_context == NULL)
	{
		cout << "alloc encoder context fail" << endl;
		exit(-1);
	}

	enc_context->width = w;
	enc_context->height = h;
	enc_context->time_base = { 1, frame_rate };
	enc_context->framerate = { frame_rate, 1 };
	enc_context->gop_size = GOP_SIZE;
	enc_context->pix_fmt = DEFALUT_PIX_FMT;

	enc_context->codec_type = AVMEDIA_TYPE_VIDEO;
	enc_context->rc_buffer_size = enc_context->rc_max_rate;

	int error_code;
	error_code = av_opt_set(enc_context->priv_data, "preset", "ultrafast", 0);
	if (error_code < 0)
	{
		if (error_code == AVERROR_OPTION_NOT_FOUND)
			cout << "AVERROR_OPTION_NOT_FOUND" << endl;
		else if(error_code == AVERROR(ERANGE))
			cout << "Out of range" << endl;
		else if (error_code == AVERROR(EINVAL))
			cout << "wrong value" << endl;
		else
			cout << "av_opt_set fail" << endl;
	}
	error_code = av_opt_set(enc_context->priv_data, "tune", "zerolatency", 0);
	if (error_code < 0)
	{
		if (error_code == AVERROR_OPTION_NOT_FOUND)
			cout << "AVERROR_OPTION_NOT_FOUND" << endl;
		else if (error_code == AVERROR(ERANGE))
			cout << "Out of range" << endl;
		else if (error_code == AVERROR(EINVAL))
			cout << "wrong value" << endl;
		else
			cout << "av_opt_set fail" << endl;
	}
	error_code = av_opt_set(enc_context->priv_data, "crf", "18", 0);
	if (error_code < 0)
	{
		if (error_code == AVERROR_OPTION_NOT_FOUND)
			cout << "AVERROR_OPTION_NOT_FOUND" << endl;
		else if (error_code == AVERROR(ERANGE))
			cout << "Out of range" << endl;
		else if (error_code == AVERROR(EINVAL))
			cout << "wrong value" << endl;
		else
			cout << "av_opt_set fail" << endl;
	}

	ret = avcodec_open2(enc_context, enc_codec, nullptr);
	if (ret < 0)
	{
		char errorStr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
		av_make_error_string(errorStr, AV_ERROR_MAX_STRING_SIZE, ret);
		cout << "avcodec_open fail: " << errorStr << endl;
		exit(ret);
	}
}

_Check_return_ bool FrameEncoder::EncodeFrame(shared_ptr<SharedAVFrame> input)
{
	lock_guard<std::mutex> lock(encoder_mtx);
	static int64_t prev_capture_time = 0;
	static int pts_count = 0;

	auto yuv_frame_data = input.get()->getPointer();

	if (yuv_frame_data->pts < prev_capture_time)
		return false;
	else
	{
		prev_capture_time = yuv_frame_data->pts;
		yuv_frame_data->pts = pts_count++;
	}

	bool is_success = true;
	int ret = avcodec_send_frame(enc_context, yuv_frame_data);
	if (ret != 0)
	{
		if (ret == AVERROR(EAGAIN))
		{
			while (FillPacketBuf());
			ret = avcodec_send_frame(enc_context, yuv_frame_data);
		}

		if (ret == AVERROR_EOF)
		{
			cout << "end of encoder" << endl;
			is_success = false;
		}
		else if (ret == AVERROR(EINVAL))
		{
			cout << "codec not opened" << endl;
			is_success = false;
		}
		else if (ret == AVERROR(ENOMEM))
		{
			cout << "failed to add packet to encoder queue" << endl;
			is_success = false;
		}
		else if (ret < 0)
		{
			char errorStr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
			av_make_error_string(errorStr, AV_ERROR_MAX_STRING_SIZE, ret);
			cout << "avcodec_send_frame() fail: " << errorStr << endl;
			exit(ret);
		}
	}

	return is_success;
}

_Check_return_ bool FrameEncoder::SendPacket(shared_ptr<SharedAVPacket>& packet)
{
	lock_guard<std::mutex> lock(encoder_mtx);

	bool is_success = true;
	if (!enced_packet_buf.pop(packet))
	{
		while (FillPacketBuf());
		is_success = enced_packet_buf.pop(packet);
	}

	return is_success;
}

const AVCodec* FrameEncoder::getEncCodec()
{
	return enc_codec;
}

AVCodecContext* FrameEncoder::getEncCodecContext()
{
	return enc_context;
}

size_t FrameEncoder::getBufferSize()
{
	return enced_packet_buf.size();
}

FrameEncoder::~FrameEncoder()
{
	avcodec_free_context(&enc_context);
}

void FrameEncoder::FlushContext()
{
	avcodec_send_frame(enc_context, NULL);
	int ret;
	while (true)
	{
		auto packet= empty_packet_buf.getEmptyObj();

		ret = avcodec_receive_packet(enc_context, packet.get()->getPointer());
		if (ret != 0)
		{
			if (ret == AVERROR_EOF)
				break;
			else
			{
				char errorStr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
				av_make_error_string(errorStr, AV_ERROR_MAX_STRING_SIZE, ret);
				cout << "avcodec_receive_packet() fail: " << errorStr << endl;
				exit(ret);
			}
		}

		enced_packet_buf.push(packet);
	}
}

_Check_return_ bool FrameEncoder::FillPacketBuf()
{
	bool ret = true;
	auto packet = empty_packet_buf.getEmptyObj();

	int error_code = avcodec_receive_packet(enc_context, packet.get()->getPointer());
	if (error_code != 0)
	{
		ret = false;

		if (error_code == AVERROR(EAGAIN))
		{
			//output is not available in the current state - user must try to send input
		}
		else if (error_code == AVERROR_EOF)
			cout << "end of encoder" << endl;
		else if (error_code == AVERROR(EINVAL))
			cout << "codec not opened" << endl;
		else
		{
			char errorStr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
			av_make_error_string(errorStr, AV_ERROR_MAX_STRING_SIZE, error_code);
			cout << "avcodec_receive_packet() fail: " << errorStr << endl;
			exit(error_code);
		}
	}
	else
	{
		enced_packet_buf.push(packet);
		ret = true;
	}
	
	return ret;
}