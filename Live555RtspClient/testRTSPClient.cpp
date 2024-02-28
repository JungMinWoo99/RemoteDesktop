/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// Copyright (c) 1996-2023, Live Networks, Inc.  All rights reserved
// A demo application, showing how to create and run a RTSP client (that can potentially receive multiple streams concurrently).
//
// NOTE: This code - although it builds a running application - is intended only to illustrate how to develop your own RTSP
// client application.  For a full-featured RTSP client application - with much more functionality, and many options - see
// "openRTSP": http://www.live555.com/openRTSP/

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "Live555RtspClient/RtspHandler.h"

#include "MemoryManage/AVStructPool.h"
#include "MemoryManage/AVPacketManage.h"
#include "FramePacketizer/CoderThread/DecoderThread.h"
#include "MultiThreadFrameGetter/CaptureThread.h"
#include "MultiThreadFrameGetter/PeriodicDataCollector.h"
#include "MemoryManage/PixFmtConverter.h"
#include "FramePacketizer/FrameDecoder.h"
#include "MemoryManage/AVFrameManage.h"
#include "FramePacketizer/CoderThread/DecoderThread.h"
#include "ScreenPrinter/WinScreenPrinter.h"
#include "ScreenCapture/WinScreenCapture.h"

#pragma warning(disable: 4996)



class RTPFrameProcessor : public AVFrameProcessor
{
public:
	RTPFrameProcessor(const BITMAPINFO& bmi)
		:s_printer(DEFALUT_WIDTH, DEFALUT_HEIGHT, bmi, frame_ref)
	{
		yuv_frame_data = std::make_shared<VideoFrameData>(DEFALUT_WIDTH * DEFALUT_HEIGHT * 4 / 2 * 3 / 4);
		s_printer.StartPrint();
	}

	~RTPFrameProcessor()
	{
	}

	void FrameProcess(AVFrame* frame) override
	{
		//std::this_thread::sleep_for(std::chrono::microseconds(1000000 / DEFALUT_FRAME_RATE));
		CopyAVFrameToRaw(frame, yuv_frame_data);
		frame_ref = cnv.ConvertYUVToBGR(yuv_frame_data);
	}

	void EndEncoding()
	{
		s_printer.EndPrint();
	}
private:
	WinScreenPrinter s_printer;
	std::shared_ptr<VideoFrameData> yuv_frame_data;
	PixFmtConverter cnv;
	std::shared_ptr<VideoFrameData> frame_ref;
};

WinScreenCapture* capture_obj;
FrameDecoder* decoding_obj;
RTPFrameProcessor* prc_obj;
AVFrameHandlerThread* frm_thr;

// Forward function definitions:

// RTSP 'response handlers':
void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString);
void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString);
void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString);

// Other event handler functions:
void subsessionAfterPlaying(void* clientData); // called when a stream's subsession (e.g., audio or video substream) ends
void subsessionByeHandler(void* clientData, char const* reason);
// called when a RTCP "BYE" is received for a subsession
void streamTimerHandler(void* clientData);
// called at the end of a stream's expected duration (if the stream has not already signaled its end using a RTCP "BYE")

// The main streaming routine (for each "rtsp://" URL):
void openURL(UsageEnvironment& env, char const* progName, char const* rtspURL);

// Used to iterate through each stream's 'subsessions', setting up each one:
void setupNextSubsession(RTSPClient* rtspClient);

// Used to shut down and close a stream (including its "RTSPClient" object):
void shutdownStream(RTSPClient* rtspClient, int exitCode = 1);

void usage(UsageEnvironment& env, char const* progName) {
	env << "Usage: " << progName << " <rtsp-url-1> ... <rtsp-url-N>\n";
	env << "\t(where each <rtsp-url-i> is a \"rtsp://\" URL)\n";
}

char eventLoopWatchVariable = 0;

