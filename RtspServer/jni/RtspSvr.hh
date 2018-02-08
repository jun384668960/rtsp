#ifndef __RTSP_SVR_HH__
#define __RTSP_SVR_HH__

#include <pthread.h>
#include "LiveStreamMediaSource.hh"
#include "BasicUsageEnvironment.hh"
#include "RTSPServer.hh"

class CRtspServer
{
public:
	static CRtspServer* GetInstance();

	bool Create();
	bool Destory();
	bool Start();
	bool Stop();
	bool Restart();

	bool LiveSourcePush(char* data, long len, long pts, int flag);
	bool LiveSourceSync(int flag);
	void RegisterSeekFunc();
	
protected:
	CRtspServer();
	~CRtspServer();
	
	static void* ThreadRtspServerProcImpl(void* arg);
	void ThreadRtspServer();
	
private:
	static  CRtspServer* instance;
	bool 	m_Stop;
	LiveStreamMediaSource* m_Input;
	char m_watchVariable;
	TaskScheduler* m_scheduler;
    UsageEnvironment* m_env;
    RTSPServer* m_rtspServer;
    pthread_t m_pThread;
};

#endif
