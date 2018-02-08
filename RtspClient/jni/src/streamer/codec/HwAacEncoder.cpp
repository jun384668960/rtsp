#include "HwAacEncoder.h"

CLASS_LOG_IMPLEMENT(CHwAacEncoder, "CEncoder[aac.hw]");
////////////////////////////////////////////////////////////////////////////////
CHwAacEncoder::CHwAacEncoder(AVCodecContext *audioctx, int fmt, CHwMediaEncoder *pEncode)
  : CMediaEncoder(audioctx), m_pEncode(pEncode)
{
	THIS_LOGT_print("is created. audio: channels=%d, sample_rate=%d, bitrate=%lld, fmt=%d", m_pCodecs->channels, m_pCodecs->sample_rate, m_pCodecs->bit_rate, m_pCodecs->sample_fmt);
	assert(m_pEncode != 0); 
}

CHwAacEncoder::~CHwAacEncoder()
{
	delete m_pEncode;
	THIS_LOGT_print("is deleted");
}

CBuffer* CHwAacEncoder::Encode(CBuffer *pSrcdata/*PCM*/)
{//pcm->aac
	if( m_pBufferManager == 0 ) m_pBufferManager = new CBufferManager(m_pCodecs->frame_size * m_pCodecs->channels * sizeof(short));
	AVPacket  *pSrcPacket = pSrcdata->GetPacket();
	THIS_LOGT_print("do encode[%p]: pts=%lld, data=%p, size=%d", pSrcdata, pSrcPacket->pts, pSrcPacket->data, pSrcPacket->size);
	CScopeBuffer pDstdata(m_pBufferManager->Pop(pSrcPacket->pts));
	AVPacket  *pDstPacket = pDstdata->GetPacket();

	pDstPacket->size = m_pEncode->Encode(pSrcPacket->data, pSrcPacket->size, pDstPacket->pts, pDstPacket->data);
	if( pDstPacket->size <= 2 ) return NULL; //skip csd while size=2

	pDstPacket->dts      = pDstPacket->pts;
	pDstPacket->pos 	 =-1;
	pDstPacket->duration = m_pCodecs->frame_size;
	return pDstdata.Detach();
}

CBuffer *CHwAacEncoder::GetDelayedFrame()
{
	if( m_pBufferManager == 0 ) return NULL;

	CScopeBuffer pDstdata(m_pBufferManager->Pop());
	AVPacket  *pDstPacket = pDstdata->GetPacket();

	pDstPacket->size = m_pEncode->Encode(NULL, 0, pDstPacket->pts, pDstPacket->data);
	if( pDstPacket->size <= 2 ) return NULL; //skip csd while size=2

	pDstPacket->dts      = pDstPacket->pts;
	pDstPacket->pos 	 =-1;
	pDstPacket->duration = m_pCodecs->frame_size;
	return pDstdata.Detach();
}
