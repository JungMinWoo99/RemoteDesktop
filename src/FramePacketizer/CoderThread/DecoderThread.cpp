#include "FramePacketizer/CoderThread/DecoderThread.h"

using namespace std;

AVFrameHandlerThread::AVFrameHandlerThread(FrameDecoder& decoder, AVFrameProcessor& proc_obj) :decoder(decoder), proc_obj(proc_obj)
{
	is_processing = false;

	//frame memory size calculate
	auto c_context = decoder.getDecCodecContext();
	enum AVPixelFormat pix_fmt = c_context->pix_fmt;
	int bytes_per_pixel = av_get_bits_per_pixel(av_pix_fmt_desc_get(pix_fmt));
	int mem_size = c_context->width * c_context->height* bytes_per_pixel;

	recent_frame = make_shared<FrameData>(mem_size);
}

void AVFrameHandlerThread::StartHandle()
{
	is_processing = true;
	proc_thread = thread(&AVFrameHandlerThread::HandlerFunc, this);
}

void AVFrameHandlerThread::EndHandle()
{
	is_processing = false;
	proc_thread.join();
}

void AVFrameHandlerThread::HandlerFunc()
{
	SharedAVFrame recv_frame;
	while (is_processing)
	{
		while (decoder.SendFrame(recv_frame))
			proc_obj.FrameProcess(recv_frame);
	}
	decoder.FlushContext();
	while (decoder.SendFrame(recv_frame))
		proc_obj.FrameProcess(recv_frame);
}

PacketDecoderThread::PacketDecoderThread(FrameDecoder& decoder):decoder(decoder)
{
	is_processing = false;
}

void PacketDecoderThread::StartDecoding()
{
	is_processing = true;
	proc_thread = thread(&PacketDecoderThread::DecodeFunc, this);
}

void PacketDecoderThread::EndDecoding()
{
	is_processing = false;
	proc_thread.join();
}

void PacketDecoderThread::InputPacket(SharedAVPacket input)
{
	wait_que.push(input);
}

void PacketDecoderThread::DecodeFunc()
{
	SharedAVPacket packet;
	while (is_processing)
	{
		if (wait_que.pop(packet))
			decoder.DecodePacket(packet);
	}
}
