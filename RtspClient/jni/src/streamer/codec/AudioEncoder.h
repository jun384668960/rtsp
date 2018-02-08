#ifndef __AUDIO_ENCODER_H__
#define __AUDIO_ENCODER_H__

#include "MediaEncoder.h"

//软编码: PCM->XXX
///////////////////////////////////////////////////////////////////
class CAudioEncoder : public CMediaEncoder
{
public:
	CAudioEncoder(AVCodecContext *pCodecs, int fmt);
	virtual ~CAudioEncoder();

public:
	virtual CBuffer *Encode(CBuffer *pRawdata/*PCM*/);
	virtual CBuffer *GetDelayedFrame();

private:
	AVFrame *m_frame;
	uint64_t m_ref;
	CLASS_LOG_DECLARE(CAudioEncoder);	
};

#endif
