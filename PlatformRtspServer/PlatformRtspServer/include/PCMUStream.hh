#ifndef _PCMU_STREAM_HH
#define _PCMU_STREAM_HH

#include "FramedSource.hh"
#include "OnDemandServerMediaSubsession.hh"
#include "SimpleRTPSink.hh"
#include "GssLiveConn.h"

class PCMUStream: public FramedSource {
public:
	static PCMUStream* createNew(UsageEnvironment& env, char* uid,GssLiveConn* liveSource);
	unsigned samplingFrequency() const { return fSamplingFrequency; }
	unsigned numChannels() const { return fNumChannels; }
	char const* configStr() const { return fConfigStr; }
	// returns the 'AudioSpecificConfig' for this stream (in ASCII form)

private:
	PCMUStream(UsageEnvironment& env, char* uid, u_int8_t profile,u_int8_t samplingFrequencyIndex, u_int8_t channelConfiguration, GssLiveConn* liveSource);
	// called only by createNew()
	virtual ~PCMUStream();

	static void incomingDataHandler(PCMUStream* source);
	void incomingDataHandler1();

private:
	// redefined virtual functions:
	virtual void doGetNextFrame();

private:
	unsigned fSamplingFrequency;
	unsigned fNumChannels;
	unsigned fuSecsPerFrame;
	char fConfigStr[5];
    GssLiveConn* m_LiveSource;
	unsigned int m_ref;
	int						m_nullTimes;
};



class PCMUServerMediaSubsession: public OnDemandServerMediaSubsession{
public:
	static PCMUServerMediaSubsession* createNew(UsageEnvironment& env, char* uid, GssLiveConn* liveSource, Boolean reuseFirstSource);
protected:
	PCMUServerMediaSubsession(UsageEnvironment& env, char* uid, GssLiveConn* liveSource, Boolean reuseFirstSource);
	// called only by createNew();
	virtual ~ PCMUServerMediaSubsession();
	virtual void startStream(unsigned clientSessionId,
							void* streamToken,
							TaskFunc* rtcpRRHandler,
							void* rtcpRRHandlerClientData,
							unsigned short& rtpSeqNum,
							unsigned& rtpTimestamp,
							ServerRequestAlternativeByteHandler* serverRequestAlternativeByteHandler,
							void* serverRequestAlternativeByteHandlerClientData);

protected: // redefined virtual functions
	virtual FramedSource* createNewStreamSource(unsigned clientSessionId,unsigned& estBitrate);
	virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
                		unsigned char rtpPayloadTypeIfDynamic,
				       	FramedSource* inputSource);
	char	m_Uid[128];
	GssLiveConn* m_LiveSource;
};

#endif
