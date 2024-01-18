#include "FramePacketizer/CoderThread/EncoderThread.h"

using namespace std;

AVPacketHandlerThread::AVPacketHandlerThread(FrameEncoder& encoder, AVPacketProcessor& proc_obj) :encoder(encoder), proc_obj(proc_obj)
{
	is_processing = false;
}

void AVPacketHandlerThread::StartHandle()
{
	is_processing = true;
	proc_thread = thread(&AVPacketHandlerThread::HandlerFunc, this);
}

void AVPacketHandlerThread::EndHandle()
{
	is_processing = false;
	proc_thread.join();
}

void AVPacketHandlerThread::HandlerFunc()
{
	SharedAVPacket recv_packet;
	while (is_processing)
	{	
		while (encoder.SendPacket(recv_packet))
			proc_obj.PacketProcess(recv_packet);
	}
	encoder.FlushContext();
	while (encoder.SendPacket(recv_packet))
		proc_obj.PacketProcess(recv_packet);
}

FrameEncoderThread::FrameEncoderThread(FrameEncoder& encoder):encoder(encoder)
{
	is_processing = false;
}

void FrameEncoderThread::StartEncoding()
{
	is_processing = true;
	proc_thread = thread(&FrameEncoderThread::EncoderFunc, this);
}

void FrameEncoderThread::EndEncoding()
{
	is_processing = false;
	proc_thread.join();
}

void FrameEncoderThread::InputFrame(SharedAVFrame input)
{
	wait_que.push(input);
}

void FrameEncoderThread::EncoderFunc()
{
	SharedAVFrame frame;
	while (is_processing)
	{
		if (wait_que.pop(frame))
		{
			encoder.EncodeFrame(frame);
		}
	}
}

