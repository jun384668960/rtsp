#include "DynamicRTSPServer.hh"
#include "RtspSvr.hh"
#include "debuginfo.h"

CRtspServer* CRtspServer::instance = NULL;

CRtspServer* CRtspServer::GetInstance()
{
	if(instance == NULL)
	{
		instance = new CRtspServer();
	}

	return instance;
}

CRtspServer::CRtspServer()
	:m_Stop(true),m_Input(NULL),m_watchVariable(0),m_scheduler(NULL),m_env(NULL),m_rtspServer(NULL),m_pThread(0)
{

}

CRtspServer::~CRtspServer()
{
	if(m_env == NULL && m_scheduler == NULL)
	{
		Destory();
	}
}

void * CRtspServer::ThreadRtspServerProcImpl(void* arg)
{
	CRtspServer* cRtspServer = (CRtspServer*)arg;

	cRtspServer->ThreadRtspServer();
	
	return NULL;
}

void CRtspServer::ThreadRtspServer()
{

	UserAuthenticationDatabase* authDB = NULL;
#ifdef ACCESS_CONTROL
	authDB = new UserAuthenticationDatabase;
	authDB->addUserRecord("username1", "password1"); // replace these with real strings
#endif

	portNumBits rtspServerPortNum = 8554;
	m_rtspServer = DynamicRTSPServer::createNew(*m_env, rtspServerPortNum, authDB);
	if (m_rtspServer == NULL) 
	{
		debug_log(LEVEL_ERROR,"Failed to create RTSP server: %s", m_env->getResultMsg());
		exit(1);
	}

	m_env->taskScheduler().doEventLoop(&m_watchVariable); // does not return

}

bool CRtspServer::Create()
{
	debug_set(LEVEL_DEBUG, TARGET_LOGFILE, "/mnt/sdcard/RtspServer.txt");
	if(m_env != NULL && m_scheduler != NULL)
	{
		debug_log(LEVEL_ERROR,"CRtspServer is already Create yet, call Create error!");
		return false;
	}
	
	m_scheduler = BasicTaskScheduler::createNew();
	m_env = BasicUsageEnvironment::createNew(*m_scheduler);
	
	m_Input = LiveStreamMediaSource::createNew(*m_env);

	return true;
}

bool CRtspServer::Destory()	
{
	if(!m_Stop)
	{
		Stop();
	}

	if(m_env == NULL && m_scheduler == NULL)
	{
		debug_log(LEVEL_ERROR,"CRtspServer is already Destory yet, call Destory error!");
		return false;
	}
	
	m_env->reclaim(); m_env = NULL;
    delete m_scheduler; m_scheduler = NULL;

	return true;
}

bool CRtspServer::Start()
{
	
	if(m_env == NULL || m_scheduler == NULL)
	{
		debug_log(LEVEL_ERROR,"CRtspServer is not Create yet, call Create first!");
		return false;
	}

	if(!m_Stop)
	{
		debug_log(LEVEL_ERROR,"CRtspServer is already start, call Start error!");
		return false;
	}
	
	m_watchVariable = 0;
	if(pthread_create(&m_pThread,NULL, ThreadRtspServerProcImpl,this))
	{
		debug_log(LEVEL_ERROR,"ThreadRtspServerProcImpl false!\n");
		return false;
	}
		
	m_Stop = false;
	
	return true;
}

bool CRtspServer::Stop()
{
	if(m_Stop)
	{
		debug_log(LEVEL_ERROR,"CRtspServer is stop, call stop error!");
		return false;
	}
	
	m_watchVariable = 1;
	::pthread_join(m_pThread, 0); 
	m_pThread = 0;
	Medium::close(m_rtspServer); m_rtspServer = NULL;
	m_Stop = true;

	return true;
}

bool CRtspServer::Restart()
{
	bool ret = false;
	ret = Stop();
	if(!ret)
	{
		return ret;
	}
	ret = Start();

	return ret;
}

bool CRtspServer::LiveSourcePush(char* data, long len, long pts, int flag)
{
	if(flag == 0)//video
	{
		VideoElem elem;
		if(len > DEFAULT_MAX_VEDIOFRAME_SIZE)
		{
			//drop data
			elem.lenght = DEFAULT_MAX_VEDIOFRAME_SIZE;
		}
		else
		{
			elem.lenght = (unsigned int)len;
		}
		memcpy(elem.data, data, elem.lenght);
		elem.pts = pts;
		
		m_Input->m_H265VideoSrc->PushBack(elem);
	}
	else if(flag == 1)//audio
	{
		AudioElem elem;
		if(len > DEFAULT_MAX_AUDIOFRAME_SIZE)
		{
			//drop data
			elem.lenght = DEFAULT_MAX_AUDIOFRAME_SIZE;
		}
		else
		{
			elem.lenght = (unsigned int)len;
		}
		memcpy(elem.data, data, elem.lenght);
		elem.pts = pts;

		m_Input->m_AdtsAudioSrc->PushBack(elem);
	}
	return true;
}

bool CRtspServer::LiveSourceSync(int flag)
{
	if(flag == 0)//video
	{
		m_Input->m_H265VideoSrc->SyncRwPoint();
	}
	else if(flag == 1)//audio
	{
		m_Input->m_AdtsAudioSrc->SyncRwPoint();
	}
	return true;
}

void CRtspServer::RegisterSeekFunc()
{

}

