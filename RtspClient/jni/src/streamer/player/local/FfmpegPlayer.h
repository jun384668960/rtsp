#ifndef __FFMPEGPLAYER_H__
#define __FFMPEGPLAYER_H__

#include "player/Player.h"

//本地文件播放器: 通过ffmpeg播放本地文件
////////////////////////////////////////////////////////////////////////////////
class CFfmpegPlayer : public CPlayer
{
public:
	CFfmpegPlayer(const char *file);
	virtual ~CFfmpegPlayer();

public:
	virtual int Prepare();
	virtual int Play();
	virtual int Pause();
	virtual int Seek(int seek);
	virtual int Stop();

public:
	virtual int GetVideoConfig(int *fps, int *width, int *height);
	virtual int GetAudioConfig(int *channels, int *depth, int *samplerate);

protected:
	static void *ThreadProc(void *param);
	void Loop(); //线程执行体

protected:
	AVFormatContext *m_avfmtctx;
	pthread_t m_pThread; //线程
	int       m_astream;
	int       m_vstream;
	int       m_feof;
	JNIEnv   *m_pJNIEnv;
	CLASS_LOG_DECLARE(CFfmpegPlayer);	
};

#endif
