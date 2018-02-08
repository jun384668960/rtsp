#ifndef __RTSPCLIENTPLAYER_H__
#define __RTSPCLIENTPLAYER_H__

#include <string>
#include <BasicUsageEnvironment.hh>
#include <liveMedia.hh>
#include "player/Player.h"

//rtsp播放器: 通过liblive完成处理
////////////////////////////////////////////////////////////////////////////////
class CRtspClientPlayer : public CPlayer
{
public:
	CRtspClientPlayer(const char *url);
	virtual ~CRtspClientPlayer();

public:
	virtual int Prepare();
	virtual int Play();
	virtual int Pause();
	virtual int Seek(int seek/*时间偏移，毫秒*/);
	virtual int Stop();

public:
	virtual int GetVideoConfig(int *fps, int *width, int *height);
	virtual int GetAudioConfig(int *channels, int *depth, int *samplerate);
	virtual int SetParamConfig(const char *key, const char *val);

protected:
	static void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString);
	static void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString);
	static void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString);
	static void continueAfterPAUSE(RTSPClient* rtspClient, int resultCode, char* resultString);	
	static void subsessionAfterPlaying(void* clientData);
	static void subsessionByeHandler(void* clientData);
	static void streamTimeoutHandler(void* clientData);
	static void setupNextSubsession(RTSPClient* rtspClient);

protected:
	static void *ThreadProc(void *param);
	void Loop(); //线程执行体

	void Shutdown(int exitCode);

public:
	pthread_t m_pThread; //线程	
	JNIEnv   *m_pJNIEnv;
	std::string   m_url;
	int       m_MEDIASINK_RECV_BUFFER_SIZE;
	int       m_CHKSTREAM_PERIOD;
	int       m_STREAMING_METHOD;
	int       m_VIDEOMFPS;
	int		  m_VIDEOFFPS;
	int		  m_DISABLERTCP;
	int 	  m_nCycles;
	UsageEnvironment  *m_pUsageEnvironment;	
	TaskScheduler	  *m_pTaskScheduler;	
	RTSPClient 		  *m_pRTSPClient;
	MediaSubsessionIterator *m_pMediaSubsessionIterator;
	MediaSession      *m_pMediaSession;
	MediaSubsession   *m_pMediaSubsession;
	TaskToken          m_pTimeDealTask;
	char m_bEventLoopWatchVariable;
	bool m_idr;
	CLASS_LOG_DECLARE(CRtspClientPlayer);	
};

#endif
