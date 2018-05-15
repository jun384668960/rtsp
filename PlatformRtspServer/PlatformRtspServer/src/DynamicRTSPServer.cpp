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
// Copyright (c) 1996-2018, Live Networks, Inc.  All rights reserved
// A subclass of "RTSPServer" that creates "ServerMediaSession"s on demand,
// based on whether or not the specified stream name exists as a file
// Implementation

#include "DynamicRTSPServer.hh"
#include <liveMedia.hh>
#include <string.h>
#include "H264MainStream.hh"
#include "H264SubStream.hh"
#include "ADTSMainStream.hh"
#include "PCMUStream.hh"
#include "utils_log.h"
#include "Log.h"

DynamicRTSPServer*
DynamicRTSPServer::createNew(UsageEnvironment& env, Port ourPort,
			     UserAuthenticationDatabase* authDatabase,
			     unsigned reclamationTestSeconds,
				 bool supportTls12 ) {
  int ourSocket = setUpOurSocket(env, ourPort);
  if (ourSocket == -1) return NULL;

  return new DynamicRTSPServer(env, ourSocket, ourPort, authDatabase, reclamationTestSeconds, supportTls12);
}

DynamicRTSPServer::DynamicRTSPServer(UsageEnvironment& env, int ourSocket,
				     Port ourPort,
				     UserAuthenticationDatabase* authDatabase, unsigned reclamationTestSeconds, bool supportTls12)
  : RTSPServerSupportingHTTPStreaming(env, ourSocket, ourPort, authDatabase, reclamationTestSeconds,supportTls12) {
}

DynamicRTSPServer::~DynamicRTSPServer() {
}

static ServerMediaSession* createNewSMS(UsageEnvironment& env,
					char const* fileName, FILE* fid, void* pClientConn); // forward

ServerMediaSession* DynamicRTSPServer
::lookupServerMediaSession(char const* streamName, void* pClientConn, Boolean isFirstLookupInSession) {
	if(strstr(streamName,"uid=") != NULL)
	{
		//LOGI_print("streamName:%s%p", streamName, pClientConn);
		ServerMediaSession* sms = NULL;
		char pcharClientconn[64] = {0};
		sprintf(pcharClientconn,"%p",pClientConn);
		if(strstr(streamName,pcharClientconn) != NULL)
			sms = RTSPServer::lookupServerMediaSession(streamName);
		else
			sms = RTSPServer::lookupServerMediaSession(streamName,pClientConn);
		Boolean smsExists = sms != NULL;

		if (smsExists && isFirstLookupInSession) { 
			// Remove the existing "ServerMediaSession" and create a new one, in case the underlying
			// file has changed in some way:
			removeServerMediaSession(sms); 
			sms = NULL;
		} 
		if (sms == NULL) {
			sms = createNewSMS(envir(), streamName, NULL, pClientConn); 
			addServerMediaSession(sms);
		}
		return sms;
	}
	else
	{
		return NULL;
	}
}

ServerMediaSession* DynamicRTSPServer
::lookupServerMediaSession(char const* streamName, Boolean isFirstLookupInSession) {
	if(strstr(streamName,"uid=") != NULL)
	{
		//LOGI_print("streamName:%s", streamName);
		ServerMediaSession* sms = RTSPServer::lookupServerMediaSession(streamName);
		Boolean smsExists = sms != NULL;

		if (smsExists && isFirstLookupInSession) { 
			// Remove the existing "ServerMediaSession" and create a new one, in case the underlying
			// file has changed in some way:
			removeServerMediaSession(sms); 
			sms = NULL;
		} 
		if (sms == NULL) {
			

			sms = createNewSMS(envir(), streamName, NULL, NULL); 
			addServerMediaSession(sms);
		}
		return sms;
	}
  else
  {
	// First, check whether the specified "streamName" exists as a local file:
	FILE* fid = fopen(streamName, "rb");
	Boolean fileExists = fid != NULL;

	// Next, check whether we already have a "ServerMediaSession" for this file:
	ServerMediaSession* sms = RTSPServer::lookupServerMediaSession(streamName);
	Boolean smsExists = sms != NULL;

	// Handle the four possibilities for "fileExists" and "smsExists":
	if (!fileExists) {
	  if (smsExists) {
		// "sms" was created for a file that no longer exists. Remove it:
		removeServerMediaSession(sms);
		sms = NULL;
	  }

	  return NULL;
	} else {
	  if (smsExists && isFirstLookupInSession) { 
		// Remove the existing "ServerMediaSession" and create a new one, in case the underlying
		// file has changed in some way:
		removeServerMediaSession(sms); 
		sms = NULL;
	  } 

	  if (sms == NULL) {
		sms = createNewSMS(envir(), streamName, fid, NULL); 
		addServerMediaSession(sms);
	  }

	  fclose(fid);
	  return sms;
	}
  }
}

