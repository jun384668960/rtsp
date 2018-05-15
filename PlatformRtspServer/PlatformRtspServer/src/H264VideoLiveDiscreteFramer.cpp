#include "H264VideoLiveDiscreteFramer.hh"
#include "MPEGVideoStreamFramer.hh"

H264VideoLiveDiscreteFramer*
  H264VideoLiveDiscreteFramer::createNew(UsageEnvironment& env, FramedSource* inputSource)
{
	return new H264VideoLiveDiscreteFramer(env, inputSource);
}

void H264VideoLiveDiscreteFramer::SetFrameRate(double frameRate)
{
	fFrameRate = frameRate;
}

H264VideoLiveDiscreteFramer
::H264VideoLiveDiscreteFramer(UsageEnvironment& env, FramedSource* inputSource)
	: H264VideoStreamFramer(env, inputSource, False/*don't create a parser*/, False) {
	
	fFrameRate = 20.0;
	//fNewPresentationTime = fPresentationTimeBase;
	fNewPresentationTime.tv_sec = 0;
	fNewPresentationTime.tv_usec = 0;
}

H264VideoLiveDiscreteFramer::~H264VideoLiveDiscreteFramer() {
	fFrameRate = 20.0;
}

void H264VideoLiveDiscreteFramer::doGetNextFrame() {
  // Arrange to read data (which should be a complete H.264 or H.265 NAL unit)
  // from our data source, directly into the client's input buffer.
  // After reading this, we'll do some parsing on the frame.
	fInputSource->getNextFrame(fTo,fMaxSize,                             
		afterGettingFrame,this,                             
		FramedSource::handleClosure,this);
}

void H264VideoLiveDiscreteFramer
::afterGettingFrame(void* clientData,
	unsigned frameSize,
	unsigned numTruncatedBytes,
	struct timeval presentationTime,
	unsigned durationInMicroseconds) {
	H264VideoLiveDiscreteFramer* source = (H264VideoLiveDiscreteFramer*)clientData;
	source->afterGettingFrame1(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

void H264VideoLiveDiscreteFramer
::afterGettingFrame1(unsigned frameSize,
	unsigned numTruncatedBytes,
	struct timeval presentationTime,
	unsigned durationInMicroseconds) {

#if 1

	// Get the "nal_unit_type", to see if this NAL unit is one that we want to save a copy of:
	u_int8_t nal_unit_type;
	if (fHNumber == 264 && frameSize >= 1) {
		nal_unit_type = fTo[0] & 0x1F;
	}
	else if (fHNumber == 265 && frameSize >= 2) {
		nal_unit_type = (fTo[0] & 0x7E) >> 1;
	}
	else {
	  // This is too short to be a valid NAL unit, so just assume a bogus nal_unit_type
		nal_unit_type = 0xFF;
	}

	  // Begin by checking for a (likely) common error: NAL units that (erroneously) begin with a
	  // 0x00000001 or 0x000001 'start code'.  (Those start codes should only be in byte-stream data;
	  // *not* data that consists of discrete NAL units.)
	  // Once again, to be clear: The NAL units that you feed to a "H264or5VideoStreamDiscreteFramer"
	  // MUST NOT include start codes.
	if (frameSize >= 4 && fTo[0] == 0 && fTo[1] == 0 && ((fTo[2] == 0 && fTo[3] == 1) || fTo[2] == 1)) {
		envir() << "H264or5VideoStreamDiscreteFramer error: MPEG 'start code' seen in the input\n";
	}
	else if (isVPS(nal_unit_type)) { // Video parameter set (VPS)
		saveCopyOfVPS(fTo, frameSize);
	}
	else if (isSPS(nal_unit_type)) { // Sequence parameter set (SPS)
		saveCopyOfSPS(fTo, frameSize);
	}
	else if (isPPS(nal_unit_type)) { // Picture parameter set (PPS)
		saveCopyOfPPS(fTo, frameSize);
	}

	fPictureEndMarker = fHNumber == 264
	  ? (nal_unit_type <= 5 && nal_unit_type > 0)
	  : (nal_unit_type <= 31);
#endif

	// Finally, complete delivery to the client:
	fFrameSize = frameSize;
	fNumTruncatedBytes = numTruncatedBytes;
	fPresentationTime = presentationTime;
	
	fDurationInMicroseconds = durationInMicroseconds;
	if (nal_unit_type == 7 || nal_unit_type == 8)
	{
		fDurationInMicroseconds = 0;
	}
	afterGetting(this);
}

