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
#include "debuginfo.h"

ADTSAudioLiveServerMediaSubsession*
ADTSAudioLiveServerMediaSubsession::createNew(UsageEnvironment& env,
LiveStreamMediaSource& wisInput,
Boolean reuseFirstSource) {
	return new ADTSAudioLiveServerMediaSubsession(env, wisInput, reuseFirstSource);
}

ADTSAudioLiveServerMediaSubsession
::ADTSAudioLiveServerMediaSubsession(UsageEnvironment& env,
LiveStreamMediaSource& wisInput, Boolean reuseFirstSource)
: OnDemandServerMediaSubsession(env, True /*reuse the first source*/),
fWISInput(wisInput){
}

ADTSAudioLiveServerMediaSubsession
::~ADTSAudioLiveServerMediaSubsession() {
}

FramedSource* ADTSAudioLiveServerMediaSubsession
::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) {
	estBitrate = 96; // kbps, estimate

	FramedSource* fileSource = fWISInput.audioSource();

	return fileSource;
}

RTPSink* ADTSAudioLiveServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock,
unsigned char rtpPayloadTypeIfDynamic,
FramedSource* inputSource) {

	AudioOpenSource* adtsSource = (AudioOpenSource*)inputSource;
	debug_log(LEVEL_DEBUG, "samplingFrequency:%d configStr:%s numChannels:%d",adtsSource->samplingFrequency(), adtsSource->configStr(), adtsSource->numChannels());
	
	return MPEG4GenericRTPSink::createNew(envir(), rtpGroupsock,
		rtpPayloadTypeIfDynamic,
		adtsSource->samplingFrequency(),
		"audio", "AAC-hbr", adtsSource->configStr(),
		adtsSource->numChannels());
}
