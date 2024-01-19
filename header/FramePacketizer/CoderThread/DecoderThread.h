#include <thread>

#include "FramePacketizer/FrameDecoder.h"
#include "FramePacketizer/AVFrameManage.h"

class AVFrameProcessor abstract
{
public:
	virtual void FrameProcess(SharedAVFrame) abstract;
};

class AVFrameHandlerThread
{
public:
	AVFrameHandlerThread(FrameDecoder& decoder, AVFrameProcessor& proc_obj);
	void StartHandle();
	void EndHandle();

protected:
	FrameDecoder& decoder;
	AVFrameProcessor& proc_obj;
	
	std::shared_ptr<FrameData> recent_frame;

	std::thread proc_thread;

	bool is_processing;

	virtual void HandlerFunc();
};

class PacketDecoderThread
{
public:
	PacketDecoderThread(FrameDecoder& decoder);
	void StartDecoding();
	void EndDecoding();
	void InputPacket(SharedAVPacket input);

protected:
	FrameDecoder& decoder;
	MutexQueue<SharedAVPacket> wait_que;

	std::thread proc_thread;

	bool is_processing;

	void DecodeFunc();
};