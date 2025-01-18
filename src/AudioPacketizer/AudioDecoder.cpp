#include "AudioPacketizer/AudioDecoder.h"

using namespace std;

AudioDecoder::AudioDecoder(int audio_channels, int sample_rate, AVSampleFormat raw_data_format, AVCodecID codec_id)
	:FrameDecoder("AudioDecoder"), audio_channels(audio_channels), sample_rate(sample_rate), raw_data_format(raw_data_format), codec_id(codec_id)
{
	int ret;

	dec_codec = avcodec_find_decoder(codec_id);
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

	dec_context->codec_tag = MKTAG('m', 'p', '4', 'a');
	dec_context->bit_rate = RECOMMAND_BIT_RATE;
	dec_context->codec_type = AVMEDIA_TYPE_AUDIO;
	dec_context->request_sample_fmt = AV_SAMPLE_FMT_FLTP;
	ret = avcodec_open2(dec_context, dec_codec, nullptr);
	if (ret < 0)
	{
		char errorStr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
		av_make_error_string(errorStr, AV_ERROR_MAX_STRING_SIZE, ret);
		log_stream << "avcodec_open2 fail: " << errorStr << endl;
		exit(ret);
	}
}
