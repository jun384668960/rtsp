#ifndef _VIDEO_DECODER_H_
#define _VIDEO_DECODER_H_

#include "MediaDecoder.h"

//软解码: XXX->YUV
////////////////////////////////////////////////////////////////////////////////
class CVideoDecoder : public CMediaDecoder
{
public:
	CVideoDecoder(AVCodecContext *pCodecs);
	virtual ~CVideoDecoder();

public:
	virtual CBuffer *Decode(CBuffer *pRawdata/*XXX*/);

private:
	AVFrame *m_frame; //YUV数据
	CLASS_LOG_DECLARE(CVideoDecoder);
};

#endif