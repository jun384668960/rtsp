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
// LIVE555 Media Server
// main program

#include <BasicUsageEnvironment.hh>
#include <MyBasicUsageEnvironment.hh>
#include "DynamicRTSPServer.hh"
#include "version.hh"
#include "gss_transport.h" 
#include "utils_log.h"
#include "TlsSocketHelper.hh"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int g_loglvl = 15;
int g_livesecs = 0;
int g_daylivemins = 0;
char g_mysqlPwd[1024] = {0};
char g_logpath[1024] = {0};
char g_dipatchServer[1024] = {0};
int g_enablelog = 1;
char g_certPath[256] = {0};
char g_keyPath[256] = {0};
int g_bTlsSupport = 0;

enum {
	type_opt_help = 0,
	type_opt_dispath,
	type_opt_cert,
	type_opt_key,
	type_opt_log,
	type_opt_disable_log,
	type_opt_loglvl,
	type_opt_livesecs,
	type_opt_daylivemins,
	type_opt_pmysql,
	type_opt_count,
};

char g_optval[type_opt_count][20] = {
	"--help",
	"--dispatch",
	"--cert",
	"--key",
	"--log",
	"--disable_log",
	"--loglvl",
	"--livesecs",
	"--daylivemins",
	"--pmysql",
};


void PrintHelp()
{
	printf("argc >= 2, Dispatch server address must be specified\n"
		"%s\n"
		"%-15s %-25s [DISPATCH SERVER] -> ex : \"120.34.23.33:6001\"\n"
		"%-15s %-25s [PATH] -> ex: \"cert.pem\",The certificates must be in PEM format \n"
		"%-15s %-25s [PATH] -> ex: \"key.pem\",The Private key must be in PEM format\n"
		"%-15s %-25s [PATH] -> ex /var/log/ or log/, the director must be exist; default path = ./\n"
		"%-15s %-25s [yes/no] ->default log is opened, use this option to disable log\n"
		"%-15s %-25s [VALUE] -> [1->Error, 2 -> warn, 4 -> info, 8 -> debug], combination this value\n"
		"%-15s %-25s [SECS] -> keep alive in seconds\n"
		"%-15s %-25s [STRING] -> mysql root password\n" ,
		g_optval[type_opt_help],
		g_optval[type_opt_dispath],"= [DISPATCH SERVER]",
		g_optval[type_opt_cert],"= [PATH]",
		g_optval[type_opt_key],"= [PATH]",
		g_optval[type_opt_log],"= [PATH]",
		g_optval[type_opt_disable_log],"= [yes/no]",
		g_optval[type_opt_loglvl],"= [VALUE]",
		g_optval[type_opt_livesecs],"= [SECS]",
		g_optval[type_opt_pmysql],"= [STRING]"
	);
}

int GetOpt(int type, int argc, char** argv)
{
	int rlt = -1;
	int start = 0;
	int isHasequal = 0;
	char* pFinddest = NULL;
	for (int i = 1; i < argc; i++)
	{
		if(start == 0)
		{
			if (strstr(argv[i],g_optval[type]) != NULL)
			{
				if(type == type_opt_help)
				{
					rlt = 0;
					break;
				}
				else
				{
					start = 1;
					char* pFind = strstr(argv[i],"=");
					if (pFind != NULL)
					{
						if (*(pFind+1) != '\0')
						{
							pFinddest = pFind + 1;
						}
						else
						{
							if ( i + 1 < argc	)
							{
								pFinddest = argv[i+1];
							}
						}
						break;
					}
					else
					{

					}
				}
			}

		}
		else
		{
			if(strcmp("=",argv[i]) == 0)
			{
				if ( i + 1 < argc	)
				{
					pFinddest = argv[i+1];
				}
			}
			else
			{
				char* pFind = strstr(argv[i],"=");
				if (pFind && (*(pFind+1) != '\0'))
				{
					pFinddest = pFind + 1;
				}
			}
			break;
		}
	}

	if(pFinddest)
	{
		switch(type)
		{
		case type_opt_dispath:
			strcpy(g_dipatchServer,pFinddest);
			break;
		case type_opt_cert:
			strcpy(g_certPath,pFinddest);
			break;
		case type_opt_key:
			strcpy(g_keyPath,pFinddest);
			break;
		case type_opt_log:
			strcpy(g_logpath,pFinddest);
			break;
		case type_opt_loglvl:
			g_loglvl = atoi(pFinddest);
			break;
		case type_opt_livesecs:
			g_livesecs = atoi(pFinddest);
			break;
		case type_opt_daylivemins:
			g_daylivemins = atoi(pFinddest);
			break;
		case type_opt_pmysql:
			strcpy(g_mysqlPwd,pFinddest);
			break;
		case type_opt_disable_log:
			{
				char pTmp[1024] = {0};
				strcpy(pTmp,pFinddest);
				for (int i = 0; i < strlen(pTmp); i++)
				{
					tolower(pTmp[i]);
				}

				if(strcmp("yes",pTmp) == 0)
				{
					g_enablelog = 1;
				}
				else
				{
					g_enablelog = 0;
				}
			}

			break;
		}
		rlt = 0;
	}

	return rlt;
}

