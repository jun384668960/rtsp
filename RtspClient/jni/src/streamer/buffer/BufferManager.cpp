#include "BufferManager.h"

CLASS_LOG_IMPLEMENT(CBufferManager, "CBufferManager");
///////////////////////////////////////////////////////////////////
void CBufferManager::Add(CBuffer *pBuffer)
{
//	THIS_LOGT_print("add Buffer: %p, data=%p, size=%d", pBuffer, pPacket->data, pPacket->size);
	if( m_nSize[1] < m_nSize[0]) {
		m_mtxIdleBuffers.Enter();
		m_lstIdleBuffers.push_back(pBuffer);
		m_nSize[1] ++;
		pBuffer = 0;
		m_mtxIdleBuffers.Leave();
	}
	if( pBuffer ) delete pBuffer;
	Release();
}

CBuffer *CBufferManager::Pop(int64_t pts) //返回空闲Buffer
{
	CBuffer *pBuffer = NULL;
	AddRef(); //必须先保证CBufferManager有效	

	m_mtxIdleBuffers.Enter();
	if( m_lstIdleBuffers.empty() == false) {
		pBuffer = m_lstIdleBuffers.front();
		m_lstIdleBuffers.pop_front();
		m_nSize[1] --;
	}
	m_mtxIdleBuffers.Leave();
	if( pBuffer == NULL) 
	{
		pBuffer = new CBuffer(this, m_nMemSize);
	}
	else
	{
		pBuffer->SetSize(m_nMemSize);
	}

//	THIS_LOGD_print("pop Buffer: %p, ref: %d", pBuffer, m_nRef);
	pBuffer->GetPacket()->pts = pts;
	pBuffer->GetPacket()->dts = pts;
	return pBuffer;
}

CBufferManager *CBufferManager::AddRef()
{
	m_mtxIdleBuffers.Enter();
	unsigned int nRefs = ++ m_nRefs;
	m_mtxIdleBuffers.Leave();

	return this;
}

void CBufferManager::Release()
{
	m_mtxIdleBuffers.Enter();
	unsigned int nRefs = -- m_nRefs;
	m_mtxIdleBuffers.Leave();

	if( nRefs == 0 )
	{
		delete this;
	}
}
