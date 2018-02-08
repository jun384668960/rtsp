/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2014 Live Networks, Inc.  All rights reserved.
// A 'ServerMediaSubsession' object that creates new, unicast, "RTPSink"s
// on demand, from a H264 video file.
// Implementation

#include "H264VideoLiveServerMediaSubsession.hh"
#include "LiveStreamMediaSource.hh"
#include "H264VideoRTPSink.hh"
#include "H264VideoStreamFramer.hh"
#include "H264VideoLiveDiscreteFramer.hh"
#include "Base64.hh"
#include "RtspSvr.hh"

H264VideoLiveServerMediaSubsession*
H264VideoLiveServerMediaSubsession::createNew(UsageEnvironment& env,
LiveStreamMediaSource& wisInput,
Boolean reuseFirstSource) {
	return new H264VideoLiveServerMediaSubsession(env, wisInput, reuseFirstSource);
}

H264VideoLiveServerMediaSubsession::H264VideoLiveServerMediaSubsession(UsageEnvironment& env,
	LiveStreamMediaSource& wisInput, Boolean reuseFirstSource)
	: OnDemandServerMediaSubsession(env, reuseFirstSource/*reuseFirstSource True*/ /*reuse the first source*/)
	, fReuseFirstSource(reuseFirstSource)
	, fAuxSDPLine(NULL), fDoneFlag(0), fDummyRTPSink(NULL), fWISInput(wisInput) 
	,mSps(NULL),mSpsLen(0),mPps(NULL),mPpsLen(0){

	mSps = new u_int8_t[64];      
	mSpsLen = 63;	
	mPps = new u_int8_t[64];      
	mPpsLen = 63;

	fMultiplexRTCPWithRTP = false;
}

H264VideoLiveServerMediaSubsession::~H264VideoLiveServerMediaSubsession() {
	delete[] fAuxSDPLine;
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
	LOGD_print("DynamicRTSPServer", "~H264VideoLiveServerMediaSubsession");
}

void H264VideoLiveServerMediaSubsession::startStream(unsigned clientSessionId,
						void* streamToken,
						TaskFunc* rtcpRRHandler,
						void* rtcpRRHandlerClientData,
						unsigned short& rtpSeqNum,
						unsigned& rtpTimestamp,
						ServerRequestAlternativeByteHandler* serverRequestAlternativeByteHandler,
						void* serverRequestAlternativeByteHandlerClientData) {

	//clear buffer, audio and video
	fWISInput.SrcPointerSync(0);
	//demand idr
	fWISInput.VideoDemandIDR();
	
	StreamState* streamState = (StreamState*)streamToken;
	Destinations* destinations
		= (Destinations*)(fDestinationsHashTable->Lookup((char const*)clientSessionId));
	if (streamState != NULL) 
	{
		streamState->startPlaying(destinations, clientSessionId,
				      rtcpRRHandler, rtcpRRHandlerClientData,
				      serverRequestAlternativeByteHandler, serverRequestAlternativeByteHandlerClientData);
		RTPSink* rtpSink = streamState->rtpSink(); // alias
		if (rtpSink != NULL) 
		{
			rtpSeqNum = rtpSink->currentSeqNo();
			//rtpTimestamp = rtpSink->presetNextTimestamp();
			rtpTimestamp = 0;
			LOGD_print("ServerMediaSubsession", "H264VideoLiveServerMediaSubsession rtpSeqNum:%d rtpTimestamp:%u", rtpSeqNum, rtpTimestamp);
		}
	}
}


void H264VideoLiveServerMediaSubsession::pauseStream(unsigned /*clientSessionId*/,
						void* streamToken) {
  // Pausing isn't allowed if multiple clients are receiving data from
  // the same source:
  if (fReuseFirstSource) return;

  StreamState* streamState = (StreamState*)streamToken;
  if (streamState != NULL) streamState->pause();
}

void H264VideoLiveServerMediaSubsession::seekStream(unsigned /*clientSessionId*/,
					       void* streamToken, double& seekNPT, double streamDuration, u_int64_t& numBytes) {
	numBytes = 0; // by default: unknown

	// Seeking isn't allowed if multiple clients are receiving data from the same source:
	if (fReuseFirstSource) return;

	StreamState* streamState = (StreamState*)streamToken;
	if (streamState != NULL && streamState->mediaSource() != NULL) {
		seekStreamSource(streamState->mediaSource(), seekNPT, streamDuration, numBytes);

		streamState->startNPT() = (float)seekNPT;
		RTPSink* rtpSink = streamState->rtpSink(); // alias
		if (rtpSink != NULL) rtpSink->resetPresentationTimes();
	}
}

