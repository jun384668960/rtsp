#ifndef HW_H264_ENCODE_H
#define HW_H264_ENCODE_H

#ifdef __cplusplus
extern "C" {
#endif
#include "media/NdkMediaCodec.h"
#ifdef __cplusplus
}
#endif

#include "logimp.h"
#include "HwMediaEncoder.h" 

class CHw264Encoder: public CHwMediaEncoder
{
public:
	CHw264Encoder(AMediaCodec *pCodec, StreamEncodedataParameter *Parameter);
	virtual ~CHw264Encoder();

public:
	virtual int Encode(unsigned char *pktBuf, int pktLen, int64_t &presentationTimeUs, unsigned char *outBuf);

public:
	AMediaCodec *m_pCodec;
	bool m_bHasCSD; //sps/pps
	int64_t  m_ref;
	int64_t  m_adj;
	CLASS_LOG_DECLARE(CHw264Encoder);
};

#endif
