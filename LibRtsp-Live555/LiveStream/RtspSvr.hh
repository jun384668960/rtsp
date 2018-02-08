#ifndef __RTSP_SVR_HH__
#define __RTSP_SVR_HH__

#include <pthread.h>
#include "LiveStreamMediaSource.hh"
#include "BasicUsageEnvironment.hh"
//#include "service/Service.h"
#include "RTSPServer.hh"

class CRtspServer //: public CService
{
public:
	CRtspServer();
	virtual ~CRtspServer();
	virtual int PrepareStream(const char *pStreamName);	
	virtual int ReleaseStream();
	virtual int Start(int port);
	virtual int Stop();

	virtual bool PushVideo(unsigned char* data, unsigned int length, uint64_t pts);
	virtual bool PushAudio(unsigned char* data, unsigned int length, uint64_t pts);
	
	virtual int SetParamConfig(const char *key, const char *val);

	//virtual int Java_fireNotify(JNIEnv *env, int type, int arg1, int arg2, void* obj);

	void ParserUrl(const char* streamName, char* fileName, int maxLen);
	char* LiveUrl();
	bool IsAudioEnable();
	static CRtspServer * pThis;   //æ≤Ã¨∂‘œÛ÷∏’Î
	static void DemandIDR(void* arg, int streamId);
protected:
	static void* ThreadRtspServerProcImpl(void* arg);
	void ThreadRtspServer();
	
private:
	int m_port;
	char m_StreamName[128];
	bool 	m_Stop;
	LiveStreamMediaSource* m_Input;
	char m_watchVariable;
	TaskScheduler* m_scheduler;
    UsageEnvironment* m_env;
    RTSPServer* m_rtspServer;
    pthread_t m_pThread;
    int m_pushVCnt;
    int m_pushACnt;
    bool m_EnableAudio;
    FILE* m_264;
    //
};

#define LOGI_print(n, t, ...) fprintf(stderr,    n" "t"\n", ##__VA_ARGS__)
#define LOGD_print(n, t, ...) fprintf(stderr,    n" "t"\n", ##__VA_ARGS__)
#define LOGW_print(n, t, ...) fprintf(stderr,    n" "t"\n", ##__VA_ARGS__)
#define LOGE_print(n, t, ...) fprintf(stderr,    n" "t"\n", ##__VA_ARGS__)

#endif
