#ifndef HW_264_DECODER_H
#define HW_264_DECODER_H

#include   "MediaDecoder.h"
#include "HwMediaDecoder.h" //from libhwcodec
#include "SwsScale.h"

//硬解码: h264->YUV[420p]
////////////////////////////////////////////////////////////////////////////////
class CHw264Decoder : public CMediaDecoder
{
public:
	CHw264Decoder(AVCodecContext *videoctx, CHwMediaDecoder *pDecode);
	virtual ~CHw264Decoder();

public:
	virtual CBuffer *Decode(CBuffer *pRawdata/*h264*/);

private:
	CHwMediaDecoder *m_pDecode;
	CSwsScale	    *m_pScales;	
	CLASS_LOG_DECLARE(CHw264Decoder);	
};

#endif