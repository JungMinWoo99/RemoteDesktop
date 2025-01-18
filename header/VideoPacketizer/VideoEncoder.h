#pragma once

#include "Constant/VideoConstants.h"
#include "FramePacketizer/FrameEncoder.h"

class VideoEncoder : public FrameEncoder
{
public:
	VideoEncoder(	int w = DEFALUT_WIDTH, 
					int h = DEFALUT_HEIGHT, 
					int frame_rate = DEFALUT_FRAME_RATE, 
					AVCodecID codec_id = AV_CODEC_ID_H264);

private:
	int w;
	int h;
	int frame_rate;
	AVCodecID codec_id;
};
