#ifndef __MEDIADECODER_H__
#define __MEDIADECODER_H__

#include "buffer/BufferManager.h"

//解码成PCM/YUV格式
///////////////////////////////////////////////////////////////////
class CMediaDecoder 
{
protected: //通过CMediaDecoder::Create创建对象
	CMediaDecoder(AVCodecContext *pCodecs) { m_pCodecs = pCodecs; m_pBufferManager = 0;}
public:
	virtual ~CMediaDecoder()
	{
		if( m_pBufferManager ) m_pBufferManager->Release();
	}

public:
	//解码工厂
	static CMediaDecoder *Create(AVCodecContext *pCodecs, bool hwmode = true, void *param2 = 0);

public:
	virtual CBuffer *Decode(CBuffer *pRawdata/*XXX*/) = 0;

protected:
	CBufferManager *m_pBufferManager;	
	AVCodecContext *m_pCodecs; //引用: 解码参数上下文
};

#endif
