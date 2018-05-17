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
// Copyright (c) 1996-2013 Live Networks, Inc.  All rights reserved.
// A 'ServerMediaSubsession' object that creates new, unicast, "RTPSink"s
// on demand, from an AAC audio file in ADTS format
// Implementation

#include "ADTSMainStream.hh"
#include "MPEG4GenericRTPSink.hh"
#include "GroupsockHelper.hh"
#include "utils_log.h"
#include "nalu_utils.hh"

#define AUDIOSTREAM 2

struct timeval audiotimespace;
static unsigned const samplingFrequencyTable[16] = 
{
  96000, 88200, 64000, 48000,
  44100, 32000, 24000, 22050,
  16000, 12000, 11025, 8000,
  7350, 0, 0, 0
};

ADTSMainSource * ADTSMainSource::createNew(UsageEnvironment& env, int sub, GssLiveConn* liveSource) 
{
    return new ADTSMainSource(env, sub, 1, 8, 2, liveSource);
	//return new ADTSMainSource(env, fid, profile, sampling_frequency_index, channel_configuration);
}

ADTSMainSource::ADTSMainSource(UsageEnvironment& env, int sub, u_int8_t profile,u_int8_t samplingFrequencyIndex, u_int8_t channelConfiguration, GssLiveConn* liveSource)
: FramedSource(env) 
{
	fSamplingFrequency = samplingFrequencyTable[samplingFrequencyIndex];
	fNumChannels = channelConfiguration == 0 ? 2 : channelConfiguration;
	fuSecsPerFrame
	  = (1024/*samples-per-frame*/*1000000) / fSamplingFrequency/*samples-per-second*/;
	
	// Construct the 'AudioSpecificConfig', and from it, the corresponding ASCII string:
	unsigned char audioSpecificConfig[2];
	u_int8_t const audioObjectType = profile + 1;
	audioSpecificConfig[0] = (audioObjectType<<3) | (samplingFrequencyIndex>>1);
	audioSpecificConfig[1] = (samplingFrequencyIndex<<7) | (channelConfiguration<<3);
	sprintf(fConfigStr, "%02X%02x", audioSpecificConfig[0], audioSpecificConfig[1]);
	m_LiveSource = liveSource;
		
	m_ref = -1;
}

ADTSMainSource::~ADTSMainSource() 
{

}


void ADTSMainSource::doGetNextFrame() {
	//do read from memory
	incomingDataHandler(this);
}

void ADTSMainSource::incomingDataHandler(ADTSMainSource* source) {
	if (!source->isCurrentlyAwaitingData()) 
	{
		source->doStopGettingFrames(); // we're not ready for the data yet
		return;
	}
	source->incomingDataHandler1();
}

void ADTSMainSource::incomingDataHandler1()
{
	if(m_LiveSource != NULL)
	{
		fFrameSize = 0;
		unsigned char* pData = NULL;
		int datalen;

		if(m_LiveSource->GetAudioFrame(&pData, datalen))
		{
			//			LOGI_print("datalen:%d", datalen);

			fFrameSize = datalen - 7;
			if(fFrameSize > fMaxSize)
			{
				fNumTruncatedBytes = fFrameSize - fMaxSize;
				fFrameSize = fMaxSize;
			}
			else
			{
				fNumTruncatedBytes = 0;
			}
			memcpy(fTo, pData+7, fFrameSize);

			gettimeofday(&fPresentationTime, NULL);
			fDurationInMicroseconds = fuSecsPerFrame;

			//LOGI_print("framer audio %p pData length:%d fFrameSize:%d", m_LiveSource, datalen, fFrameSize);
			m_LiveSource->FreeAudioFrame();

			nextTask() = envir().taskScheduler().scheduleDelayedTask(0,(TaskFunc*)FramedSource::afterGetting, this);
		}
		else
		{
			//			LOGE_print("GetVideoFrame error");
			nextTask() = envir().taskScheduler().scheduleDelayedTask(5*1000,
				(TaskFunc*)incomingDataHandler, this);
		}
	}
	else
	{
		LOGE_print("m_LiveSource:%p", m_LiveSource);
		handleClosure();
		return;
	}
}

void ADTSMainServerMediaSubsession::startStream(unsigned clientSessionId,
						void* streamToken,
						TaskFunc* rtcpRRHandler,
						void* rtcpRRHandlerClientData,
						unsigned short& rtpSeqNum,
						unsigned& rtpTimestamp,
						ServerRequestAlternativeByteHandler* serverRequestAlternativeByteHandler,
						void* serverRequestAlternativeByteHandlerClientData) {
						
  OnDemandServerMediaSubsession::startStream(clientSessionId,streamToken,rtcpRRHandler,rtcpRRHandlerClientData,rtpSeqNum,rtpTimestamp,serverRequestAlternativeByteHandler,
	           serverRequestAlternativeByteHandlerClientData);
}

ADTSMainServerMediaSubsession* ADTSMainServerMediaSubsession::createNew(UsageEnvironment& env, int sub, GssLiveConn* liveSource, Boolean reuseFirstSource) 
{
  	return new ADTSMainServerMediaSubsession(env, sub, liveSource, reuseFirstSource);
}

ADTSMainServerMediaSubsession::ADTSMainServerMediaSubsession(UsageEnvironment& env, int sub, GssLiveConn* liveSource, Boolean reuseFirstSource)
:OnDemandServerMediaSubsession(env, reuseFirstSource)
{
	this->sub = sub;
	m_LiveSource = liveSource;
}

ADTSMainServerMediaSubsession::~ADTSMainServerMediaSubsession() 
{
}

FramedSource* ADTSMainServerMediaSubsession::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) 
{
	estBitrate = 96; // kbps, estimate
	return ADTSMainSource::createNew(envir(), sub, m_LiveSource);
}

RTPSink* ADTSMainServerMediaSubsession::createNewRTPSink(Groupsock* rtpGroupsock,
		   						unsigned char rtpPayloadTypeIfDynamic,
		   						FramedSource* inputSource)
{	
	ADTSMainSource* adtsSource = (ADTSMainSource*)inputSource;
	return MPEG4GenericRTPSink::createNew(envir(), rtpGroupsock,
					  rtpPayloadTypeIfDynamic,
					  adtsSource->samplingFrequency(),
					  "audio", "AAC-hbr", adtsSource->configStr(),
					  adtsSource->numChannels());
}
