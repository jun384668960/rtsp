#ifndef __MEDIAENCODER_H__
#define __MEDIAENCODER_H__

#include "buffer/BufferManager.h"

//编码PCM/YUV[xxx]成指定格式
///////////////////////////////////////////////////////////////////
class CMediaEncoder 
{
protected: //通过CMediaEncoder::Create创建对象
	CMediaEncoder(AVCodecContext *pCodecs) { m_pCodecs = pCodecs; m_pBufferManager = 0;}
public:
	virtual ~CMediaEncoder()
	{
		if( m_pBufferManager ) m_pBufferManager->Release();
	}

public:
	//编码工厂
	static CMediaEncoder *Create(AVCodecContext *pCodecs, int fmt = -1);

public:
	virtual CBuffer *Encode(CBuffer *pRawdata/*PCM/YUV[xxx]*/) = 0;
	virtual CBuffer *GetDelayedFrame() = 0;

protected:
	CBufferManager *m_pBufferManager;	
	AVCodecContext *m_pCodecs; //引用: 编码参数上下文
};

#endif