#include <GroupsockHelper.hh>
#include "H264SubStream.hh"

#include <pthread.h>
#include "utils_log.h"
#include "H264VideoLiveDiscreteFramer.hh"
#include "nalu_utils.hh"

#define SUBSTREAM 1

H264LiveVideoSubSource*
H264LiveVideoSubSource::createNew(UsageEnvironment& env,
	unsigned preferredFrameSize,
	unsigned playTimePerFrame) {
  
	H264LiveVideoSubSource* videosource = new H264LiveVideoSubSource(env, preferredFrameSize, playTimePerFrame);
    return videosource;
 }
 
H264LiveVideoSubSource::H264LiveVideoSubSource(UsageEnvironment& env,
	unsigned preferredFrameSize,
	unsigned playTimePerFrame)
    : FramedSource(env),fPreferredFrameSize(preferredFrameSize), fPlayTimePerFrame(playTimePerFrame), fLastPlayTime(0)
{
 	fPresentationTime.tv_sec  = 0;
	fPresentationTime.tv_usec = 0;
	
	m_LiveSource = new GssLiveConn("119.23.128.209", 6000, "A99762101001002");
	if(m_LiveSource != NULL)
	{
		m_LiveSource->Start();
	}

	m_framer = NULL;
	m_front_nalu_type = 0;
	m_front_nalu_len = 0;
	m_ref = -1;
}
  
H264LiveVideoSubSource::~H264LiveVideoSubSource() 
{
	if(m_LiveSource != NULL)
		delete m_LiveSource;

}

void H264LiveVideoSubSource::setFramer(H264VideoStreamFramer* framer)
{
	m_framer = framer;
}

void H264LiveVideoSubSource::doGetNextFrame()
{	
	incomingDataHandler(this);
}

void H264LiveVideoSubSource::incomingDataHandler(H264LiveVideoSubSource* source)
{
	if (!source->isCurrentlyAwaitingData()) 
	{
		source->doStopGettingFrames(); // we're not ready for the data yet
		return;
	}
	source->incomingDataHandler1();
}

void H264LiveVideoSubSource::incomingDataHandler1()
{
	
}
	
 H264SubLiveVideoServerMediaSubsession*
	H264SubLiveVideoServerMediaSubsession::createNew(UsageEnvironment& env, Boolean reuseFirstSource ) 
{
	return new H264SubLiveVideoServerMediaSubsession(env, reuseFirstSource);
}
 
static void checkForAuxSDPLine(void* clientData) {

	H264SubLiveVideoServerMediaSubsession* subsess = (H264SubLiveVideoServerMediaSubsession*)clientData;
	subsess->checkForAuxSDPLine1();
}
 
 
void H264SubLiveVideoServerMediaSubsession::checkForAuxSDPLine1() {
	char const* dasl;

	if (fAuxSDPLine != NULL) {
		// Signal the event loop that we're done:
		setDoneFlag();
	} else if (fDummyRTPSink != NULL && (dasl = fDummyRTPSink->auxSDPLine()) != NULL) {
		fAuxSDPLine = strDup(dasl);
		fDummyRTPSink = NULL;

		// Signal the event loop that we're done:
		setDoneFlag();
	} else {
		// try again after a brief delay:
		int uSecsToDelay = 100000; // 100 ms
		nextTask() = envir().taskScheduler().scheduleDelayedTask(uSecsToDelay,
		        (TaskFunc*)checkForAuxSDPLine, this);
	}
}
 
void H264SubLiveVideoServerMediaSubsession::afterPlayingDummy1() {

	// Unschedule any pending 'checking' task:
	envir().taskScheduler().unscheduleDelayedTask(nextTask());
	// Signal the event loop that we're done:
	setDoneFlag();
}
 
void H264SubLiveVideoServerMediaSubsession::startStream(unsigned clientSessionId,
						 void* streamToken,
						 TaskFunc* rtcpRRHandler,
						 void* rtcpRRHandlerClientData,
						 unsigned short& rtpSeqNum,
						 unsigned& rtpTimestamp,
						 ServerRequestAlternativeByteHandler* serverRequestAlternativeByteHandler,
						 void* serverRequestAlternativeByteHandlerClientData) {
 
	OnDemandServerMediaSubsession::startStream(clientSessionId,streamToken,rtcpRRHandler,rtcpRRHandlerClientData,rtpSeqNum,rtpTimestamp,serverRequestAlternativeByteHandler,
	           serverRequestAlternativeByteHandlerClientData);
 }

H264SubLiveVideoServerMediaSubsession::H264SubLiveVideoServerMediaSubsession(UsageEnvironment& env, Boolean reuseFirstSource ) 
     :OnDemandServerMediaSubsession(env, reuseFirstSource),
	fAuxSDPLine(NULL), fDoneFlag(0), fDummyRTPSink(NULL) {
}
 
 
H264SubLiveVideoServerMediaSubsession::~H264SubLiveVideoServerMediaSubsession() {
	delete[] fAuxSDPLine;
}
 
 
static void afterPlayingDummy(void* clientData) {

	H264SubLiveVideoServerMediaSubsession* subsess = (H264SubLiveVideoServerMediaSubsession*)clientData;
	subsess->afterPlayingDummy1();
}
 
char const* H264SubLiveVideoServerMediaSubsession::getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource) {

	if (fAuxSDPLine != NULL) return fAuxSDPLine; // it's already been set up (for a previous client)

	if (fDummyRTPSink == NULL) { // we're not already setting it up for another, concurrent stream
		// Note: For H264 video files, the 'config' information ("profile-level-id" and "sprop-parameter-sets") isn't known
		// until we start reading the file.  This means that "rtpSink"s "auxSDPLine()" will be NULL initially,
		// and we need to start reading data from our file until this changes.
		fDummyRTPSink = rtpSink;

		// Start reading the file:
		fDummyRTPSink->startPlaying(*inputSource, afterPlayingDummy, this);

		// Check whether the sink's 'auxSDPLine()' is ready:
		checkForAuxSDPLine(this);
	}

	envir().taskScheduler().doEventLoop(&fDoneFlag);

	return fAuxSDPLine;
}

FramedSource* H264SubLiveVideoServerMediaSubsession::createNewStreamSource( unsigned /*clientSessionId*/, unsigned& estBitrate)
{
	estBitrate = 500;

	H264LiveVideoSubSource *buffsource = H264LiveVideoSubSource::createNew(envir());
	if (buffsource == NULL) return NULL;

#if (!USE_SEL_FRAMER)
	H264VideoStreamFramer* videoSource = H264VideoStreamFramer::createNew(envir(), (FramedSource*)buffsource, False,(double)stVEncCtrl[1].FrameRate-(double)stVEncCtrl[1].FrameRate/10000.0, 0.0);//-0.003

	if(videoSource == NULL)
	{
		LOGE_print("createNewStreamSource return NULL");
	}
	buffsource->setFramer(videoSource);

	return videoSource;
#else
	H264VideoLiveDiscreteFramer* videoSource = H264VideoLiveDiscreteFramer::createNew(envir(), (FramedSource*)buffsource);
	return videoSource;
#endif
}

RTPSink* H264SubLiveVideoServerMediaSubsession::createNewRTPSink(
		Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic,
		FramedSource* /*inputSource*/) {
		
	return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}
