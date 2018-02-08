#include <pthread.h>
#include "DynamicRTSPServer.hh"
#include "H264VideoLiveServerMediaSubsession.hh"
#include "ADTSAudioLiveServerMediaSubsession.hh"
#include "GroupsockHelper.hh"

#include "RtspSvr.hh"
//#include "JNI_load.h"

#define PUT_SPS_PPS	1

//extern JNIService g_jBQLService;
CRtspServer* CRtspServer::pThis = NULL;


unsigned char* check_sps_with_pps(unsigned char* data, unsigned int length)
{
	int i = 0;
	while(i < length - 4 )
	{
		if (data[i] == 0x0 && data[i + 1] == 0x0 && data[i + 2] == 0x0 && data[i + 3] == 0x1)
		{
			return data + i + 4;
		}
		i++;
	}
	return NULL;
}
int CRtspServer::PrepareStream(const char *pStreamName)
{
	/*
	if(m_env != NULL && m_scheduler != NULL)
	{
		LOGE_print("CRtspServer", "CRtspServer is already Create yet, call Create error!");
		return -1;
	}
	*/
	
    strcpy(m_StreamName, pStreamName);
	LOGD_print("CRtspServer", "Call CRtspServer::PrepareStream! m_Input:%p!", m_Input);
	
	return 0;

}
int CRtspServer::ReleaseStream()
{
	LOGD_print("CRtspServer", "Call CRtspServer::ReleaseStream!");
	if(!m_Stop)
	{
		Stop();
	}

	/*
	m_Input->UnRegisterVideoDemandIDR();
	
	if(m_env != NULL)
	{
		LOGW_print("CRtspServer", "CRtspServer is m_env != NULL Destory!");
		//m_env->reclaim(); 
		m_env = NULL;
	}

	if (m_scheduler != NULL)
	{
		LOGW_print("CRtspServer", "CRtspServer is m_scheduler != NULL Destory!");
		//delete m_scheduler; 
		m_scheduler = NULL;
	}
	*/

	return true;

}
int CRtspServer::Start(int port)
{
	LOGD_print("CRtspServer", "Call CRtspServer::Start!");
	/*
	if(m_env == NULL || m_scheduler == NULL)
	{
		LOGE_print("CRtspServer", "CRtspServer is not Create yet, call Create first!");
		return -1;
	}
	*/
	if(!m_Stop)
	{
		LOGE_print("CRtspServer", "CRtspServer is already start, call Start error!");
		return -1;
	}

	m_port = port;
	m_watchVariable = 0;
	if(pthread_create(&m_pThread,NULL, ThreadRtspServerProcImpl,this))
	{
		LOGE_print("CRtspServer", "ThreadRtspServerProcImpl false!");
		return -1;
	}

	/*
	struct sched_param param;
	int policy;

	pthread_getschedparam(&m_pThread, &policy, &param);
	param.sched_priority = sched_get_priority_max(policy); policy = SCHED_RR;
	pthread_setschedparam(&m_pThread,  policy, &param); 
	*/
	m_Stop = false;
	
	return 0;

}
int CRtspServer::Stop()
{
	LOGD_print("CRtspServer", "Call CRtspServer::Stop!");
	if(m_Stop)
	{
		LOGE_print("CRtspServer", "CRtspServer is stop, call stop error!");
		return -1;
	}
	
	m_watchVariable = 1;
	::pthread_join(m_pThread, 0); 
	m_pThread = 0;
	//Medium::close(m_rtspServer); m_rtspServer = NULL;
	m_Stop = true;

	return 0;

}

