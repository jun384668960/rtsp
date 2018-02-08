#ifndef __RTMPCLIENTPLAYER_H__
#define __RTMPCLIENTPLAYER_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <rtmp.h>
#ifdef __cplusplus
}
#endif
#include <string>
#include "player/Player.h"

//rtmp播放器: 通过librtmp完成处理
////////////////////////////////////////////////////////////////////////////////
class CRtmpClientPlayer : public CPlayer
{
public:
	CRtmpClientPlayer(const char *uri);
	virtual ~CRtmpClientPlayer();

public:
	virtual int Prepare();
	virtual int Play();
	virtual int Pause();
	virtual int Seek(int seek);
	virtual int Stop();

public:
	virtual int GetVideoConfig(int *fps, int *width, int *height);
	virtual int GetAudioConfig(int *channels, int *depth, int *samplerate);
	virtual int SetParamConfig(const char *key, const char *val);

protected:
	static void *ThreadProc(void *param);
	void Loop(); //线程执行体

protected:
	pthread_t m_pThread; //线程
	JNIEnv   *m_pJNIEnv;
	RTMP	  m_rtmpctx;
	int       m_MEDIASINK_RECV_BUFFER_SIZE;	
	int       m_VIDEOMFPS;	
	std::string   m_uri;
	bool m_feof;
	CLASS_LOG_DECLARE(CRtmpClientPlayer);	
};

#endif