// Special code for handling Matroska files:
struct MatroskaDemuxCreationState {
  MatroskaFileServerDemux* demux;
  char watchVariable;
};
static void onMatroskaDemuxCreation(MatroskaFileServerDemux* newDemux, void* clientData) {
  MatroskaDemuxCreationState* creationState = (MatroskaDemuxCreationState*)clientData;
  creationState->demux = newDemux;
  creationState->watchVariable = 1;
}
// END Special code for handling Matroska files:

// Special code for handling Ogg files:
struct OggDemuxCreationState {
  OggFileServerDemux* demux;
  char watchVariable;
};
static void onOggDemuxCreation(OggFileServerDemux* newDemux, void* clientData) {
  OggDemuxCreationState* creationState = (OggDemuxCreationState*)clientData;
  creationState->demux = newDemux;
  creationState->watchVariable = 1;
}
// END Special code for handling Ogg files:

#define NEW_SMS(description) do {\
char const* descStr = description\
    ", streamed by the LIVE555 Media Server";\
sms = ServerMediaSession::createNew(env, fileName, fileName, descStr);\
} while(0)

#define NEW_SMS1(description) do {\
	char const* descStr = description\
	", streamed by the LIVE555 Media Server";\
	sms = ServerMediaSession::createNew(env, dstFileName, fileName, descStr);\
} while(0)


