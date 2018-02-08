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
// A subclass of "RTSPServer" that creates "ServerMediaSession"s on demand,
// based on whether or not the specified stream name exists as a file
// Implementation

#include "DynamicRTSPServer.hh"
#include <liveMedia.hh>
#include <string.h>
#include "LiveStreamMediaSource.hh"
#include "H264VideoLiveServerMediaSubsession.hh"
#include "ADTSAudioLiveServerMediaSubsession.hh"
//#include "Gp3FileVideoServerMediaSubsession.hh"
//#include "Gp3FileAudioServerMediaSubsession.hh"
#include "RtspSvr.hh"

char DynamicRTSPServer::m_Path[128];

DynamicRTSPServer*
DynamicRTSPServer::createNew(UsageEnvironment& env,
	Port ourPort,
	UserAuthenticationDatabase* authDatabase,
	CRtspServer* rtsp,
	unsigned reclamationTestSeconds) {
	int ourSocket = setUpOurSocket(env, ourPort);
	if (ourSocket == -1) return NULL;

	return new DynamicRTSPServer(env, ourSocket, ourPort, authDatabase, rtsp, reclamationTestSeconds);
}

DynamicRTSPServer::DynamicRTSPServer(UsageEnvironment& env,
	int ourSocket,
	Port ourPort,
	UserAuthenticationDatabase* authDatabase,
	CRtspServer* rtsp,
	unsigned reclamationTestSeconds)
	: RTSPServerSupportingHTTPStreaming(env, ourSocket, ourPort, authDatabase, reclamationTestSeconds)
	,m_CRtspServer(rtsp){
	
}

DynamicRTSPServer::~DynamicRTSPServer() {
}


static ServerMediaSession* createNewSMS(UsageEnvironment& env,
	char const* fileName,
	FILE* fid); // forward

