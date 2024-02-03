#include "Live555RtspServer/EncodedPacketFramedSource.h"

using namespace std;

EncodedPacketFramedSource::EncodedPacketFramedSource(UsageEnvironment& env)
	: FramedSource(env), packet_handler(this)
{
	event_trigger_id = envir().taskScheduler().createEventTrigger(EncodedPacketFramedSource::DeliverFrameStub);
}

void EncodedPacketFramedSource::DeliverFrameStub(void* clientData)
{
	((EncodedPacketFramedSource*)clientData)->DeliverPacketToSrc();
}

void EncodedPacketFramedSource::DeliverPacketToSrc()
{
	if (!isCurrentlyAwaitingData())
		return;

	shared_ptr<PacketData> recv_packet;

	if (packet_handler.SendPacket(recv_packet))
	{
		auto p_data = recv_packet.get();

		if (p_data->getMemPointer() != NULL)
		{
			if (p_data->getMemSize() > fMaxSize)
			{
				fFrameSize = fMaxSize;
				fNumTruncatedBytes = p_data->getMemSize() - fMaxSize;
			}
			else
			{
				fFrameSize = p_data->getMemSize();
			}

			gettimeofday(&fPresentationTime, NULL);
			memcpy(fTo, p_data->getMemPointer(), fFrameSize);
		}
		else//if recv null packet, close FramedSource
		{
			fFrameSize = 0;
			fTo = NULL;
			handleClosure(this);
		}
	}
	else
	{
		fFrameSize = 0;
	}

	if (fFrameSize > 0)
		FramedSource::afterGetting(this);
}

void EncodedPacketFramedSource::doGetNextFrame()
{
	DeliverPacketToSrc();
}

void EncodedPacketFramedSource::doStopGettingFrames()
{
	FramedSource::doStopGettingFrames();
}

std::shared_ptr<PacketData> EncodedPacketFramedSource::GetSPS()
{
	return sps_data;
}

std::shared_ptr<PacketData> EncodedPacketFramedSource::GetPPS()
{
	return pps_data;
}

EncodedPacketFramedSource::FramedSourcePacketHandler& EncodedPacketFramedSource::GetPacketHandler()
{
	return packet_handler;
}

EncodedPacketFramedSource::FramedSourcePacketHandler::FramedSourcePacketHandler(EncodedPacketFramedSource* this_ptr)
	:this_ptr(this_ptr), packet_queue("EncodedPacketFramedSource")
{
}

void EncodedPacketFramedSource::FramedSourcePacketHandler::PacketProcess(AVPacket* input)
{
	if(this_ptr->has_sps_data && this_ptr->has_pps_data)
	{
		shared_ptr<PacketData> p_data = ConvertAVPacketToRawWithoutHeader(input);
		packet_queue.push(p_data);
		this_ptr->envir().taskScheduler().triggerEvent(this_ptr->event_trigger_id, this_ptr);
	}
	else
	{
		shared_ptr<PacketData> p_data = make_shared<PacketData>(input);
		const BYTE* p_start = p_data.get()->getMemPointer();
		const BYTE* p_end = p_data.get()->getMemPointer() + p_data.get()->getMemSize();

		p_start = FindNextNAL(p_start, p_end);
		const BYTE* nalu_end = FindNextNAL(p_start + 1, p_end);
		while(true)
		{
			if (p_start == p_end)
				break;

			if (isSPSNALU(p_start))
			{
				CopyNALU(p_data.get()->getMemPointer(), nalu_end, this_ptr->sps_data);
				this_ptr->has_sps_data = true;
			}
			else if (isPPSNALU(p_start))
			{
				CopyNALU(p_data.get()->getMemPointer(), nalu_end, this_ptr->pps_data);
				this_ptr->has_pps_data = true;
			}
			p_start = nalu_end;
			nalu_end = FindNextNAL(p_start + 1, p_end);
		}
	}
}

bool EncodedPacketFramedSource::FramedSourcePacketHandler::SendPacket(shared_ptr<PacketData>& output)
{
	return packet_queue.pop(output);
}