int main(int argc, char** argv) {
	capture_obj = new WinScreenCapture;
	decoding_obj = new FrameDecoder;
	prc_obj = new RTPFrameProcessor(capture_obj->getBMI());
	frm_thr = new AVFrameHandlerThread(*decoding_obj, *prc_obj);
	// Begin by setting up our usage environment:
	TaskScheduler* scheduler = BasicTaskScheduler::createNew();
	UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);

	/*
	// We need at least one "rtsp://" URL argument:
	if (argc < 2) {
		usage(*env, argv[0]);
		return 1;
	}

	// There are argc-1 URLs: argv[1] through argv[argc-1].  Open and start streaming each one:
	for (int i = 1; i <= argc - 1; ++i) {
		openURL(*env, argv[0], argv[i]);
	}
	*/

	openURL(*env, "abc", "rtsp://59.17.74.89/stream1");

	frm_thr->StartHandle();
	// All subsequent activity takes place within the event loop:
	env->taskScheduler().doEventLoop(&eventLoopWatchVariable);
	// This function call does not return, unless, at some point in time, "eventLoopWatchVariable" gets set to something non-zero.
	frm_thr->EndHandle();
	return 0;

	// If you choose to continue the application past this point (i.e., if you comment out the "return 0;" statement above),
	// and if you don't intend to do anything more with the "TaskScheduler" and "UsageEnvironment" objects,
	// then you can also reclaim the (small) memory used by these objects by uncommenting the following code:
	/*
	  env->reclaim(); env = NULL;
	  delete scheduler; scheduler = NULL;
	*/
}

// Define a class to hold per-stream state that we maintain throughout each stream's lifetime:

class StreamClientState_ {
public:
	StreamClientState_();
	virtual ~StreamClientState_();

public:
	MediaSubsessionIterator* iter;
	MediaSession* session;
	MediaSubsession* subsession;
	TaskToken streamTimerTask;
	double duration;
};

// If you're streaming just a single stream (i.e., just from a single URL, once), then you can define and use just a single
// "StreamClientState_" structure, as a global variable in your application.  However, because - in this demo application - we're
// showing how to play multiple streams, concurrently, we can't do that.  Instead, we have to have a separate "StreamClientState_"
// structure for each "RTSPClient".  To do this, we subclass "RTSPClient", and add a "StreamClientState_" field to the subclass:

class ourRTSPClient_ : public RTSPClient {
public:
	static ourRTSPClient_* createNew(UsageEnvironment& env, char const* rtspURL,
		int verbosityLevel = 0,
		char const* applicationName = NULL,
		portNumBits tunnelOverHTTPPortNum = 0);

protected:
	ourRTSPClient_(UsageEnvironment& env, char const* rtspURL,
		int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum);
	// called only by createNew();
	virtual ~ourRTSPClient_();

public:
	StreamClientState_ scs;
};

// Define a data sink (a subclass of "MediaSink") to receive the data for each subsession (i.e., each audio or video 'substream').
// In practice, this might be a class (or a chain of classes) that decodes and then renders the incoming audio or video.
// Or it might be a "FileSink", for outputting the received data into a file (as is done by the "openRTSP" application).
// In this example code, however, we define a simple 'dummy' sink that receives incoming data, but does nothing with it.

class DummySink_ : public MediaSink {
public:
	static DummySink_* createNew(UsageEnvironment& env,
		MediaSubsession& subsession, // identifies the kind of data that's being received
		char const* streamId = NULL); // identifies the stream itself (optional)

private:
	DummySink_(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId);
	// called only by "createNew()"
	virtual ~DummySink_();

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
	u_int8_t* fReceiveBuffer;
	MediaSubsession& fSubsession;
	char* fStreamId;
};

#define RTSP_CLIENT_VERBOSITY_LEVEL 1 // by default, print verbose output from each "RTSPClient"

static unsigned rtspClientCount = 0; // Counts how many streams (i.e., "RTSPClient"s) are currently in use.

