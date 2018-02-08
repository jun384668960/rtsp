#include "LiveStreamMediaSource.hh"
#include "debuginfo.h"
#include "media/NdkMediaCodec.h"
#include "media/NdkMediaExtractor.h"

//**********************************************************************//
FILE* fp_acc = NULL;
///*
int read_AAC(unsigned char *buf, int buf_size){
	if(fp_acc == NULL)
	{
		fp_acc = fopen("test.aac", "rb");
	}
	unsigned char aac_header[7];
	int true_size = fread(aac_header, 1, 7, fp_acc);
	if(true_size <= 0)
	{
		fseek(fp_acc, 0L, SEEK_SET);
		true_size = fread(aac_header, 1, 7, fp_acc);
	}

	u_int16_t frame_length
		= ((aac_header[3] & 0x03) << 11) | (aac_header[4] << 3) | ((aac_header[5] & 0xE0) >> 5);
	unsigned numBytesToRead
		= frame_length > sizeof aac_header ? frame_length - sizeof aac_header : 0;
	
	true_size = fread(buf, 1, numBytesToRead, fp_acc);
	//printf("true_size:%d body_size:%d\n",true_size,body_size);
	return true_size;
}
//*/
//**********************************************************************//
FILE *fp_h265 = NULL;
int read_h265(unsigned char *buf, int buf_size){
	if (fp_h265 == NULL)
	{
		fp_h265 = fopen("test.video", "rb");
	}

	int true_size = fread(buf, 1, buf_size, fp_h265);
	if(true_size <= 0)
	{
		fseek(fp_h265, 0L, SEEK_SET);
		true_size = fread(buf, 1, buf_size, fp_h265);
	}
	
	return true_size;
}
//**********************************************************************//

LiveStreamMediaSource* LiveStreamMediaSource::instance = NULL;

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
: Medium(env),funDemandIDR(NULL),fOurVideoSource(NULL),fOurAudioSource(NULL),m_H265VideoSrc(NULL),m_AdtsAudioSrc(NULL)
{
	m_H265VideoSrc = new CCircularQueue<VideoElem>();
	m_AdtsAudioSrc = new CCircularQueue<AudioElem>();
}

