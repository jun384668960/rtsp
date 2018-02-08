#ifndef __VIDEODECODETHREAD_H__
#define __VIDEODECODETHREAD_H__

#include <list>
#include "codec/MediaDecoder.h"
#include "codec/MediaEncoder.h"
#include "SwsScale.h"

class CPlayer;

//视频解码线程
///////////////////////////////////////////////////////////////////
class CVideoDecodeThread : public CSemaphore
{
public:
	CVideoDecodeThread(CPlayer *pPlayer)
	  : m_pPlayer(pPlayer), m_pDecode(0), m_pThread(0), m_pJNIEnv(0), m_nFrames(0), m_pCodecs(0), m_p264Codecs(0), m_pEncode(0), m_pScales(0)
	{
		THIS_LOGT_print("is created, player: %p", m_pPlayer);
	}
	virtual ~CVideoDecodeThread()
	{
		assert(m_lstWaitDecodeFrames.empty() != false);
		if( m_pScales!=0 ) delete m_pScales;
		if( m_pEncode!=0 ) delete m_pEncode;
		if( m_p264Codecs ) avcodec_free_context(&m_p264Codecs); //will call avcodec_close		
		THIS_LOGT_print("is deleted");
	}

public:
	void Start(AVCodecContext *pCodecs, CBuffer *pNilframe);
	void Stop();

public:
	bool Push(CBuffer *pFrame);

	unsigned int GetFrameSize() const { return m_nFrames;}
	void DropFrames();
	CBuffer *Take(int64_t now, int adj, int &val);

protected:
	static void *ThreadProc(void *pParam);
	void Loop(); //线程执行体
	void Draw(JNIEnv *env, CBuffer *pSrcdata);

protected:
	std::list<CBuffer*> m_lstWaitDecodeFrames;
	int  m_nFrames; //m_lstWaitDecodeFrames.size
	CMutex              m_mtxWaitDecodeFrames;
	pthread_t       m_pThread;//线程	
	CPlayer	       *m_pPlayer;
	CMediaDecoder  *m_pDecode;
	JNIEnv         *m_pJNIEnv;
	AVCodecContext *m_pCodecs;
	CSwsScale	   *m_pScales;
	CMediaEncoder  *m_pEncode;
	AVCodecContext *m_p264Codecs;
	CLASS_LOG_DECLARE(CVideoDecodeThread);	
};

#endif
