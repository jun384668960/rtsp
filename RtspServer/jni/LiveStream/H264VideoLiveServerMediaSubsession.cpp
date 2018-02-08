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
#include "debuginfo.h"

H264VideoLiveServerMediaSubsession*
H264VideoLiveServerMediaSubsession::createNew(UsageEnvironment& env,
LiveStreamMediaSource& wisInput,
Boolean reuseFirstSource) {
	return new H264VideoLiveServerMediaSubsession(env, wisInput, reuseFirstSource);
}

H264VideoLiveServerMediaSubsession::H264VideoLiveServerMediaSubsession(UsageEnvironment& env,
	LiveStreamMediaSource& wisInput, Boolean reuseFirstSource)
	: OnDemandServerMediaSubsession(env, True /*reuse the first source*/),
	fAuxSDPLine(NULL), fDoneFlag(0), fDummyRTPSink(NULL), fWISInput(wisInput) {
}

H264VideoLiveServerMediaSubsession::~H264VideoLiveServerMediaSubsession() {
	delete[] fAuxSDPLine;
}

void H264VideoLiveServerMediaSubsession::startStream(unsigned clientSessionId,
						void* streamToken,
						TaskFunc* rtcpRRHandler,
						void* rtcpRRHandlerClientData,
						unsigned short& rtpSeqNum,
						unsigned& rtpTimestamp,
						ServerRequestAlternativeByteHandler* serverRequestAlternativeByteHandler,
						void* serverRequestAlternativeByteHandlerClientData) {
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
			rtpTimestamp = rtpSink->presetNextTimestamp();
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
	debug_log(LEVEL_DEBUG,"H264VideoLiveServerMediaSubsession::seekStream1");
	//call out size seek -- add by donyj
	
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
	debug_log(LEVEL_DEBUG,"H264VideoLiveServerMediaSubsession::seekStream2");
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

	return fAuxSDPLine;
}

FramedSource* H264VideoLiveServerMediaSubsession::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) {
	estBitrate = 1500; // kbps, estimate

	// Create the video source:
	FramedSource* fileSource = fWISInput.videoSource();

	if (fileSource == NULL)
		return NULL;

	// Create a framer for the Video Elementary Stream:
	return H264VideoStreamFramer::createNew(envir(), fileSource);
}

RTPSink* H264VideoLiveServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock,
unsigned char rtpPayloadTypeIfDynamic,
FramedSource* /*inputSource*/) {
	return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}
