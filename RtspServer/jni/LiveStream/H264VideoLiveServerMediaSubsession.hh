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
// on demand, from a H264 Elementary Stream video file.
// C++ header

#ifndef _H264_VIDEO_LIVE_SERVER_MEDIA_SUBSESSION_HH
#define _H264_VIDEO_LIVE_SERVER_MEDIA_SUBSESSION_HH

#ifndef _ON_DEMAND_SERVER_MEDIA_SUBSESSION_HH
#include "OnDemandServerMediaSubsession.hh"
#endif

#include "LiveStreamMediaSource.hh"

class H264VideoLiveServerMediaSubsession : public OnDemandServerMediaSubsession {
public:
	static H264VideoLiveServerMediaSubsession*
		createNew(UsageEnvironment& env, LiveStreamMediaSource& wisInput, Boolean reuseFirstSource);

	// Used to implement "getAuxSDPLine()":
	void checkForAuxSDPLine1();
	void afterPlayingDummy1();

protected:
	H264VideoLiveServerMediaSubsession(UsageEnvironment& env,
		LiveStreamMediaSource& wisInput, Boolean reuseFirstSource);
	// called only by createNew();
	virtual ~H264VideoLiveServerMediaSubsession();

	void setDoneFlag() { fDoneFlag = ~0; }

	virtual void startStream(unsigned clientSessionId, void* streamToken,
		   TaskFunc* rtcpRRHandler,
		   void* rtcpRRHandlerClientData,
		   unsigned short& rtpSeqNum,
	                   unsigned& rtpTimestamp,
		   ServerRequestAlternativeByteHandler* serverRequestAlternativeByteHandler,
	                   void* serverRequestAlternativeByteHandlerClientData);
	virtual void pauseStream(unsigned /*clientSessionId*/, void* streamToken);
	virtual void seekStream(unsigned clientSessionId, void* streamToken, double& seekNPT, double streamDuration, u_int64_t& numBytes);
    virtual void seekStream(unsigned clientSessionId, void* streamToken, char*& absStart, char*& absEnd);
  

protected: // redefined virtual functions
	virtual char const* getAuxSDPLine(RTPSink* rtpSink,
		FramedSource* inputSource);
	virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
		unsigned& estBitrate);
	virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
		unsigned char rtpPayloadTypeIfDynamic,
		FramedSource* inputSource);

private:
    Boolean fReuseFirstSource;
	char* fAuxSDPLine;
	char fDoneFlag; // used when setting up "fAuxSDPLine"
	RTPSink* fDummyRTPSink; // ditto

	LiveStreamMediaSource& fWISInput;
};

#endif
