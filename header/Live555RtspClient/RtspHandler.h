/*
   This code was written with reference to the following source.
   source: https://github.com/rgaufman/live555/blob/master/testProgs/testRTSPClient.cpp
*/
#pragma once

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"

// Forward function definitions:

// RTSP response handlers
namespace RtspResponseHandler
{
	void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString);
	void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString);
	void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString);
}

// Other event handler functions
namespace Live555EventHandler
{
	// called when a stream's subsession (e.g., audio or video substream) ends
	void subsessionAfterPlaying(void* clientData); 
	// called when a RTCP "BYE" is received for a subsession
	void subsessionByeHandler(void* clientData, char const* reason); 
	// called at the end of a stream's expected duration (if the stream has not already signaled its end using a RTCP "BYE")
	void streamTimerHandler(void* clientData); 
}

// A function that outputs a string that identifies each stream (for debugging output).  Modify this if you wish:
UsageEnvironment& operator<<(UsageEnvironment& env, const RTSPClient& rtspClient);

// A function that outputs a string that identifies each subsession (for debugging output).  Modify this if you wish:
UsageEnvironment& operator<<(UsageEnvironment& env, const MediaSubsession& subsession);