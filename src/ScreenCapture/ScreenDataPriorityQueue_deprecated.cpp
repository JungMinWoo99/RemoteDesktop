#include "ScreenCapture/ScreenDataPriorityQueue.h"

using namespace std;

bool operator<(shared_ptr<FrameData> t1, shared_ptr<FrameData> t2)
{
	return t1.get()->getCaptureTime() > t2.get()->getCaptureTime();
}

ScreenDataPriorityQueue::ScreenDataPriorityQueue(unsigned int max_buf_size):max_buf_size(max_buf_size)
{
}

void ScreenDataPriorityQueue::PushFrameData(shared_ptr<FrameData> input)
{
	lock_guard<mutex> lock(buf_mtx);

	buf.push(input);

	while (buf.size() > max_buf_size)
		buf.pop();
}

bool ScreenDataPriorityQueue::PopFrameData(shared_ptr<FrameData>& recv)
{
	lock_guard<mutex> lock(buf_mtx);
	bool ret;

	if (buf.empty())
		ret = false;
	else
	{
		recv = buf.top();
		buf.pop();
		ret = true;
	}

	return ret;
}