void H264VideoLiveServerMediaSubsession::seekStream(unsigned /*clientSessionId*/,
					       void* streamToken, char*& absStart, char*& absEnd) {
	// Seeking isn't allowed if multiple clients are receiving data from the same source:
	if (fReuseFirstSource) return;

	StreamState* streamState = (StreamState*)streamToken;
	if (streamState != NULL && streamState->mediaSource() != NULL) {
		seekStreamSource(streamState->mediaSource(), absStart, absEnd);
	}
}


static void afterPlayingDummy(void* clientData) {
	H264VideoLiveServerMediaSubsession* subsess = (H264VideoLiveServerMediaSubsession*)clientData;
	subsess->afterPlayingDummy1();
}

void H264VideoLiveServerMediaSubsession::afterPlayingDummy1() {
	// Unschedule any pending 'checking' task:
	envir().taskScheduler().unscheduleDelayedTask(nextTask());
	// Signal the event loop that we're done:
	setDoneFlag();
}

static void checkForAuxSDPLine(void* clientData) {
	H264VideoLiveServerMediaSubsession* subsess = (H264VideoLiveServerMediaSubsession*)clientData;
	subsess->checkForAuxSDPLine1();
}

void H264VideoLiveServerMediaSubsession::checkForAuxSDPLine1() {
	char const* dasl;

	if (fAuxSDPLine != NULL) {
		// Signal the event loop that we're done:
		setDoneFlag();
	}
	else if (fDummyRTPSink != NULL && (dasl = fDummyRTPSink->auxSDPLine()) != NULL) {
		fAuxSDPLine = strDup(dasl);
		fDummyRTPSink = NULL;

		// Signal the event loop that we're done:
		setDoneFlag();
	}
	else if (!fDoneFlag) {
		// try again after a brief delay:
		int uSecsToDelay = 100000; // 100 ms
		nextTask() = envir().taskScheduler().scheduleDelayedTask(uSecsToDelay,
			(TaskFunc*)checkForAuxSDPLine, this);
	}
}

char const* H264VideoLiveServerMediaSubsession::getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource) {

	if (fAuxSDPLine != NULL) return fAuxSDPLine; // it's already been set up (for a previous client)

	if (fDummyRTPSink == NULL) { // we're not already setting it up for another, concurrent stream
		// Note: For H264 video files, the 'config' information (used for several payload-format
		// specific parameters in the SDP description) isn't known until we start reading the file.
		// This means that "rtpSink"s "auxSDPLine()" will be NULL initially,
		// and we need to start reading data from our file until this changes.
		fDummyRTPSink = rtpSink;

		// Start reading the file:
		fDummyRTPSink->startPlaying(*inputSource, afterPlayingDummy, this);

		// Check whether the sink's 'auxSDPLine()' is ready:
		checkForAuxSDPLine(this);
	}

	envir().taskScheduler().doEventLoop(&fDoneFlag);
	LOGD_print("ServerMediaSubsession", "H264VideoLiveServerMediaSubsession getAuxSDPLine:\n%s", fAuxSDPLine);
	return fAuxSDPLine;
}

char const* H264VideoLiveServerMediaSubsession::sdpLines() {
	LOGD_print("ServerMediaSubsession", "fSDPLines:\n%s", fSDPLines);
	if (fSDPLines == NULL) {
		// We need to construct a set of SDP lines that describe this
		// subsession (as a unicast stream).  To do so, we first create
		// dummy (unused) source and "RTPSink" objects,
		// whose parameters we use for the SDP lines:
		unsigned estBitrate;
		FramedSource* inputSource = createNewStreamSource(0, estBitrate);
		if (inputSource == NULL) return NULL; // file not found

		struct in_addr dummyAddr;
		dummyAddr.s_addr = 0;
		Groupsock* dummyGroupsock = createGroupsock(dummyAddr, 0);
		unsigned char rtpPayloadType = 96 + trackNumber()-1; // if dynamic
		RTPSink* dummyRTPSink = createNewRTPSink(dummyGroupsock, rtpPayloadType, inputSource);
		if (dummyRTPSink != NULL && dummyRTPSink->estimatedBitrate() > 0) estBitrate = dummyRTPSink->estimatedBitrate();

		setSDPLinesFromRTPSink(dummyRTPSink, inputSource, estBitrate);
		Medium::close(dummyRTPSink);
		delete dummyGroupsock;
		closeStreamSource(inputSource);
	}

	return fSDPLines;
}