void openURL(UsageEnvironment& env, char const* progName, char const* rtspURL) {
	// Begin by creating a "RTSPClient" object.  Note that there is a separate "RTSPClient" object for each stream that we wish
	// to receive (even if more than stream uses the same "rtsp://" URL).
	RTSPClient* rtspClient = ourRTSPClient_::createNew(env, rtspURL, RTSP_CLIENT_VERBOSITY_LEVEL, progName);
	if (rtspClient == NULL) {
		env << "Failed to create a RTSP client for URL \"" << rtspURL << "\": " << env.getResultMsg() << "\n";
		return;
	}

	++rtspClientCount;

	// Next, send a RTSP "DESCRIBE" command, to get a SDP description for the stream.
	// Note that this command - like all RTSP commands - is sent asynchronously; we do not block, waiting for a response.
	// Instead, the following function call returns immediately, and we handle the RTSP response later, from within the event loop:
	rtspClient->sendDescribeCommand(continueAfterDESCRIBE);
}


// Implementation of the RTSP 'response handlers':

void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString) {
	do {
		UsageEnvironment& env = rtspClient->envir(); // alias
		StreamClientState_& scs = ((ourRTSPClient_*)rtspClient)->scs; // alias

		if (resultCode != 0) {
			env << *rtspClient << "Failed to get a SDP description: " << resultString << "\n";
			delete[] resultString;
			break;
		}

		char* const sdpDescription = resultString;
		env << *rtspClient << "Got a SDP description:\n" << sdpDescription << "\n";

		// Create a media session object from this SDP description:
		scs.session = MediaSession::createNew(env, sdpDescription);
		delete[] sdpDescription; // because we don't need it anymore
		if (scs.session == NULL) {
			env << *rtspClient << "Failed to create a MediaSession object from the SDP description: " << env.getResultMsg() << "\n";
			break;
		}
		else if (!scs.session->hasSubsessions()) {
			env << *rtspClient << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
			break;
		}

		// Then, create and set up our data source objects for the session.  We do this by iterating over the session's 'subsessions',
		// calling "MediaSubsession::initiate()", and then sending a RTSP "SETUP" command, on each one.
		// (Each 'subsession' will have its own data source.)
		scs.iter = new MediaSubsessionIterator(*scs.session);
		setupNextSubsession(rtspClient);
		return;
	} while (0);

	// An unrecoverable error occurred with this stream.
	shutdownStream(rtspClient);
}

// By default, we request that the server stream its data using RTP/UDP.
// If, instead, you want to request that the server stream via RTP-over-TCP, change the following to True:
#define REQUEST_STREAMING_OVER_TCP False

void setupNextSubsession(RTSPClient* rtspClient) {
	UsageEnvironment& env = rtspClient->envir(); // alias
	StreamClientState_& scs = ((ourRTSPClient_*)rtspClient)->scs; // alias

	scs.subsession = scs.iter->next();
	if (scs.subsession != NULL) {
		if (!scs.subsession->initiate()) {
			env << *rtspClient << "Failed to initiate the \"" << *scs.subsession << "\" subsession: " << env.getResultMsg() << "\n";
			setupNextSubsession(rtspClient); // give up on this subsession; go to the next one
		}
		else {
			env << *rtspClient << "Initiated the \"" << *scs.subsession << "\" subsession (";
			if (scs.subsession->rtcpIsMuxed()) {
				env << "client port " << scs.subsession->clientPortNum();
			}
			else {
				env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum() + 1;
			}
			env << ")\n";

			// Continue setting up this subsession, by sending a RTSP "SETUP" command:
			rtspClient->sendSetupCommand(*scs.subsession, continueAfterSETUP, False, REQUEST_STREAMING_OVER_TCP);
		}
		return;
	}

	// We've finished setting up all of the subsessions.  Now, send a RTSP "PLAY" command to start the streaming:
	if (scs.session->absStartTime() != NULL) {
		// Special case: The stream is indexed by 'absolute' time, so send an appropriate "PLAY" command:
		rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY, scs.session->absStartTime(), scs.session->absEndTime());
	}
	else {
		scs.duration = scs.session->playEndTime() - scs.session->playStartTime();
		rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY);
	}
}

