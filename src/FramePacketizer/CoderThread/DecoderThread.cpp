#include "FramePacketizer/CoderThread/DecoderThread.h"

#include <iostream>

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
	//You should not create more than one thread calling this function
	static mutex mtx;
	lock_guard<mutex> lock(mtx);

	shared_ptr<SharedAVFrame> recv_frame;
	while (is_processing)
	{
		if (decoder.SendFrame(recv_frame))
			proc_obj.FrameProcess(recv_frame.get()->getPointer());
	}
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

void PacketDecoderThread::InputPacket(std::shared_ptr<SharedAVPacket> input)
{
	wait_que.push(input);
}

void PacketDecoderThread::DecodeFunc()
{
	//You should not create more than one thread calling this function
	static mutex mtx;
	lock_guard<mutex> lock(mtx);

	shared_ptr<SharedAVPacket> packet;

	while (is_processing)
	{
		if (wait_que.pop(packet))
		{
			bool ret = decoder.DecodePacket(packet);
			if (ret == false)
			{
				cout << "packet decoding fail" << endl;
				ret = decoder.DecodePacket(packet);
				if (ret == false)
				{
					cout << "packet decoding fail again!" << endl;

					//error handler required
					//add after
				}
			}
		}
	}

	while (wait_que.pop(packet))
	{
		bool ret = decoder.DecodePacket(packet);
		if (ret == false)
		{
			cout << "frame encoding fail" << endl;
			ret = decoder.DecodePacket(packet);
			if (ret == false)
			{
				cout << "frame encoding fail again!" << endl;

				//error handler required
				//add after
			}
		}
	}
}
