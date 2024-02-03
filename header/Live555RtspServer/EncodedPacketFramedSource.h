#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "UsageEnvironment.hh"
#include "liveMedia.hh"

#include "FramePacketizer/FrameEncoder.h"
#include "FramePacketizer/AVPacketManage.h"
#include "FramePacketizer/CoderThread/EncoderThread.h"

class EncodedPacketFramedSource : public FramedSource
{
public:
	class FramedSourcePacketHandler : public AVPacketProcessor
	{
	public:
		FramedSourcePacketHandler(EncodedPacketFramedSource* this_ptr);

		virtual void PacketProcess(AVPacket* input);
		bool SendPacket(std::shared_ptr<PacketData>& output);

	private:
		MutexQueue<std::shared_ptr<PacketData>> packet_queue;
		EncodedPacketFramedSource* this_ptr;
	};

	EncodedPacketFramedSource(UsageEnvironment& env);
	~EncodedPacketFramedSource() = default;

	std::shared_ptr<PacketData> GetSPS();
	std::shared_ptr<PacketData> GetPPS();
	FramedSourcePacketHandler& GetPacketHandler();

private:
	static void DeliverFrameStub(void* clientData);

	void DeliverPacketToSrc();
	void doGetNextFrame() override;
	void doStopGettingFrames() override;

	friend class FramedSourcePacketHandler;

	FramedSourcePacketHandler packet_handler;
	EventTriggerId event_trigger_id;

	bool has_sps_data = false;
	bool has_pps_data = false;

	std::shared_ptr<PacketData> sps_data = nullptr;
	std::shared_ptr<PacketData> pps_data = nullptr;
};