static ServerMediaSession* createNewSMS(UsageEnvironment& env,
					char const* fileName, FILE* /*fid*/, void* pClientConn) {
  ServerMediaSession* sms = NULL;
    char const* pUID = NULL;
	if((pUID = strstr(fileName,"uid=")) != NULL){
		char dstFileName[256] = {0};
		char pcharClientconn[64] = {0};
		sprintf(pcharClientconn,"%p",pClientConn);
		if(strstr(fileName,pcharClientconn) != NULL)
			strcpy(dstFileName,fileName);
		else
			sprintf(dstFileName,"%s%s",fileName,pcharClientconn);
		//printf("-------------------dstFileName = %s\n",dstFileName);
		NEW_SMS1("H264_G711A"); //dstFileName->对应mediasession中的streamname
		char uid[128] = {0};
		strcpy(uid, pUID + strlen("uid="));
		
		//LOGI_print("createNewSMS:%s", dstFileName);

		//rtspServer->addSeverLiveConns(fileName);
		RTSPServer::RTSPClientConnection* pClientConnections = (RTSPServer::RTSPClientConnection*)pClientConn;
		GssLiveConn* liveConn = NULL;
		if(pClientConnections)
			liveConn = pClientConnections->GetGssLiveConn();
		if(liveConn && !liveConn->Start() && !liveConn->IsConnected() )
		{
			LOG_ERROR("[CREATENEWSMS] connect device is failed,dest file name = %s",dstFileName);
			//printf("XXXXXXXXXX failed , start lookupservermediasession liveConn(%p) of %s\n",liveConn,fileName);
		}
// 		if(liveConn == NULL)
// 		{
// 			LOGI_print("liveConn is null, new subsession h264 only");
// 			sms->addSubsession(H264LiveVideoServerMediaSubsession::createNew(env, uid,NULL, False));
// 		}
// 		else 
		if(liveConn && liveConn->IsConnected())
		{
			//LOGI_print("liveconn is %p, isconnect = %d",liveConn, liveConn->IsConnected());
			int tmpAudioTypeIndex = 0;
			while (tmpAudioTypeIndex++ < 40)
			{
				if(liveConn->IsKnownAudioType() || !liveConn->IsConnected())
				{
					//printf("isaudiotype = %d,is connected = %d\n",liveConn->IsKnownAudioType(),liveConn->IsConnected());
					break;
				}
				usleep(1000*100);
			}
			//LOGI_print("audio is known,is g711 = %d, aac = %d",liveConn->IsAudioG711AType(), liveConn->IsAudioAacType());
			if( liveConn->IsKnownAudioType() && liveConn->IsConnected())
			{
				//LOGI_print("create sub session");
				OutPacketBuffer::maxSize = 300000;
				sms->addSubsession(H264LiveVideoServerMediaSubsession::createNew(env, uid,liveConn, False));
				//怎么判断是G711A还AAC呢
 				if( liveConn->IsAudioG711AType() )
 					sms->addSubsession(PCMUServerMediaSubsession::createNew(env, uid,liveConn, False));
 				else if (liveConn->IsAudioAacType())
 					sms->addSubsession(ADTSMainServerMediaSubsession::createNew(env,0,liveConn,False));
				//sms->addSubsession(H264LiveVideoServerMediaSubsession::createNew(env, uid,liveConn, False));
				liveConn->incrementReferenceCount();
			}
			else
			{
				//时间太久，无法判断音频类型
				LOG_ERROR("[CREATENEWSMS] it's too long to get audio type");
				//LOGI_print("there is unknown audiotype");
				sms->decrementReferenceCount();
			}
		}

		return sms;
	}

  // Use the file name extension to determine the type of "ServerMediaSession":
  char const* extension = strrchr(fileName, '.');
  if (extension == NULL) return NULL;

  
  Boolean const reuseSource = False;
  if (strcmp(extension, ".aac") == 0) {
    // Assumed to be an AAC Audio (ADTS format) file:
    NEW_SMS("AAC Audio");
    sms->addSubsession(ADTSAudioFileServerMediaSubsession::createNew(env, fileName, reuseSource));
  } else if (strcmp(extension, ".amr") == 0) {
    // Assumed to be an AMR Audio file:
    NEW_SMS("AMR Audio");
    sms->addSubsession(AMRAudioFileServerMediaSubsession::createNew(env, fileName, reuseSource));
  } else if (strcmp(extension, ".ac3") == 0) {
    // Assumed to be an AC-3 Audio file:
    NEW_SMS("AC-3 Audio");
    sms->addSubsession(AC3AudioFileServerMediaSubsession::createNew(env, fileName, reuseSource));
  } else if (strcmp(extension, ".m4e") == 0) {
    // Assumed to be a MPEG-4 Video Elementary Stream file:
    NEW_SMS("MPEG-4 Video");
    sms->addSubsession(MPEG4VideoFileServerMediaSubsession::createNew(env, fileName, reuseSource));
  } else if (strcmp(extension, ".264") == 0) {
    // Assumed to be a H.264 Video Elementary Stream file:
    NEW_SMS("H.264 Video");
    OutPacketBuffer::maxSize = 100000; // allow for some possibly large H.264 frames
    sms->addSubsession(H264VideoFileServerMediaSubsession::createNew(env, fileName, reuseSource));
  } else if (strcmp(extension, ".265") == 0) {
    // Assumed to be a H.265 Video Elementary Stream file:
    NEW_SMS("H.265 Video");
    OutPacketBuffer::maxSize = 100000; // allow for some possibly large H.265 frames
    sms->addSubsession(H265VideoFileServerMediaSubsession::createNew(env, fileName, reuseSource));
  } else if (strcmp(extension, ".mp3") == 0) {
    // Assumed to be a MPEG-1 or 2 Audio file:
    NEW_SMS("MPEG-1 or 2 Audio");
    // To stream using 'ADUs' rather than raw MP3 frames, uncomment the following:
//#define STREAM_USING_ADUS 1
    // To also reorder ADUs before streaming, uncomment the following:
//#define INTERLEAVE_ADUS 1
    // (For more information about ADUs and interleaving,
    //  see <http://www.live555.com/rtp-mp3/>)
    Boolean useADUs = False;
    Interleaving* interleaving = NULL;
#ifdef STREAM_USING_ADUS
    useADUs = True;
#ifdef INTERLEAVE_ADUS
    unsigned char interleaveCycle[] = {0,2,1,3}; // or choose your own...
    unsigned const interleaveCycleSize
      = (sizeof interleaveCycle)/(sizeof (unsigned char));
    interleaving = new Interleaving(interleaveCycleSize, interleaveCycle);
#endif
#endif
    sms->addSubsession(MP3AudioFileServerMediaSubsession::createNew(env, fileName, reuseSource, useADUs, interleaving));
  } else if (strcmp(extension, ".mpg") == 0) {
    // Assumed to be a MPEG-1 or 2 Program Stream (audio+video) file:
    NEW_SMS("MPEG-1 or 2 Program Stream");
    MPEG1or2FileServerDemux* demux
      = MPEG1or2FileServerDemux::createNew(env, fileName, reuseSource);
    sms->addSubsession(demux->newVideoServerMediaSubsession());
    sms->addSubsession(demux->newAudioServerMediaSubsession());
  } else if (strcmp(extension, ".vob") == 0) {
    // Assumed to be a VOB (MPEG-2 Program Stream, with AC-3 audio) file:
    NEW_SMS("VOB (MPEG-2 video with AC-3 audio)");
    MPEG1or2FileServerDemux* demux
      = MPEG1or2FileServerDemux::createNew(env, fileName, reuseSource);
    sms->addSubsession(demux->newVideoServerMediaSubsession());
    sms->addSubsession(demux->newAC3AudioServerMediaSubsession());
  } else if (strcmp(extension, ".ts") == 0) {
    // Assumed to be a MPEG Transport Stream file:
    // Use an index file name that's the same as the TS file name, except with ".tsx":
    unsigned indexFileNameLen = strlen(fileName) + 2; // allow for trailing "x\0"
    char* indexFileName = new char[indexFileNameLen];
    sprintf(indexFileName, "%sx", fileName);
    NEW_SMS("MPEG Transport Stream");
    sms->addSubsession(MPEG2TransportFileServerMediaSubsession::createNew(env, fileName, indexFileName, reuseSource));
    delete[] indexFileName;
  } else if (strcmp(extension, ".wav") == 0) {
    // Assumed to be a WAV Audio file:
    NEW_SMS("WAV Audio Stream");
    // To convert 16-bit PCM data to 8-bit u-law, prior to streaming,
    // change the following to True:
    Boolean convertToULaw = False;
    sms->addSubsession(WAVAudioFileServerMediaSubsession::createNew(env, fileName, reuseSource, convertToULaw));
  } else if (strcmp(extension, ".dv") == 0) {
    // Assumed to be a DV Video file
    // First, make sure that the RTPSinks' buffers will be large enough to handle the huge size of DV frames (as big as 288000).
    OutPacketBuffer::maxSize = 300000;

    NEW_SMS("DV Video");
    sms->addSubsession(DVVideoFileServerMediaSubsession::createNew(env, fileName, reuseSource));
  } else if (strcmp(extension, ".mkv") == 0 || strcmp(extension, ".webm") == 0) {
    // Assumed to be a Matroska file (note that WebM ('.webm') files are also Matroska files)
    OutPacketBuffer::maxSize = 100000; // allow for some possibly large VP8 or VP9 frames
    NEW_SMS("Matroska video+audio+(optional)subtitles");

    // Create a Matroska file server demultiplexor for the specified file.
    // (We enter the event loop to wait for this to complete.)
    MatroskaDemuxCreationState creationState;
    creationState.watchVariable = 0;
    MatroskaFileServerDemux::createNew(env, fileName, onMatroskaDemuxCreation, &creationState);
    env.taskScheduler().doEventLoop(&creationState.watchVariable);

    ServerMediaSubsession* smss;
    while ((smss = creationState.demux->newServerMediaSubsession()) != NULL) {
      sms->addSubsession(smss);
    }
  } else if (strcmp(extension, ".ogg") == 0 || strcmp(extension, ".ogv") == 0 || strcmp(extension, ".opus") == 0) {
    // Assumed to be an Ogg file
    NEW_SMS("Ogg video and/or audio");

    // Create a Ogg file server demultiplexor for the specified file.
    // (We enter the event loop to wait for this to complete.)
    OggDemuxCreationState creationState;
    creationState.watchVariable = 0;
    OggFileServerDemux::createNew(env, fileName, onOggDemuxCreation, &creationState);
    env.taskScheduler().doEventLoop(&creationState.watchVariable);

    ServerMediaSubsession* smss;
    while ((smss = creationState.demux->newServerMediaSubsession()) != NULL) {
      sms->addSubsession(smss);
    }
  }

  return sms;
}
