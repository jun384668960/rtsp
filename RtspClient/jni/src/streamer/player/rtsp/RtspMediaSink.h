#ifndef __RTSPMEDIASINK_H__
#define __RTSPMEDIASINK_H__

#include "RtspClientPlayer.h"

//实现接收数据帧处理
///////////////////////////////////////////////////////////////////
class CRtspMediaSink : public MediaSink
{
public:
	CRtspMediaSink(CRtspClientPlayer *pRtspClientPlayer, char const *streamId);
	virtual ~CRtspMediaSink()
	{
		THIS_LOGT_print("is deleted");		
	}

protected: //interface of MediaSink
	virtual Boolean continuePlaying();

protected:
	static void GotFrameProc(void* clientData, unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds);
	bool OnFrame(unsigned frameSize, struct timeval &presentationTime, unsigned durationInMicroseconds);

protected:
	CRtspClientPlayer *m_pPlayer;
	CScopeBuffer       m_pBuffer;
	unsigned int m_gop;
	int64_t      m_pts;
	CLASS_LOG_DECLARE(CRtspMediaSink);
};

#endif
