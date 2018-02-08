#ifndef HW_264_ENCODER_H
#define HW_264_ENCODER_H

#include   "MediaEncoder.h"
#include "HwMediaEncoder.h" //from libhwcodec
#include "SwsScale.h"

//硬编码: YUV->h264
////////////////////////////////////////////////////////////////////////////////
class CHw264Encoder : public CMediaEncoder
{
public:
	CHw264Encoder(AVCodecContext *videoctx, int fmt, CHwMediaEncoder *pEncode);
	virtual ~CHw264Encoder();

public:
	virtual CBuffer *Encode(CBuffer *pRawdata/*YUV*/);
	virtual CBuffer *GetDelayedFrame();

private:
	CHwMediaEncoder *m_pEncode;
	CSwsScale	    *m_pScales;	
	CLASS_LOG_DECLARE(CHw264Encoder);
};

#endif
