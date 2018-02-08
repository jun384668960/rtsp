#include "Buffer.h"
#include "BufferManager.h"

CLASS_LOG_IMPLEMENT(CBuffer, "CBuffer");
///////////////////////////////////////////////////////////////////
void CBuffer::Release()
{
	{//check ref
		CScopeMutex lock(m_mutex);
		if( m_nRefs >= 2 ) 
		{
			m_nRefs --;
			return;
		}
	}

	if( m_pPacket->buf != NULL) { //恢复工作
	    m_pPacket->data = m_pPacket->buf->data; 
	    m_pPacket->size = m_pPacket->buf->size - AV_INPUT_BUFFER_PADDING_SIZE;
	} else {
	    m_pPacket->data = 0;
	    m_pPacket->size = 0;	
	}

	m_pBufferManager->Add(this);
}

void CBuffer::SetSize(int len)
{
	if( len == 0 ) return;

	if( m_pPacket->buf == NULL ||
		m_pPacket->buf->size < len + AV_INPUT_BUFFER_PADDING_SIZE ) 
	{
//		av_buffer_unref(&m_pPacket->buf);
//		m_pPacket->buf = NULL;
		int ret = av_buffer_realloc(&m_pPacket->buf, len + AV_INPUT_BUFFER_PADDING_SIZE );
		assert( ret>= 0 );
		memset( m_pPacket->buf->data + len, 0, AV_INPUT_BUFFER_PADDING_SIZE );
	}
	m_pPacket->data = m_pPacket->buf->data; //refix data/size
	m_pPacket->size	= len;
}
