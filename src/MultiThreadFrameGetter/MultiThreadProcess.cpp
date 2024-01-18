#include "MultiThreadFrameGetter/MultiThreadProcess.h"

#include <queue>
#include <iostream>

using namespace std;

MultiThreadProcess::MultiThreadProcess(ScreenDataBuffer& input_buf, ScreenDataBuffer& output_buf, const BITMAPINFO& bmi, unsigned int thread_num)
	: input_buf(input_buf), output_buf(output_buf), thread_num(thread_num)
{
	for (unsigned int i = 0; i < thread_num; i++)
		prc_obj_vector.push_back(new ScreenToMat(bmi));
}

void MultiThreadProcess::StartProcess()
{
	process_continue = true;

	prc_thread = thread(&MultiThreadProcess::ProcessFunc, this);
}

void MultiThreadProcess::EndProcess()
{
	process_continue = false;

	prc_thread.join();
}

MultiThreadProcess::~MultiThreadProcess()
{
	for (unsigned int i = 0; i < thread_num; i++)
		delete prc_obj_vector[i];
}

void MultiThreadProcess::ProcessFunc()
{
	queue<std::thread*> thread_que;

	auto cap_screen = [this](ScreenToMat* prc_obj, thread* prev_thread)
		{
			shared_ptr<FrameData> frame;
			input_buf.SendFrameData(frame);
			prc_obj->RecvFrameData(frame);
			prc_obj->getYUVFrameData(frame);
			if (prev_thread != NULL && prev_thread->joinable())
				prev_thread->join();
			output_buf.RecvFrameData(frame);
		};

	thread* prev_thread = NULL;
	int thread_count = 0;
	while (process_continue)
	{

		prev_thread = new thread(cap_screen, prc_obj_vector[thread_count], prev_thread);

		thread_count = (thread_count + 1) % thread_num;
		thread_que.push(prev_thread);

		while (thread_que.size() == thread_num)
		{
			while (!thread_que.front()->joinable())
				thread_que.pop();
		}
	}

	while (thread_que.size() > 1)
	{
		while (thread_que.front()->joinable());

		thread_que.pop();
	}
	thread_que.front()->join();//ë§ˆì?ë§??¤ë ˆ?œë? join??ì¤??¤ë ˆ?œê? ?†ìœ¼ë¯€ë¡?
}