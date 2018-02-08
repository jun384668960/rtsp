#ifndef _LIVE_STREAM_MEDIA_SOURCE_HH
#define _LIVE_STREAM_MEDIA_SOURCE_HH

#include <list>
#include <MediaSink.hh>
#include "CircularQueue.h"
#include "BufferManager.h"

typedef void(*callback_func)(void *ptr, int streamid);

#define DEFAULT_MAX_VEDIOFRAME_RATE 60
#define DEFAULT_MAX_VEDIOFRAME_SIZE (512 * 1024)
#define DEFAULT_MAX_AUDIOFRAME_SIZE (1024 * 2)

struct VideoElem
{
	unsigned int lenght;
	unsigned char data[DEFAULT_MAX_VEDIOFRAME_SIZE];
	unsigned char FrameCompleted;
	uint64_t pts;
};

struct AudioElem
{
	unsigned int lenght;
	unsigned char data[DEFAULT_MAX_AUDIOFRAME_SIZE];
	unsigned char FrameCompleted;
	uint64_t pts;
};

class VideoOpenSource;
class AudioOpenSource;
class LiveStreamMediaSource :public Medium
{
public:

	~LiveStreamMediaSource();
	static LiveStreamMediaSource* createNew(UsageEnvironment& env);
	static LiveStreamMediaSource* GetInstance(UsageEnvironment& env);

	VideoOpenSource* videoSource();
	AudioOpenSource* audioSource();
	void SrcPointerSync(int flag);	//flag 1 video 2 audio 0 all
	void RegisterVideoDemandIDR(callback_func handler);
	void UnRegisterVideoDemandIDR();
	void VideoDemandIDR();

	void SaveSpsAndPps(u_int8_t* sps, unsigned sps_len, u_int8_t* pps, unsigned pps_len);
	bool GetSpsAndPps(u_int8_t* &sps, unsigned& spsLen, u_int8_t* &pps,unsigned& ppsLen);

	void SetFrameRate(double frameRate){ m_FrameRate = frameRate; }
	double FrameRate(){ return m_FrameRate; }

	unsigned samplingFrequency() const { return fSamplingFrequency; }
	unsigned numChannels() const { return fNumChannels; }
	unsigned biteRate(){ return fbiteRate; }

	void SetsamplingFrequency(unsigned frequency){ fSamplingFrequency = frequency; 
		fuSecsPerFrame
			= (1024/*samples-per-frame*/ * 1000000) / fSamplingFrequency/*samples-per-second*/;
	}
	void SetnumChannels(unsigned channels){ fNumChannels = channels; }
	void SetbiteRate(unsigned biteRate) { fbiteRate = biteRate; }
	void SetProfile(u_int8_t profile) { fprofile = profile; }
	char const* configStr();
	
private:
	LiveStreamMediaSource(UsageEnvironment& env);
public:
	static LiveStreamMediaSource* instance;
	
	callback_func funDemandIDR;

	FramedSource* fOurVideoSource;
	FramedSource* fOurAudioSource; 

	//h264
	u_int8_t* mSps; unsigned mSpsLen;
  	u_int8_t* mPps; unsigned mPpsLen;
  	double m_FrameRate;

	//aac
  	unsigned fSamplingFrequency;
	unsigned fNumChannels;
	unsigned fuSecsPerFrame;
	unsigned fbiteRate;
	u_int8_t fprofile;
	char fConfigStr[5];

  	CBufferManager_t* VbuffMgr;
  	CBufferManager_t* AbuffMgr;
};


#ifndef _FRAMED_FILE_SOURCE_HH
#include "FramedSource.hh"
#endif

class VideoOpenSource : public FramedSource, public CBufferInfo_t
{
public:
	static VideoOpenSource* createNew(UsageEnvironment& env,
		char const* fileName,
		LiveStreamMediaSource& input,
		unsigned preferredFrameSize = 0,
		unsigned playTimePerFrame = 0);
	// "preferredFrameSize" == 0 means 'no preference'
	// "playTimePerFrame" is in microseconds

	unsigned maxFrameSize() const {
		//return DEFAULT_MAX_VEDIOFRAME_SIZE;
		return 256*1024;
	}

protected:
	VideoOpenSource(UsageEnvironment& env,
		LiveStreamMediaSource& input,
		unsigned preferredFrameSize,
		unsigned playTimePerFrame);

	virtual ~VideoOpenSource();

	static void incomingDataHandler(VideoOpenSource* source);
	void incomingDataHandler1();

private:
	// redefined virtual functions:
	virtual void doGetNextFrame();

protected:
	LiveStreamMediaSource& fInput;

private:
	unsigned fPreferredFrameSize;
	unsigned fPlayTimePerFrame;
	Boolean fFidIsSeekable;
	unsigned fLastPlayTime;
	Boolean fHaveStartedReading;
	Boolean fLimitNumBytesToStream;
	u_int64_t fNumBytesToStream; // used iff "fLimitNumBytesToStream" is True

	int  uSecsToDelay;
	int  uSecsToDelayMax;
	int  m_NoDataCnt;
	uint64_t m_ref;
	char mName[128];
};


class AudioOpenSource : public FramedSource, public CBufferInfo_t
{
public:
	static AudioOpenSource* createNew(UsageEnvironment& env,
		char const* fileName,
		LiveStreamMediaSource& input,
		unsigned preferredFrameSize = 0,
		unsigned playTimePerFrame = 0);

protected:
	AudioOpenSource(UsageEnvironment& env,
		LiveStreamMediaSource& input,
		unsigned preferredFrameSize,
		unsigned playTimePerFrame);

	virtual ~AudioOpenSource();

	static void incomingDataHandler(AudioOpenSource* source);
	void incomingDataHandler1();

private:
	// redefined virtual functions:
	virtual void doGetNextFrame();

protected:
	LiveStreamMediaSource& fInput;

	int  m_NoDataCnt;
	uint64_t m_ref;
	char mName[128];
};

#endif
