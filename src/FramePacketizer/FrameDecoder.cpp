#include "FramePacketizer/FrameDecoder.h"

#include <iostream>

#define DEFALUT_SEGMENT_SEC 1
#define DEFALUT_TIME_BASE_UNIT 1000
#define DEFALUT_PIX_FMT AV_PIX_FMT_YUV420P

using namespace std;

FrameDecoder::FrameDecoder(int w, int h, int frame_rate, AVCodecID coedec_id)
	:frame_rate(frame_rate)
{
	int ret;

	dec_codec = avcodec_find_decoder(coedec_id);
	if (dec_codec == NULL)
	{
		cout << "avcodec_find_decoder fail" << endl;
		exit(-1);
	}
	dec_context = avcodec_alloc_context3(dec_codec);
	if (dec_context == NULL)
	{
		cout << "avcodec_alloc_context fail" << endl;
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
		cout << "avcodec_open2 fail: " << errorStr << endl;
		exit(ret);
	}
}

bool FrameDecoder::DecodePacket(std::shared_ptr<SharedAVPacket> input)
{
	decoder_mtx.lock();

	auto avpkt = input.get()->getPointer();

	FillFrameBuf();

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
			cout << "encoder input full" << endl;
			is_success = false;
		}
		else if (ret == AVERROR_EOF)
		{
			cout << "end of decoder" << endl;
			is_success = false;
		}
		else if (ret == AVERROR(EINVAL))
		{
			cout << "codec not opened" << endl;
			is_success = false;
		}
		else if (ret == AVERROR(ENOMEM))
		{
			cout << "failed to add packet to decoder queue" << endl;
			is_success = false;
		}
		else if (ret < 0)
		{
			char errorStr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
			av_make_error_string(errorStr, AV_ERROR_MAX_STRING_SIZE, ret);
			cout << "avcodec_send_packet() fail: " << errorStr << endl;
			exit(ret);
		}
	}

	decoder_mtx.unlock();
	return is_success;
}

bool FrameDecoder::SendFrame(shared_ptr<SharedAVFrame>& frame)
{
	decoder_mtx.lock();
	FillFrameBuf();
	decoder_mtx.unlock();
	return deced_frame_buf.pop(frame);
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

bool FrameDecoder::FillFrameBuf()
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
			cout << "nothing to fill" << endl;
		else if (error_code == AVERROR(EINVAL))
			cout << "codec not opened" << endl;
		else
		{
			char errorStr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
			av_make_error_string(errorStr, AV_ERROR_MAX_STRING_SIZE, error_code);
			cout << "avcodec_receive_frame() fail: " << errorStr << endl;
			exit(error_code);
		}
		ret = false;
	}

	return ret;
}

