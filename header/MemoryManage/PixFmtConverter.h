#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#include "Constant/VideoConstants.h"
#include "MemoryManage/Framedata.h"

#define DEFALUT_SEGMENT_SEC 1
#define DEFALUT_TIME_BASE_UNIT 1000
#define DEFALUT_PIX_FMT AV_PIX_FMT_YUV420P
#define DEFALUT_SAM_FMT AV_SAMPLE_FMT_FLTP

#include <memory>

class ImgFmtConverter
{
public:
	ImgFmtConverter(int width = DEFALUT_WIDTH, int height = DEFALUT_HEIGHT);

	std::shared_ptr<VideoFrameData> ConvertBGRToYUV(std::shared_ptr<VideoFrameData> bgr_data);
	std::shared_ptr<VideoFrameData> ConvertYUVToBGR(std::shared_ptr<VideoFrameData> yuv_data);

	~ImgFmtConverter();
private:
	SwsContext* rgb_to_yuv_ctx;
	SwsContext* yuv_to_bgr_ctx;

	int frame_width;
	int frame_height;

	int bgr_stride[1];
	int yuv_stride[3];

	void FlipData(std::shared_ptr<VideoFrameData> yuv_frame_data);
};

class SmpFmtConverter
{
public:
	SmpFmtConverter(int sample_rate = DEFALUT_SAMPLE_RATE, int audio_channel = DEFALUT_AUDIO_CHANNEL)
	{
		AVChannelLayout defalut_ch;
		av_channel_layout_default(&defalut_ch, DEFALUT_AUDIO_CHANNEL);

		// Create audio conversion context
		int ret = swr_alloc_set_opts2(&f32_to_s16_ctx,
			&defalut_ch,
			DEFALUT_SAM_FMT,
			sample_rate,
			&defalut_ch,
			AV_SAMPLE_FMT_S16,
			sample_rate,
			0,
			NULL);

		if (f32_to_s16_ctx == NULL)
		{
			cout << "swr_alloc_set_opts2 fail" << endl;
			exit(ret);
		}
	}

	std::shared_ptr<AudioFrameData> ConvertS16ToFLTP(std::shared_ptr<AudioFrameData> s16_data, int sample_pe_channel)
	{
		int ret;
		std::shared_ptr<AudioFrameData> output_pkt = make_shared<AudioFrameData>(s16_data.get()->getMemSize());
		ret = swr_convert(f32_to_s16_ctx, (const uint8_t*)output_pkt.get()->getMemPointer(), sample_pe_channel, (const uint8_t**)s16_data.get()->getMemPointer(), sample_pe_channel);
		if (ret < 0) 
		{
			fprintf(stderr, "data convert fail\n");
			exit(1);
		}
	}

	~SmpFmtConverter();

private:
	SwrContext* f32_to_s16_ctx;
};