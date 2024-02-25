/*
   This code was written with reference to the following source.
   source: https://infoarts.tistory.com/35
*/

#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "UsageEnvironment.hh"
#include "liveMedia.hh"

class CaptureMediaSubsession : public OnDemandServerMediaSubsession
{
public:
	CaptureMediaSubsession(UsageEnvironment& env, StreamReplicator* replicator);

	void setPPSNAL(const BYTE* ptr, int size);
	void setSPSNAL(const BYTE* ptr, int size);
	void setBitRate(unsigned long br);

protected:
	FramedSource* createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate) override;
	RTPSink* createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource* inputSource) override;

	StreamReplicator* fReplicator;
	unsigned long fBitRate;
	const BYTE* fPPSNAL;
	int fPPSNALSize;
	const BYTE* fSPSNAL;
	int fSPSNALSize;
};

