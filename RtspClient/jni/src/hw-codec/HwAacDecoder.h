#ifndef HW_AAC_DECODER_H
#define HW_AAC_DECODER_H

#ifdef __cplusplus
extern "C" {
#endif
#include "media/NdkMediaCodec.h"
#ifdef __cplusplus
}
#endif

#include "logimp.h"
#include "HwMediaDecoder.h"

class CHwAacDecoder: public CHwMediaDecoder
{
public:
	CHwAacDecoder(AMediaCodec *pCodec, StreamDecodedataParameter *Parameter);
	virtual ~CHwAacDecoder();

public:
	virtual int Decode(unsigned char *pktBuf, int pktLen, int64_t presentationTimeUs, unsigned char *outBuf);

public:
	AMediaCodec *m_pCodec;
	CLASS_LOG_DECLARE(CHwAacDecoder);
};

#endif