#ifndef __HW_H264_DECODER_H__
#define __HW_H264_DECODER_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "media/NdkMediaCodec.h"
#ifdef __cplusplus
}
#endif

#include "logimp.h"
#include "HwMediaDecoder.h"

class CHw264Decoder: public CHwMediaDecoder
{
public:
	CHw264Decoder(AMediaCodec *pCodec, StreamDecodedataParameter *Parameter);
	virtual ~CHw264Decoder();

public:
	virtual int Decode(unsigned char *pktBuf, int pktLen, int64_t presentationTimeUs, unsigned char *outBuf);

public:
	ANativeWindow   *m_pWindow;
	AMediaCodec     *m_pCodec;
	CLASS_LOG_DECLARE(CHw264Decoder);
};

#endif