void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString) {
	do {
		UsageEnvironment& env = rtspClient->envir(); // alias
		StreamClientState_& scs = ((ourRTSPClient_*)rtspClient)->scs; // alias

		if (resultCode != 0) {
			env << *rtspClient << "Failed to set up the \"" << *scs.subsession << "\" subsession: " << resultString << "\n";
			break;
		}

		env << *rtspClient << "Set up the \"" << *scs.subsession << "\" subsession (";
		if (scs.subsession->rtcpIsMuxed()) {
			env << "client port " << scs.subsession->clientPortNum();
		}
		else {
			env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum() + 1;
		}
		env << ")\n";

		// Having successfully setup the subsession, create a data sink for it, and call "startPlaying()" on it.
		// (This will prepare the data sink to receive data; the actual flow of data from the client won't start happening until later,
		// after we've sent a RTSP "PLAY" command.)

		scs.subsession->sink = DummySink_::createNew(env, *scs.subsession, rtspClient->url());
		// perhaps use your own custom "MediaSink" subclass instead
		if (scs.subsession->sink == NULL) {
			env << *rtspClient << "Failed to create a data sink for the \"" << *scs.subsession
				<< "\" subsession: " << env.getResultMsg() << "\n";
			break;
		}

		env << *rtspClient << "Created a data sink for the \"" << *scs.subsession << "\" subsession\n";
		scs.subsession->miscPtr = rtspClient; // a hack to let subsession handler functions get the "RTSPClient" from the subsession 
		scs.subsession->sink->startPlaying(*(scs.subsession->readSource()),
			subsessionAfterPlaying, scs.subsession);
		// Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
		if (scs.subsession->rtcpInstance() != NULL) {
			scs.subsession->rtcpInstance()->setByeWithReasonHandler(subsessionByeHandler, scs.subsession);
		}
	} while (0);
	delete[] resultString;

	// Set up the next subsession, if any:
	setupNextSubsession(rtspClient);
}

void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString) {
	Boolean success = False;

	do {
		UsageEnvironment& env = rtspClient->envir(); // alias
		StreamClientState_& scs = ((ourRTSPClient_*)rtspClient)->scs; // alias

		if (resultCode != 0) {
			env << *rtspClient << "Failed to start playing session: " << resultString << "\n";
			break;
		}

		// Set a timer to be handled at the end of the stream's expected duration (if the stream does not already signal its end
		// using a RTCP "BYE").  This is optional.  If, instead, you want to keep the stream active - e.g., so you can later
		// 'seek' back within it and do another RTSP "PLAY" - then you can omit this code.
		// (Alternatively, if you don't want to receive the entire stream, you could set this timer for some shorter value.)
		if (scs.duration > 0) {
			unsigned const delaySlop = 2; // number of seconds extra to delay, after the stream's expected duration.  (This is optional.)
			scs.duration += delaySlop;
			unsigned uSecsToDelay = (unsigned)(scs.duration * 1000000);
			scs.streamTimerTask = env.taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc*)streamTimerHandler, rtspClient);
		}

		env << *rtspClient << "Started playing session";
		if (scs.duration > 0) {
			env << " (for up to " << scs.duration << " seconds)";
		}
		env << "...\n";

		success = True;
	} while (0);
	delete[] resultString;

	if (!success) {
		// An unrecoverable error occurred with this stream.
		shutdownStream(rtspClient);
	}
}


// Implementation of the other event handlers:

