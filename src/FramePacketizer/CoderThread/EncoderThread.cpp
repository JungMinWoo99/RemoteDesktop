#include "FramePacketizer/CoderThread/EncoderThread.h"

#include <iostream>

using namespace std;

std::condition_variable cv;

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
	//You should not create more than one thread calling this function
	static mutex mtx;
	lock_guard<mutex> lock(mtx);
	
	shared_ptr<SharedAVPacket> recv_packet;
	while (is_processing)
	{	
		if (encoder.SendPacketBlocking(recv_packet))
			proc_obj.PacketProcess(recv_packet.get()->getPointer());
	}
}

FrameEncoderThread::FrameEncoderThread(FrameEncoder& encoder) :encoder(encoder), wait_que("FrameEncoderThread")
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

void FrameEncoderThread::InputFrame(std::shared_ptr<SharedAVFrame> input)
{
	wait_que.push(input);
}

void FrameEncoderThread::EncoderFunc()
{
	//You should not create more than one thread calling this function
	static mutex mtx;
	lock_guard<mutex> lock(mtx);

	shared_ptr<SharedAVFrame> frame;
	while (is_processing)
	{
		if (wait_que.wait_and_pop_utill_not_empty(frame))
		{
			bool ret = encoder.EncodeFrame(frame);
			if (ret == false)
			{
				cout << "frame encoding fail" << endl;
				ret = encoder.EncodeFrame(frame);
				if (ret == false)
				{
					cout << "frame encoding fail again!" << endl;

					//error handler required
					//add after
				}
			}
		}
	}
	
	while (wait_que.pop(frame))
	{
		bool ret = encoder.EncodeFrame(frame);
		if (ret == false)
		{
			cout << "frame encoding fail" << endl;
			ret = encoder.EncodeFrame(frame);
			if (ret == false)
			{
				cout << "frame encoding fail again!" << endl;

				//error handler required
				//add after
			}
		}
	}
}

