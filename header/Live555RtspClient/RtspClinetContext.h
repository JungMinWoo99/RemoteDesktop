/*
   This code was written with reference to the following source.
   source: https://github.com/rgaufman/live555/blob/master/testProgs/testRTSPClient.cpp
*/
#pragma once

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "FramePacketizer/CoderThread/DecoderThread.h"
#include <string>
class RtspClinetContext
{
public:
	RtspClinetContext( PacketDecoderThread& packet_proc_obj);

	// The main streaming routine (for each "rtsp://" URL):
	void openURL(char const* progName, char const* rtspURL);

	void Run();

	// Used to iterate through each stream's 'subsessions', setting up each one:
	static void setupNextSubsession(RTSPClient* rtspClient);

	// Used to shut down and close a stream (including its "RTSPClient" object):
	static void shutdownStream(RTSPClient* rtspClient, int exitCode = 1);

private:
	PacketDecoderThread& packet_proc_obj;

	UsageEnvironment* env;
	TaskScheduler* scheduler;

	char eventLoopWatchVariable = 0;
	static unsigned int rtspClientCount;
};