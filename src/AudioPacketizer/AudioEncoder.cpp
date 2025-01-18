#include "AudioPacketizer/AudioEncoder.h"

#include <iostream>

#define RECOMMANED_BIT_RATE 576000

using namespace std;

AudioEncoder::AudioEncoder(int audio_channels, int sample_rate, AVSampleFormat raw_data_format, AVCodecID codec_id)
	:FrameEncoder("AudioEncoder"), audio_channels(audio_channels), sample_rate(sample_rate), raw_data_format(raw_data_format), codec_id(codec_id)
{
	enc_codec = avcodec_find_encoder(codec_id);
	if (enc_codec == NULL)
	{
		log_stream << "find encoder fail" << endl;
		exit(-1);
	}

	enc_context = avcodec_alloc_context3(enc_codec);
	if (enc_context == NULL)
	{
		log_stream << "alloc encoder context fail" << endl;
		exit(-1);
	}

	/* put sample parameters */
	enc_context->bit_rate = RECOMMANED_BIT_RATE;

	/* check that the encoder supports float input */
	enc_context->sample_fmt = AV_SAMPLE_FMT_FLTP;
	if (!check_sample_fmt(enc_codec, enc_context->sample_fmt))
	{
		log_stream << "Encoder does not support sample format" << av_get_sample_fmt_name(enc_context->sample_fmt) << endl;
		exit(1);
	}

	/* select other audio parameters supported by the encoder */
	enc_context->sample_rate = select_sample_rate(enc_codec, sample_rate);
	if (select_channel_layout(enc_codec, &enc_context->ch_layout, audio_channels) < 0)
		exit(1);

	/* open it */
	if (avcodec_open2(enc_context, enc_codec, NULL) < 0)
	{
		log_stream << "Could not open codec" << endl;
		exit(1);
	}

	// Input audio parameters
	AVSampleFormat in_sample_fmt = raw_data_format;
	int in_sample_rate = sample_rate;
	int in_channels = audio_channels;

	// Output audio parameters
	AVSampleFormat out_sample_fmt = enc_context->sample_fmt;
	int out_sample_rate = sample_rate;
	int out_channels = audio_channels;

	// Create audio conversion context
	int ret = swr_alloc_set_opts2(&swr_ctx,
		&enc_context->ch_layout,
		out_sample_fmt,
		out_sample_rate,
		&enc_context->ch_layout,
		in_sample_fmt,
		in_sample_rate,
		0,
		NULL);

	if (swr_ctx == NULL)
	{
		log_stream << "swr_alloc_set_opts2 fail" << endl;
		exit(ret);
	}

	swr_init(swr_ctx);
}

int AudioEncoder::check_sample_fmt(const AVCodec* codec, AVSampleFormat sample_fmt)
{
	const enum AVSampleFormat* p = codec->sample_fmts;

	while (*p != AV_SAMPLE_FMT_NONE) {
		if (*p == sample_fmt)
			return 1;
		p++;
	}
	return 0;
}

int AudioEncoder::select_sample_rate(const AVCodec* codec, int spec_rate)
{
	const int* p;
	int best_samplerate = 0;

	if (!codec->supported_samplerates)
		return 44100;

	p = codec->supported_samplerates;
	while (*p) {
		if (!best_samplerate || abs(44100 - *p) < abs(44100 - best_samplerate))
			best_samplerate = *p;
		if (spec_rate == best_samplerate)
			break;
		p++;
	}
	if (best_samplerate != spec_rate)
		log_stream << "Encoder does not support sample rate " << spec_rate << endl;

	return best_samplerate;
}

int AudioEncoder::select_channel_layout(const AVCodec* codec, AVChannelLayout* dst, int spec_channels)
{
	const AVChannelLayout* p;
	const AVChannelLayout* best_ch_layout = NULL;
	int best_nb_channels = 0;


	if (!codec->ch_layouts)
	{
		AVChannelLayout av_audio_layout = AV_CHANNEL_LAYOUT_STEREO;
		return av_channel_layout_copy(dst, &av_audio_layout);
	}


	p = codec->ch_layouts;
	while (p->nb_channels) {
		int nb_channels = p->nb_channels;

		if (nb_channels > best_nb_channels) {
			best_ch_layout = p;
			best_nb_channels = nb_channels;
		}
		p++;
	}

	if (spec_channels != 0)
	{
		if (spec_channels <= best_nb_channels)
			best_nb_channels = spec_channels;
		else
			log_stream << "Encoder does not support channels " << spec_channels << endl;
	}

	return av_channel_layout_copy(dst, best_ch_layout);
}
