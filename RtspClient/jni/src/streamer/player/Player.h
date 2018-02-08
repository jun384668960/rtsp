#ifndef __PLAYER_H__
#define __PLAYER_H__

#include <string>
#include "render/AudioTrack.h"
#include "buffer/BufferManager.h"
#include "thread/VideoDecodeThread.h"
#include "thread/AudioDecodeThread.h"

enum PLAYSTATUS
{
	PS_Prepare,
	PS_Bufferinga, //正在缓冲，等待音频帧: 纯音频的情况
	PS_Buffering1, // 已经SETUP 但还没有PLAY
	PS_Buffering2, // 已经PLAY 但还没有收到第一帧数据
	PS_Bufferingv, //正在缓冲，等待视频帧: IDR
	PS_Restart,
	PS_Playing,    //正在播放
	PS_Pause1,     //开始暂停
	PS_Pause2,
	PS_Paused,	   //暂停	
	PS_Stopped     //停止
};

//播放器基础类: 提供Prepare/Play/Pause/Seek/Stop接口、获取播放状态接口
////////////////////////////////////////////////////////////////////////////////
class CPlayer : public JNIStreamerContext
{
public:
	CPlayer();
	virtual ~CPlayer()
	{
		if( m_pBufferManager ) m_pBufferManager->Release();	
		assert(m_nStatus == PS_Stopped);
	}

public: //播放接口，返回值=0: 表示成功，其他值表示失败
	virtual int Prepare() = 0;
	virtual int Play() = 0; //播放
	virtual int Pause() = 0; //暂停
	virtual int Seek(int seek/*时间偏移，毫秒*/) = 0; 
	virtual int Stop() = 0; //停止

public:
	virtual int GetVideoConfig(int *fps, int *width, int *height) = 0; //获取视频宽/高
	virtual int GetAudioConfig(int *channels, int *depth, int *samplerate) = 0;
	virtual int SetParamConfig(const char *key, const char *val) { return 1;}

public:
	virtual int OnUpdatePosition(int position)
	{
		if( m_duration ) {
			if( m_duration < position ) m_duration = position; //refix m_duration
			if( m_position < position ) m_position = position;
		}
		return 0;
	}

public:
	int GetDuration() const { return m_duration;}
	int GetPosition() const { return m_position;}

	int GetPlayBufferTime() const { return m_nBufferInterval;}
	int SetPlayBufferTime(int time) { if( time < 0 ) return 1; m_nBufferInterval = time; return 0;}

	int GetAudioFrameSize()
	{
		if( m_pAudioDecodeThread == 0 )
			return 0;
		else
			return m_pAudioDecodeThread->GetFrameSize();
	}
	int GetVideoFrameSize()
	{
		if( m_pVideoDecodeThread == 0 )
			return 0;
		else
			return m_pVideoDecodeThread->GetFrameSize();
	}

	int Subscribe(int codecid, bool onoff)
	{
		assert(codecid != 0);
		if( m_hwmode == 3 ) 
			return 0; //����������
		else
			return SetCodecidEnabled(0, codecid, onoff);
	}

	CBuffer *GetVideoSpspps(int idx, CBuffer *frame);
	int SetVideoSpspps(int idx, int nadd, unsigned char *data, int size)
	{
		if( idx == 0 )
		{
			m_videoSps.resize(nadd + size);
			if( nadd ) memcpy(m_videoSps.c_str(), "\x00\x00\x00\x01", 4);
			memcpy(m_videoSps.c_str() + nadd, data, size);
		}
		else
		{
			m_videoPps.resize(nadd + size);
			if( nadd ) memcpy(m_videoPps.c_str(), "\x00\x00\x00\x01", 4);
			memcpy(m_videoPps.c_str() + nadd, data, size);
		}
	}

	int SetSurfaceWindows(JNIEnv *env, ANativeWindow *pSurface) { if( m_pSurfaceWindow ) return 1; m_pSurfaceWindow = pSurface; return 0;}
	int SetVolume(JNIEnv *env, jfloat a, jfloat b) { return m_pAudioTrack->SetVolume(env, a, b);}
	int SetMute(JNIEnv *env, jboolean bMute) { m_bIsMute = bMute? 1:0; return 0;}
	int Snapshot(JNIEnv *env, jint w, jint h, char *file);

	int GetStatus() const { return m_nStatus;} //获取播放状态
	int SetStatus(int st) { m_nStatus = st; return m_nStatus;} //设置播放状态							
	int SetStatus(int st, int64_t pts, int adj = 100) { m_clock.Set(pts, adj); LOGI_print("player", "update clock, pts=%lld, ref=%lld", m_clock.pts, m_clock.ref);	return SetStatus(st);} //设置播放状态

	void Doinit(JNIEnv *env, jobject obj);
	void Uninit(JNIEnv *env);

public:
	CVideoDecodeThread *m_pVideoDecodeThread;
	CAudioDecodeThread *m_pAudioDecodeThread;
	AVCodecContext *m_videoctx;
	AVCodecContext *m_audioctx;	
	int   		    m_duration;	
	volatile int    m_position; //单位: 毫秒，播放位置
	int 	        m_nSeekpos; //单位: 毫秒, -1表示没有设置开始播放位置
	std::string     m_videoSps;
	std::string     m_videoPps;
	CBufferManager *m_pBufferManager;
	ReferTimeScale  m_clock;
	int  m_isFirst;	
	int  m_bIsMute;
	int  m_nStatus; //播放状态	
	int  m_nBufferInterval;
};

#endif
