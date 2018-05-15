#ifndef H264SUBSTREAM_HH_
#include "H264VideoFileServerMediaSubsession.hh"
#define H264SUBSTREAM_HH_
 
#include "FramedSource.hh"
#include "H264VideoRTPSink.hh"
#include "H264VideoStreamFramer.hh"
#include "ByteStreamMemoryBufferSource.hh"
#include "ByteStreamFileSource.hh"
#include "GssLiveConn.h"

class H264LiveVideoSubSource: public FramedSource 
{
public:
	static H264LiveVideoSubSource* createNew(UsageEnvironment& env,
		unsigned preferredFrameSize = 0,
		unsigned playTimePerFrame = 0);
    // "preferredFrameSize" == 0 means 'no preference'
    // "playTimePerFrame" is in microseconds

 	static void incomingDataHandler(H264LiveVideoSubSource* source);
	void incomingDataHandler1();
	void setFramer(H264VideoStreamFramer* framer);
protected:
    H264LiveVideoSubSource(UsageEnvironment& env,
		unsigned preferredFrameSize,
		unsigned playTimePerFrame);
	// called only by createNew()
	virtual ~H264LiveVideoSubSource();
 
  private:
    // redefined virtual functions:
    virtual void doGetNextFrame();
 	virtual unsigned maxFrameSize() const{return 250*1024;}
  private:
	unsigned fPreferredFrameSize;
	unsigned fPlayTimePerFrame;
	unsigned fLastPlayTime;
	Boolean fLimitNumBytesToStream;
	u_int64_t fNumBytesToStream; // used iff "fLimitNumBytesToStream" is True
    GssLiveConn* m_LiveSource;
	H264VideoStreamFramer* m_framer;
	int					   m_front_nalu_type;
	int					   m_front_nalu_len;
	unsigned int		m_ref;
 };
 
 
class H264SubLiveVideoServerMediaSubsession: public OnDemandServerMediaSubsession{
public:
	static H264SubLiveVideoServerMediaSubsession* createNew( UsageEnvironment& env, Boolean reuseFirstSource);
	 
	void checkForAuxSDPLine1();
	void afterPlayingDummy1();
private:
	H264SubLiveVideoServerMediaSubsession(UsageEnvironment& env, Boolean reuseFirstSource );
	virtual ~H264SubLiveVideoServerMediaSubsession();

	void setDoneFlag() { fDoneFlag = ~0; }
	void startStream(unsigned clientSessionId,
					 void* streamToken,
					 TaskFunc* rtcpRRHandler,
					 void* rtcpRRHandlerClientData,
					 unsigned short& rtpSeqNum,
					 unsigned& rtpTimestamp,
					 ServerRequestAlternativeByteHandler* serverRequestAlternativeByteHandler,
					 void* serverRequestAlternativeByteHandlerClientData);
	
private: // redefined virtual functions
	virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
	         			unsigned& estBitrate);
	virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
						unsigned char rtpPayloadTypeIfDynamic,
						FramedSource* inputSource);
	virtual char const* getAuxSDPLine(RTPSink* rtpSink,
	   					FramedSource* inputSource);

private:
	char* fAuxSDPLine;
	char fDoneFlag; // used when setting up "fAuxSDPLine"
	RTPSink* fDummyRTPSink; // ditto
};
#endif /* H264_SUBLIVE_VIDEO_STREAM_HH_ */