int ParseArgs(int argc, char** argv)
{
	if (argc	< 2 || GetOpt(type_opt_help,argc,argv) == 0)
	{
		PrintHelp();
		return -1;
	}

	if( GetOpt(type_opt_dispath,argc,argv) != 0)
	{
		LOGI_print("no dispatch server to be specified");
		return -1;
	}

	if( GetOpt(type_opt_cert,argc,argv) != 0)
	{
		g_bTlsSupport = 0;
		LOGI_print("no certificates file to be specified, program will not support tls");
	}
	else
	{
		g_bTlsSupport = 1;
	}

	if( GetOpt(type_opt_key,argc,argv) != 0)
	{
		g_bTlsSupport = 0;
		LOGI_print("no private key file to be specified, program will not support tls");
	}
	else
	{
		g_bTlsSupport = 1;
		LOGI_print("program will support tls, tls default port is 443!");
	}

	if( GetOpt(type_opt_livesecs,argc,argv) != 0)
	{
		LOGI_print("no live secs limit\n");
	}
	else
	{
		LOGI_print("live secs limits in %d sec", g_livesecs);
	}

	if( GetOpt(type_opt_pmysql,argc,argv) != 0)
	{
		LOGI_print("mysql root password no set");
		return -1;
	}
	else
	{
		LOGI_print("mysql root password :%s", g_mysqlPwd);
	}
	
	for (int i = type_opt_log; i< type_opt_count; i++)
	{
		GetOpt(i,argc	, argv);
	}

	return 0;
}

int main(int argc, char** argv) {

  	int ret;
	int daylivemins = 0;
	int liveseconds = 0;
	if (ParseArgs(argc,argv) != 0)
	{
		return -1;
	}
	
	char* pLogPath = NULL;
	if(g_enablelog)
		pLogPath = g_logpath;
	if(g_livesecs > 0)
	{
		liveseconds = g_livesecs;
	}
	else
	{
		liveseconds = 600;
	}
	
	if(g_daylivemins > 0)
	{
		daylivemins = g_daylivemins;
	}
	else
	{
		daylivemins = 60;
	}
	
	GssLiveConn::SetForceLiveSec(liveseconds);
	LOGI_print("set single live seconds:%d day live minutes:%d", liveseconds, daylivemins);
	
 	if(!GssLiveConn::GlobalInit(g_dipatchServer,pLogPath,g_loglvl,"127.0.0.1",3306,"root",g_mysqlPwd,"rtsp_db",4,daylivemins))
 	{
 		LOGE_print("GlobalInit error");
 		return ret;
 	}

// 	ret = p2p_init(NULL);
// 	if(ret != 0)
// 	{
// 		LOGE_print("p2p_init error");
// 		return ret;
// 	}
// 
// 	p2p_log_set_level(0);
// 	
// 	//设置全局属性
// 	int maxRecvLen = 1024*1024;
// 	int maxClientCount = 10;
// 	p2p_set_global_opt(P2P_MAX_CLIENT_COUNT, &maxClientCount, sizeof(int));
// 	p2p_set_global_opt(P2P_MAX_RECV_PACKAGE_LEN, &maxRecvLen, sizeof(int));
	
  // Begin by setting up our usage environment:
  TaskScheduler* scheduler = BasicTaskScheduler::createNew();
  //TaskScheduler* scheduler = MyBasicTaskScheduler::createNew();
  UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);

  UserAuthenticationDatabase* authDB = NULL;
