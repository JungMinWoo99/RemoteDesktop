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
	void setBitRate(BYTE br);

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
