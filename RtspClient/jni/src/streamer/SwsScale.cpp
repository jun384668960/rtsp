#include "SwsScale.h"

CLASS_LOG_IMPLEMENT(CSwsScale, "CSwsScale");
////////////////////////////////////////////////////////////////////////////////
CBuffer *CSwsScale::Scale(uint64_t pts, unsigned char *data)
{
	AVPicture avpic;
	avpicture_fill(&avpic, data, m_srcFormat, m_srcW, m_srcH );
	return Scale( pts, avpic.data, avpic.linesize );
}

CBuffer *CSwsScale::Scale(uint64_t pts, unsigned char **data, int size)
{
	COST_STAT_DECLARE(cost);
	int ret = sws_scale( m_pSwsctxs, data, size, 0, m_srcH, m_avpic.data, m_avpic.linesize ); 
	if( ret < 0 )
	{
		THIS_LOGE_print("call sws_scale return %d", ret);
		return NULL;
	}
	else
	{
		CScopeBuffer pDstdata(m_pBufferManager->Pop(pts));
		int ret = avpicture_layout(&m_avpic, m_dstFormat, m_dstW, m_dstH, pDstdata->GetPacket()->data, pDstdata->GetPacket()->size );		
		THIS_LOGD_print("scale: %dX%d[%d]->%dX%d[%d], data=%p, size=%d, cost=%lld", m_srcW, m_srcH, m_srcFormat, m_dstW, m_dstH, m_dstFormat, pDstdata->GetPacket()->data, pDstdata->GetPacket()->size, cost.Get());
		return pDstdata.Detach();
	}
}
