#include "FramePacketizer/FrameDecoder.h"

#include <iostream>

#define DEFALUT_SEGMENT_SEC 1
#define DEFALUT_TIME_BASE_UNIT 1000
#define DEFALUT_PIX_FMT AV_PIX_FMT_YUV420P

using namespace std;

std::ofstream FrameDecoder::log_stream("decoder_log.txt", std::ios::out | std::ios::trunc);

FrameDecoder::FrameDecoder(int w, int h, int frame_rate, AVCodecID coedec_id)
	:frame_rate(frame_rate), deced_frame_buf("FrameDecoder")
{
	int ret;

	dec_codec = avcodec_find_decoder(coedec_id);
	if (dec_codec == NULL)
	{
		log_stream << "avcodec_find_decoder fail" << endl;
		exit(-1);
	}
	dec_context = avcodec_alloc_context3(dec_codec);
	if (dec_context == NULL)
	{
		log_stream << "avcodec_alloc_context fail" << endl;
		exit(-1);
	}

	dec_context->codec_tag = MKTAG('a', 'v', 'c', '1');
	dec_context->width = w;
	dec_context->height = h;
	dec_context->codec_id = coedec_id;
	dec_context->pix_fmt = DEFALUT_PIX_FMT;
	dec_context->codec_type = AVMEDIA_TYPE_VIDEO;
	dec_context->field_order = AV_FIELD_PROGRESSIVE;
	dec_context->lowres = 0;
	dec_context->skip_loop_filter = AVDISCARD_NONE;
	dec_context->skip_idct = AVDISCARD_NONE;
	dec_context->skip_frame = AVDISCARD_NONE;
	dec_context->pkt_timebase = { 1, frame_rate };

	ret = avcodec_open2(dec_context, dec_codec, nullptr);
	if (ret < 0)
	{
		char errorStr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
		av_make_error_string(errorStr, AV_ERROR_MAX_STRING_SIZE, ret);
		log_stream << "avcodec_open2 fail: " << errorStr << endl;
		exit(ret);
	}
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
			log_stream << "nothing to fill" << endl;
		else if (error_code == AVERROR(EINVAL))
			log_stream << "codec not opened" << endl;
		else
		{
			char errorStr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
			av_make_error_string(errorStr, AV_ERROR_MAX_STRING_SIZE, error_code);
			log_stream << "avcodec_receive_frame() fail: " << errorStr << endl;
			exit(error_code);
		}
		ret = false;
	}

	return ret;
}