ServerMediaSession* DynamicRTSPServer
::lookupServerMediaSession(char const* streamName, Boolean isFirstLookupInSession) {
	LOGD_print("DynamicRTSPServer", "Call DynamicRTSPServer::lookupServerMediaSession!!");

	if(strlen(streamName) > 128)
	{
		return NULL;
	}
	
	memset(m_Path, 0x0, 128);
	char const* extension = strrchr(streamName, '.');
	if (extension != NULL && (strcmp(extension, ".3gp") == 0 || strcmp(extension, ".3GP") == 0)) {
		//m_CRtspServer->ParserUrl(streamName, m_Path, 127);
		return NULL;
	}
	else
	{
		strcpy(m_Path, streamName);
	}
  // First, check whether the specified "streamName" exists as a local file:
	FILE* fid = fopen(m_Path, "rb");
	Boolean fileExists = fid != NULL;

	char* live = m_CRtspServer->LiveUrl();
	LOGD_print("DynamicRTSPServer", "streamName:%s m_LiveString:%s", m_Path, live);
	if (strcmp(m_Path, live) == 0)
	{
		fileExists = 1;
	}
	// Next, check whether we already have a "ServerMediaSession" for this file:
	ServerMediaSession* sms = RTSPServer::lookupServerMediaSession(m_Path);
	Boolean smsExists = sms != NULL;

	  // Handle the four possibilities for "fileExists" and "smsExists":
	if (!fileExists) {
		if (smsExists) {
		  // "sms" was created for a file that no longer exists. Remove it:
			removeServerMediaSession(sms);
			sms = NULL;
		}

		return NULL;
	}
	else {
		//if (smsExists && isFirstLookupInSession && strcmp(m_Path, live) != 0) { 
		if (smsExists && isFirstLookupInSession) { 
		  // Remove the existing "ServerMediaSession" and create a new one, in case the underlying
			removeServerMediaSession(sms); 
			sms = NULL;
			LOGD_print("DynamicRTSPServer", "removeServerMediaSession!!");
		} 

		if (sms == NULL) {
			if(!strcmp(m_Path, live))
			{
				char const* descStr = "LIVE555 LiveMedia Server";
				sms = ServerMediaSession::createNew(envir(), m_Path, m_Path, descStr);
				
				OutPacketBuffer::maxSize = DEFAULT_MAX_VEDIOFRAME_SIZE;//100000; // allow for some possibly large frames
				LiveStreamMediaSource* pInput = LiveStreamMediaSource::createNew(envir());
				sms->addSubsession(H264VideoLiveServerMediaSubsession::createNew(envir(), *pInput, true));
				if(m_CRtspServer->IsAudioEnable())
				{
					sms->addSubsession(ADTSAudioLiveServerMediaSubsession::createNew(envir(), *pInput, true));
				}
				LOGD_print("DynamicRTSPServer", "descStr:%s!!", descStr);
			}
			else
			{
				sms = createNewSMS(envir(), m_Path, fid); 
			}
			
			addServerMediaSession(sms);
			LOGD_print("DynamicRTSPServer", "addServerMediaSession!!");
		}

		if (fid != NULL)
			fclose(fid);
		return sms;
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
} while (0)


static ServerMediaSession* createNewSMS(UsageEnvironment& env,
		char const* fileName,
		FILE* /*fid*/) {
// Use the file name extension to determine the type of "ServerMediaSession":
	char const* extension = strrchr(fileName, '.');
	if (extension == NULL) return NULL;

	ServerMediaSession* sms = NULL;
	Boolean const reuseSource = False;
	if (strcmp(extension, ".3gp") == 0 || strcmp(extension, ".3GP") == 0) {
		//parser to full path file name
		
		NEW_SMS("3pg media");//streamName  "/mnt/sdcard/test.3pg"
		OutPacketBuffer::maxSize = DEFAULT_MAX_VEDIOFRAME_SIZE;
		//sms->addSubsession(Gp3FileAudioServerMediaSubsession::createNew(env, fileName, reuseSource, True));
		//sms->addSubsession(Gp3FileVideoServerMediaSubsession::createNew(env, fileName, reuseSource, False));
	}
	else if (strcmp(extension, ".aac") == 0) {
	    // Assumed to be an AAC Audio (ADTS format) file:
		NEW_SMS("AAC Audio");
		sms->addSubsession(ADTSAudioFileServerMediaSubsession::createNew(env, fileName, reuseSource));
	}
	else if (strcmp(extension, ".amr") == 0) {
	  // Assumed to be an AMR Audio file:
		NEW_SMS("AMR Audio");
		sms->addSubsession(AMRAudioFileServerMediaSubsession::createNew(env, fileName, reuseSource));
	}
	else if (strcmp(extension, ".ac3") == 0) {
	  // Assumed to be an AC-3 Audio file:
		NEW_SMS("AC-3 Audio");
		sms->addSubsession(AC3AudioFileServerMediaSubsession::createNew(env, fileName, reuseSource));
	}
	else if (strcmp(extension, ".m4e") == 0) {
	  // Assumed to be a MPEG-4 Video Elementary Stream file:
		NEW_SMS("MPEG-4 Video");
		sms->addSubsession(MPEG4VideoFileServerMediaSubsession::createNew(env, fileName, reuseSource));
	}
	else if (strcmp(extension, ".264") == 0) {
	  // Assumed to be a H.264 Video Elementary Stream file:
		NEW_SMS("H.264 Video");
		OutPacketBuffer::maxSize = 100000; // allow for some possibly large H.264 frames
		sms->addSubsession(H264VideoFileServerMediaSubsession::createNew(env, fileName, reuseSource));
	}
	else if (strcmp(extension, ".265") == 0) {
	  // Assumed to be a H.265 Video Elementary Stream file:
		NEW_SMS("H.265 Video");
		OutPacketBuffer::maxSize = 100000; // allow for some possibly large H.265 frames
		sms->addSubsession(H265VideoFileServerMediaSubsession::createNew(env, fileName, reuseSource));
	}
	else if (strcmp(extension, ".mp3") == 0) {
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
		unsigned char interleaveCycle[] = { 0, 2, 1, 3 }; // or choose your own...
		unsigned const interleaveCycleSize
		  = (sizeof interleaveCycle) / (sizeof(unsigned char));
		interleaving = new Interleaving(interleaveCycleSize, interleaveCycle);
#endif
#endif
		sms->addSubsession(MP3AudioFileServerMediaSubsession::createNew(env, fileName, reuseSource, useADUs, interleaving));
	}
	else if (strcmp(extension, ".mpg") == 0) {
	  // Assumed to be a MPEG-1 or 2 Program Stream (audio+video) file:
		NEW_SMS("MPEG-1 or 2 Program Stream");
		MPEG1or2FileServerDemux* demux
		  = MPEG1or2FileServerDemux::createNew(env, fileName, reuseSource);
		sms->addSubsession(demux->newVideoServerMediaSubsession());
		sms->addSubsession(demux->newAudioServerMediaSubsession());
	}
	else if (strcmp(extension, ".vob") == 0) {
	  // Assumed to be a VOB (MPEG-2 Program Stream, with AC-3 audio) file:
		NEW_SMS("VOB (MPEG-2 video with AC-3 audio)");
		MPEG1or2FileServerDemux* demux
		  = MPEG1or2FileServerDemux::createNew(env, fileName, reuseSource);
		sms->addSubsession(demux->newVideoServerMediaSubsession());
		sms->addSubsession(demux->newAC3AudioServerMediaSubsession());
	}
	else if (strcmp(extension, ".ts") == 0) {
	  // Assumed to be a MPEG Transport Stream file:
	  // Use an index file name that's the same as the TS file name, except with ".tsx":
		unsigned indexFileNameLen = strlen(fileName) + 2; // allow for trailing "x\0"
		char* indexFileName = new char[indexFileNameLen];
		sprintf(indexFileName, "%sx", fileName);
		NEW_SMS("MPEG Transport Stream");
		sms->addSubsession(MPEG2TransportFileServerMediaSubsession::createNew(env, fileName, indexFileName, reuseSource));
		delete[] indexFileName;
	}
	else if (strcmp(extension, ".wav") == 0) {
	  // Assumed to be a WAV Audio file:
		NEW_SMS("WAV Audio Stream");
		// To convert 16-bit PCM data to 8-bit u-law, prior to streaming,
		// change the following to True:
		Boolean convertToULaw = False;
		sms->addSubsession(WAVAudioFileServerMediaSubsession::createNew(env, fileName, reuseSource, convertToULaw));
	}
	else if (strcmp(extension, ".dv") == 0) {
	  // Assumed to be a DV Video file
	  // First, make sure that the RTPSinks' buffers will be large enough to handle the huge size of DV frames (as big as 288000).
		OutPacketBuffer::maxSize = 300000;

		NEW_SMS("DV Video");
		sms->addSubsession(DVVideoFileServerMediaSubsession::createNew(env, fileName, reuseSource));
	}
	else if (strcmp(extension, ".mkv") == 0 || strcmp(extension, ".webm") == 0) {
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
	}
	else if (strcmp(extension, ".ogg") == 0 || strcmp(extension, ".ogv") == 0 || strcmp(extension, ".opus") == 0) {
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
