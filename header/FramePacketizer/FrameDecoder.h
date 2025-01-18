#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include <fstream>
#include<memory>


#include "Constant/VideoConstants.h"
#include "MemoryManage/Framedata.h"
#include "MutexQueue/MutexQueue.h"
#include "MemoryManage/AVStructPool.h"


class FrameDecoder
{
public:
	FrameDecoder(std::string decoder_name);

	_Check_return_ bool DecodePacket(std::shared_ptr<SharedAVPacket> input);

	_Check_return_ bool SendFrame(std::shared_ptr<SharedAVFrame>& frame);

	_Check_return_ bool SendFrameBlocking(std::shared_ptr<SharedAVFrame>& frame);

	void FlushContext();

	const AVCodec* getDecCodec();

	AVCodecContext* getDecCodecContext();

	~FrameDecoder();

protected:
	static std::ofstream log_stream;

	const AVCodec* dec_codec;
	AVCodecContext* dec_context;

	virtual void PrintLog(std::string log) final;

private:
	_Check_return_ bool FillFrameBuf();

	AVStructPool<AVFrame*>& empty_frame_buf = AVStructPool<AVFrame*>::getInstance();

	MutexQueue<std::shared_ptr<SharedAVFrame>> deced_frame_buf;

	std::mutex decoder_mtx;

	std::string decoder_name;
};