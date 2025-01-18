#include "MP4Muxer/MP4Muxer.h"

MP4Muxer::MP4Muxer(AVCodecID codec_id)
	:FrameEncoder("MP4Muxer"), codec_id(codec_id)
{
	enc_codec = avcodec_find_encoder(codec_id);
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
}