void H264VideoLiveServerMediaSubsession
::setSDPLinesFromRTPSink(RTPSink* rtpSink, FramedSource* inputSource, unsigned estBitrate) {
	if (rtpSink == NULL) return;

	char const* mediaType = rtpSink->sdpMediaType();
	unsigned char rtpPayloadType = rtpSink->rtpPayloadType();
	AddressString ipAddressStr(fServerAddressForSDP);
	char* rtpmapLine = rtpSink->rtpmapLine();
	char const* rtcpmuxLine = fMultiplexRTCPWithRTP ? "a=rtcp-mux\r\n" : "";
	char const* rangeLine = rangeSDPLine();
	char const* auxSDPLine = getAuxSDPLine(rtpSink, inputSource);
	if (auxSDPLine == NULL) auxSDPLine = "";

	char const* const sdpFmt =
	"m=%s %u RTP/AVP %d\r\n"
	"c=IN IP4 %s\r\n"
	"b=AS:%u\r\n"
	"%s"
	"%s"
	"%s"
	"%s"
	"a=control:%s\r\n";
	unsigned sdpFmtSize = strlen(sdpFmt)
	+ strlen(mediaType) + 5 /* max short len */ + 3 /* max char len */
	+ strlen(ipAddressStr.val())
	+ 20 /* max int len */
	+ strlen(rtpmapLine)
	+ strlen(rtcpmuxLine)
	+ strlen(rangeLine)
	+ strlen(auxSDPLine)
	+ strlen(trackId());
	char* sdpLines = new char[sdpFmtSize];
	sprintf(sdpLines, sdpFmt,
	  mediaType, // m= <media>
	  fPortNumForSDP, // m= <port>
	  rtpPayloadType, // m= <fmt list>
	  ipAddressStr.val(), // c= address
	  estBitrate, // b=AS:<bandwidth>
	  rtpmapLine, // a=rtpmap:... (if present)
	  rtcpmuxLine, // a=rtcp-mux:... (if present)
	  rangeLine, // a=range:... (if present)
	  auxSDPLine, // optional extra SDP line
	  trackId()); // a=control:<track-id>
	delete[] (char*)rangeLine; delete[] rtpmapLine;

	fSDPLines = strDup(sdpLines);
	delete[] sdpLines;
}


FramedSource* H264VideoLiveServerMediaSubsession::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) {
	estBitrate = 160000; // kbps, estimate

	// Create the video source:
	FramedSource* fileSource = fWISInput.videoSource();

	if (fileSource == NULL)
		return NULL;

	// Create a framer for the Video Elementary Stream:
	//return H264VideoStreamFramer::createNew(envir(), fileSource);
	H264VideoLiveDiscreteFramer* framer = H264VideoLiveDiscreteFramer::createNew(envir(), fileSource);
	if (framer == NULL)
		return NULL;

	u_int8_t* sps = NULL;
	u_int8_t* pps = NULL;
	if(fWISInput.GetSpsAndPps(sps, mSpsLen, pps, mPpsLen))
	{
		memcpy(mSps, sps, mSpsLen);
		memcpy(mPps, pps, mPpsLen);
		framer->setVPSandSPSandPPS(NULL, 0, mSps, mSpsLen, mPps, mPpsLen);
	}
	
	framer->SetFrameRate(fWISInput.FrameRate());

	LOGD_print("ServerMediaSubsession", "H264VideoLiveServerMediaSubsession createNewStreamSource");
	
	return framer;
}

RTPSink* H264VideoLiveServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock,
unsigned char rtpPayloadTypeIfDynamic,
FramedSource* /*inputSource*/) {
	OutPacketBuffer::maxSize = DEFAULT_MAX_VEDIOFRAME_SIZE;
	return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}