LiveStreamMediaSource::~LiveStreamMediaSource()
{
	if (m_H265VideoSrc != NULL)
	{
		delete[] m_H265VideoSrc;
		m_H265VideoSrc = NULL;
	}
	if (m_AdtsAudioSrc != NULL)
	{
		delete[] m_AdtsAudioSrc;
		m_AdtsAudioSrc = NULL;
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

}


void LiveStreamMediaSource::SrcPointerSync(int flag)
{
	switch(flag)
	{
	case 1:
		m_H265VideoSrc->SyncRwPoint();
		break;
	case 2:
		m_AdtsAudioSrc->SyncRwPoint();
		break;
	default:
		m_H265VideoSrc->SyncRwPoint();
		m_AdtsAudioSrc->SyncRwPoint();
		break;
	}
}

void LiveStreamMediaSource::RegisterVideoDemandIDR(callback_func handler)
{
	funDemandIDR = handler;
}

void LiveStreamMediaSource::VideoDemandIDR()
{
	if(	funDemandIDR != NULL)
	{
		funDemandIDR(NULL, 0);
	}
}

FramedSource* LiveStreamMediaSource::videoSource()
{
	m_H265VideoSrc->SyncRwPoint();
	VideoDemandIDR();

	if (fOurVideoSource == NULL)
	{
		fOurVideoSource = VideoOpenSource::createNew(envir(), NULL, *this);
	}

	return fOurVideoSource;
}

FramedSource* LiveStreamMediaSource::audioSource()
{
	m_AdtsAudioSrc->SyncRwPoint();
	if (fOurAudioSource == NULL)
	{
		fOurAudioSource = AudioOpenSource::createNew(envir(), NULL, *this);
	}

	return fOurAudioSource;
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
	, m_FrameRate(DEFAULT_MAX_VEDIOFRAME_RATE){

	uSecsToDelay = 1000;
	uSecsToDelayMax = 1666;
}

VideoOpenSource::~VideoOpenSource()
{
	if (fInput.fOurVideoSource != NULL)
	{
		Medium::close(fInput.fOurVideoSource);
		fInput.fOurVideoSource = NULL;
	}
	if(fp_h265 != NULL)
	{	
		fclose(fp_h265);
		fp_h265 = NULL;
	}
	VideoElem pushElem;
	pushElem.lenght = 0;
	fInput.m_H265VideoSrc->PushBack(pushElem);
}

void VideoOpenSource::doGetNextFrame() {
	//do read from memory
	incomingDataHandler(this);
}
void VideoOpenSource::incomingDataHandler(VideoOpenSource* source) {
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
	VideoElem elem;
	int unHandleCnt = 0;

	//add for file simulation test
	//if(CCircularQueue::getInstance()->Size() < 2)
	if (fInput.m_H265VideoSrc->Size() < 2)
	{
		//read file , push 
		VideoElem pushElem;
		int rdSize = read_h265(pushElem.data, 128*1024);
		pushElem.lenght = rdSize;
		pushElem.FrameCompleted = 1;
		//CCircularQueue::getInstance()->PushBack(pushElem);
		fInput.m_H265VideoSrc->PushBack(pushElem);
	}

	//if(CCircularQueue::getInstance()->PopFront(&elem,unHandleCnt))
	if (fInput.m_H265VideoSrc->PopFront(elem, unHandleCnt))
	{
		//DEBUG_DEBUG("====>>>data:%x elem.lenght:%d",elem.data,elem.lenght);
		fFrameSize = elem.lenght;

		if (fFrameSize <= 0)
		{
			handleClosure(this);
			return;
		}
		else
		{
			if (fFrameSize > fMaxSize)
			{
				debug_log(LEVEL_DEBUG,"----->>>>fFrameSize > fMaxSize lost data!!!");
				fFrameSize = fMaxSize;
			}

			fNumBytesToStream -= fFrameSize;
			memcpy(fTo , elem.data, fFrameSize);

			// Set the 'presentation time':
			if (fPlayTimePerFrame > 0 && fPreferredFrameSize > 0) 
			{
				if (fPresentationTime.tv_sec == 0 && fPresentationTime.tv_usec == 0) 
				{
					// This is the first frame, so use the current time:
					gettimeofday(&fPresentationTime, NULL);
				} 
				else 
				{
					// Increment by the play time of the previous data:
					unsigned uSeconds	= fPresentationTime.tv_usec + fLastPlayTime;
					fPresentationTime.tv_sec += uSeconds/1000000;
					fPresentationTime.tv_usec = uSeconds%1000000;
				}

				// Remember the play time of this data:
				fLastPlayTime = (fPlayTimePerFrame*fFrameSize)/fPreferredFrameSize;
				fDurationInMicroseconds = fLastPlayTime;
			} 
			else 
			{
				// We don't know a specific play time duration for this data,
				// so just record the current time as being the 'presentation time':
				gettimeofday(&fPresentationTime, NULL);
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

		nextTask() = envir().taskScheduler().scheduleDelayedTask(uSecsToDelay,
			(TaskFunc*)incomingDataHandler, this);
	}
}



//===============================================================================================================
static unsigned const samplingFrequencyTable[16] = {
	96000, 88200, 64000, 48000,
	44100, 32000, 24000, 22050,
	16000, 12000, 11025, 8000,
	7350, 0, 0, 0
};

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
{

	fSamplingFrequency = 44100;
	fNumChannels = 2;
	fuSecsPerFrame
		= (1024/*samples-per-frame*/ * 1000000) / fSamplingFrequency/*samples-per-second*/;

	if (fp_acc == NULL)
	{
		fp_acc = fopen("test.aac", "rb");
	}

	//parser spec info
	ParserInfo(fp_acc);
}

void AudioOpenSource::ParserInfo(FILE* fid)
{
	do {
		if (fid == NULL) break;

		unsigned char fixedHeader[4]; // it's actually 3.5 bytes long
		if (fread(fixedHeader, 1, sizeof fixedHeader, fid) < sizeof fixedHeader) break;

		// Check the 'syncword':
		if (!(fixedHeader[0] == 0xFF && (fixedHeader[1] & 0xF0) == 0xF0)) {
			debug_log(LEVEL_ERROR,"Bad 'syncword' at start of ADTS file");
			break;
		}

		// Get and check the 'profile':
		u_int8_t profile = (fixedHeader[2] & 0xC0) >> 6; // 2 bits
		if (profile == 3) {
			debug_log(LEVEL_ERROR,"Bad (reserved) 'profile': 3 in first frame of ADTS file");
			break;
		}

		// Get and check the 'sampling_frequency_index':
		u_int8_t sampling_frequency_index = (fixedHeader[2] & 0x3C) >> 2; // 4 bits
		if (samplingFrequencyTable[sampling_frequency_index] == 0) {
			debug_log(LEVEL_ERROR,"Bad 'sampling_frequency_index' in first frame of ADTS file");
			break;
		}

		// Get and check the 'channel_configuration':
		fNumChannels
			= ((fixedHeader[2] & 0x01) << 2) | ((fixedHeader[3] & 0xC0) >> 6); // 3 bits

		// If we get here, the frame header was OK.
		// Reset the fid to the beginning of the file:
		fseek(fid, 0, SEEK_SET);

		unsigned char audioSpecificConfig[2];
		u_int8_t const audioObjectType = profile + 1;
		audioSpecificConfig[0] = (audioObjectType << 3) | (sampling_frequency_index >> 1);
		audioSpecificConfig[1] = (sampling_frequency_index << 7) | (sampling_frequency_index << 3);
		sprintf(fConfigStr, "%02X%02x", audioSpecificConfig[0], audioSpecificConfig[1]);

		fSamplingFrequency = samplingFrequencyTable[sampling_frequency_index];

		fuSecsPerFrame
			= (1024/*samples-per-frame*/ * 1000000) / fSamplingFrequency/*samples-per-second*/;

		debug_log(LEVEL_DEBUG,"Read first frame: profile %d, "
			"sampling_frequency_index %d => samplingFrequency %d, "
			"channel_configuration %d fuSecsPerFrame:%d\n",
			profile,
			sampling_frequency_index, fSamplingFrequency,
			fNumChannels, fuSecsPerFrame);
	} while (0);


}

AudioOpenSource::~AudioOpenSource()
{
	if (fInput.fOurAudioSource != NULL)
	{
		Medium::close(fInput.fOurAudioSource);
		fInput.fOurAudioSource = NULL;
	}
	if (fp_acc != NULL)
	{
		fclose(fp_acc);
		fp_acc = NULL;
	}

	AudioElem pushElem;
	pushElem.lenght = 0;
	fInput.m_AdtsAudioSrc->PushBack(pushElem);
}

void AudioOpenSource::doGetNextFrame() {
	//do read from memory
	incomingDataHandler(this);
}

void AudioOpenSource::incomingDataHandler(AudioOpenSource* source) {
	//source->incomingDataHandler1();
	source->incomingDataHandler2();
}

void AudioOpenSource::incomingDataHandler1()
{
	// Begin by reading the 7-byte fixed_variable headers:

	unsigned char headers[7];
	if (fread(headers, 1, sizeof headers, fp_acc) < sizeof headers
		|| feof(fp_acc) || ferror(fp_acc)) {
		// The input source has ended:
		//handleClosure(this);
		//return;
		fseek(fp_acc, 0, SEEK_SET);
		fread(headers, 1, sizeof headers, fp_acc);
	}

	// Extract important fields from the headers:
	Boolean protection_absent = headers[1] & 0x01;
	u_int16_t frame_length
		= ((headers[3] & 0x03) << 11) | (headers[4] << 3) | ((headers[5] & 0xE0) >> 5);
#ifdef DEBUG
	u_int16_t syncword = (headers[0] << 4) | (headers[1] >> 4);
	fprintf(stderr, "Read frame: syncword 0x%x, protection_absent %d, frame_length %d\n", syncword, protection_absent, frame_length);
	if (syncword != 0xFFF) fprintf(stderr, "WARNING: Bad syncword!\n");
#endif
	unsigned numBytesToRead
		= frame_length > sizeof headers ? frame_length - sizeof headers : 0;

	// If there's a 'crc_check' field, skip it:
	if (!protection_absent) {
		fseeko(fp_acc, 2, SEEK_CUR);
		numBytesToRead = numBytesToRead > 2 ? numBytesToRead - 2 : 0;
	}

	// Next, read the raw frame data into the buffer provided:
	if (numBytesToRead > fMaxSize) {
		fNumTruncatedBytes = numBytesToRead - fMaxSize;
		numBytesToRead = fMaxSize;
	}
	int numBytesRead = fread(fTo, 1, numBytesToRead, fp_acc);
	if (numBytesRead < 0) numBytesRead = 0;
	fFrameSize = numBytesRead;
	fNumTruncatedBytes += numBytesToRead - numBytesRead;

	// Set the 'presentation time':
	if (fPresentationTime.tv_sec == 0 && fPresentationTime.tv_usec == 0) {
		// This is the first frame, so use the current time:
		gettimeofday(&fPresentationTime, NULL);
	}
	else {
		// Increment by the play time of the previous frame:
		unsigned uSeconds = fPresentationTime.tv_usec + fuSecsPerFrame;
		fPresentationTime.tv_sec += uSeconds / 1000000;
		fPresentationTime.tv_usec = uSeconds % 1000000;
	}

	fDurationInMicroseconds = fuSecsPerFrame;

	// Switch to another task, and inform the reader that he has data:
	nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
		(TaskFunc*)FramedSource::afterGetting, this);
}

void AudioOpenSource::incomingDataHandler2()
{
	//*
	fFrameSize = 0;
	AudioElem elem;
	int unHandleCnt = 0;

	if (fInput.m_AdtsAudioSrc->Size() < 5)
	{
		//read file , push
		AudioElem pushElem;
		int rdSize = read_AAC(pushElem.data, 1024);
		pushElem.lenght = rdSize;
		pushElem.FrameCompleted = 1;
		fInput.m_AdtsAudioSrc->PushBack(pushElem);
	}

	if (!fInput.m_AdtsAudioSrc->PopFront(elem, unHandleCnt))
	{
		fNumTruncatedBytes = 0;
		gettimeofday(&fPresentationTime, NULL);

		nextTask() = envir().taskScheduler().scheduleDelayedTask(500,
			(TaskFunc*)FramedSource::afterGetting, this);
	}
	else
	{
		//DEBUG_DEBUG("====>>>data:%x elem.lenght:%d",elem.data,elem.lenght);
		fFrameSize = elem.lenght;
		if (fFrameSize <= 0)
		{
			handleClosure(this);
			return;
		}
		
		if (fFrameSize > fMaxSize) {
			memcpy(fTo, elem.data, fMaxSize);
		}
		else
		{
			memcpy(fTo, elem.data, fFrameSize);
		}

		// Set the 'presentation time':
		if (fPresentationTime.tv_sec == 0 && fPresentationTime.tv_usec == 0) {
			// This is the first frame, so use the current time:
			gettimeofday(&fPresentationTime, NULL);
		}
		else {
			// Increment by the play time of the previous frame:
			unsigned uSeconds = fPresentationTime.tv_usec + fuSecsPerFrame;
			fPresentationTime.tv_sec += uSeconds / 1000000;
			fPresentationTime.tv_usec = uSeconds % 1000000;
		}

		fDurationInMicroseconds = fuSecsPerFrame;

		// Switch to another task, and inform the reader that he has data:
		nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
		(TaskFunc*)FramedSource::afterGetting, this);
	}
	//*/
}