#ifdef ACCESS_CONTROL
  // To implement client access control to the RTSP server, do the following:
  authDB = new UserAuthenticationDatabase;
  authDB->addUserRecord("username1", "password1"); // replace these with real strings
  // Repeat the above with each <username>, <password> that you wish to allow
  // access to the server.
#endif

  // Create the RTSP server.  Try first with the default port number (554),
  // and then with the alternative port number (8554):
  RTSPServer* rtspServer;
  portNumBits rtspServerPortNum = 554;
  if(g_bTlsSupport)
  {
	  TlsHelper_Init();
	  TlsHelper_InitServer(NULL,NULL,g_certPath,g_keyPath);
  }
  rtspServer = DynamicRTSPServer::createNew(*env, rtspServerPortNum, authDB);
  if (rtspServer == NULL) {
    rtspServerPortNum = 8554;
    rtspServer = DynamicRTSPServer::createNew(*env, rtspServerPortNum, authDB);
  }
  if (rtspServer == NULL) {
    *env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
    exit(1);
  }

  *env << "LIVE555 Media Server\n";
  *env << "\tversion " << MEDIA_SERVER_VERSION_STRING
       << " (LIVE555 Streaming Media library version "
       << LIVEMEDIA_LIBRARY_VERSION_STRING << ").\n";

  char* urlPrefix = rtspServer->rtspURLPrefix();
  *env << "Play streams from this server using the URL\n\t"
       << urlPrefix << "<filename>\nwhere <filename> is a file present in the current directory.\n";
  *env << "Each file's type is inferred from its name suffix:\n";
  *env << "\t\".264\" => a H.264 Video Elementary Stream file\n";
  *env << "\t\".265\" => a H.265 Video Elementary Stream file\n";
  *env << "\t\".aac\" => an AAC Audio (ADTS format) file\n";
  *env << "\t\".ac3\" => an AC-3 Audio file\n";
  *env << "\t\".amr\" => an AMR Audio file\n";
  *env << "\t\".dv\" => a DV Video file\n";
  *env << "\t\".m4e\" => a MPEG-4 Video Elementary Stream file\n";
  *env << "\t\".mkv\" => a Matroska audio+video+(optional)subtitles file\n";
  *env << "\t\".mp3\" => a MPEG-1 or 2 Audio file\n";
  *env << "\t\".mpg\" => a MPEG-1 or 2 Program Stream (audio+video) file\n";
  *env << "\t\".ogg\" or \".ogv\" or \".opus\" => an Ogg audio and/or video file\n";
  *env << "\t\".ts\" => a MPEG Transport Stream file\n";
  *env << "\t\t(a \".tsx\" index file - if present - provides server 'trick play' support)\n";
  *env << "\t\".vob\" => a VOB (MPEG-2 video with AC-3 audio) file\n";
  *env << "\t\".wav\" => a WAV Audio file\n";
  *env << "\t\".webm\" => a WebM audio(Vorbis)+video(VP8) file\n";
  *env << "See http://www.live555.com/mediaServer/ for additional documentation.\n";

  // Also, attempt to create a HTTP server for RTSP-over-HTTP tunneling.
  // Try first with the default HTTP port (80), and then with the alternative HTTP
  // port numbers (8000 and 8080).

  bool brtspOverHttp = false;
  if(g_bTlsSupport)
  {
	  if( rtspServer->setUpTunnelingOverHTTP(443) )
		  brtspOverHttp = true;
  }
 else
 {
	if ( rtspServer->setUpTunnelingOverHTTP(80) || rtspServer->setUpTunnelingOverHTTP(8000) || rtspServer->setUpTunnelingOverHTTP(8080))
		brtspOverHttp = true;
  }

  if(brtspOverHttp)
  {
    *env << "(We use port " << rtspServer->httpServerPortNum() << " for optional RTSP-over-HTTP tunneling, or for HTTP live streaming (for indexed Transport Stream files only).)\n";
  } else {
    *env << "(RTSP-over-HTTP tunneling is not available.)\n";
	exit(1);
  }

  env->taskScheduler().doEventLoop(); // does not return

  if(g_bTlsSupport)
	  TlsHelper_UnInit();
  GssLiveConn::GlobalUnInit();
//   p2p_uninit();
  return 0; // only to prevent compiler warning
}
