/*
   This code was written with reference to the following source.
   source: https://github.com/rgaufman/live555/blob/master/testProgs/testRTSPClient.cpp
*/
#pragma once
/*
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include <string>

class ourRTSPClient : public RTSPClient
{
public:
	ourRTSPClient(UsageEnvironment* env, std::string rtspURL,
		std::string applicationName = "RemoteDesktop",
		int verbosityLevel = RTSP_CLIENT_VERBOSITY_LEVEL,
		portNumBits tunnelOverHTTPPortNum = 0);
private:
	MediaSubsessionIterator* iter;
	MediaSession* session;
	MediaSubsession* subsession;
};
*/

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "FramePacketizer/CoderThread/DecoderThread.h"

#ifdef _DEBUG
#define RTSP_CLIENT_VERBOSITY_LEVEL 1
#else
#define RTSP_CLIENT_VERBOSITY_LEVEL 0
#endif

// Define a class to hold per-stream state that we maintain throughout each stream's lifetime:

class StreamClientState {
public:
	StreamClientState(PacketDecoderThread& packet_proc_obj);
	virtual ~StreamClientState();

public:
	PacketDecoderThread& packet_proc_obj;
	MediaSubsessionIterator* iter;
	MediaSession* session;
	MediaSubsession* subsession;
	TaskToken streamTimerTask;
	double duration;
};

// If you're streaming just a single stream (i.e., just from a single URL, once), then you can define and use just a single
// "StreamClientState" structure, as a global variable in your application.  However, because - in this demo application - we're
// showing how to play multiple streams, concurrently, we can't do that.  Instead, we have to have a separate "StreamClientState"
// structure for each "RTSPClient".  To do this, we subclass "RTSPClient", and add a "StreamClientState" field to the subclass:

class ourRTSPClient : public RTSPClient {
public:
	static ourRTSPClient* createNew(PacketDecoderThread& packet_proc_obj, UsageEnvironment& env, char const* rtspURL,
		int verbosityLevel = RTSP_CLIENT_VERBOSITY_LEVEL,
		char const* applicationName = NULL,
		portNumBits tunnelOverHTTPPortNum = 0);

	// Used to iterate through each stream's 'subsessions', setting up each one:
	static void setupNextSubsession(RTSPClient* rtspClient);

	// Used to shut down and close a stream (including its "RTSPClient" object):
	static void shutdownStream(RTSPClient* rtspClient, int exitCode = 1);

protected:
	ourRTSPClient(PacketDecoderThread& packet_proc_obj, UsageEnvironment& env, char const* rtspURL,
		int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum);
	// called only by createNew();
	virtual ~ourRTSPClient();

public:
	StreamClientState scs;
};