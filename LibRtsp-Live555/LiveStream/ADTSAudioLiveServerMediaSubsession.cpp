/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
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
// Copyright (c) 1996-2017 Live Networks, Inc.  All rights reserved.
// A 'ServerMediaSubsession' object that creates new, unicast, "RTPSink"s
// on demand, from an AAC audio file in ADTS format
// Implementation

#include "ADTSAudioLiveServerMediaSubsession.hh"
#include "MPEG4GenericRTPSink.hh"
#include "RtspSvr.hh"

ADTSAudioLiveServerMediaSubsession*
ADTSAudioLiveServerMediaSubsession::createNew(UsageEnvironment& env,
LiveStreamMediaSource& wisInput,
Boolean reuseFirstSource) {
	return new ADTSAudioLiveServerMediaSubsession(env, wisInput, reuseFirstSource);
}

ADTSAudioLiveServerMediaSubsession
::ADTSAudioLiveServerMediaSubsession(UsageEnvironment& env,
LiveStreamMediaSource& wisInput, Boolean reuseFirstSource)
: OnDemandServerMediaSubsession(env, reuseFirstSource/*reuseFirstSource True*/ /*reuse the first source*/)
, fReuseFirstSource(reuseFirstSource)
, fWISInput(wisInput){
}

ADTSAudioLiveServerMediaSubsession
::~ADTSAudioLiveServerMediaSubsession() {
}

void ADTSAudioLiveServerMediaSubsession::startStream(unsigned clientSessionId,
						void* streamToken,
						TaskFunc* rtcpRRHandler,
						void* rtcpRRHandlerClientData,
						unsigned short& rtpSeqNum,
						unsigned& rtpTimestamp,
						ServerRequestAlternativeByteHandler* serverRequestAlternativeByteHandler,
						void* serverRequestAlternativeByteHandlerClientData) {

	//clear buffer, audio and video
	fWISInput.SrcPointerSync(0);
	
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
			LOGW_print("ServerMediaSubsession", "ADTSAudioLiveServerMediaSubsession rtpSeqNum:%d rtpTimestamp:%u", rtpSeqNum, rtpTimestamp);
		}
	}
}

void ADTSAudioLiveServerMediaSubsession::pauseStream(unsigned /*clientSessionId*/,
						void* streamToken) {
  // Pausing isn't allowed if multiple clients are receiving data from
  // the same source:
  if (fReuseFirstSource) return;

  StreamState* streamState = (StreamState*)streamToken;
  if (streamState != NULL) streamState->pause();
}


FramedSource* ADTSAudioLiveServerMediaSubsession
::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) {
	estBitrate = 128; // kbps, estimate

	AudioOpenSource* fileSource = fWISInput.audioSource();
	estBitrate = fWISInput.biteRate();

	return fileSource;
}

RTPSink* ADTSAudioLiveServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock,
unsigned char rtpPayloadTypeIfDynamic,
FramedSource* inputSource) {

	LOGD_print("AudioOpenSource", "samplingFrequency:%d configStr:%s numChannels:%d",fWISInput.samplingFrequency(), fWISInput.configStr(), fWISInput.numChannels());
	
	return MPEG4GenericRTPSink::createNew(envir(), rtpGroupsock,
		rtpPayloadTypeIfDynamic,
		fWISInput.samplingFrequency(),
		"audio", "AAC-hbr", fWISInput.configStr(),
		fWISInput.numChannels());
}
