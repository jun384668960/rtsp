#ifndef __VIDEO_ENCODER_H__
#define __VIDEO_ENCODER_H__

#include "MediaEncoder.h"
#include "SwsScale.h"

//软编码: YUV[xxx]->xxx
///////////////////////////////////////////////////////////////////
class CVideoEncoder : public CMediaEncoder
{
public:
	CVideoEncoder(AVCodecContext *pCodecs, int fmt);
	virtual ~CVideoEncoder();

public:
	virtual CBuffer *Encode(CBuffer *pRawdata/*YUV[xxx]*/);
	virtual CBuffer *GetDelayedFrame();

private:
	CSwsScale *m_pScales;
	AVFrame   *m_frame; //YUV数据	
	CLASS_LOG_DECLARE(CVideoEncoder);	
};

#endif
