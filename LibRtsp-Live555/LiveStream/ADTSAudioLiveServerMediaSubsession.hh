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
// on demand, from a H265 Elementary Stream video file.
// C++ header

#ifndef _ADTS_AUDIO_LIVE_SERVER_MEDIA_SUBSESSION_HH
#define _ADTS_AUDIO_LIVE_SERVER_MEDIA_SUBSESSION_HH

#ifndef _ON_DEMAND_SERVER_MEDIA_SUBSESSION_HH
#include "OnDemandServerMediaSubsession.hh"
#endif

#include "LiveStreamMediaSource.hh"

class ADTSAudioLiveServerMediaSubsession : public OnDemandServerMediaSubsession {
public:
	static ADTSAudioLiveServerMediaSubsession*
		createNew(UsageEnvironment& env, LiveStreamMediaSource& wisInput, Boolean reuseFirstSource);

protected:
	ADTSAudioLiveServerMediaSubsession(UsageEnvironment& env,
		LiveStreamMediaSource& wisInput, Boolean reuseFirstSource);
	// called only by createNew();
	virtual ~ADTSAudioLiveServerMediaSubsession();

void startStream(unsigned clientSessionId,
						void* streamToken,
						TaskFunc* rtcpRRHandler,
						void* rtcpRRHandlerClientData,
						unsigned short& rtpSeqNum,
						unsigned& rtpTimestamp,
						ServerRequestAlternativeByteHandler* serverRequestAlternativeByteHandler,
						void* serverRequestAlternativeByteHandlerClientData);
virtual void pauseStream(unsigned /*clientSessionId*/, void* streamToken);
protected: // redefined virtual functions
	virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
		unsigned& estBitrate);
	virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
		unsigned char rtpPayloadTypeIfDynamic,
		FramedSource* inputSource);
private:
	Boolean fReuseFirstSource;
	LiveStreamMediaSource& fWISInput;
};

#endif
