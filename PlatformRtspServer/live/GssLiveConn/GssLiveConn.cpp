#include "GssLiveConn.h"
#include "gss_transport.h" 
//#include "gss_common.h"
#include "p2p_dispatch.h"
#include "Log.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

bool GssLiveConn::m_isInitedGlobal = false;
SGlobalInfo GssLiveConn::m_sGlobalInfos;

GssLiveConn::GssLiveConn()
{
	memset(m_server,0,sizeof(m_server));
	memset(m_dispathServer,0,sizeof(m_dispathServer));
	memset(m_uid,0,sizeof(m_uid));
	memset(m_bufferVideo, 0, MAX_AV_SIZE*sizeof(GssBuffer*));
	memset(m_bufferAudio, 0, MAX_AV_SIZE*sizeof(GssBuffer*));
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

	InitVideoBuffer();
	InitAudioBuffer();
}

GssLiveConn::GssLiveConn(const char* server, unsigned short port,const char* uid , bool bDispath)
{
//	printf("GssLiveConn::GssLiveConn start\n");
	m_isConnected = false;
	m_clientPullConn = NULL;
	m_stusRlt = 0;
	memset(m_bufferVideo, 0, MAX_AV_SIZE*sizeof(GssBuffer*));
	memset(m_bufferAudio, 0, MAX_AV_SIZE*sizeof(GssBuffer*));
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
		LOG_INFO("[GSSLIVECONN] GssLiveConn::Stop");
		return true;
	}
	return true;
}

bool GssLiveConn::IsConnected()
{
	return m_isConnected;
}

bool GssLiveConn::GetVideoFrame( unsigned char** pData, int &datalen )
{
	bool bSuc = false;
//	printf("********%p GetVideoFrame count = %d\n",this,m_countsVideo);
	if (m_countsVideo > 0 && pData && *pData == NULL)
	{
		*pData = m_bufferVideo[m_curVideoIndex]->m_buffer;
		datalen = m_bufferVideo[m_curVideoIndex]->m_bufferlen;
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

bool GssLiveConn::GetAudioFrame( unsigned char** pData, int &datalen )
{
	bool bSuc = false;
//	printf("********%p GetAudioFrame count = %d\n",this,m_countsAudio);
	if (m_countsAudio > 0 && pData && *pData == NULL)
	{
		*pData = m_bufferAudio[m_curAudioIndex]->m_buffer;
		datalen = m_bufferAudio[m_curAudioIndex]->m_bufferlen;
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

void GssLiveConn::OnRecv( void *transport, void *user_data, char* data, int len, char type, unsigned int time_stamp )
{
//	printf("GssLiveConn::OnRecv len = %d\n",len);
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
	for (int i = 0; i < MAX_AV_SIZE; i++)
	{
		GssBuffer* tmpbuf = new GssBuffer();
		m_bufferVideo[i] = tmpbuf;
	}
}

void GssLiveConn::ClearVideoBuffer()
{
	for (int i = 0; i < MAX_AV_SIZE; i++)
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
		for (int i = 0;  i < MAX_AV_SIZE; i++)
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
	for (int i = 0; i < MAX_AV_SIZE; i++)
	{
		GssBuffer* tmpbuf = new GssBuffer(false);
		m_bufferAudio[i] = tmpbuf;
	}
}

void GssLiveConn::ClearAudioBuffer()
{
	for (int i = 0; i < MAX_AV_SIZE; i++)
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
	bool bContinue = true;
	do 
	{
		if (pData == NULL || datalen <= 0)
		{
			break;
		}
		m_lockVideo.Lock();

		if(m_bDroped)
		{
			GosFrameHead *header = (GosFrameHead*)pData;
			if (header->nFrameType == gos_video_i_frame)
			{
				m_bDroped = false;
			}
			else
			{
				bContinue = false;
			}
		}

		if (!m_bDroped && m_countsVideo < MAX_AV_SIZE)
		{
			bsuc = m_bufferVideo[m_currNextIndexVideoInsert]->SetBuffer(pData,datalen);
			if(bsuc)
			{
				if(m_audioType == gos_codec_unknown)
				{
					m_audioType = (gos_codec_type_t)m_bufferAudio[m_currNextIndexAudioInsert]->m_frameHeader.nCodeType;
//					printf("m_audioType = %d\n",m_audioType);
				}
				IncVideoNextInsertIndex();
				m_countsVideo++;
//				printf("********** add video frame success, %p\n",this);
			}
			else 
			{
				DropAllVideoBuffer();
			}
			bContinue = false;
		}
		else
		{
			DropAllVideoBuffer();
		}
		m_lockVideo.Unlock();
		
	} while (bContinue);

//	printf("end AddVideoFrame \n");
	return bsuc;
}

bool GssLiveConn::AddAudioFrame( unsigned char* pData, int datalen )
{
	bool bsuc = false;
	bool bContinue = true;
	do 
	{
		if (pData == NULL || datalen <= 0)
		{
			break;
		}
		m_lockAudio.Lock();
		if (m_countsAudio < MAX_AV_SIZE)
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
//				printf("********** add audio frame success %p\n",this);
			}
			bContinue = false; //假设出现SetBuffer失败，则会跳过该音频
		}
		else
		{
			IncAudioIndex();
			m_countsAudio--;
		}
		m_lockAudio.Unlock();
	} while (bContinue);

	return bsuc;
}

bool GssLiveConn::AddFrame( unsigned char* pData, int datalen )
{
	bool bsuc = false;
	do 
	{
		if (pData == NULL || datalen <= 0)
		{
			break;
		}

		GP2pHead header;
		memcpy(&header,pData,sizeof(GP2pHead));
		//字节序 ?

		//? header.size ?= datalen - sizeof(GP2pHead)
//		printf("this is %p, type = %x\n",this,header.msgType);
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

//	printf("GssliveConn add frame end\n");
	return bsuc;
}

void GssLiveConn::IncVideoIndex()
{
	m_curVideoIndex++;
	if (m_curVideoIndex >= MAX_AV_SIZE)
	{
		m_curVideoIndex = 0;
	}
}

void GssLiveConn::IncVideoNextInsertIndex()
{
	m_currNextIndexVideoInsert++;
	if (m_currNextIndexVideoInsert >= MAX_AV_SIZE)
	{
		m_currNextIndexVideoInsert = 0;
	}
}

void GssLiveConn::IncAudioIndex()
{
	m_curAudioIndex++;
	if (m_curAudioIndex >= MAX_AV_SIZE)
	{
		m_curAudioIndex = 0;
	}
}

void GssLiveConn::IncAudioNextInsertIndex()
{
	m_currNextIndexAudioInsert++;
	if(m_currNextIndexAudioInsert >= MAX_AV_SIZE)
		m_currNextIndexAudioInsert = 0;
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

bool GssLiveConn::GlobalInit(const char* pserver, const char* plogpath, int loglvl)
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
	}
	if (plogpath)
	{
		strcpy(GssLiveConn::m_sGlobalInfos.logs,plogpath);
		LogInit(GssLiveConn::m_sGlobalInfos.logs,"live555",20000);
		LogSetLevel(loglvl);
	}
	GssLiveConn::m_isInitedGlobal = true;
	return true;
}

void GssLiveConn::GlobalUnInit()
{
	if(GssLiveConn::m_isInitedGlobal)
		p2p_uninit();
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