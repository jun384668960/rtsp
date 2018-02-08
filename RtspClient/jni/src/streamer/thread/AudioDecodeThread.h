#ifndef __AUDIODECODETHREAD_H__
#define __AUDIODECODETHREAD_H__

#include <list>
#include "codec/MediaDecoder.h"

class CPlayer;

//音频解码线程
///////////////////////////////////////////////////////////////////
class CAudioDecodeThread : public CSemaphore
{
public:
	CAudioDecodeThread(CPlayer *pPlayer)
	  : m_pPlayer(pPlayer), m_pDecode(0), m_pThread(0), m_pJNIEnv(0), m_nFrames(0), m_pCodecs(0)
	{
		THIS_LOGT_print("is created, player: %p", m_pPlayer);
	}
	virtual ~CAudioDecodeThread()
	{
		assert(m_lstWaitDecodeFrames.empty() != false);
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
	void Play(JNIEnv *env, CBuffer *pSrcdata);

protected:
	std::list<CBuffer*> m_lstWaitDecodeFrames;
	int  m_nFrames; //m_lstWaitDecodeFrames.size
	CMutex              m_mtxWaitDecodeFrames;	
	pthread_t       m_pThread;//线程	
	CPlayer	       *m_pPlayer;
	CMediaDecoder  *m_pDecode;
	JNIEnv         *m_pJNIEnv;
	AVCodecContext *m_pCodecs;
	CLASS_LOG_DECLARE(CAudioDecodeThread);	
};

#endif
