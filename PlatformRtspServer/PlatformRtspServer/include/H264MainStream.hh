#ifndef H264MainSTREAM_HH_
#include "H264VideoFileServerMediaSubsession.hh"
#define H264MainSTREAM_HH_
 
#include "FramedSource.hh"
#include "H264VideoRTPSink.hh"
#include "H264VideoStreamFramer.hh"
#include "ByteStreamMemoryBufferSource.hh"
#include "ByteStreamFileSource.hh"  
#include "GssLiveConn.h"

class H264LiveVideoSource: public FramedSource {
public:
	static H264LiveVideoSource* createNew(UsageEnvironment& env,
		char* uid,
		GssLiveConn* liveSource,
		unsigned preferredFrameSize = 0,
		unsigned playTimePerFrame = 0);
        // "preferredFrameSize" == 0 means 'no preference'
        // "playTimePerFrame" is in microseconds
 	void setFramer(H264VideoStreamFramer* framer);
protected:
	H264LiveVideoSource(UsageEnvironment& env,
		char* uid,
		GssLiveConn* liveSource,
		unsigned preferredFrameSize = 0,
		unsigned playTimePerFrame = 0);
      // called only by createNew()
 
	virtual ~H264LiveVideoSource();

	static void incomingDataHandler(H264LiveVideoSource* source);
	void incomingDataHandler1();
	
private:
    // redefined virtual functions:
    virtual void doGetNextFrame();
 	virtual unsigned maxFrameSize() const{return 300*1024;}
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
	unsigned int		   m_ref;
	int						m_nullTimes;							
 };
 
class H264LiveVideoServerMediaSubsession: public OnDemandServerMediaSubsession{
public:
	static H264LiveVideoServerMediaSubsession* createNew( UsageEnvironment& env, char* uid, GssLiveConn* liveSource, Boolean reuseFirstSource);
	     
	void checkForAuxSDPLine1();
	void afterPlayingDummy1();
private:
	H264LiveVideoServerMediaSubsession(
	     UsageEnvironment& env, char* uid, GssLiveConn* liveSource, Boolean reuseFirstSource );
	virtual ~H264LiveVideoServerMediaSubsession();

	void setDoneFlag() { fDoneFlag = ~0; }

	virtual void startStream(unsigned clientSessionId, void* streamToken,
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
	char	m_Uid[128];
	GssLiveConn* m_LiveSource;
};
#endif /* H264_LIVE_VIDEO_STREAM_HH_ */