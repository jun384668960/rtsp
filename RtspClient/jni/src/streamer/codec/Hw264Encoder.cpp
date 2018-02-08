#include "Hw264Encoder.h"

CLASS_LOG_IMPLEMENT(CHw264Encoder, "CEncoder[264.hw]");
////////////////////////////////////////////////////////////////////////////////
CHw264Encoder::CHw264Encoder(AVCodecContext *videoctx, int fmt, CHwMediaEncoder *pEncode)
  : CMediaEncoder(videoctx), m_pEncode(pEncode), m_pScales(0)
{
	THIS_LOGT_print("is created. video(%dX%d): bitrate=%lld, val=%d, gop=%d", m_pCodecs->width, m_pCodecs->height, m_pCodecs->bit_rate, m_pCodecs->qmax, m_pCodecs->gop_size);
	assert(m_pEncode != 0);	
	assert(m_pCodecs->pix_fmt == AV_PIX_FMT_YUV420P);
	if( fmt < AV_PIX_FMT_YUV420P ) fmt = AV_PIX_FMT_YUV420P;	
	if( fmt!=m_pEncode->m_codecid) m_pScales = new CSwsScale(m_pCodecs->width, m_pCodecs->height, fmt, m_pCodecs->width, m_pCodecs->height, m_pEncode->m_codecid);	
}

CHw264Encoder::~CHw264Encoder()
{
	if( m_pScales != 0 ) delete m_pScales;
	delete m_pEncode;
	THIS_LOGT_print("is deleted");	
}

CBuffer *CHw264Encoder::Encode(CBuffer *pRawdata/*YUV*/)
{//yuv[xxx]->h264
	CScopeBuffer pSrcdata(pRawdata->AddRef());

	if( m_pBufferManager == 0 ) m_pBufferManager = new CBufferManager(512*1024); //512K
	THIS_LOGT_print("do encode[%p]: pts=%lld, data=%p, size=%d", pSrcdata.p, pSrcdata->GetPacket()->pts, pSrcdata->GetPacket()->data, pSrcdata->GetPacket()->size);
	if( m_pScales != 0 ) pSrcdata.Attach(m_pScales->Scale(pSrcdata->GetPacket()->pts, pSrcdata->GetPacket()->data));
	if( pSrcdata.p== 0 ) return NULL;

	CScopeBuffer pDstdata(m_pBufferManager->Pop(pRawdata->GetPacket()->pts));
	AVPacket  *pDstPacket = pDstdata->GetPacket();
	AVPacket  *pSrcPacket = pSrcdata->GetPacket();
	pDstPacket->size = m_pEncode->Encode(pSrcPacket->data, pSrcPacket->size, pDstPacket->pts, pDstPacket->data);
	if( pDstPacket->size <= 0 ) return NULL;

	pDstPacket->dts      = pDstPacket->pts;
	pDstPacket->pos 	 =-1;
	pDstPacket->duration = 1;
	return pDstdata.Detach();
}

CBuffer *CHw264Encoder::GetDelayedFrame()
{
	if( m_pBufferManager == 0 ) return NULL;

	CScopeBuffer pDstdata(m_pBufferManager->Pop());
	AVPacket  *pDstPacket = pDstdata->GetPacket();

	pDstPacket->size = m_pEncode->Encode(NULL, 0, pDstPacket->pts, pDstPacket->data);
	if( pDstPacket->size <= 0 ) return NULL;

	pDstPacket->dts      = pDstPacket->pts;	
	pDstPacket->pos 	 =-1;
	pDstPacket->duration = 1;
	return pDstdata.Detach();
}
