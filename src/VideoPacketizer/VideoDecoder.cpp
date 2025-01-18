#include "VideoPacketizer/VideoDecoder.h"

using namespace std;

VideoDecoder::VideoDecoder(int w, int h, int frame_rate, AVCodecID coedec_id)
	:FrameDecoder("VideoDecoder"), w(w), h(h), frame_rate(frame_rate), coedec_id(coedec_id)
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
