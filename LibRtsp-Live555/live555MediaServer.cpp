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
// Copyright (c) 1996-2017, Live Networks, Inc.  All rights reserved
// LIVE555 Media Server
// main program

#include <BasicUsageEnvironment.hh>
#include "DynamicRTSPServer.hh"
#include "version.hh"
//#include "debuginfo.h"
//#include "RtspServer.hh"
#include "RtspSvr.hh"
#include <stdio.h>
#include "H264Handle.h"
#include "mp4v2/mp4v2.h"
#include "sps_pps_parser.h"
#include "Mp4Parser.h"

int STOP = 1;

#include <sys/time.h>
int64_t GetTickCount()
{
	struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (int64_t)now.tv_sec * 1000000 + now.tv_nsec / 1000;
}


void* ThreadRtspServerPutV(void* arg)
{
	
	CRtspServer* server = (CRtspServer*)arg;

	/*
	unsigned char nalu[1024*128];
	uint64_t pts = 0;
	int length = 0;
	
	unsigned char *frame = (unsigned char*)malloc(MAX_NALU_SIZE);
	while((length = h264Handle->GetAnnexbFrame(frame, MAX_NALU_SIZE))>0 && STOP)
	{
		if(server != NULL)
		{
			fprintf(stderr, "h264Handle->ReadOneNalu length:%d\n", length);
			server->PushVideo(frame, length, pts);
			int type = frame[4] & 0x1f;
			if(type == 5 || type == 1)
			{
				pts += 41000;
				usleep(40000);
			}
		}
	}
	free(frame);
	delete h264Handle;
	*/
	char nalHeader[] = {0x00, 0x00, 0x00, 0x01};
	CMP4Parser* parser = new CMP4Parser();
	parser->SetDataSource("S06.mp4");
	parser->SelectTrack(0);
	u_int8_t* sps;
	uint32_t spsLen;
	u_int8_t* pps;
	uint32_t ppsLen;
	bool r = parser->GetSpsAndPps(sps, spsLen, pps, ppsLen);
	
	unsigned char spspps[128] = {0};
	memcpy(spspps, nalHeader, 4);
	memcpy(spspps + 4, sps, spsLen);
	server->PushVideo(spspps, spsLen + 4, 0);
	memcpy(spspps + 4, pps, ppsLen);
	server->PushVideo(spspps, ppsLen + 4, 0);
	
	int perFrameUs = 1000000/parser->GetFrameRate();
	
	unsigned char* data = new unsigned char[512*1024];
	int length = 512*1024;
	uint64_t pts;
	int64_t tt = GetTickCount();
	int64_t t = GetTickCount();
	while(parser->ReadSampleData(data,length,pts))
	{	
		int offset = 0;
		int type = data[4] & 0x1f;
		t = GetTickCount();
		
		if(type != 5 && type != 1)
		{
			int _len = data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3];
			fprintf(stderr, "_len:%x _len:%d data[0]:%x data[1]:%x data[2]:%x data[3]:%x\n",_len, _len, data[0], data[1], data[2], data[3]);
			offset = _len + 4;
		}
		int spend = t - tt;
		
        memcpy(data + offset, nalHeader, 4);
		if(server != NULL)
		{
			type = data[offset + 4] & 0x1f;
			
			if(type == 5 || type == 1)
			{
				fprintf(stderr, "parser->ReadSampleData type:%d length:%d perFrameUs:%d pts:%d\n", type, length, perFrameUs, pts);
				server->PushVideo(data + offset, length - offset, pts * 1000);
				if(spend < 0 || spend > 41000)
				{
					spend = 0;
				}
				
				usleep(41000 - spend);
				
			}
			else
			{
				fprintf(stderr, ">>>>>>>>>>>> type:%d\n ", type);
			}
			tt = GetTickCount();
		}
		length = 512*1024;
	}
	free(data);
	delete parser;
	
	return NULL;
}

