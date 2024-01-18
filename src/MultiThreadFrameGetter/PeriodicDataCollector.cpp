#include "MultiThreadFrameGetter/PeriodicDataCollector.h"

using namespace std;
PeriodicDataCollector::PeriodicDataCollector(ScreenDataBuffer& input_buf, ScreenDataBuffer& output_buf, int collect_rate)
	: input_buf(input_buf), output_buf(output_buf), collect_rate(collect_rate)
{

}

void PeriodicDataCollector::StartCollect()
{
	collect_continue = true;

	clt_thread = thread(&PeriodicDataCollector::CollectFunc, this);
}

void PeriodicDataCollector::EndCollect()
{
	collect_continue = false;

	clt_thread.join();
}

ScreenDataBuffer& PeriodicDataCollector::getOutputBuf()
{
	return output_buf;
}

void PeriodicDataCollector::CollectFunc()
{
	shared_ptr<FrameData> frame;
	auto until = chrono::high_resolution_clock::now() + chrono::microseconds(1000000 / collect_rate);
	while (collect_continue)
	{
		this_thread::sleep_until(until);
		until = until + chrono::microseconds(1000000 / collect_rate);
		if(input_buf.SendFrameData(frame))
			output_buf.RecvFrameData(frame);
	}
}