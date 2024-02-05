#include "MultiThreadFrameGetter/PeriodicDataCollector.h"

#define ONE_SECOND_IN_NANOSECONDS std::chrono::nanoseconds(1000000000)
#define ALLOWABLE_LIMIT_FOR_SKIPPED_FRAME 2

using namespace std;

PeriodicDataCollector::PeriodicDataCollector(ScreenDataBuffer& input_buf, ScreenDataBuffer& output_buf, unsigned int collect_rate)
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
	static unsigned int next_pts = 0;

	shared_ptr<FrameData> frame;

	while (collect_continue)
	{
		if(input_buf.SendFrameDataBlocking(frame))
		{
			auto frame_pts = frame.get()->getCaptureTime() / (ONE_SECOND_IN_NANOSECONDS / collect_rate);
			auto div_remain = frame.get()->getCaptureTime() % (ONE_SECOND_IN_NANOSECONDS / collect_rate);
			if (div_remain > (ONE_SECOND_IN_NANOSECONDS / 2))
				frame_pts++;

			if (frame_pts >= next_pts)
			{
				if ((frame_pts - next_pts) > ALLOWABLE_LIMIT_FOR_SKIPPED_FRAME)
				{
					cout << "PeriodicDataCollector error: Too many frames skipped." << endl;
					cout << "Skipped frame: "<< frame_pts - next_pts << endl;
				}
				
				while (frame_pts > next_pts)
				{
					shared_ptr<FrameData> tem = make_shared<FrameData>(frame.get()->getMemSize());
					memcpy(tem.get()->getMemPointer(), frame.get()->getMemPointer(), frame.get()->getMemSize());
					tem.get()->setCaptureTime(frame.get()->getCaptureTime());
					tem.get()->setPts(next_pts);
					next_pts++;
					output_buf.RecvFrameData(tem);
				}

				frame.get()->setPts(next_pts);
				next_pts++;
				output_buf.RecvFrameData(frame);
			}
			else if((frame_pts+1) < next_pts)
			{
				cout << "PeriodicDataCollector fatal error: An older frame than the previously input one has been received" << frame_pts<< ' ' << next_pts << endl;
				exit(-1);
			}

			
		}
	}
}