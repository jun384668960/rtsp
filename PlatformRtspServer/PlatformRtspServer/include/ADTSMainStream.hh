#ifndef _ADTS_MAIN_STREAM_HH
#define _ADTS_MAIN_STREAM_HH
#include "FramedSource.hh"
#include "OnDemandServerMediaSubsession.hh"
#include "GssLiveConn.h"

class ADTSMainSource: public FramedSource {
public:
	static ADTSMainSource* createNew(UsageEnvironment& env, int sub , GssLiveConn* liveSource );
	unsigned samplingFrequency() const { return fSamplingFrequency; }
	unsigned numChannels() const { return fNumChannels; }
	char const* configStr() const { return fConfigStr; }
	// returns the 'AudioSpecificConfig' for this stream (in ASCII form)

private:
	ADTSMainSource(UsageEnvironment& env, int sub, u_int8_t profile,u_int8_t samplingFrequencyIndex, u_int8_t channelConfiguration, GssLiveConn* liveSource);
	// called only by createNew()
	virtual ~ADTSMainSource();

	static void incomingDataHandler(ADTSMainSource* source);
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
	unsigned int uSecond;
	unsigned int m_ref;
};

class ADTSMainServerMediaSubsession: public OnDemandServerMediaSubsession{
public:
	static ADTSMainServerMediaSubsession*createNew(UsageEnvironment& env, int sub, GssLiveConn* liveSource, Boolean reuseFirstSource);
protected:
	ADTSMainServerMediaSubsession(UsageEnvironment& env, int sub, GssLiveConn* liveSource, Boolean reuseFirstSource);
	// called only by createNew();
	virtual ~ADTSMainServerMediaSubsession();

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
	int sub;
	GssLiveConn* m_LiveSource;
};

#endif
