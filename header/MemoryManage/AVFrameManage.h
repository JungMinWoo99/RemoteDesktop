#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}
#include <memory>
#include "MemoryManage/Framedata.h"

bool AllocAVFrameBuf(AVFrame*& output_av, const AVCodecContext* c_context);

void CopyAVFrameToRaw(const AVFrame* src ,std::shared_ptr<VideoFrameData> dst);

void CopyRawToAVFrame(const std::shared_ptr<VideoFrameData> src, AVFrame* dst);