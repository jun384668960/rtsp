#ifndef HW_AAC_ENCODE_H
#define HW_AAC_ENCODE_H

#ifdef __cplusplus
extern "C" {
#endif
#include "media/NdkMediaCodec.h"
#ifdef __cplusplus
}
#endif

#include "logimp.h"
#include "HwMediaEncoder.h" 

class CHwAacEncoder: public CHwMediaEncoder
{
public:
	CHwAacEncoder(AMediaCodec *pCodec, StreamEncodedataParameter *Parameter);
	virtual ~CHwAacEncoder();

public:
	virtual int Encode(unsigned char *pktBuf, int pktLen, int64_t &presentationTimeUs, unsigned char *outdata);

public:
	AMediaCodec *m_pCodec;
	CLASS_LOG_DECLARE(CHwAacEncoder);
};

#endif
