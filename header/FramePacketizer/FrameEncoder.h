#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
}

#include "Constant/VideoConstants.h"
#include "ScreenCapture/FrameData.h"
#include "FramePacketizer/AVStructPool.h"

#include<queue>
#include<memory>
#include<mutex>

class FrameEncoder
{
public:
	FrameEncoder(int w = DEFALUT_WIDTH, int h = DEFALUT_HEIGHT, int frame_rate = DEFALUT_FRAME_RATE, AVCodecID coedec_id = AV_CODEC_ID_H264);
	
	_Check_return_ bool EncodeFrame(std::shared_ptr<SharedAVFrame> yuv_frame_data);

	_Check_return_ bool SendPacket(std::shared_ptr<SharedAVPacket>& packet);

	void FlushContext();

	const AVCodec* getEncCodec();

	AVCodecContext* getEncCodecContext();

	size_t getBufferSize();

	~FrameEncoder();

private:
	_Check_return_ bool FillPacketBuf();

	const AVCodec* enc_codec;
	AVCodecContext* enc_context;

	AVStructPool<AVPacket*>& empty_packet_buf = AVStructPool<AVPacket*>::getInstance();

	MutexQueue<std::shared_ptr<SharedAVPacket>> enced_packet_buf;

	std::mutex encoder_mtx;

	int frame_rate;
};