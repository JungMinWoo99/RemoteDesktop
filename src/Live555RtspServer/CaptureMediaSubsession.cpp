#include "Live555RtspServer/CaptureMediaSubsession.h"
#include <iostream>

using namespace std;

CaptureMediaSubsession::CaptureMediaSubsession(UsageEnvironment& env, StreamReplicator* replicator)
	: OnDemandServerMediaSubsession(env, True), fReplicator(replicator), fBitRate(1024), fPPSNAL(NULL), fPPSNALSize(0), fSPSNAL(NULL), fSPSNALSize(0)
{
}

void CaptureMediaSubsession::setPPSNAL(const BYTE* ptr, int size)
{
	fPPSNAL = ptr;
	fPPSNALSize = size;
}

void CaptureMediaSubsession::setSPSNAL(const BYTE* ptr, int size)
{
	fSPSNAL = ptr;
	fSPSNALSize = size;
}

void CaptureMediaSubsession::setBitRate(BYTE br)
{
	if (br > 102400)
	{
		fBitRate = static_cast<unsigned long>(br / 1024);
	}
	else
	{
		cout<<"Estimated bit rate too small, set to 100K!"<<endl;
		fBitRate = 100;
	}
}

FramedSource* CaptureMediaSubsession::createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate)
{
	estBitrate = fBitRate;
	FramedSource* source = fReplicator->createStreamReplica();
	return H264VideoStreamDiscreteFramer::createNew(envir(), source);
}

RTPSink* CaptureMediaSubsession::createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource* inputSource)
{
	//return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic, fSPSNAL, fSPSNALSize, fPPSNAL, fPPSNALSize);
	return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}