#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
}

#include "Constant/VideoConstants.h"
#include "MemoryManage/Framedata.h"
#include "MemoryManage/AVStructPool.h"

#include<queue>
#include<memory>
#include<mutex>
#include <fstream>

class FrameEncoder
{
public:
	FrameEncoder(std::string encoder_name);

	FrameEncoder(int w = DEFALUT_WIDTH, int h = DEFALUT_HEIGHT, int frame_rate = DEFALUT_FRAME_RATE, AVCodecID coedec_id = AV_CODEC_ID_H264);
	
	_Check_return_ virtual bool EncodeFrame(std::shared_ptr<SharedAVFrame> input) final;

	_Check_return_ virtual bool SendPacket(std::shared_ptr<SharedAVPacket>& packet) final;

	_Check_return_ virtual bool SendPacketBlocking(std::shared_ptr<SharedAVPacket>& packet) final;

	virtual void FlushContext() final;

	virtual const AVCodec* getEncCodec() final;

	virtual AVCodecContext* getEncCodecContext()final;

	virtual size_t getBufferSize() final;

	virtual ~FrameEncoder();

protected:
	virtual void PrintLog(std::string log) final;

	static std::ofstream log_stream;

	const AVCodec* enc_codec;
	AVCodecContext* enc_context;

	int frame_rate;

private:
	_Check_return_ bool FillPacketBuf();

	AVStructPool<AVPacket*>& empty_packet_buf = AVStructPool<AVPacket*>::getInstance();

	MutexQueue<std::shared_ptr<SharedAVPacket>> enced_packet_buf;

	std::mutex encoder_mtx;

	std::string encoder_name;
};