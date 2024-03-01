#include "VideoPacketizer/VideoEncoder.h"

using namespace std;

VideoEncoder::VideoEncoder(int w, int h, int frame_rate, AVCodecID coedec_id)
	:FrameEncoder("VideoEncoder")
{
	int ret;

	enc_codec = avcodec_find_encoder(coedec_id);
	if (enc_codec == NULL)
	{
		PrintLog("find_encoder fail");
		exit(-1);
	}

	enc_context = avcodec_alloc_context3(enc_codec);
	if (enc_context == NULL)
	{
		PrintLog("alloc encoder context fail");
		exit(-1);
	}

	enc_context->width = w;
	enc_context->height = h;
	enc_context->time_base = { 1, frame_rate };
	enc_context->framerate = { frame_rate, 1 };
	enc_context->gop_size = GOP_SIZE;
	enc_context->pix_fmt = DEFALUT_PIX_FMT;
	enc_context->bit_rate = RECOMMAND_BIT_RATE;
	enc_context->max_b_frames = 1; // no b frame
	enc_context->codec_type = AVMEDIA_TYPE_VIDEO;
	enc_context->flags |= AV_CODEC_FLAG_QSCALE;
	enc_context->rc_buffer_size = enc_context->rc_max_rate;

	int error_code;
	error_code = av_opt_set(enc_context->priv_data, "preset", "ultrafast", 0);
	if (error_code < 0)
	{
		if (error_code == AVERROR_OPTION_NOT_FOUND)
			PrintLog("AVERROR_OPTION_NOT_FOUND");
		else if (error_code == AVERROR(ERANGE))
			PrintLog("Out of range");
		else if (error_code == AVERROR(EINVAL))
			PrintLog("wrong value");
		else
			PrintLog("av_opt_set fail");
	}

	error_code = av_opt_set(enc_context->priv_data, "tune", "zerolatency", 0);
	if (error_code < 0)
	{
		if (error_code == AVERROR_OPTION_NOT_FOUND)
			PrintLog("AVERROR_OPTION_NOT_FOUND");
		else if (error_code == AVERROR(ERANGE))
			PrintLog("Out of range");
		else if (error_code == AVERROR(EINVAL))
			PrintLog("wrong value");
		else
			PrintLog("av_opt_set fail");
	}
	error_code = av_opt_set(enc_context->priv_data, "crf", "23", 0);
	if (error_code < 0)
	{
		if (error_code == AVERROR_OPTION_NOT_FOUND)
			PrintLog("AVERROR_OPTION_NOT_FOUND");
		else if (error_code == AVERROR(ERANGE))
			PrintLog("Out of range");
		else if (error_code == AVERROR(EINVAL))
			PrintLog("wrong value");
		else
			PrintLog("av_opt_set fail");
	}

	ret = avcodec_open2(enc_context, enc_codec, nullptr);
	if (ret < 0)
	{
		char errorStr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
		av_make_error_string(errorStr, AV_ERROR_MAX_STRING_SIZE, ret);
		PrintLog(std::format("avcodec_open fail: {}", errorStr));
		exit(ret);
	}
}
