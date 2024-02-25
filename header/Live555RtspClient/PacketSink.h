/*
   This code was written with reference to the following source.
   source: https://github.com/rgaufman/live555/blob/master/testProgs/testRTSPClient.cpp
*/
#pragma once

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "MemoryManage/AVStructPool.h"
#include "MemoryManage/AVPacketManage.h"
#include "FramePacketizer/CoderThread/DecoderThread.h"

class DummySink : public MediaSink {
public:
	static DummySink* createNew(PacketDecoderThread& packet_processor,
		UsageEnvironment& env,
		MediaSubsession& subsession, // identifies the kind of data that's being received
		char const* streamId = NULL); // identifies the stream itself (optional)

private:
	DummySink(PacketDecoderThread& packet_processor, UsageEnvironment& env, MediaSubsession& subsession, char const* streamId);
	// called only by "createNew()"
	virtual ~DummySink();

	static void afterGettingFrame(void* clientData, unsigned frameSize,
		unsigned numTruncatedBytes,
		struct timeval presentationTime,
		unsigned durationInMicroseconds);
	void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
		struct timeval presentationTime, unsigned durationInMicroseconds);

private:
	// redefined virtual functions:
	virtual Boolean continuePlaying();

private:
	PacketDecoderThread& packet_processor;
	u_int8_t* fReceiveBuffer;
	MediaSubsession& fSubsession;
	char* fStreamId;
};