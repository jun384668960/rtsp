#ifndef __BUFFER_H__
#define __BUFFER_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#ifdef __cplusplus
}
#endif
#include "common.h"

class CBufferManager;

//内存对象类，目的: 解决数据被频繁拷贝[java/jni之间的数据, 只跟BufferManager相关]
///////////////////////////////////////////////////////////////////
class CBuffer
{
public:
	CBuffer(CBufferManager *pBufferManager, int size/*分配内存大小*/)
	  : m_pBufferManager(pBufferManager), m_codecid(0), m_nRefs(1)
	{
		m_pPacket = av_packet_alloc();
		if( size != 0 ) SetSize(size);
		THIS_LOGT_print("is created, data: %p, size: %d, owner: %p", m_pPacket->data, m_pPacket->size, m_pBufferManager);
	}
	virtual ~CBuffer()
	{
		av_packet_free(&m_pPacket);
		THIS_LOGT_print("is deleted");
	}

public:
	CBuffer *AddRef() { CScopeMutex lock(m_mutex); m_nRefs ++; return this;}
	void Release(); //调用者通知: 不再使用Buffer
	void SetSize(int len);
	unsigned int Skip(int i) { /*THIS_LOGT_print("skip Buffer[%p]: %d", this, i); */m_pPacket->data += i; m_pPacket->size -= i; return m_pPacket->size;}
	AVPacket    *GetPacket() const { return m_pPacket;}

public:
	int 		    m_codecid; //for recorder

protected:
	CBufferManager *m_pBufferManager; //引用
	AVPacket       *m_pPacket;
	CMutex          m_mutex;
	int 			m_nRefs;	
	CLASS_LOG_DECLARE(CBuffer);
};

///////////////////////////////////////////////////////////////////
class CScopeBuffer
{
public:
	CScopeBuffer(CBuffer *pBuffer = 0)
	  : p(pBuffer)
	{		
	}
	~CScopeBuffer()
	{
		Release();
	}

public:
	CBuffer *operator->() { return p;}
	void     Attach(CBuffer *pBuffer) { if( p ) p->Release(); p = pBuffer;}
	CBuffer *Detach()
	{
		CBuffer *pBuffer = p; p = NULL;
		return pBuffer;
	}
	void     Release()
	{
		if( p )
		{
			p->Release();
			p = NULL;
		}
	}

public:
	CBuffer *p;
};

#endif