/*
FILE* fd_w = NULL;
void writeH264(unsigned char* data, unsigned int length)
{
	if(fd_w == NULL)
	{
		fd_w = fopen("/sdcard/test1.video", "wb");
	}
	fwrite(data,1,length,fd_w);
}
*/
bool CRtspServer::PushVideo(unsigned char* data, unsigned int length, uint64_t pts)
{	
	m_pushVCnt++;
	if(m_pushVCnt >= 200)
	{
		m_pushVCnt = 0;
		LOGE_print("CRtspServer","RtspSvr push Video data...");
	}

	if(m_Input != NULL && m_Input->VbuffMgr != NULL)
	{
		//make sure 
		if(length <= 4)
		{
			return false;
		}

		u_int8_t nal_unit_type;
		nal_unit_type = data[4] & 0x1F;
		
		if (data[0] == 0x0 && data[1] == 0x0 && data[2] == 0x0 && data[3] == 0x1)//sps
		{
			if (nal_unit_type == 7)
			{
				//LOGD_print("CRtspServer", "SrcPointerSync");
				m_Input->SrcPointerSync(1);
				
				//LOGD_print("CRtspServer", "nal_unit_type == 7");
				for(int i = 0; i< length; i++)
				{
					//LOGD_print("CRtspServer", "%2X", data[i]);
				}
				
				//check contains pps
				unsigned char* pps = check_sps_with_pps(data + 4, length - 4);
				if(pps != NULL)
				{
					int sps_len = pps - data - 4 - 4;
					m_Input->SaveSpsAndPps(data + 4, sps_len, pps, length - 4 - sps_len - 4);
#ifdef PUT_SPS_PPS
 
					//LOGD_print("CRtspServer", "==>>Video time:%llu skip start_bit nal_unit_type:%d elem.lenght:%d elem.pts:%llu",GetTickCount(),(int)nal_unit_type, length, pts);
 
 					m_Input->VbuffMgr->Write(data + 4, sps_len, pts);
					m_Input->VbuffMgr->Write(pps, length - 4 - sps_len - 4, pts);
#endif
				}
				else
				{
					m_Input->SaveSpsAndPps(data + 4, length - 4, NULL, 0);
					
#ifdef PUT_SPS_PPS 
					//LOGD_print("CRtspServer", "==>>Video time:%llu skip start_bit nal_unit_type:%d elem.lenght:%d elem.pts:%llu",GetTickCount(),(int)nal_unit_type, length - 4,  pts);
					m_Input->VbuffMgr->Write(data + 4, length - 4, pts);
#endif		
				}
			}
			else if (nal_unit_type == 8)
			{
				//m_Input->SrcPointerSync(0);
				
				//LOGD_print("CRtspServer", "nal_unit_type == 8");
				for(int i = 0; i< length; i++)
				{
					//LOGD_print("CRtspServer", "%2X", data[i]);
				}
				m_Input->SaveSpsAndPps(NULL, 0, data + 4, length - 4);
				
				
#ifdef PUT_SPS_PPS
				m_Input->VbuffMgr->Write(data + 4, length - 4, pts);
#endif
			}
			else
			{ 
				LOGD_print("CRtspServer", "==>>Video skip start_bit nal_unit_type:%d elem.lenght:%d elem.pts:%llu",(int)nal_unit_type, length - 4,  pts);
				m_Input->VbuffMgr->Write(data + 4, length - 4, pts);
			}
		}
		//fwrite(data, length, 1, m_264);
	}
	
	return true;
}

bool CRtspServer::PushAudio(unsigned char* data, unsigned int length, uint64_t pts)
{
	m_pushACnt++;
	if(m_pushACnt >= 200)
	{
		m_pushACnt = 0;
		LOGE_print("CRtspServer","RtspSvr push Audio data...");
	}
 
	LOGD_print("CRtspServer", "Audio elem.lenght:%d elem.pts:%llu", length, pts);
	if(m_Input != NULL && m_Input->AbuffMgr != NULL)
	{
		m_Input->AbuffMgr->Write(data, length, pts);
	}
	
	return true;
}

int CRtspServer::SetParamConfig(const char *key, const char *val)
{
	if( strcmp(key, "videomfps") == 0 )
	{
		m_Input->SetFrameRate(atoi(val));
		return 0;
	}
	else if( strcmp(key, "audiofrequency") == 0 )
	{
		m_Input->SetsamplingFrequency(atoi(val));
		return 0;
	}
	else if( strcmp(key, "audiochannels") == 0 )
	{
		m_Input->SetnumChannels(atoi(val));
		return 0;
	}
	else if( strcmp(key, "audioprofile") == 0 )
	{
		m_Input->SetProfile(atoi(val));
		return 0;
	}
	else if( strcmp(key, "audiobiterate") == 0 )
	{
		m_Input->SetbiteRate(atoi(val));
		return 0;
	}
	else if( strcmp(key, "enableaudio") == 0 )
	{
		LOGD_print("CRtspServer", "enableaudio = %s!",val);
		int enable = atoi(val);
		if(enable == 1)
		{
			m_EnableAudio = true;
		}
		else
		{
			m_EnableAudio = false;
		}

		return 0;
	}

	return 1;
}


