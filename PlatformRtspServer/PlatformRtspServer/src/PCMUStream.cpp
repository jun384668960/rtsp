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

#include "PCMUStream.hh"
#include "MPEG4GenericRTPSink.hh"
#include "GroupsockHelper.hh"
#include "utils_log.h"
#include "nalu_utils.hh"


PCMUStream * PCMUStream::createNew(UsageEnvironment& env, char* uid, GssLiveConn* liveSource) 
{
    return new PCMUStream(env, uid, 0, 0, 0,liveSource);
}

PCMUStream::PCMUStream(UsageEnvironment& env, char* uid, u_int8_t profile,u_int8_t samplingFrequencyIndex, u_int8_t channelConfiguration ,GssLiveConn* liveSource)
:FramedSource(env) 
{	
	fPresentationTime.tv_sec  = 0;
	fPresentationTime.tv_usec = 0;
	fuSecsPerFrame = (320/*samples-per-frame*/*1000000) / 8000/*samples-per-second*/;
	
	m_LiveSource = liveSource;
	//LOGI_print("new GssLiveConn done %p",m_LiveSource);

	m_ref = -1;
	m_LiveSource->incrementReferenceCount();
	LOGI_print("PCMUStream m_LiveSource:%p referenceCount:%d ", m_LiveSource, m_LiveSource->referenceCount());
}

PCMUStream::~PCMUStream() 
{
	m_LiveSource->decrementReferenceCount();
	
	LOGI_print("~PCMUStream m_LiveSource:%p referenceCount:%d ", m_LiveSource, m_LiveSource->referenceCount());
	if(m_LiveSource->referenceCount() <= 0)
	{
		delete m_LiveSource;
	}
}

void PCMUStream::doGetNextFrame() {
	//do read from memory
	incomingDataHandler(this);
}

void PCMUStream::incomingDataHandler(PCMUStream* source) {
	if (!source->isCurrentlyAwaitingData()) 
	{
		source->doStopGettingFrames(); // we're not ready for the data yet
		return;
	}
	source->incomingDataHandler1();
}

void PCMUStream::incomingDataHandler1()
{
	if(m_LiveSource != NULL)
	{
		fFrameSize = 0;
		unsigned char* pData = NULL;
		int datalen;
		
		if(m_LiveSource->GetAudioFrame(&pData, datalen))
		{
//			LOGI_print("datalen:%d", datalen);
			
			fFrameSize = datalen;
			if(fFrameSize > fMaxSize)
			{
				fNumTruncatedBytes = fFrameSize - fMaxSize;
				fFrameSize = fMaxSize;
			}
			else
			{
				fNumTruncatedBytes = 0;
			}
			memcpy(fTo, pData, fFrameSize);

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

PCMUServerMediaSubsession* PCMUServerMediaSubsession::createNew(UsageEnvironment& env, char* uid, GssLiveConn* liveSource, Boolean reuseFirstSource) 
{
  	return new PCMUServerMediaSubsession(env, uid, liveSource, reuseFirstSource);
}

PCMUServerMediaSubsession::PCMUServerMediaSubsession(UsageEnvironment& env, char* uid, GssLiveConn* liveSource, Boolean reuseFirstSource)
:OnDemandServerMediaSubsession(env, reuseFirstSource)
{
	strcpy(m_Uid, uid);
	m_LiveSource = liveSource;
}

PCMUServerMediaSubsession::~PCMUServerMediaSubsession() 
{
}

FramedSource* PCMUServerMediaSubsession::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) 
{
	estBitrate = 64; // kbps, estimate
	return PCMUStream::createNew(envir(), m_Uid, m_LiveSource);
}

RTPSink* PCMUServerMediaSubsession::createNewRTPSink(Groupsock* rtpGroupsock,unsigned char rtpPayloadTypeIfDynamic,FramedSource* inputSource)
{	
	LOGD_print("enc_type E1_AENC_G711A");
// 	return SimpleRTPSink::createNew(envir(), rtpGroupsock, 8, 8000, "audio", "PCMA", 1, False);//for test
	return SimpleRTPSink::createNew(envir(), rtpGroupsock, 97, 8000, "audio", "PCMA", 1, False);//for test
}
