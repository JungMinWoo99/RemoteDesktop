#include "FramePacketizer/CoderThread/DecoderThread.h"

#include <iostream>

using namespace std;

AVFrameHandlerThread::AVFrameHandlerThread(FrameDecoder& decoder, AVFrameProcessor& proc_obj) :decoder(decoder), proc_obj(proc_obj)
{
	is_processing = false;
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
		if (decoder.SendFrameBlocking(recv_frame))
			proc_obj.FrameProcess(recv_frame);
	}
}

PacketDecoderThread::PacketDecoderThread(FrameDecoder& decoder):decoder(decoder), wait_que("PacketDecoderThread")
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
		if (wait_que.wait_and_pop_utill_not_empty(packet))
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
