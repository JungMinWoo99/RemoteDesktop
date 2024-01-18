#include<memory>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include "FramePacketizer/FramePacket.h"

std::shared_ptr<FramePacket> FramePack(AVPacket* avpkt);

AVPacket* FrameUnpack(std::shared_ptr<FramePacket> fpkt);