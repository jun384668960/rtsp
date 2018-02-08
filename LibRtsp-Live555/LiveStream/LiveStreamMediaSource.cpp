#include "LiveStreamMediaSource.hh"
#include "GroupsockHelper.hh"
#include "RtspSvr.hh"

LiveStreamMediaSource* LiveStreamMediaSource::instance = NULL;

//default sps pps
static char sps[15] = { 0x67,0x42,0x00 ,0x29,    
						0x8d,0x8d,0x40,0x3c ,
						0x03 ,0xcd ,0x00,0xf0,
						0x88 ,0x45,0x38};

static char pps[4] = { 0x68,0xca,0x43,0xc8};

static unsigned const samplingFrequencyTable[16] = {
	96000, 88200, 64000, 48000,
	44100, 32000, 24000, 22050,
	16000, 12000, 11025, 8000,
	7350, 0, 0, 0
};

static u_int8_t get_frequency_index(unsigned frequency)
{
	int index = 4;
	for(int i=0; i<sizeof(samplingFrequencyTable)/sizeof(samplingFrequencyTable[0]); i++)
	{
		if(samplingFrequencyTable[i] == frequency)
		{
			index = i;
			break;
		}
	}

	return  index;
}


LiveStreamMediaSource* LiveStreamMediaSource::GetInstance(UsageEnvironment& env)
{	
	if(instance == NULL)
	{
		instance =  new LiveStreamMediaSource(env);
	}
		
	return instance;
}
LiveStreamMediaSource* LiveStreamMediaSource::createNew(UsageEnvironment& env)
{
	if(instance == NULL)
	{
		instance =  new LiveStreamMediaSource(env);
	}
		
	return instance;
}

LiveStreamMediaSource::LiveStreamMediaSource(UsageEnvironment& env)
	: Medium(env),funDemandIDR(NULL),fOurVideoSource(NULL),fOurAudioSource(NULL) 
	,mSps(NULL),mSpsLen(0)
  	,mPps(NULL),mPpsLen(0)
{
	m_FrameRate = DEFAULT_MAX_VEDIOFRAME_RATE;
	mSps = new u_int8_t[64];
	mSpsLen = 63;
	mPps = new u_int8_t[64];
	mPpsLen = 63;

	memcpy(mSps, sps, 15);
	mSpsLen = 15;
	memcpy(mPps, pps, 4);
	mPpsLen = 4;


	fbiteRate = 128000;
	fSamplingFrequency = 32000;
	fNumChannels = 1;
	fprofile = 1;
	
	fuSecsPerFrame
		= (1024/*samples-per-frame*/ * 1000000) / fSamplingFrequency/*samples-per-second*/;

	unsigned char audioSpecificConfig[2];
	
	u_int8_t sampling_frequency_index = get_frequency_index(fSamplingFrequency); //44100
	u_int8_t channel_configuration = fNumChannels;
	u_int8_t const audioObjectType = fprofile + 1;
	
	audioSpecificConfig[0] = (audioObjectType << 3) | (sampling_frequency_index >> 1);
	audioSpecificConfig[1] = (sampling_frequency_index << 7) | (channel_configuration << 3);
	
	memset(fConfigStr,0x0,5);
	sprintf(fConfigStr, "%02X%02x", audioSpecificConfig[0], audioSpecificConfig[1]);
	

	VbuffMgr = new CBufferManager_t();
	VbuffMgr->Init(5 * 1024 * 1024);
	
  	AbuffMgr = new CBufferManager_t();
	AbuffMgr->Init(20 * 1024);
}

LiveStreamMediaSource::~LiveStreamMediaSource()
{
	if (VbuffMgr != NULL)
	{
		delete[] VbuffMgr;
		VbuffMgr = NULL;
	}
	if (AbuffMgr != NULL)
	{
		delete[] AbuffMgr;
		AbuffMgr = NULL;
	}
	
	if (fOurVideoSource != NULL)
	{
		Medium::close(fOurVideoSource);
		fOurVideoSource = NULL;
	}
	if (fOurVideoSource != NULL)
	{
		Medium::close(fOurAudioSource);
		fOurAudioSource = NULL;
	}
	if(mSps != NULL)
	{
		delete[] mSps;
		mSps = NULL;
	}
	if(mPps != NULL)
	{
		delete[] mPps;
		mPps = NULL;
	}
	LOGD_print("LiveStreamMediaSource", "~LiveStreamMediaSource");
}


