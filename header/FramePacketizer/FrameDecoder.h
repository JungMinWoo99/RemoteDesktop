#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include "Constant/VideoConstants.h"
#include "ScreenCapture/FrameData.h"
#include "MutexQueue/MutexQueue.h"
#include "FramePacketizer/AVStructPool.h"

#include<memory>

class FrameDecoder
{
public:
	FrameDecoder(int w = DEFALUT_WIDTH, int h = DEFALUT_HEIGHT, int frame_rate = DEFALUT_FRAME_RATE, AVCodecID coedec_id = AV_CODEC_ID_H264);

	bool DecodePacket(std::shared_ptr<SharedAVPacket> input);

	bool SendFrame(std::shared_ptr<SharedAVFrame>& frame);

	void FlushContext();

	const AVCodec* getDecCodec();

	AVCodecContext* getDecCodecContext();

	~FrameDecoder();
private:
	_Check_return_ bool FillFrameBuf();

	const AVCodec* dec_codec;
	AVCodecContext* dec_context;

	AVStructPool<AVFrame*>& empty_frame_buf = AVStructPool<AVFrame*>::getInstance();

	MutexQueue<std::shared_ptr<SharedAVFrame>> deced_frame_buf;

	std::mutex decoder_mtx;

	int frame_rate;
};