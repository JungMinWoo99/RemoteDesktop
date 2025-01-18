#pragma once

#include "Constant/VideoConstants.h"
#include "FramePacketizer/FrameEncoder.h"

class MP4Muxer : public FrameEncoder
{
public:
	MP4Muxer(AVCodecID codec_id = DEFALUT_MULTI_MEDIA_CODEC);
private:
	AVCodecID codec_id;
};