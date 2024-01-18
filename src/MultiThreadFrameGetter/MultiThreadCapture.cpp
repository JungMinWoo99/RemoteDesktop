#include <queue>
#include <iostream>

#include "MultiThreadFrameGetter/MultiThreadCapture.h"

using namespace std;

MultiThreadCapture::MultiThreadCapture(ScreenDataBuffer& data_buf, unsigned int thread_num) :
	data_buf(data_buf), thread_num(thread_num)
{
	for (unsigned int i = 0; i < thread_num; i++)
		cap_obj_vector.push_back(new ScreenCapture());
}

void MultiThreadCapture::StartCapture()
{
	capture_continue = true;
	next_cap_start = chrono::high_resolution_clock::now() + chrono::microseconds(1000000 / DEFALUT_FRAME_RATE);

	cap_thread = thread(&MultiThreadCapture::CaptureFunc,this);
}

void MultiThreadCapture::EndCapture()
{
	capture_continue = false;
	
	cap_thread.join();
}

const ScreenCapture& MultiThreadCapture::getCapInfo()
{
	return *cap_obj_vector[0];
}

ScreenDataBuffer& MultiThreadCapture::getDataBuf()
{
	return data_buf;
}

MultiThreadCapture::~MultiThreadCapture()
{
	for (unsigned int i = 0; i < thread_num; i++)
		delete cap_obj_vector[i];
}

void MultiThreadCapture::CaptureFunc()
{
	queue<std::thread*> thread_que;

	auto cap_screen = [this](ScreenCapture* cap_obj, chrono::high_resolution_clock::time_point thread_start, thread* prev_thread)
	{
		this_thread::sleep_until(thread_start);
		shared_ptr<FrameData> frame = cap_obj->CaptureCurrentScreen();
		if(prev_thread != NULL && prev_thread->joinable())
			prev_thread->join();
		this->getDataBuf().RecvFrameData(frame);
	};

	thread* prev_thread = NULL;
	int thread_count = 0;
	while (capture_continue)
	{
		
		prev_thread = new thread(cap_screen, cap_obj_vector[thread_count], next_cap_start, prev_thread);
		
		next_cap_start = next_cap_start + chrono::microseconds(1000000 / (DEFALUT_FRAME_RATE * TIME_STEP_DIVISION));
		thread_count = (thread_count + 1) % thread_num;
		thread_que.push(prev_thread);

		while (thread_que.size() == thread_num)
		{
			while (!thread_que.front()->joinable())
				thread_que.pop();
		}
	}

	while (thread_que.size()>1)
	{
		while (thread_que.front()->joinable());

		thread_que.pop();
	}
	thread_que.front()->join();
}