void subsessionAfterPlaying(void* clientData) {
	MediaSubsession* subsession = (MediaSubsession*)clientData;
	RTSPClient* rtspClient = (RTSPClient*)(subsession->miscPtr);

	// Begin by closing this subsession's stream:
	Medium::close(subsession->sink);
	subsession->sink = NULL;

	// Next, check whether *all* subsessions' streams have now been closed:
	MediaSession& session = subsession->parentSession();
	MediaSubsessionIterator iter(session);
	while ((subsession = iter.next()) != NULL) {
		if (subsession->sink != NULL) return; // this subsession is still active
	}

	// All subsessions' streams have now been closed, so shutdown the client:
	shutdownStream(rtspClient);
}

void subsessionByeHandler(void* clientData, char const* reason) {
	MediaSubsession* subsession = (MediaSubsession*)clientData;
	RTSPClient* rtspClient = (RTSPClient*)subsession->miscPtr;
	UsageEnvironment& env = rtspClient->envir(); // alias

	env << *rtspClient << "Received RTCP \"BYE\"";
	if (reason != NULL) {
		env << " (reason:\"" << reason << "\")";
		delete[](char*)reason;
	}
	env << " on \"" << *subsession << "\" subsession\n";

	// Now act as if the subsession had closed:
	subsessionAfterPlaying(subsession);
}

void streamTimerHandler(void* clientData) {
	ourRTSPClient_* rtspClient = (ourRTSPClient_*)clientData;
	StreamClientState_& scs = rtspClient->scs; // alias

	scs.streamTimerTask = NULL;

	// Shut down the stream:
	shutdownStream(rtspClient);
}

void shutdownStream(RTSPClient* rtspClient, int exitCode) {
	UsageEnvironment& env = rtspClient->envir(); // alias
	StreamClientState_& scs = ((ourRTSPClient_*)rtspClient)->scs; // alias

	// First, check whether any subsessions have still to be closed:
	if (scs.session != NULL) {
		Boolean someSubsessionsWereActive = False;
		MediaSubsessionIterator iter(*scs.session);
		MediaSubsession* subsession;

		while ((subsession = iter.next()) != NULL) {
			if (subsession->sink != NULL) {
				Medium::close(subsession->sink);
				subsession->sink = NULL;

				if (subsession->rtcpInstance() != NULL) {
					subsession->rtcpInstance()->setByeHandler(NULL, NULL); // in case the server sends a RTCP "BYE" while handling "TEARDOWN"
				}

				someSubsessionsWereActive = True;
			}
		}

		if (someSubsessionsWereActive) {
			// Send a RTSP "TEARDOWN" command, to tell the server to shutdown the stream.
			// Don't bother handling the response to the "TEARDOWN".
			rtspClient->sendTeardownCommand(*scs.session, NULL);
		}
	}

	env << *rtspClient << "Closing the stream.\n";
	Medium::close(rtspClient);
	// Note that this will also cause this stream's "StreamClientState_" structure to get reclaimed.

	if (--rtspClientCount == 0) {
		// The final stream has ended, so exit the application now.
		// (Of course, if you're embedding this code into your own application, you might want to comment this out,
		// and replace it with "eventLoopWatchVariable = 1;", so that we leave the LIVE555 event loop, and continue running "main()".)
		exit(exitCode);
	}
}


// Implementation of "ourRTSPClient_":

ourRTSPClient_* ourRTSPClient_::createNew(UsageEnvironment& env, char const* rtspURL,
	int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum) {
	return new ourRTSPClient_(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum);
}

ourRTSPClient_::ourRTSPClient_(UsageEnvironment& env, char const* rtspURL,
	int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum)
	: RTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum, -1) {
}

ourRTSPClient_::~ourRTSPClient_() {
}


// Implementation of "StreamClientState_":

StreamClientState_::StreamClientState_()
	: iter(NULL), session(NULL), subsession(NULL), streamTimerTask(NULL), duration(0.0) {
}

StreamClientState_::~StreamClientState_() {
	delete iter;
	if (session != NULL) {
		// We also need to delete "session", and unschedule "streamTimerTask" (if set)
		UsageEnvironment& env = session->envir(); // alias

		env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
		Medium::close(session);
	}
}


