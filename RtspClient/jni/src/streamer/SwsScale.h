#ifndef __SWSSCALE_H__
#define __SWSSCALE_H__

#include "buffer/BufferManager.h"

//图像转换
////////////////////////////////////////////////////////////////////////////////
class CSwsScale
{
public:
	CSwsScale(int srcW, int srcH, AVPixelFormat srcFormat, int dstW, int dstH, AVPixelFormat dstFormat, int flags = SWS_FAST_BILINEAR)
	  : m_srcW(srcW), m_srcH(srcH), m_dstW(dstW), m_dstH(dstH), m_srcFormat(srcFormat), m_dstFormat(dstFormat)
	{
		m_pSwsctxs =  sws_getContext(srcW, srcH, srcFormat, dstW, dstH, dstFormat, flags, 0, 0, 0);	
		int len = avpicture_get_size(dstFormat, dstW, dstH);
		m_pBufferManager = new CBufferManager(len);
		m_avbuf = av_malloc(len);
		avpicture_fill(&m_avpic, m_avbuf, m_dstFormat, m_dstW, m_dstH );	
	}
	~CSwsScale()
	{
		m_pBufferManager->Release();
		if( m_pSwsctxs != 0 ) sws_freeContext(m_pSwsctxs);
		av_free(m_avbuf);
	}

public:
	CBuffer *Scale(uint64_t pts, unsigned char  *data);
	CBuffer *Scale(uint64_t pts, unsigned char **data, int size);

protected:
	CBufferManager    *m_pBufferManager;
	struct SwsContext *m_pSwsctxs;
	AVPicture          m_avpic;
	uint8_t			  *m_avbuf;
public:
	int m_srcW, m_srcH;
	int m_dstW, m_dstH;
	AVPixelFormat m_srcFormat, m_dstFormat;
	CLASS_LOG_DECLARE(CSwsScale);	
};

#endif //__SWSSCALE_H__
