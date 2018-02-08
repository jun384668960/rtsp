#ifndef __AUDIO_DECODE_H__
#define __AUDIO_DECODE_H__

#include "MediaDecoder.h"

//软解码：XXX->PCM
////////////////////////////////////////////////////////////////////////////////
class CAudioDecoder : public CMediaDecoder
{
public:
	CAudioDecoder(AVCodecContext *pCodecs);
	virtual ~CAudioDecoder();

public:
	virtual CBuffer *Decode(CBuffer *pRawdata/*XXX*/);

private:
	AVFrame *m_frame;
	CLASS_LOG_DECLARE(CAudioDecoder);
};

#endif
