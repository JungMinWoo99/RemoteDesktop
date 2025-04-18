#pragma once

#include <thread>

#include "FramePacketizer/FrameDecoder.h"
#include "MemoryManage/AVFrameManage.h"

class AVFrameProcessor abstract
{
public:
	virtual void FrameProcess(std::shared_ptr<SharedAVFrame>) abstract;
};

class AVFrameHandlerThread
{
public:
	AVFrameHandlerThread(FrameDecoder& decoder, AVFrameProcessor& proc_obj);
	void StartHandle();
	void EndHandle();

private:
	FrameDecoder& decoder;
	AVFrameProcessor& proc_obj;

	std::thread proc_thread;

	bool is_processing;

	void HandlerFunc();
};

class PacketDecoderThread
{
public:
	PacketDecoderThread(FrameDecoder& decoder);
	void StartDecoding();
	void EndDecoding();
	void InputPacket(std::shared_ptr<SharedAVPacket> input);

private:
	FrameDecoder& decoder;
	MutexQueue<std::shared_ptr<SharedAVPacket>> wait_que;

	std::thread proc_thread;

	bool is_processing;

	void DecodeFunc();
};
