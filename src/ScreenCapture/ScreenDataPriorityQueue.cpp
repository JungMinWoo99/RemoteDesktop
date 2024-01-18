#include "ScreenCapture/ScreenDataPriorityQueue.h"

using namespace std;

bool operator<(shared_ptr<FrameData> t1, shared_ptr<FrameData> t2)
{
	return t1.get()->getCaptureTime() > t2.get()->getCaptureTime();
}

void ScreenDataPriorityQueue::PushFrameData(shared_ptr<FrameData> input)
{
	buf_mtx.lock();

	buf.push(input);

	while (buf.size() > max_buf_size)
		buf.pop();

	buf_mtx.unlock();
}

void ScreenDataPriorityQueue::PopFrameData(shared_ptr<FrameData>& recv)
{
	pop_frame_mtx.lock();

	while (buf.size() == 0);

	buf_mtx.lock();

	recv = buf.top();

	buf.pop();

	buf_mtx.unlock();

	pop_frame_mtx.unlock();
}