void LiveStreamMediaSource::SrcPointerSync(int flag)
{
	switch(flag)
	{
	case 1:
		VbuffMgr->Sync();
		break;
	case 2:
		AbuffMgr->Sync();
		break;
	default:
		VbuffMgr->Sync();
		AbuffMgr->Sync();
		break;
	}
}

void LiveStreamMediaSource::RegisterVideoDemandIDR(callback_func handler)
{
	funDemandIDR = handler;
}

void LiveStreamMediaSource::UnRegisterVideoDemandIDR()
{
	funDemandIDR = NULL;
}
void LiveStreamMediaSource::VideoDemandIDR()
{
	if(	funDemandIDR != NULL)
	{
		funDemandIDR(NULL, 0);
	}
}

VideoOpenSource* LiveStreamMediaSource::videoSource()
{
	LOGD_print("LiveStreamMediaSource", "----->>>>LiveStreamMediaSource::videoSource()");

#if 1
	return VideoOpenSource::createNew(envir(), NULL, *this);
#else
	if (fOurVideoSource == NULL)
	{
		fOurVideoSource = VideoOpenSource::createNew(envir(), NULL, *this);
	}

	//VbuffMgr->Sync();
	//VideoDemandIDR();

	return fOurVideoSource;
	
#endif

}

AudioOpenSource* LiveStreamMediaSource::audioSource()
{
	//AbuffMgr->SyncRwPoint();
#if 1
	return AudioOpenSource::createNew(envir(), NULL, *this);
#else
	if (fOurAudioSource == NULL)
	{
		fOurAudioSource = AudioOpenSource::createNew(envir(), NULL, *this);
	}

	return fOurAudioSource;
#endif

}

void LiveStreamMediaSource::SaveSpsAndPps(u_int8_t* sps, unsigned sps_len, u_int8_t* pps, unsigned pps_len)
{
	if(sps != NULL)
	{
		memcpy(mSps, sps, sps_len);
		mSpsLen = sps_len;
	}
	if( pps != NULL)
	{
		memcpy(mPps, pps, pps_len);
		mPpsLen = pps_len;
	}
}

bool LiveStreamMediaSource::GetSpsAndPps(u_int8_t* &sps, unsigned& spsLen, u_int8_t* &pps,unsigned& ppsLen)
{
	if(mSps == NULL || mPps == NULL)
	{
		return false;
	}
	else
	{
		sps = mSps;
		pps = mPps; 
		spsLen = mSpsLen;
		ppsLen = mPpsLen;
	}
	return true;
}

char const* LiveStreamMediaSource::configStr()
{
	unsigned char audioSpecificConfig[2];
	
	u_int8_t sampling_frequency_index = get_frequency_index(fSamplingFrequency); //44100
	u_int8_t channel_configuration = fNumChannels;
	u_int8_t const audioObjectType = fprofile + 1;
	
	audioSpecificConfig[0] = (audioObjectType << 3) | (sampling_frequency_index >> 1);
	audioSpecificConfig[1] = (sampling_frequency_index << 7) | (channel_configuration << 3);
	
	memset(fConfigStr,0x0,5);
	sprintf(fConfigStr, "%02X%02x", audioSpecificConfig[0], audioSpecificConfig[1]);
	
	return fConfigStr;
}

VideoOpenSource* VideoOpenSource::createNew(UsageEnvironment& env, char const* fileName, LiveStreamMediaSource& input,
	unsigned preferredFrameSize,
	unsigned playTimePerFrame) {

	VideoOpenSource* newSource = new VideoOpenSource(env, input, preferredFrameSize, playTimePerFrame);

	return newSource;
}

VideoOpenSource::VideoOpenSource(UsageEnvironment& env,
	LiveStreamMediaSource& input,
	unsigned preferredFrameSize,
	unsigned playTimePerFrame)
	: FramedSource(env), fInput(input), fPreferredFrameSize(preferredFrameSize),
	fPlayTimePerFrame(playTimePerFrame), fLastPlayTime(0),
	fHaveStartedReading(False), fLimitNumBytesToStream(False), fNumBytesToStream(0)
	,m_NoDataCnt(0),m_ref(0)
	{

	fPresentationTime.tv_sec = 0;
	fPresentationTime.tv_usec = 0;
	uSecsToDelay = 1000;
	uSecsToDelayMax = 1666;

	memset(mName, 0x0, 128);
	sprintf(mName, "VideoOpenSource_%p",this);
	fInput.VbuffMgr->Register(mName,this);

}

