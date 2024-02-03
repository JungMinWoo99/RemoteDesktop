#include "Live555RtspServer/RtspServerContext.h"

using namespace std;

RtspServerContext::RtspServerContext()
{
	scheduler = BasicTaskScheduler::createNew();
	env = BasicUsageEnvironment::createNew(*scheduler);

	OutPacketBuffer::increaseMaxSizeTo(5242880);//구체적인 설정값을 아직 정하지 못함

	rtspServer = RTSPServer::createNew(*env, 554);
	if (rtspServer == NULL)
	{
		*env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
		exit(-1);
	}

	if (rtspServer->setUpTunnelingOverHTTP(80) || rtspServer->setUpTunnelingOverHTTP(8080)) 
	{
		*env << "\n(We use port " << rtspServer->httpServerPortNum() << " for optional RTSP-over-HTTP tunneling.)\n";
	}
	else 
	{
		*env << "\n(RTSP-over-HTTP tunneling is not available.)\n";
	}

	packet_source = new EncodedPacketFramedSource(*env);
	replicator = StreamReplicator::createNew(*env, packet_source, false);

	sms = ServerMediaSession::createNew(*env, "stream1");
	sub = new CaptureMediaSubsession(*env, replicator);
}

RtspServerContext::~RtspServerContext()
{
	Medium::close(rtspServer);
	Medium::close(replicator);

	env->reclaim();
	delete scheduler;
}

void RtspServerContext::Run()
{
	while (packet_source->GetSPS() == nullptr || packet_source->GetPPS() == nullptr);
	auto sps_data = packet_source->GetSPS();
	auto pps_data = packet_source->GetPPS();
	sub->setSPSNAL(sps_data->getMemPointer(), sps_data->getMemSize());
	sub->setPPSNAL(pps_data->getMemPointer(), sps_data->getMemSize());

	sub->setBitRate(RECOMMAND_BIT_RATE);

	sms->addSubsession(sub);
	rtspServer->addServerMediaSession(sms);

	char* url = rtspServer->rtspURL(sms);
	*env << "Play this stream using the URL \"" << url << "\"\n";
	delete[] url;

	env->taskScheduler().doEventLoop(&eventLoopWatchVariable);
}

EncodedPacketFramedSource::FramedSourcePacketHandler& RtspServerContext::GetPacketHandler()
{
	return packet_source->GetPacketHandler();
}
