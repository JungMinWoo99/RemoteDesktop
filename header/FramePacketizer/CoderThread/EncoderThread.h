#include <thread>

#include "FramePacketizer/FrameEncoder.h"
#include "FramePacketizer/AVFrameManage.h"

class AVPacketProcessor abstract
{
public:
	virtual void PacketProcess(SharedAVPacket) abstract;
};

class AVPacketHandlerThread
{
public:
	AVPacketHandlerThread(FrameEncoder& encoder, AVPacketProcessor& proc_obj);
	void StartHandle();
	void EndHandle();

private:
	FrameEncoder& encoder;
	AVPacketProcessor& proc_obj;

	std::thread proc_thread;

	bool is_processing;

	void HandlerFunc();
};

class FrameEncoderThread
{
public:
	FrameEncoderThread(FrameEncoder& encoder);
	void StartEncoding();
	void EndEncoding();
	void InputFrame(SharedAVFrame input);

private:
	FrameEncoder& encoder;
	MutexQueue<SharedAVFrame> wait_que;

	std::thread proc_thread;

	bool is_processing;

	void EncoderFunc();
};