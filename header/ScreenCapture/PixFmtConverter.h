#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include "Constant/VideoConstants.h"
#include "ScreenCapture/FrameData.h"

#define DEFALUT_SEGMENT_SEC 1
#define DEFALUT_TIME_BASE_UNIT 1000
#define DEFALUT_PIX_FMT AV_PIX_FMT_YUV420P

#include <memory>

class PixFmtConverter
{
public:
	PixFmtConverter(int width = DEFALUT_WIDTH, int height = DEFALUT_HEIGHT);

	std::shared_ptr<FrameData> ConvertBGRToYUV(std::shared_ptr<FrameData> bgr_data);
	std::shared_ptr<FrameData> ConvertYUVToBGR(std::shared_ptr<FrameData> yuv_data);

	~PixFmtConverter();
private:
	SwsContext* rgb_to_yuv_ctx;
	SwsContext* yuv_to_bgr_ctx;

	int frame_width;
	int frame_height;

	int bgr_stride[1];
	int yuv_stride[3];

	void FlipData(std::shared_ptr<FrameData> yuv_frame_data);
};