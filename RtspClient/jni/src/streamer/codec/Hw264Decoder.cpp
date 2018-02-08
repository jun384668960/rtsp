#include "Hw264Decoder.h"

CLASS_LOG_IMPLEMENT(CHw264Decoder, "CDecoder[264.hw]");
////////////////////////////////////////////////////////////////////////////////
CHw264Decoder::CHw264Decoder(AVCodecContext *videoctx, CHwMediaDecoder *pDecode)
  : CMediaDecoder(videoctx), m_pDecode(pDecode), m_pScales(0)
{
	THIS_LOGT_print("is created: fmt=%d", m_pCodecs->pix_fmt);
	assert(m_pDecode != 0);
	assert(m_pCodecs->pix_fmt == AV_PIX_FMT_YUV420P);	
}

CHw264Decoder::~CHw264Decoder()
{
	if( m_pScales != 0 ) delete m_pScales;
	delete m_pDecode;
	THIS_LOGT_print("is deleted");
}

CBuffer *CHw264Decoder::Decode(CBuffer *pRawdata)
{//h264->yuv[420p]
	COST_STAT_DECLARE(cost);

	if( m_pBufferManager == 0 ) m_pBufferManager = new CBufferManager(avpicture_get_size(m_pCodecs->pix_fmt, m_pCodecs->width, m_pCodecs->height));
	AVPacket  *pSrcPacket = pRawdata->GetPacket();
	THIS_LOGT_print("do decode[%p]: pts=%lld, data=%p, size=%d, nalu.type=%d", pRawdata, pSrcPacket->pts, pSrcPacket->data, pSrcPacket->size, (int)pSrcPacket->data[4] & 0x1f);
	CScopeBuffer pYuvdata(m_pBufferManager->Pop(pSrcPacket->pts));
	AVPacket  *pYuvPacket = pYuvdata->GetPacket();

	pYuvPacket->size = m_pDecode->Decode(pSrcPacket->data, pSrcPacket->size, pSrcPacket->pts, pYuvPacket->data);
	if( pYuvPacket->size ==-4 )
	{
		m_pBufferManager->Release();
		m_pCodecs->width = m_pDecode->m_videow; m_pCodecs->height = m_pDecode->m_videoh;
		m_pBufferManager = new CBufferManager(avpicture_get_size(m_pDecode->m_codecid, m_pDecode->m_param1, m_pDecode->m_param2));
		if( m_pScales != NULL ) { delete m_pScales; m_pScales = NULL;}
		return NULL;
	}
	if( pYuvPacket->size <= 0 ) return NULL;

	if( m_pDecode->m_codecid > 0 ||
		m_pCodecs->width != m_pDecode->m_param1 || m_pCodecs->height != m_pDecode->m_param2 )
	{
		if( m_pScales == NULL ) m_pScales = new CSwsScale(m_pDecode->m_param1, m_pDecode->m_param2, m_pDecode->m_codecid, m_pCodecs->width, m_pCodecs->height, m_pCodecs->pix_fmt);
		pYuvdata.Attach(m_pScales->Scale(pSrcPacket->pts, pYuvPacket->data));
		if( pYuvdata.p== NULL ) return NULL;
	}

	THIS_LOGT_print("got frame[%p]: data=%p, size=%d, cost=%lld", pYuvdata.p, pYuvdata->GetPacket()->data, pYuvdata->GetPacket()->size, cost.Get());
	return pYuvdata.Detach();
}