CRtspServer::CRtspServer()
	:m_port(8554),m_Stop(true),m_Input(NULL),m_watchVariable(0),m_scheduler(NULL),m_env(NULL),m_rtspServer(NULL),m_pThread(0)
{
	pThis = this;
	m_pushVCnt = 0;
	m_pushACnt = 0;
	m_EnableAudio = true;

	m_scheduler = BasicTaskScheduler::createNew();
	m_env = BasicUsageEnvironment::createNew(*m_scheduler);
	m_Input = LiveStreamMediaSource::createNew(*m_env);
	
	m_Input->RegisterVideoDemandIDR(DemandIDR);

	m_264 = fopen("v.264","wb");
}

CRtspServer::~CRtspServer()
{
	/*
	if(m_env != NULL && m_scheduler != NULL)
	{
		ReleaseStream();
	}
	*/
	if( m_rtspServer != 0 ) Medium::close(m_rtspServer); m_rtspServer = NULL;
	m_env->reclaim();
	delete m_scheduler;
	fclose(m_264);
}


void * CRtspServer::ThreadRtspServerProcImpl(void* arg)
{
	LOGD_print("CRtspServer", "Call CRtspServer::ThreadRtspServerProcImpl!");
	CRtspServer* cRtspServer = (CRtspServer*)arg;

	cRtspServer->ThreadRtspServer();
	
	return NULL;
}

void CRtspServer::ThreadRtspServer()
{
	LOGD_print("CRtspServer", "ThreadRtspServer() begin");
	
	UserAuthenticationDatabase* authDB = NULL;
#ifdef ACCESS_CONTROL
	authDB = new UserAuthenticationDatabase;
	authDB->addUserRecord("username1", "password1"); // replace these with real strings
#endif

	portNumBits rtspServerPortNum = m_port;
	m_rtspServer = DynamicRTSPServer::createNew(*m_env, rtspServerPortNum, authDB, this);
	if (m_rtspServer == NULL) 
	{
		LOGE_print("CRtspServer", "Failed to create RTSP server: %s rtspServerPortNum:%d", m_env->getResultMsg(), rtspServerPortNum);
		exit(1);
	}

	char* urlPrefix = m_rtspServer->rtspURLPrefix();
	LOGD_print("CRtspServer", "create RTSP server: %s<filename> ", urlPrefix);
	LOGD_print("CRtspServer", "create RTSP server: %s rtspServerPortNum:%d m_StreamName:%s m_watchVariable:%d", m_env->getResultMsg(), rtspServerPortNum, m_StreamName, m_watchVariable);
	m_env->taskScheduler().doEventLoop(&m_watchVariable); // does not return
	LOGD_print("CRtspServer", "doEventLoop done");
}


/*
int CRtspServer::Java_fireNotify(JNIEnv *env, int type, int arg1, int arg2, void* obj)
{
	COST_STAT_DECLARE(cost);
	JNI_ATTACH_JVM(env);
	env->CallStaticVoidMethod(g_jBQLService.thizClass, g_jBQLService.post, m_owner, type, arg1, arg2, obj);
	LOGD_print("com_bql_streamuser", "Notify RtspService[%p]: %d, %d, %d, %p, cost=%lld", m_owner, type, arg1, arg2, obj, cost.Get());
	JNI_DETACH_JVM(env);
	return 0;

}
*/

void CRtspServer::ParserUrl(const char* streamName, char* fileName, int maxLen)
{
	/*
	JNIEnv *env;
	JNI_ATTACH_JVM(env);
	
	jstring str = env->NewStringUTF(streamName);
	jstring msg = (jstring)env->CallStaticObjectMethod(g_jBQLService.thizClass, g_jBQLService.getFilePath, m_owner, str);
	char *path = NULL;
	JNI_GET_UTF_CHAR(path, msg);
	LOGD_print("CRtspServer", "CallStaticObjectMethod :%s", path);
	if (strlen(path) >= maxLen)
	{
		strncpy(fileName, path, strlen(maxLen));
	}
	else
	{
		strncpy(fileName, path, strlen(path));
	}
	JNI_RELEASE_STR_STR(path, msg);
	JNI_DETACH_JVM(env);
	*/
}

char* CRtspServer::LiveUrl()
{
	char* url = m_StreamName;
	return url;
}

bool CRtspServer::IsAudioEnable()
{
	return m_EnableAudio;
}


void CRtspServer::DemandIDR(void* arg, int streamId)
{
	if(pThis != NULL)
	{
		//pThis->Java_fireNotify(NULL, 0, 1, 0, NULL);
	}
}


