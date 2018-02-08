#ifndef __MEDIARECORDER_H__
#define __MEDIARECORDER_H__

#include <list>
#include "codec/MediaDecoder.h"
#include "codec/MediaEncoder.h"
#include "SwsScale.h"

class CPlayer;

///////////////////////////////////////////////////////////////////
class CMediaRecorder : public CWriter, public CSemaphore
{
public:
	CMediaRecorder(CPlayer *pPlayer, int vcodecid, int acodecid)
	  : m_pPlayer(pPlayer), m_pScales(0), m_pThread(0), m_now(-1), m_ref(-1)
	{
		assert(vcodecid == 0 ||
			   vcodecid == AV_CODEC_ID_MJPEG || vcodecid == AV_CODEC_ID_H264);
		assert(acodecid == 0 ||
			   acodecid == AV_CODEC_ID_AAC );
		memset(m_pCodecs, 0, sizeof(m_pCodecs));
		memset(m_pDecode, 0, sizeof(m_pDecode));
		memset(m_pEncode, 0, sizeof(m_pEncode));
		memset(m_nframes, 0, sizeof(m_nframes));
	}
	virtual ~CMediaRecorder()
	{
		ReleaseBuffers( m_lstWaitEncodeFrames );
		if( m_pScales!= NULL ) delete m_pScales;
		for(int i = 0; i < 2; i ++) {//free m_pDecode/m_pEncode/m_pCodecs
			if( m_pDecode[i] ) delete m_pDecode[i];
			if( m_pEncode[i] ) delete m_pEncode[i];
			if( m_pCodecs[i] ) avcodec_free_context(&m_pCodecs[i]); //will call avcodec_close
		}
	}

public: //interface of CWriter
	virtual int Write(JNIEnv *env, void *buffer = 0);

public:
	int SetVideoStream(int fps, int videow, int videoh, int qval, int bitrate);
	int SetAudioStream(int channels, int depth, int samplerate, int bitrate);

	virtual int PrepareWrite();
	int GetRecordTime() { return m_now<0? (0):(m_now-m_ref);}
	int Stop();

public:
	void Doinit(JNIEnv *env, jobject obj);
	void Uninit(JNIEnv *env);

protected:
	static void *ThreadProc(void *pParam);
	void Loop(); //线程执行体
	void ReleaseBuffers(std::list<CBuffer*> &lstFrames)
	{
		for(std::list<CBuffer*>::iterator it = lstFrames.begin();
			it != lstFrames.end(); it ++)
		{
			CScopeBuffer frame(*it);
		}
	}

protected:
	virtual int Write(int type, AVPacket *pPacket) = 0;

protected:
	std::list<CBuffer *> m_lstWaitEncodeFrames;
	CMutex               m_mtxWaitEncodeFrames;	
	AVCodecContext  *m_pCodecs[4]; //decode: 0-video 1-audio encode: 2-video 3-audio
	CMediaDecoder   *m_pDecode[2]; 
	CMediaEncoder   *m_pEncode[2];
	int64_t   	 	 m_nframes[2];
	int64_t			 m_ref;
	int64_t 		 m_now;
	CPlayer         *m_pPlayer;
	CSwsScale	    *m_pScales;
	pthread_t 	 	 m_pThread; //线程	
	jobject m_owner;
	CLASS_LOG_DECLARE(CMediaRecorder);		
};

#endif