VideoOpenSource::~VideoOpenSource()
{
	fInput.VbuffMgr->UnRegister(mName);
	
	if (fInput.fOurVideoSource != NULL)
	{
		Medium::close(fInput.fOurVideoSource);
		fInput.fOurVideoSource = NULL;
	}
	LOGD_print("LiveStreamMediaSource", "----->>>>~VideoOpenSource");
}

void VideoOpenSource::doGetNextFrame() {
	//do read from memory
	incomingDataHandler(this);
}
void VideoOpenSource::incomingDataHandler(VideoOpenSource* source) {
	
	if (!source->isCurrentlyAwaitingData()) 
	{
		source->doStopGettingFrames(); // we're not ready for the data yet
		return;
	}
	
	source->incomingDataHandler1();
}

void VideoOpenSource::incomingDataHandler1()
{
	if (fLimitNumBytesToStream && fNumBytesToStream < (u_int64_t)fMaxSize) {
		fMaxSize = (unsigned)fNumBytesToStream;
	}
	if (fPreferredFrameSize > 0 && fPreferredFrameSize < fMaxSize) {
		fMaxSize = fPreferredFrameSize;
	}

	fFrameSize = 0; 
	int unHandleCnt = 0;

	CBuffer_t buffer;
	if (usingQueue()->PopFront(buffer, unHandleCnt))
	{
		m_NoDataCnt = 0;
		if (buffer.lenght <= 0)
		{
			handleClosure(this);
			return;
		}
		else
		{
			int type = buffer.data[0]&0x1f;
			
			fFrameSize = buffer.lenght;
			if (fFrameSize > fMaxSize)
			{
				LOGD_print("VideoOpenSource", "----->>>>fFrameSize > fMaxSize lost data!!!");
				fFrameSize = fMaxSize;
				fNumTruncatedBytes = fFrameSize - fMaxSize;
			}
			else
			{
				fNumTruncatedBytes = 0;
			}
			
			memcpy(fTo , buffer.data, fFrameSize);

			// Set the 'presentation time':
			if(type != 7 && type != 8)
			{
				if (fPresentationTime.tv_sec == 0 && fPresentationTime.tv_usec == 0 && m_ref == 0)
				{
					m_ref = buffer.pts;
				}
				else
				{
					fPresentationTime.tv_sec  = (buffer.pts-m_ref)/1000000;  
					fPresentationTime.tv_usec = (buffer.pts-m_ref)%1000000;  
				}
			}

			fDurationInMicroseconds = 100000000 / (fInput.m_FrameRate * 100 * unHandleCnt);;

			struct timeval tv;
			getTickCount(&tv, NULL);
			int64_t _time = (int64_t)tv.tv_sec*1000 + tv.tv_usec/1000;
			int64_t _presentationTime = (int64_t)fPresentationTime.tv_sec*1000000 + fPresentationTime.tv_usec;
			
			LOGD_print("VideoOpenSource", "Call incomingDataHandler1 Time:%lld type:%d fFrameSize:%u unHandleCnt:%d _presentationTime:%llu fMaxSize:%u buffer.pts:%llu"
				,/*GetTickCount()*/_time, type, fFrameSize, unHandleCnt, _presentationTime, fMaxSize, buffer.pts);
			if(unHandleCnt >= 1 || type == 7 || type == 8)
			{
				//LOGI_print("VideoOpenSource","type:%d unHandleCnt:%d",type, unHandleCnt);
			}
			
			nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
				(TaskFunc*)FramedSource::afterGetting, this);
		}
	}
	else
	{
		if (uSecsToDelay >= uSecsToDelayMax)
		{
			uSecsToDelay = uSecsToDelayMax;
		}
		else{
			uSecsToDelay *= 2;
		}
		
		// if for long time no data
		//*
		//m_NoDataCnt++;
		if(m_NoDataCnt > 500)
		{
			handleClosure(this);
			return;
		}
		//*/
		
		nextTask() = envir().taskScheduler().scheduleDelayedTask(uSecsToDelay,
			(TaskFunc*)incomingDataHandler, this);
	}
}



