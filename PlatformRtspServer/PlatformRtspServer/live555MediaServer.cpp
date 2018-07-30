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
#include "inifile.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define THIS_VERSION "V1.0.5"
#define GSS_CONF_NAME		"gss_globle.conf"

typedef struct gss_globel_conf_T
{
	char 	server[128];		// gss分派服务器地址
	char 	logpath[128];		// 日志存放目录
	int 	loglvl;				// 日志级别
	char 	sqlHost[64];		// mysql地址，计时数据库
	int 	sqlPort;			// mysql端口
	char	sqlUser[64]; 		// mysql用户名
	char	sqlPasswd[64];		// mysql密码
	char	dbName[64];			// mysql数据库名
	int 	maxCounts;			// 连接池中数据库连接的最大数,假设有n个业务线程使用该连接池，建议:maxCounts=n,假设n>20, 建议maxCounts=20
	int 	maxPlayTime;		// 一天内最大播放时长，单位秒
	int 	type;				// 类型0:rtsp 1:hls
	int		live_sec;			// 单次连接最大保活时长，超时会主动断开
	int		tls_support;		// 是否支持TLS
	char	cert[64];			// 证书
	char	key[64];			// 秘钥
}gss_globel_conf_t;

int rtsp_load_config(gss_globel_conf_t* conf)
{
//	int read_profile_string( const char *section, const char *key,const char *default_value, char *value, int size,  const char *file);
//	int read_profile_int( const char *section, const char *key,int default_value, const char *file);
	char ini_file[128] = {0};
	char ptemp[128] = {0};
	int	ret;
	
	sprintf(ini_file, "%s", GSS_CONF_NAME);
	ret = read_profile_string("common", "server", "", ptemp, sizeof(ptemp), ini_file);
	if(ret == 0 || ptemp[0]=='\0')
	{
		LOGM_print("server not set");
		exit(-1);
	}
	else
	{
		strcpy(conf->server, ptemp);
		LOGI_print("server set :%s ", conf->server);
	}

	ret = read_profile_string("common", "logpath", "logs/", ptemp, sizeof(ptemp), ini_file);
	if(ret == 0 || ptemp[0]=='\0')
	{
		LOGM_print("logpath not set");
		exit(-1);
	}
	else
	{
		strcpy(conf->logpath, ptemp);
		LOGI_print("logpath set :%s ", conf->logpath);
	}

	conf->loglvl = read_profile_int("common", "loglvl", 1, ini_file);
	LOGI_print("loglvl set :%d ", conf->loglvl);

	ret = read_profile_string("common", "sqlHost", "", ptemp, sizeof(ptemp), ini_file);
	if(ret == 0 || ptemp[0]=='\0')
	{
		LOGM_print("sqlHost not set");
		exit(-1);
	}
	else
	{
		strcpy(conf->sqlHost, ptemp);
		LOGI_print("sqlHost set :%s ", conf->sqlHost);
	}

	conf->sqlPort = read_profile_int("common", "sqlPort", 3306, ini_file);
	LOGI_print("sqlPort set :%d ", conf->sqlPort);

	ret = read_profile_string("common", "sqlUser", "", ptemp, sizeof(ptemp), ini_file);
	if(ret == 0 || ptemp[0]=='\0')
	{
		LOGM_print("sqlUser not set");
		exit(-1);
	}
	else
	{
		strcpy(conf->sqlUser, ptemp);
		LOGI_print("sqlUser set :%s ", conf->sqlUser);
	}

	ret = read_profile_string("common", "sqlPasswd", "", ptemp, sizeof(ptemp), ini_file);
	if(ret == 0 || ptemp[0]=='\0')
	{
		LOGM_print("sqlPasswd not set");
		exit(-1);
	}
	else
	{
		strcpy(conf->sqlPasswd, ptemp);
		LOGI_print("sqlPasswd set :%s ", conf->sqlPasswd);
	}

	ret = read_profile_string("common", "dbName", "", ptemp, sizeof(ptemp), ini_file);
	if(ret == 0 || ptemp[0]=='\0')
	{
		LOGM_print("dbName not set");
		exit(-1);
	}
	else
	{
		strcpy(conf->dbName, ptemp);
		LOGI_print("dbName set :%s ", conf->dbName);
	}

	conf->maxCounts = read_profile_int("common", "maxCounts", 4, ini_file);
	LOGI_print("maxCounts set :%d ", conf->maxCounts);
	conf->maxPlayTime = read_profile_int("common", "maxPlayTime", 60, ini_file);
	LOGI_print("maxPlayTime set :%d ", conf->maxPlayTime);
	conf->type = read_profile_int("common", "type", 1, ini_file);
	LOGI_print("type set :%d ", conf->type);
	conf->live_sec = read_profile_int("common", "live_sec", 1, ini_file);
	LOGI_print("live_sec set :%d ", conf->live_sec);

	conf->tls_support = read_profile_int("common", "tls_support", 1, ini_file);
	LOGI_print("tls_support set :%d ", conf->tls_support);

	ret = read_profile_string("common", "cert", "", ptemp, sizeof(ptemp), ini_file);
	if(ret == 0 || ptemp[0]=='\0')
	{
		LOGM_print("dbName not set");
		exit(-1);
	}
	else
	{
		strcpy(conf->cert, ptemp);
		LOGI_print("cert set :%s ", conf->cert);
	}

	ret = read_profile_string("common", "key", "", ptemp, sizeof(ptemp), ini_file);
	if(ret == 0 || ptemp[0]=='\0')
	{
		LOGM_print("dbName not set");
		exit(-1);
	}
	else
	{
		strcpy(conf->key, ptemp);
		LOGI_print("key set :%s ", conf->key);
	}

	return 0;
}


int main(int argc, char** argv) {

	gss_globel_conf_t conf;
  	rtsp_load_config(&conf);
	
	GssLiveConn::SetForceLiveSec(conf.live_sec);

	bool result = GssLiveConn::GlobalInit(conf.server, conf.logpath, conf.loglvl,
					conf.sqlHost, conf.sqlPort, conf.sqlUser, conf.sqlPasswd, conf.dbName,
					conf.maxCounts, conf.maxPlayTime, (EGSSCONNTYPE)conf.type);
 	if(!result)
 	{
 		LOGE_print("GlobalInit error");
 		return result;
 	}
	
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
  if(conf.tls_support == 1)
  {
	  TlsHelper_Init();
	  TlsHelper_InitServer(NULL,NULL,conf.cert,conf.key);
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
  if(conf.tls_support == 1)
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

  if(conf.tls_support == 1)
	  TlsHelper_UnInit();
  GssLiveConn::GlobalUnInit();

  return 0; // only to prevent compiler warning
}
