#include "FramePacketizer/FrameEncoder.h"

#include <iostream>

using namespace std;

std::ofstream FrameEncoder::log_stream("encoder_log.txt", std::ios::out | std::ios::trunc);

FrameEncoder::FrameEncoder(string encoder_name)
	:encoder_name(encoder_name),enced_packet_buf(encoder_name) 
{
	enc_codec = NULL;
	enc_context = NULL;
}

_Check_return_ bool FrameEncoder::EncodeFrame(shared_ptr<SharedAVFrame> input)
{
	auto yuv_frame_data = input.get()->getPointer();
	
	while(FillPacketBuf());

	bool is_success = true;
	int ret = avcodec_send_frame(enc_context, yuv_frame_data);

	if (ret == 0)
	{
		is_success = true;
	}
	else
	{
		if (ret == AVERROR(EAGAIN))
		{
			log_stream << "encoder input full" << endl;
			is_success = false;
		}
		else if (ret == AVERROR_EOF)
		{
			log_stream << "end of encoder" << endl;
			is_success = false;
		}
		else if (ret == AVERROR(EINVAL))
		{
			log_stream << "codec not opened" << endl;
			is_success = false;
		}
		else if (ret == AVERROR(ENOMEM))
		{
			log_stream << "failed to add packet to encoder queue" << endl;
			is_success = false;
		}
		else if (ret < 0)
		{
			char errorStr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
			av_make_error_string(errorStr, AV_ERROR_MAX_STRING_SIZE, ret);
			log_stream << "avcodec_send_frame() fail: " << errorStr << endl;
			exit(ret);
		}
	}

	return is_success;
}

_Check_return_ bool FrameEncoder::SendPacket(shared_ptr<SharedAVPacket>& packet)
{
	return enced_packet_buf.pop(packet);
}

_Check_return_ bool FrameEncoder::SendPacketBlocking(std::shared_ptr<SharedAVPacket>& packet)
{
	return enced_packet_buf.wait_and_pop_utill_not_empty(packet);
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

void FrameEncoder::PrintLog(std::string log)
{
	log_stream << encoder_name<< ":" << log << endl;
}

void FrameEncoder::FlushContext()
{
	avcodec_send_frame(enc_context, NULL);
	while (FillPacketBuf());
}

_Check_return_ bool FrameEncoder::FillPacketBuf()
{
	bool ret;

	auto packet = empty_packet_buf.getEmptyObj();

	int error_code = avcodec_receive_packet(enc_context, packet.get()->getPointer());

	if (error_code == 0)
	{
		enced_packet_buf.push(packet);
		ret = true;
	}
	else
	{
		if (error_code == AVERROR(EAGAIN))
		{
			//output is not available in the current state - user must try to send input
		}
		else if (error_code == AVERROR_EOF)
			PrintLog("end of encoder");
		else if (error_code == AVERROR(EINVAL))
			PrintLog("codec not opened");
		else
		{
			char errorStr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
			av_make_error_string(errorStr, AV_ERROR_MAX_STRING_SIZE, error_code);
			PrintLog(std::format("avcodec_receive_packet() fail: {}", errorStr));
			exit(error_code);
		}
		ret = false;
	}
	
	return ret;
}