// Implementation of "DummySink_":

// Even though we're not going to be doing anything with the incoming data, we still need to receive it.
// Define the size of the buffer that we'll use:
#define DUMMY_SINK_RECEIVE_BUFFER_SIZE 100000

DummySink_* DummySink_::createNew(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId) {
	return new DummySink_(env, subsession, streamId);
}

DummySink_::DummySink_(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId)
	: MediaSink(env),
	fSubsession(subsession) {
	fStreamId = strDup(streamId);
	fReceiveBuffer = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE];
}

DummySink_::~DummySink_() {
	delete[] fReceiveBuffer;
	delete[] fStreamId;
}

void DummySink_::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes,
	struct timeval presentationTime, unsigned durationInMicroseconds) {
	DummySink_* sink = (DummySink_*)clientData;
	sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

// If you don't want to see debugging output for each received frame, then comment out the following line:
#define DEBUG_PRINT_EACH_RECEIVED_FRAME 1

void DummySink_::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
	struct timeval presentationTime, unsigned /*durationInMicroseconds*/) {
	// We've just received a frame of data.  (Optionally) print out information about it:
#ifdef DEBUG_PRINT_EACH_RECEIVED_FRAME
	if (fStreamId != NULL) envir() << "Stream \"" << fStreamId << "\"; ";
	envir() << fSubsession.mediumName() << "/" << fSubsession.codecName() << ":\tReceived " << frameSize << " bytes";
	if (numTruncatedBytes > 0) envir() << " (with " << numTruncatedBytes << " bytes truncated)";
	char uSecsStr[6 + 1]; // used to output the 'microseconds' part of the presentation time
	sprintf(uSecsStr, "%06u", (unsigned)presentationTime.tv_usec);
	envir() << ".\tPresentation time: " << (int)presentationTime.tv_sec << "." << uSecsStr;
	if (fSubsession.rtpSource() != NULL && !fSubsession.rtpSource()->hasBeenSynchronizedUsingRTCP()) {
		envir() << "!"; // mark the debugging output to indicate that this presentation time is not RTCP-synchronized
	}
#ifdef DEBUG_PRINT_NPT
	envir() << "\tNPT: " << fSubsession.getNormalPlayTime(presentationTime);
#endif
	envir() << "\n";
#endif

	
	static unsigned int pts = 0;
	std::shared_ptr<PacketData> raw_packet = std::make_shared<PacketData>(frameSize);
	memcpy(raw_packet.get()->getMemPointer(), fReceiveBuffer, frameSize);
	auto recv_packet = ConvertRawToAVPacketWithHeader(raw_packet);
	auto av_packet_data = recv_packet.get()->getPointer();
	auto packet_raw_data = recv_packet.get()->getPointer()->data;
	int nal_type = (packet_raw_data[0] & 0x1F);
	av_packet_data->pts = ++pts;
	av_packet_data->dts = pts;
	av_packet_data->flags = (nal_type == 5) ? AV_PKT_FLAG_KEY : 0;
	av_packet_data->size = frameSize;
	decoding_obj->DecodePacket(recv_packet);
	/*
	static bool keyframe_received = false;
	if(keyframe_received)
	{
		dec_thr->InputPacket(recv_packet);
	}
	else
	{
		auto packet_raw_data = recv_packet.get()->getPointer()->data;
		int nal_type = (packet_raw_data[0] & 0x1F);
		if (nal_type == 5) {
			// This is an I-frame
			dec_thr->InputPacket(recv_packet);
			keyframe_received = true;
		}
	}
	*/
	// Then continue, to request the next frame of data:
	continuePlaying();
}

Boolean DummySink_::continuePlaying() {
	if (fSource == NULL) return False; // sanity check (should not happen)

	// Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
	fSource->getNextFrame(fReceiveBuffer, DUMMY_SINK_RECEIVE_BUFFER_SIZE,
		afterGettingFrame, this,
		onSourceClosure, this);
	return True;
}
