#ifndef HW_AUDIO_ENCODER_H
#define HW_AUDIO_ENCODER_H

#include   "MediaEncoder.h"
#include "HwMediaEncoder.h" //from libhwcodec

//硬编码: PCM->aac
////////////////////////////////////////////////////////////////////////////////
class CHwAacEncoder: public CMediaEncoder
{
public:
	CHwAacEncoder(AVCodecContext *audioctx, int fmt, CHwMediaEncoder *pEncode);
	virtual ~CHwAacEncoder();

public:
	virtual CBuffer *Encode(CBuffer *pRawdata/*PCM*/);
	virtual CBuffer *GetDelayedFrame();

private:
	CHwMediaEncoder *m_pEncode;
	CLASS_LOG_DECLARE(CHwAacEncoder);
};

#endif