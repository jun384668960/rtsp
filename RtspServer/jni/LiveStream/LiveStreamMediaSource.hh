#ifndef _LIVE_STREAM_MEDIA_SOURCE_HH
#define _LIVE_STREAM_MEDIA_SOURCE_HH

#include <MediaSink.hh>
#include "CircularQueue.h"

typedef void(*callback_func)(void *ptr, int streamid);

#define DEFAULT_MAX_VEDIOFRAME_RATE 30
#define DEFAULT_MAX_VEDIOFRAME_SIZE (200 * 1024)
#define DEFAULT_MAX_AUDIOFRAME_SIZE (1024)

struct VideoElem
{
	unsigned int lenght;
	unsigned char data[DEFAULT_MAX_VEDIOFRAME_SIZE];
	unsigned char FrameCompleted;
	long pts;
};

struct AudioElem
{
	unsigned int lenght;
	unsigned char data[DEFAULT_MAX_AUDIOFRAME_SIZE];
	unsigned char FrameCompleted;
	long pts;
};

class LiveStreamMediaSource :public Medium
{
public:

	~LiveStreamMediaSource();
	static LiveStreamMediaSource* createNew(UsageEnvironment& env);
	static LiveStreamMediaSource* GetInstance(UsageEnvironment& env);

	FramedSource* videoSource();
	FramedSource* audioSource();
	void SrcPointerSync(int flag);	//flag 1 video 2 audio 0 all
	void RegisterVideoDemandIDR(callback_func handler);
	void VideoDemandIDR();

private:
	LiveStreamMediaSource(UsageEnvironment& env);
public:
	static LiveStreamMediaSource* instance;
	
	callback_func funDemandIDR;

	FramedSource* fOurVideoSource;
	FramedSource* fOurAudioSource;

	CCircularQueue<VideoElem>* m_H265VideoSrc;
	CCircularQueue<AudioElem>* m_AdtsAudioSrc;
};


#ifndef _FRAMED_FILE_SOURCE_HH
#include "FramedSource.hh"
#endif

class VideoOpenSource : public FramedSource
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
		return DEFAULT_MAX_VEDIOFRAME_SIZE;
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

	double m_FrameRate;
	int  uSecsToDelay;
	int  uSecsToDelayMax;
};


class AudioOpenSource : public FramedSource
{
public:
	static AudioOpenSource* createNew(UsageEnvironment& env,
		char const* fileName,
		LiveStreamMediaSource& input,
		unsigned preferredFrameSize = 0,
		unsigned playTimePerFrame = 0);

	unsigned samplingFrequency() const { return fSamplingFrequency; }
	unsigned numChannels() const { return fNumChannels; }
	char const* configStr() const { return fConfigStr; }

protected:
	AudioOpenSource(UsageEnvironment& env,
		LiveStreamMediaSource& input,
		unsigned preferredFrameSize,
		unsigned playTimePerFrame);

	virtual ~AudioOpenSource();

	static void incomingDataHandler(AudioOpenSource* source);
	void incomingDataHandler1();
	void incomingDataHandler2();
private:
	// redefined virtual functions:
	virtual void ParserInfo(FILE* fd);
	virtual void doGetNextFrame();

protected:
	LiveStreamMediaSource& fInput;

	unsigned fSamplingFrequency;
	unsigned fNumChannels;
	unsigned fuSecsPerFrame;
	char fConfigStr[5];
};

#endif
