/*
   This code was written with reference to the following source.
   source: https://infoarts.tistory.com/35
*/

#pragma once

#include "Live555RtspServer/EncodedPacketFramedSource.h"
#include "Live555RtspServer/CaptureMediaSubsession.h"

class RtspServerContext
{
public:
	RtspServerContext();
	~RtspServerContext();
	
	void Run();

	EncodedPacketFramedSource::FramedSourcePacketHandler& GetPacketHandler();

private:
	char eventLoopWatchVariable = 0;

	TaskScheduler* scheduler;
	UsageEnvironment* env;
	RTSPServer* rtspServer;

	EncodedPacketFramedSource* packet_source;
	StreamReplicator* replicator;
	ServerMediaSession* sms;
	CaptureMediaSubsession* sub;
};