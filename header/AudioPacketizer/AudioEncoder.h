/*
   This code was written with reference to the following source.
   source 1: https://ffmpeg.org/doxygen/trunk/encode_audio_8c-example.html#a27
*/
#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#include <thread>

#include "FramePacketizer/FrameEncoder.h"

class AudioEncoder:public FrameEncoder
{
public:
	AudioEncoder(	int audio_channels = 2,
					int sample_rate = 44100,
					AVSampleFormat raw_data_format = AV_SAMPLE_FMT_S16,
					AVCodecID codec_id = AV_CODEC_ID_AAC);

	SwrContext* swr_ctx;//추후에 다른 클래스에서 처리할 예정

private:
	int audio_channels;
	int sample_rate;
	AVSampleFormat raw_data_format;
	AVCodecID codec_id;

	/* check that a given sample format is supported by the encoder */
	static int check_sample_fmt(const AVCodec* codec, enum AVSampleFormat sample_fmt);

	/* just pick the highest supported samplerate */
	static int select_sample_rate(const AVCodec* codec, int spec_rate = 0);

	/* select layout with the highest channel count */
	static int select_channel_layout(const AVCodec* codec, AVChannelLayout* dst, int spec_channels = 0);
};