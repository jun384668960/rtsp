#include "GssLiveConn.h"
#include "gss_transport.h" 
//#include "gss_common.h"
#include "p2p_dispatch.h"
#include "Log.h"
#include "tm.h"
#include "MyuseMysql.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>

bool GssLiveConn::m_isInitedGlobal = false;
int	 GssLiveConn::m_forceLiveSec = 600;

SGlobalInfo GssLiveConn::m_sGlobalInfos;
MySqlPool* GssLiveConn::m_smysqlpool = NULL;
pthread_t GssLiveConn::m_spthread = 0;

#define TABLE_DEVICE_REC "device_rec"
#define ONE_DAY_SEC (24*60*60)

static bool s_isTimeReset = false;
static bool IsResetTime()
{
	time_t t;
    struct tm *gmt;
    t = time(NULL);
    gmt = gmtime(&t);
//	printf("sec:%d min:%d hour:%d\n", gmt->tm_sec, gmt->tm_min, gmt->tm_hour);
	if(gmt->tm_min == 0 && gmt->tm_hour == 0)
	{
		if(!s_isTimeReset)
		{
			LOG_WARN("it's time to reset!");
			s_isTimeReset = true;
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		s_isTimeReset = false;
		return false;
	}
}

static void* ThreadUpdatePlayTimeToMysql(void *arg)
{
	int nCounts = 1;
	while (GssLiveConn::m_isInitedGlobal)
	{
//		if((nCounts++%30)==0)
		{
			GssLiveConn::m_sGlobalInfos.lock.Lock();
			unsigned int nowTime = now_ms_time()/1000;
			std::map<std::string,RtspPlayTime>::iterator it = GssLiveConn::m_sGlobalInfos.mapTimes.begin();
			for ( ; it != GssLiveConn::m_sGlobalInfos.mapTimes.end(); )
			{
				int tmpSec = nowTime - it->second.time;
				it->second.time = nowTime;
				GssLiveConn::UpdatePlayTime(tmpSec,it->first.c_str());
				if(it->second.bDel)
					GssLiveConn::m_sGlobalInfos.mapTimes.erase(it++);
				else
					it++;
			}
			GssLiveConn::m_sGlobalInfos.lock.Unlock();
		}

		//如果达到清零时间
		if(IsResetTime())
		{//reset all uid time
			GssLiveConn::ResetPlayTime(NULL,GssLiveConn::GetColByType(),0);
		}
		sleep(1);
	}
	
	return (void*)0;
}

GssLiveConn::GssLiveConn()
{
	memset(m_server,0,sizeof(m_server));
	memset(m_dispathServer,0,sizeof(m_dispathServer));
	memset(m_uid,0,sizeof(m_uid));
	memset(m_bufferVideo, 0, MAX_AV_VIDEO_SIZE*sizeof(GssBuffer*));
	memset(m_bufferAudio, 0, MAX_AV_AUDIO_SIZE*sizeof(GssBuffer*));
	m_currNextIndexVideoInsert = 0;
	m_countsVideo = 0;
	m_curVideoIndex = 0;
	m_currNextIndexAudioInsert = 0;
	m_countsAudio = 0;
	m_curAudioIndex = 0;
	m_port = 0;
	m_dispathPort = 0;
	m_isConnected = false;
	m_clientPullConn = NULL;
	m_stusRlt = 0;
	m_audioType = gos_codec_unknown;
	m_referenceCount = 0;
	m_bDroped = false;
	m_isFirstFrame = true;
	
	m_forcePause = false;
	m_liveRef = 0;
	m_forceLiveSecLeftTime = GssLiveConn::m_sGlobalInfos.maxPlayTime*60;

	InitVideoBuffer();
	InitAudioBuffer();
}

GssLiveConn::GssLiveConn(const char* server, unsigned short port,const char* uid , bool bDispath)
{
//	printf("GssLiveConn::GssLiveConn start\n");
	m_isConnected = false;
	m_clientPullConn = NULL;
	m_stusRlt = 0;
	memset(m_bufferVideo, 0, MAX_AV_VIDEO_SIZE*sizeof(GssBuffer*));
	memset(m_bufferAudio, 0, MAX_AV_AUDIO_SIZE*sizeof(GssBuffer*));
	m_currNextIndexVideoInsert = 0;
	m_countsVideo = 0;
	m_curVideoIndex = 0;
	m_currNextIndexAudioInsert = 0;
	m_countsAudio = 0;
	m_curAudioIndex = 0;
	m_audioType = gos_codec_unknown;
	m_referenceCount = 0;
	m_bDroped = false;
	m_isFirstFrame = true;
	
	m_forcePause = false;
	m_liveRef = 0;
	
	if(bDispath)
	{
		if (server)
			strcpy(m_dispathServer,server);
		m_dispathPort = port;
		memset(m_server,0,sizeof(m_server));
		m_port = 0;
	}
	else
	{
		if(server)
			strcpy(m_server,server);
		m_port = port;
		memset(m_dispathServer,0,sizeof(m_dispathServer));
		m_dispathPort = 0;
	}
	if (uid)
		strcpy(m_uid,uid);

	InitVideoBuffer();
	InitAudioBuffer();
//	printf("GssLiveConn::GssLiveConn end\n");
	LOG_DEBUG("[GSSLIVECONN] new GssLiveConn ,server = %s, uid = %s, port = %d, bDispath = %d\n",server, uid, port, bDispath);
}

GssLiveConn::~GssLiveConn()
{
//	printf("start GssLiveConn::~GssLiveConn()\n");
	Stop();
	ClearVideoBuffer();
	ClearAudioBuffer();
	LOG_INFO("~GssLiveConn");
//	printf("end GssLiveConn::~GssLiveConn()\n");
}

bool GssLiveConn::Init( char* server, unsigned short port, char* uid , bool bDispath)
{
	bool busc = false;
	do 
	{
		if(server == NULL || uid == NULL || port == 0)
		{
			LOG_ERROR("[GSSLIVECONN] GssLiveConn::Init Failed, server = %s, uid = %s, port = %d, bDispath = %d\n",server, uid, port, bDispath);
			break;
		}
		if(bDispath)
		{
			strcpy(m_dispathServer,server);
			m_dispathPort = port;
		}
		else
		{
			strcpy(m_server,server);
			m_port = port;
		}
		strcpy(m_uid,uid);
		LOG_DEBUG("[GSSLIVECONN] GssLiveConn::Init success,server = %s, uid = %s, port = %d, bDispath = %d\n",server, uid, port, bDispath);
		busc = true;
	} while (0);

	return busc;
}

bool GssLiveConn::Start(bool bRestart)
{
	bool busc = false;
	do 
	{
		int result;
		gss_pull_conn_cfg cfg;
		gss_pull_conn_cb cb;

		if(!m_isInitedGlobal)
		{
//			printf("don't init global informations!\n");
			throw "don't init global informations!\n";
			break;
		}

		if(m_clientPullConn)
		{
//			printf("client pull connection is not null");
			LOG_DEBUG("[GSSLIVECONN] client pull connection is not null ,start success, m_clientPullConn = %p",m_clientPullConn);
			busc = true;
			break;
		}

		if(!bRestart && m_dispathPort != 0)
		{
//			printf("start request server information");
			if(!RequestLiveConnServer())
			{
//				printf("request server infromation failed!");
				LOG_ERROR("[GSSLIVECONN] GssLiveConn start failed by request server");
				break;
			}
		}

		int lefttime = 0;
		if (IsReachedMaxPlayTimeofDay(GssLiveConn::m_sGlobalInfos.maxPlayTime, m_uid, lefttime))
		{
			LOG_ERROR("[GSSLIVECONN] GssLiveConn start failed by reached play time");
			break;
		}
		m_forceLiveSecLeftTime = lefttime;

		if(!AddNewPlayTime(m_uid))
			LOG_ERROR("Connect new pull connections, add new play time falied!");

//		printf("start ip = %s,port = %d,uid = %s\n",m_server,m_port,m_uid);
		cfg.server = &m_server[0];
		cfg.port = m_port;
		cfg.uid = &m_uid[0];
		cfg.user_data = this;
		cfg.cb = &cb;

		cb.on_recv = &GssLiveConn::OnRecv;
		cb.on_disconnect = OnDisconnect;
		cb.on_connect_result= OnConnectResult;
		cb.on_device_disconnect= OnDeviceDisconnect;

		result = gss_client_pull_connect(&cfg, &m_clientPullConn); 
		m_stusRlt = result;
		if(result != 0)
		{
//			printf("client_pull_connect_server return %d", result);
			LOG_ERROR("[GSSLIVECONN] client_pull_connect_server return %d",result);
			m_isConnected = false;
			break;
		}
		else
		{
			m_isConnected = true;
		}


		busc = true;
	} while (0);
	if (busc)
	{
		m_isFirstFrame = true;
		LOG_INFO("[GSSLIVECONN] Gssliveconn start success, server = %s, port = %d, uid = %s, brestart = %d",m_server,m_port,m_uid,bRestart);
	}
	else
	{
		LOG_ERROR("[GSSLIVECONN] Gssliveconn start failed, server = %s, port = %d, uid = %s, brestart = %d",m_server,m_port,m_uid,bRestart);
	}

	return busc;
}

bool GssLiveConn::Stop()
{
	if(m_clientPullConn)
	{
		gss_client_pull_destroy(m_clientPullConn);
		m_clientPullConn = NULL;
		m_isConnected = false;
		if ( !DelPlayTime(m_uid) )
		{
			LOG_ERROR("stop pull connections, add delete play time falied!");
		}
		LOG_INFO("[GSSLIVECONN] GssLiveConn::Stop");
		return true;
	}
	return true;
}

bool GssLiveConn::IsConnected()
{
	return m_isConnected;
}

bool GssLiveConn::GetVideoFrame( unsigned char** pData, int &datalen, GosFrameHead& frameHeader)
{
	bool bSuc = false;
//	printf("********%p GetVideoFrame count = %d\n",this,m_countsVideo);
	if (m_countsVideo > 0 && pData && *pData == NULL)
	{
		*pData = m_bufferVideo[m_curVideoIndex]->m_buffer;
		datalen = m_bufferVideo[m_curVideoIndex]->m_bufferlen;
		frameHeader = m_bufferVideo[m_curVideoIndex]->m_frameHeader;
		bSuc = true;
	}
	return bSuc;
}

void GssLiveConn::FreeVideoFrame()
{
	m_lockVideo.Lock();
	IncVideoIndex();
	m_countsVideo--;
	m_lockVideo.Unlock();
}

bool GssLiveConn::GetAudioFrame( unsigned char** pData, int &datalen, GosFrameHead& frameHeader)
{
	bool bSuc = false;
//	printf("********%p GetAudioFrame count = %d\n",this,m_countsAudio);
	if (m_countsAudio > 0 && pData && *pData == NULL)
	{
		*pData = m_bufferAudio[m_curAudioIndex]->m_buffer;
		datalen = m_bufferAudio[m_curAudioIndex]->m_bufferlen;
		frameHeader = m_bufferAudio[m_curAudioIndex]->m_frameHeader;
		bSuc = true;
	}
	return bSuc;
}

void GssLiveConn::FreeAudioFrame()
{
	m_lockAudio.Lock();
	IncAudioIndex();
	m_countsAudio--;
	m_lockAudio.Unlock();
}

bool GssLiveConn::Send( unsigned char* pData, int datalen )
{
	return true;
}

void GssLiveConn::VideoFrameSync()
{
	m_lockVideo.Lock();
	m_currNextIndexVideoInsert = 0;
	m_countsVideo = 0;
	m_curVideoIndex = 0;
	m_lockVideo.Unlock();
}

int GssLiveConn::VideoFrameCount()
{
	int count = 0;
	m_lockVideo.Lock();
	count = m_countsVideo;
	m_lockVideo.Unlock();
	return count;
}

void GssLiveConn::AudioFrameSync()
{
	m_lockAudio.Lock();
	m_currNextIndexAudioInsert = 0;
	m_countsAudio = 0;
	m_curAudioIndex = 0;
	m_lockAudio.Unlock();
}

int GssLiveConn::AudioFrameCount()
{
	int count = 0;
	m_lockVideo.Lock();
	count = m_countsAudio;
	m_lockVideo.Unlock();
	return count;
}

void GssLiveConn::OnRecv( void *transport, void *user_data, char* data, int len, char type, unsigned int time_stamp )
{
	if(data == NULL || len <= 0)
		return;

	GssLiveConn* pConn = (GssLiveConn*)user_data;
	if(pConn)
		pConn->AddFrame((unsigned char*)data,len);
}

void GssLiveConn::OnDisconnect( void *transport, void* user_data, int status )
{
//	printf("GssLiveConn::OnDisconnect\n");
	GssLiveConn* pConn = (GssLiveConn*)user_data;
	if(pConn)
	{
		pConn->m_isConnected = false;
		pConn->m_stusRlt = status;
		pConn->Stop();
		LOG_INFO("[GSSLIVECONN] GssLiveConn::OnDisconnect transport = %p, user_data = %p, status = %d,uid = %s",transport,user_data,status,pConn->m_uid);
		pConn->Start(true);
	}
	else
	{
		LOG_ERROR("[GSSLIVECONN] GssLiveConn::OnDisconnect transport = %p, user_data = %p, status = %d",transport,user_data,status);
	}
}

void GssLiveConn::OnConnectResult( void *transport, void* user_data, int status )
{
//	printf("GssLiveConn::OnConnectResult ,status = %d\n",status);
	GssLiveConn* pConn = (GssLiveConn*)user_data;
	if(pConn)
	{
		pConn->m_stusRlt = status;
		if(pConn->m_stusRlt != 0)
		{
			pConn->m_isConnected = false;
			pConn->Stop();
		}
		else
		{
			pConn->m_isConnected = true;
		}
		LOG_INFO("[GSSLIVECONN] GssLiveConn::OnConnectResult ,status = %d, conn = %p,uid = %s",status,user_data,pConn->m_uid);
	}
	else
	{
		LOG_ERROR("[GSSLIVECONN] GssLiveConn::OnConnectResult ,status = %d, conn = %p\n",status,user_data);
	}
}

void GssLiveConn::OnDeviceDisconnect( void *transport, void *user_data )
{
//	printf("GssLiveConn::OnDeviceDisconnect\n");
	GssLiveConn* pConn = (GssLiveConn*)user_data;
	if(pConn)
	{
		LOG_INFO("[GSSLIVECONN] GssLiveConn::OnDeviceDisconnect , conn = %p, uid = %s\n", user_data,pConn->m_uid);
		pConn->Stop();
		pConn->Start(true);
	}
	else
	{
		LOG_ERROR("[GSSLIVECONN] GssLiveConn::OnDeviceDisconnect , transport = %p, conn = %p\n", transport, user_data);
	}
}

void GssLiveConn::InitVideoBuffer()
{
	for (int i = 0; i < MAX_AV_VIDEO_SIZE; i++)
	{
		GssBuffer* tmpbuf = new GssBuffer();
		m_bufferVideo[i] = tmpbuf;
	}
}

void GssLiveConn::ClearVideoBuffer()
{
	for (int i = 0; i < MAX_AV_VIDEO_SIZE; i++)
	{
		GssBuffer* tmpbuf = m_bufferVideo[i];
		delete tmpbuf;
		m_bufferVideo[i] = NULL;
	}
}

void GssLiveConn::DropAllVideoBuffer()
{
	m_bDroped = true;
	m_curVideoIndex = 0;
	m_currNextIndexVideoInsert = 0;
	int tmpiFrame = -1;
	if(m_countsVideo > 0)
	{
		for (int i = 0;  i < MAX_AV_VIDEO_SIZE; i++)
		{
			if(m_bufferVideo[i]->m_frameHeader.nFrameType == gos_video_i_frame)
			{
				tmpiFrame = i;
			}
		}
	}
	if (tmpiFrame != -1)
	{
		m_curVideoIndex = tmpiFrame;
		m_currNextIndexVideoInsert = tmpiFrame;
		IncVideoNextInsertIndex();
		m_countsVideo = 1;
	}
	else
	{
		m_countsVideo = 0;
	}
}

void GssLiveConn::InitAudioBuffer()
{
	for (int i = 0; i < MAX_AV_AUDIO_SIZE; i++)
	{
		GssBuffer* tmpbuf = new GssBuffer(false);
		m_bufferAudio[i] = tmpbuf;
	}
}

void GssLiveConn::ClearAudioBuffer()
{
	for (int i = 0; i < MAX_AV_AUDIO_SIZE; i++)
	{
		GssBuffer* tmpbuf = m_bufferAudio[i];
		delete tmpbuf;
		m_bufferAudio[i] = NULL;
	}
}

bool GssLiveConn::AddVideoFrame( unsigned char* pData, int datalen )
{
//	printf("Start AddVideoFrame \n");
	bool bsuc = false;
	do 
	{		
		//是否丢弃，直到下一个I帧
		GosFrameHead *header = (GosFrameHead*)pData;
		if(m_bDroped)
		{
			if (header->nFrameType == gos_video_i_frame)
			{
				m_bDroped = false;
			}
			else
			{
				break;
			}
		}

		//开始时间，准备计算时间
		if(GssLiveConn::m_forceLiveSec > 0 || m_forceLiveSecLeftTime > 0)
		{
			if(m_liveRef == 0)
			{
				m_liveRef = header->nTimestamp;
			}
			else if(int(header->nTimestamp - m_liveRef) > GssLiveConn::m_forceLiveSec*1000 || (int(header->nTimestamp - m_liveRef) > m_forceLiveSecLeftTime*1000))
			{
				printf("m_forcePause now\n");
				m_forcePause = true;//标志，不在添加数据
				GssLiveConn::DelPlayTime(m_uid);
			}
		}
				
		m_lockVideo.Lock();
		if (m_countsVideo < MAX_AV_VIDEO_SIZE)
		{
			bsuc = m_bufferVideo[m_currNextIndexVideoInsert]->SetBuffer(pData,datalen);
			if(bsuc)
			{
				if(m_audioType == gos_codec_unknown)
				{
					gos_codec_type_t codec_type = (gos_codec_type_t)m_bufferVideo[m_currNextIndexVideoInsert]->m_frameHeader.nCodeType;
					if(codec_type == gos_video_H264_G711)
					{
						m_audioType = gos_audio_G711A;
					}
					else if(codec_type == gos_video_H264_AAC)
					{
						m_audioType = gos_audio_AAC;
					}
				}
				IncVideoNextInsertIndex();
				m_countsVideo++;
			}
		}
		else
		{	//无法再放入时，开始丢弃到下一个I帧
			m_bDroped = true;
		}
		m_lockVideo.Unlock();
	} while (0);

	return bsuc;
}

bool GssLiveConn::AddAudioFrame( unsigned char* pData, int datalen )
{
	bool bsuc = false;
	do 
	{
		if (pData == NULL || datalen <= 0)
		{
			break;
		}
		//视频丢弃则这段音频也丢弃
		if(m_bDroped)
			break;
		
		m_lockAudio.Lock();
		if (m_countsAudio < MAX_AV_AUDIO_SIZE)
		{
			bsuc = m_bufferAudio[m_currNextIndexAudioInsert]->SetBuffer(pData,datalen);
			if (bsuc)
			{
				if(m_audioType == gos_codec_unknown)
				{
					m_audioType = (gos_codec_type_t)m_bufferAudio[m_currNextIndexAudioInsert]->m_frameHeader.nCodeType;
				}
				IncAudioNextInsertIndex();
				m_countsAudio++;
			}
		}
		m_lockAudio.Unlock();
	} while (0);

	return bsuc;
}

bool GssLiveConn::AddFrame( unsigned char* pData, int datalen )
{
	if(m_forcePause)
		return false;
	
	bool bsuc = false;
	do 
	{
		GP2pHead header;
		memcpy(&header,pData,sizeof(GP2pHead));
		//字节序 ?
		if (m_isFirstFrame)
		{
			LOG_INFO("[GSSLIVECONN] incoming first frame, is video = %d",header.msgType == SND_VIDEO);
		}

		if (header.msgType == SND_VIDEO)
		{
			bsuc = AddVideoFrame(pData + sizeof(GP2pHead),datalen - sizeof(GP2pHead));
		}
		else if (header.msgType == SND_AUDIO)
		{
			bsuc = AddAudioFrame(pData + sizeof(GP2pHead), datalen - sizeof(GP2pHead));
		}
		else
		{
			//暂不处理
			break;
		}
	} while (0);

	if (m_isFirstFrame)
	{
		m_isFirstFrame = false;
	}

	return bsuc;
}

void GssLiveConn::IncVideoIndex()
{
	m_curVideoIndex++;
	if (m_curVideoIndex >= MAX_AV_VIDEO_SIZE)
	{
		m_curVideoIndex = 0;
	}
}

void GssLiveConn::IncVideoNextInsertIndex()
{
	m_currNextIndexVideoInsert++;
	if (m_currNextIndexVideoInsert >= MAX_AV_VIDEO_SIZE)
	{
		m_currNextIndexVideoInsert = 0;
	}
}

void GssLiveConn::IncAudioIndex()
{
	m_curAudioIndex++;
	if (m_curAudioIndex >= MAX_AV_AUDIO_SIZE)
	{
		m_curAudioIndex = 0;
	}
}

void GssLiveConn::IncAudioNextInsertIndex()
{
	m_currNextIndexAudioInsert++;
	if(m_currNextIndexAudioInsert >= MAX_AV_AUDIO_SIZE)
		m_currNextIndexAudioInsert = 0;
}

bool GssLiveConn::IsReachedMaxPlayTimeofDay(int theMaxTimeMin, const char* guid, int & leftTimeSec)
{
	bool bsuc = false;
	leftTimeSec = 0;
	do 
	{
		PublicMySql *pSqlp = GssLiveConn::m_smysqlpool->GetConnection();
		if(pSqlp)
		{
			MyuseMysql useSql(pSqlp);
			int maxSec;
			unsigned int startMs;
			int rtsptime;
			unsigned int nowtimes = now_ms_time()/1000;
			if (useSql.QueryTimesByGuid(TABLE_DEVICE_REC, guid, maxSec,startMs,GetColByType(),rtsptime))
			{
				if (nowtimes - startMs > ONE_DAY_SEC)
				{
					if( !useSql.UpdateTimesByGuid(TABLE_DEVICE_REC,guid,0,nowtimes,GetColByType(),0) )
						LOG_ERROR("INSERT NEW RECORD FAILED, GUID = %s\n",guid);
					bsuc = false;
					leftTimeSec = theMaxTimeMin*60;
				}
				else
				{
					if (maxSec < theMaxTimeMin*60)
					{
						bsuc = false;
						leftTimeSec = theMaxTimeMin*60 - maxSec;
					}
					else
					{
						LOG_INFO("The %s reached play time of day!",guid);
						bsuc = true;
					}
				}
			}
			else
			{
				LOG_INFO("The %s reached play time of day, the reason is query mysql failed!",guid);
				bsuc = true;
			}
			
			GssLiveConn::m_smysqlpool->ReleaseConnection(pSqlp);
		}
		else
		{
			LOG_INFO("The %s reached play time of day, get sql connection failed!",guid);
			bsuc = true;
		}

	} while (false);

	return bsuc;
}

bool GssLiveConn::UpdatePlayTime(int onceTime, const char* guid)
{
	bool bsuc = false;
	do 
	{
		PublicMySql *pSqlp = GssLiveConn::m_smysqlpool->GetConnection();
		if(pSqlp)
		{
			MyuseMysql useSql(pSqlp);
			int maxSec;
			unsigned int startt;
			int rtsptime;
			if (useSql.QueryTimesByGuid(TABLE_DEVICE_REC, guid, maxSec,startt,GetColByType(),rtsptime))
			{
				int insertSec = maxSec + onceTime;
				rtsptime += onceTime;
				bsuc = useSql.UpdateTimesByGuid(TABLE_DEVICE_REC, guid, insertSec, 0,GetColByType(), rtsptime);
			}

			GssLiveConn::m_smysqlpool->ReleaseConnection(pSqlp);
		}
		else
		{
			LOG_ERROR("GssLiveConn::UpdatePlayTime failed, get sql connection failed!");
		}

	} while (false);

	return bsuc;
}

bool GssLiveConn::ResetPlayTime(const char* guid, int nCol, int nColValue)
{
	bool bsuc = false;
	do 
	{
		PublicMySql *pSqlp = GssLiveConn::m_smysqlpool->GetConnection();
		if(pSqlp)
		{
			MyuseMysql useSql(pSqlp);
			unsigned int starttime = now_ms_time()/1000;
			bsuc = useSql.ResetTimesByGuid(TABLE_DEVICE_REC, guid, 0, starttime, nCol, nColValue);
			GssLiveConn::m_smysqlpool->ReleaseConnection(pSqlp);
		}
		else
		{
			LOG_ERROR("GssLiveConn::UpdatePlayTime failed, get sql connection failed!");
		}

	} while (false);

	return bsuc;
}


bool GssLiveConn::AddNewPlayTime(const char* guid)
{
	bool bsuc = true;
	if (!guid)
	{
		bsuc = false;
	}
	else
	{
		GssLiveConn::m_sGlobalInfos.lock.Lock();
		RtspPlayTime rtspT;
		rtspT.time = now_ms_time()/1000;
		rtspT.bDel = false;
		GssLiveConn::m_sGlobalInfos.mapTimes.insert(std::map<std::string,RtspPlayTime>::value_type(guid,rtspT));
		GssLiveConn::m_sGlobalInfos.lock.Unlock();
	}
	return bsuc;
}

bool GssLiveConn::DelPlayTime(const char* guid)
{
	bool bsuc = true;
	if (!guid)
	{
		bsuc = false;
	}
	else
	{
		GssLiveConn::m_sGlobalInfos.lock.Lock();
		std::map<std::string,RtspPlayTime>::iterator it = GssLiveConn::m_sGlobalInfos.mapTimes.find(guid);
		if (it != GssLiveConn::m_sGlobalInfos.mapTimes.end())
		{
			(*it).second.bDel = true;
			//GssLiveConn::m_sGlobalInfos.mapTimes.erase(it);
		}		
//		GssLiveConn::m_sGlobalInfos.mapTimes.insert(std::map<std::string,unsigned int>::value_type(guid,0));
		GssLiveConn::m_sGlobalInfos.lock.Unlock();
	}
	return bsuc;
}

bool GssLiveConn::SetForceLiveSec(int sec)
{
	printf("m_forceLiveSec:%d\n", sec);
	GssLiveConn::m_forceLiveSec = sec;
	return true;
}

bool GssLiveConn::IsAudioG711AType()
{
	return (m_audioType == gos_audio_G711A) || (m_audioType == gos_video_H264_G711);
}

bool GssLiveConn::IsAudioAacType()
{
	return (m_audioType == gos_audio_AAC) || (m_audioType == gos_video_H264_AAC);
}

bool GssLiveConn::IsKnownAudioType()
{
	return IsAudioAacType() || IsAudioG711AType();
}

bool GssLiveConn::GlobalInit(	const char* pserver, const char* plogpath, int loglvl, 
												const char* sqlHost, int sqlPort,
												const char* sqlUser, const char* sqlPasswd,
												const char* dbName, int maxCounts, int maxPlayTime, EGSSCONNTYPE type)
{
	if(GssLiveConn::m_isInitedGlobal)
		return true;
	int s_retinit = -1;
	s_retinit = p2p_init(NULL);
	if(s_retinit != 0)
	{
		printf("p2p_init error");
		return false;
	}

	p2p_log_set_level(0);

	//设置全局属性
	int maxRecvLen = 1024*1024;
	int maxClientCount = 10;
	p2p_set_global_opt(P2P_MAX_CLIENT_COUNT, &maxClientCount, sizeof(int));
	p2p_set_global_opt(P2P_MAX_RECV_PACKAGE_LEN, &maxRecvLen, sizeof(int));
	if(pserver)
	{
		char pTmps[1024] = {0};
		strcpy(pTmps,pserver);
		char* ptr = strtok(pTmps,":");
		if (ptr != NULL)
		{
			strcpy(GssLiveConn::m_sGlobalInfos.domainDispath,ptr);
			ptr = strtok(NULL,":");
			if (ptr != NULL)
			{
				GssLiveConn::m_sGlobalInfos.port = atoi(ptr);
			}
		}
		GssLiveConn::m_sGlobalInfos.maxPlayTime = maxPlayTime;
	}
	if (plogpath)
	{
		strcpy(GssLiveConn::m_sGlobalInfos.logs,plogpath);
		LogInit(GssLiveConn::m_sGlobalInfos.logs,"live555",20000);
		LogSetLevel(loglvl);
	}
	GssLiveConn::m_sGlobalInfos.type = type;

	if(!GssLiveConn::m_smysqlpool)
	{
		GssLiveConn::m_smysqlpool = new MySqlPool(sqlHost,sqlPort,sqlUser,sqlPasswd,dbName,maxCounts + 1);
		if(GssLiveConn::m_smysqlpool == NULL)
		{
			LOG_ERROR("Create MysqlPool %s:%d Failed!",sqlHost,sqlPort);
			return false;
		}
		if (GssLiveConn::m_smysqlpool->InitPool(maxCounts) != 0)
		{
			LOG_ERROR("Init MysqlPool %s:%d Failed!",sqlHost,sqlPort);
			return false;
		}
	}

	GssLiveConn::m_isInitedGlobal = true;
	if ( 0 != pthread_create(&GssLiveConn::m_spthread,NULL,ThreadUpdatePlayTimeToMysql, NULL))
	{
		LOG_ERROR("create thread ThreadUpdatePlayTimeToMysql failed!");
		GssLiveConn::m_isInitedGlobal = false;
		return false;
	}

	GssLiveConn::m_isInitedGlobal = true;
	return true;
}

void GssLiveConn::GlobalUnInit()
{
	if(GssLiveConn::m_isInitedGlobal)
	{
		p2p_uninit();
		void* status;
		GssLiveConn::m_isInitedGlobal = false;
		pthread_join(GssLiveConn::m_spthread,&status);
		if(GssLiveConn::m_smysqlpool)
		{
			delete GssLiveConn::m_smysqlpool;
			GssLiveConn::m_smysqlpool = NULL;
		}
	}
}

void GssLiveConn::OnDsCallback(void* dispatcher, int status, void* user_data, char* server, unsigned short port, unsigned int server_id)
{
	GssLiveConn* pThis = (GssLiveConn*)user_data;
	if(status == P2P_SUCCESS)
	{
//		printf("get server on dscallback ok, %s:%d",server,port);
		if(pThis)
		{
			strcpy(pThis->m_server,server);
			pThis->m_port = port;
		}
	}
	else
	{
//		printf("ds_callback failed, gssliveconn = %p, %d\r\n", user_data , status);
		LOG_ERROR("[GSSLIVECONN] GssLiveConn::OnDsCallback dispatch = %p, status = %d, user_data = %p, server = %s, port = %d, server_id = %d",dispatcher,status,user_data,server,port,server_id);
	}
	if (pThis)
	{
		pThis->m_semt.Post();
	}
}

bool GssLiveConn::RequestLiveConnServer()
{
	bool bsuc =false;
	char dsSvr[256] = {0};
	void* dispatcher = NULL;
	sprintf(dsSvr,"%s:%d",m_dispathServer,m_dispathPort);
	if (!m_semt.IsInitOk())
	{
		m_semt.Init();
		if (!m_semt.IsInitOk())
		{
			return false;
		}
	}
	
	int rlt = gss_query_dispatch_server(m_uid, dsSvr, this , &GssLiveConn::OnDsCallback, &dispatcher);
	m_semt.Wait();
	if (rlt == 0 && m_port != 0)
	{
		bsuc = true;
	}
	m_semt.UnInit();
	destroy_gss_dispatch_requester(dispatcher);	
	return bsuc;
}

int GssLiveConn::GetColByType()
{
	int nCol = 0;
	switch (GssLiveConn::m_sGlobalInfos.type)
	{
	case e_gss_conn_type_rtsp:
		{
			nCol = e_table_device_rec_col_rtsp;
		}
		break;
	case e_gss_conn_type_hls:
		{
			nCol = e_table_device_rec_col_hls;
		}
		break;
	default:
		nCol = e_table_device_rec_col_rtsp;
	}
	return nCol;
}