void* ThreadRtspServerPutA(void* arg)
{
	
	CRtspServer* server = (CRtspServer*)arg;

	char nalHeader[] = {0x00, 0x00, 0x00, 0x01};
	CMP4Parser* parser = new CMP4Parser();
	parser->SetDataSource("S07.mp4");
	parser->SelectTrack(1);
	
	uint8_t profile = parser->GetAudioProfile();
	uint8_t channles = parser->GetAudioChannles();
	uint8_t freIdx = parser->GetAudioFreIdx();

	server->SetParamConfig("audioprofile", "1");
	server->SetParamConfig("audiofrequency", "44100");
	server->SetParamConfig("audiochannels", "2");
	server->SetParamConfig("audiobiterate", "128000");

	unsigned char* data = new unsigned char[2*1024];
	int length = 2*1024;
	uint8_t packet[7];
	
	uint64_t pts;
	uint64_t _pts = 0;
	int64_t tt = GetTickCount();
	int64_t t = GetTickCount();
	while(parser->ReadSampleData(data,length,pts))
	{	
		fprintf(stderr, ">>>>>>>>>>>> data:%p length:%d pts:%u\n ", data,length,pts);
		t = GetTickCount();
		int spend = t - tt;
		server->PushAudio(data, length, pts * 1000);
		//server->PushAudio(data, length, pts * 1000);

		if(spend < 0 || spend > 22000)
		{
			spend = 0;
		}
		
		unsigned fuSecsPerFrame
		= (1024 * 1000000) / 44100;
		usleep(fuSecsPerFrame - spend);
		_pts += fuSecsPerFrame;
		
		tt = GetTickCount();
		length = 2*1024;
	}
	free(data);
	delete parser;
	
	return NULL;
}


int main(int argc, char** argv) {

	//debug_set(LEVEL_DEBUG, TARGET_STDERR, "RtspServer.txt");
	pthread_t m_pThreadV;
	pthread_t m_pThreadA;
	CRtspServer* server = NULL;

	server = new CRtspServer();
	//server->SetParamConfig("enableaudio", "0");
	if( server != NULL)
		server->PrepareStream("LiveStream");
	if( server != NULL)
			server->Start(8554);
	
	STOP = 1;
	
	//*
	if(pthread_create(&m_pThreadA,NULL, ThreadRtspServerPutA,server))
	{
		LOGE_print("CRtspServer", "ThreadRtspServerProcImpl false!");
		return -1;
	}
	//*/
	//*
	if(pthread_create(&m_pThreadV,NULL, ThreadRtspServerPutV,server))
	{
		LOGE_print("CRtspServer", "ThreadRtspServerProcImpl false!");
		return -1;
	}
	//*/

	int ch;
			
	printf("===============\nc:Create()\ns:Start()\nr:Restart()\np:Stop()\nd:Destory()\n===============\nplease put in char:");
	while(1)
	{
		ch = getchar();
		if(ch == '\n')
		{
			continue;
		}
		
		switch(ch)
		{
		case 'c':
			server = new CRtspServer();
			STOP = 1;
			if(pthread_create(&m_pThreadV,NULL, ThreadRtspServerPutV,server))
			{
				LOGE_print("CRtspServer", "ThreadRtspServerProcImpl false!");
				return -1;
			}
				
			if( server != NULL)
				server->PrepareStream("LiveStream");
			break;
		case 's':
			if( server != NULL)
				server->Start(8554);
			break;
		case 'r':
			//server->Restart();
			break;
		case 'p':
			if( server != NULL)
			server->Stop();
			break;
		case 'd':
			STOP = 0;
			::pthread_join(m_pThreadV, 0); 
			if( server != NULL)
			server->ReleaseStream();
			delete server;
			server = NULL;
			break;
		default:
			break;
		}
		
		printf("===============\nc:Create()\ns:Start()\nr:Restart()\np:Stop()\nd:Destory()\n===============\nplease put in char:");
		//*/
		
	}

  return 0;
}
