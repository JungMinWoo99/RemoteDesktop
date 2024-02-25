#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}
#include <memory>

#include "MemoryManage/PacketData.h"
#include "MemoryManage/AVStructPool.h"

#define NAL_HEADER_LEN 4

std::shared_ptr<PacketData> ConvertAVPacketToRawWithoutHeader(AVPacket* src);

std::shared_ptr<PacketData> ConvertAVPacketToRawWithoutHeader(std::shared_ptr <SharedAVPacket> src);

std::shared_ptr <SharedAVPacket> ConvertRawToAVPacketWithHeader(std::shared_ptr<PacketData> src);

const BYTE* FindNextNAL(const BYTE* data_start, const BYTE* data_end);

bool isSPSNALU(const BYTE* packet_data);

bool isPPSNALU(const BYTE* packet_data);

void CopyNALU(const BYTE* nalu_start, const BYTE* nalu_end, std::shared_ptr<PacketData>& buf);
