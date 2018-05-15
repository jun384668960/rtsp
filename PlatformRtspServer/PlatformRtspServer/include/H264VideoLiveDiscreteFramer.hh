#ifndef _H264_VIDEO_LIVE_DISCRETE_FRAMER_HH_
#define _H264_VIDEO_LIVE_DISCRETE_FRAMER_HH_

#include "H264VideoStreamFramer.hh"

class H264VideoLiveDiscreteFramer:public H264VideoStreamFramer//H264VideoStreamDiscreteFramer
{

public:
  static H264VideoLiveDiscreteFramer*
  	createNew(UsageEnvironment& env, FramedSource* inputSource);

	void SetFrameRate(double frameRate);
protected:
  H264VideoLiveDiscreteFramer(UsageEnvironment& env, FramedSource* inputSource);
      // called only by createNew()
  virtual ~H264VideoLiveDiscreteFramer();


protected:
// redefined virtual functions:
	virtual void doGetNextFrame();
	
protected:

	static void afterGettingFrame(void* clientData, unsigned frameSize,
	                    unsigned numTruncatedBytes,
	                    struct timeval presentationTime,
	                    unsigned durationInMicroseconds);
	void afterGettingFrame1(unsigned frameSize,
	              unsigned numTruncatedBytes,
	              struct timeval presentationTime,
	              unsigned durationInMicroseconds);

protected:
	struct timeval fNewPresentationTime;
};

#endif