//===============================================================================================================

AudioOpenSource*
AudioOpenSource::createNew(UsageEnvironment& env, char const* fileName, LiveStreamMediaSource& input,
unsigned preferredFrameSize,
unsigned playTimePerFrame) {

	AudioOpenSource* newSource = new AudioOpenSource(env, input, preferredFrameSize, playTimePerFrame);

	return newSource;
}

AudioOpenSource::AudioOpenSource(UsageEnvironment& env,
	LiveStreamMediaSource& input,
	unsigned preferredFrameSize,
	unsigned playTimePerFrame)
	: FramedSource(env), fInput(input)
	,m_NoDataCnt(0),m_ref(0)
{
	fPresentationTime.tv_sec = 0;
	fPresentationTime.tv_usec = 0;
	
	memset(mName, 0x0, 128);
	sprintf(mName, "AudioOpenSource%p",this);
	fInput.AbuffMgr->Register(mName,this);

}

AudioOpenSource::~AudioOpenSource()
{
	fInput.AbuffMgr->UnRegister(mName);
	if (fInput.fOurAudioSource != NULL)
	{
		Medium::close(fInput.fOurAudioSource);
		fInput.fOurAudioSource = NULL;
	}
	LOGD_print("LiveStreamMediaSource", "----->>>>~AudioOpenSource");
}

void AudioOpenSource::doGetNextFrame() {
	//do read from memory
	incomingDataHandler(this);
}

void AudioOpenSource::incomingDataHandler(AudioOpenSource* source) {
	if (!source->isCurrentlyAwaitingData()) 
	{
		source->doStopGettingFrames(); // we're not ready for the data yet
		return;
	}
	source->incomingDataHandler1();
}

void AudioOpenSource::incomingDataHandler1()
{
	fFrameSize = 0;
	AudioElem elem;
	int unHandleCnt = 0;

	CBuffer_t buffer;
	if (!usingQueue()->PopFront(buffer, unHandleCnt))
	{
		// if for long time no data
		//m_NoDataCnt++;
		if(m_NoDataCnt > 500)
		{
			handleClosure(this);
			return;
		}
		
		nextTask() = envir().taskScheduler().scheduleDelayedTask(1500,
			(TaskFunc*)incomingDataHandler,this);
	}
	else
	{
		m_NoDataCnt = 0;
		if (buffer.lenght <= 0)
		{
			handleClosure(this);
			return;
		}
		
		fFrameSize = buffer.lenght;
		if (fFrameSize > fMaxSize) {
			memcpy(fTo, buffer.data, fMaxSize);
		}
		else
		{
			memcpy(fTo, buffer.data, fFrameSize);
		}
		#if 0
		// Set the 'presentation time':
		if (fPresentationTime.tv_sec == 0 && fPresentationTime.tv_usec == 0) {
			// This is the first frame, so use the current time:
			getTickCount(&fPresentationTime, NULL);
		}
		else {
			// Increment by the play time of the previous frame:
			unsigned uSeconds = fPresentationTime.tv_usec + fInput.fuSecsPerFrame;
			fPresentationTime.tv_sec += uSeconds / 1000000;
			fPresentationTime.tv_usec = uSeconds % 1000000;
		}
		#else
		if (fPresentationTime.tv_sec == 0 && fPresentationTime.tv_usec == 0 && m_ref == 0)
		{
			m_ref = buffer.pts;
		}
		else
		{
			fPresentationTime.tv_sec  = (buffer.pts-m_ref)/1000000;  
			fPresentationTime.tv_usec = (buffer.pts-m_ref)%1000000;  
		}
		#endif
		fDurationInMicroseconds = fInput.fuSecsPerFrame;
		
		LOGD_print("AudioOpenSource", "===========>>>>>>>>>>>>> fInput.fuSecsPerFrame:%d pts:%llu\n",fInput.fuSecsPerFrame, buffer.pts);
		//LOGD_print("AudioOpenSource", "Call incomingDataHandler1 fFrameSize:%d unHandleCnt:%d",fFrameSize,unHandleCnt);
		// Switch to another task, and inform the reader that he has data:
		nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
		(TaskFunc*)FramedSource::afterGetting, this);
	}
	//*/
}
