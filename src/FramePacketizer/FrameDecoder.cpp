#include "FramePacketizer/FrameDecoder.h"

#include <iostream>

#define DEFALUT_SEGMENT_SEC 1
#define DEFALUT_TIME_BASE_UNIT 1000
#define DEFALUT_PIX_FMT AV_PIX_FMT_YUV420P

using namespace std;

std::ofstream FrameDecoder::log_stream("decoder_log.txt", std::ios::out | std::ios::trunc);

FrameDecoder::FrameDecoder(std::string decoder_name)
	:decoder_name(decoder_name), deced_frame_buf(decoder_name)
{
	dec_codec = NULL;
	dec_context = NULL;
}

_Check_return_ bool FrameDecoder::DecodePacket(std::shared_ptr<SharedAVPacket> input)
{
	auto avpkt = input.get()->getPointer();

	while (FillFrameBuf());

	bool is_success = true;
	int ret = avcodec_send_packet(dec_context, avpkt);

	if (ret == 0)
	{
		is_success = true;
	}
	else
	{
		if (ret == AVERROR(EAGAIN))
		{
			log_stream << "decoder input full" << endl;
			is_success = false;
		}
		else if (ret == AVERROR_EOF)
		{
			log_stream << "end of decoder" << endl;
			is_success = false;
		}
		else if (ret == AVERROR(EINVAL))
		{
			log_stream << "codec not opened" << endl;
			is_success = false;
		}
		else if (ret == AVERROR(ENOMEM))
		{
			log_stream << "failed to add packet to decoder queue" << endl;
			is_success = false;
		}
		else if (ret < 0)
		{
			char errorStr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
			av_make_error_string(errorStr, AV_ERROR_MAX_STRING_SIZE, ret);
			log_stream << "avcodec_send_packet() fail: " << errorStr << endl;
			exit(ret);
		}
	}

	return is_success;
}

_Check_return_ bool FrameDecoder::SendFrame(shared_ptr<SharedAVFrame>& frame)
{
	return deced_frame_buf.pop(frame);
}

_Check_return_ bool FrameDecoder::SendFrameBlocking(std::shared_ptr<SharedAVFrame>& frame)
{
	return deced_frame_buf.wait_and_pop_utill_not_empty(frame);
}

const AVCodec* FrameDecoder::getDecCodec()
{
	return dec_codec;
}

AVCodecContext* FrameDecoder::getDecCodecContext()
{
	return dec_context;
}

FrameDecoder::~FrameDecoder()
{
	avcodec_free_context(&dec_context);
}

void FrameDecoder::PrintLog(std::string log)
{
	log_stream << decoder_name << ":" << log << endl;
}

void FrameDecoder::FlushContext()
{
	avcodec_send_packet(dec_context, NULL);
	while (FillFrameBuf());
}

_Check_return_ bool FrameDecoder::FillFrameBuf()
{
	bool ret;

	auto frame = empty_frame_buf.getEmptyObj();

	int error_code = avcodec_receive_frame(dec_context, frame.get()->getPointer());
	
	if (error_code == 0)
	{
		deced_frame_buf.push(frame);
		ret = true;
	}
	else
	{
		if (error_code == AVERROR(EAGAIN))
		{
			//output is not available in the current state - user must try to send input
		}
		else if (error_code == AVERROR_EOF)
			PrintLog("nothing to fill");
		else if (error_code == AVERROR(EINVAL))
			PrintLog("codec not opened");
		else
		{
			char errorStr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
			av_make_error_string(errorStr, AV_ERROR_MAX_STRING_SIZE, error_code);
			PrintLog(std::format("avcodec_receive_frame() fail: {}", errorStr));
			exit(error_code);
		}
		ret = false;
	}

	return ret;
}
