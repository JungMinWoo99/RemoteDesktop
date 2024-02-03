#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}
#include <memory>

#include "FramePacketizer/PacketData.h"

std::shared_ptr<PacketData> ConvertAVPacketToRawWithoutHeader(const AVPacket* src);

const BYTE* FindNextNAL(const BYTE* data_start, const BYTE* data_end);

bool isSPSNALU(const BYTE* packet_data);

bool isPPSNALU(const BYTE* packet_data);

void CopyNALU(const BYTE* nalu_start, const BYTE* nalu_end, std::shared_ptr<PacketData>& buf);
