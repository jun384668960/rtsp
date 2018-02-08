#ifndef HW_AUDIO_DECODER_H
#define HW_AUDIO_DECODER_H

#include   "MediaDecoder.h"
#include "HwMediaDecoder.h" //from libhwcodec

//硬解码: aac->PCM
////////////////////////////////////////////////////////////////////////////////
class CHwAacDecoder : public CMediaDecoder
{
public:
	CHwAacDecoder(AVCodecContext *videoctx, CHwMediaDecoder *pDecode);
	virtual ~CHwAacDecoder();

public:
	virtual CBuffer *Decode(CBuffer *pRawdata/*aac*/);

private:
	CHwMediaDecoder *m_pDecode;
	CLASS_LOG_DECLARE(CHwAacDecoder);	
};

#endif