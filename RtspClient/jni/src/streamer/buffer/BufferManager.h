#ifndef __BUFFERMANAGER_H__
#define __BUFFERMANAGER_H__

#include <list>
#include "Buffer.h"

//管理空闲Buffer列表, 注意内存块长度都是相同的
//支持引用计数器
///////////////////////////////////////////////////////////////////
class CBufferManager
{
	friend class CBuffer;

public:
	CBufferManager(unsigned int nMemSize/*Buffer大小*/ = 0, unsigned int nBlkSize = 2)
	  : m_nMemSize(nMemSize), m_nRefs(1)
	{
		THIS_LOGT_print("is created, size: %d/%d", nMemSize, nBlkSize);
		m_nSize[0] = nBlkSize;
		m_nSize[1] = 0;
	}
protected: //注意只能通过调用Release释放对象
	virtual ~CBufferManager()
	{
		THIS_LOGT_print("del Buffer.size=%d", m_lstIdleBuffers.size());
		for (std::list<CBuffer*>::iterator it = m_lstIdleBuffers.begin();
			it != m_lstIdleBuffers.end(); it++)
		{
			CBuffer *pBuffer = *it;
			delete pBuffer;
		}
		THIS_LOGT_print("is deleted");
	}

public:
	CBuffer *Pop(int64_t pts = 0); //返回空闲Buffer

public: //引用计数器接口
	CBufferManager *AddRef();
	void Release();

protected: //call by CBuffer
	void Add(CBuffer *pBuffer); //加入空闲Buffer

protected:
	std::list<CBuffer*> m_lstIdleBuffers; //空闲Buffer列表: FIFO
	CMutex              m_mtxIdleBuffers;
	unsigned int m_nMemSize; //内存块大小，允许等于0
	unsigned int m_nSize[2]; //0-max 1-cur
	unsigned int m_nRefs;
	CLASS_LOG_DECLARE(CBufferManager);
};

#endif
