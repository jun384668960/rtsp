#include "VideoEncoder.h"

CLASS_LOG_IMPLEMENT(CVideoEncoder, "CEncoder[video]");
////////////////////////////////////////////////////////////////////////////////
CVideoEncoder::CVideoEncoder(AVCodecContext *pCodecs, int fmt)
  : CMediaEncoder(pCodecs), m_pScales(0)
{
	THIS_LOGT_print("is created. video(%dX%d): bitrate=%lld, val=%d, gop=%d, fmt=%d", m_pCodecs->width, m_pCodecs->height, m_pCodecs->bit_rate, m_pCodecs->qmax, m_pCodecs->gop_size, m_pCodecs->pix_fmt);
	m_frame = av_frame_alloc();
	if( fmt < AV_PIX_FMT_YUV420P ) fmt = AV_PIX_FMT_YUV420P;
	if( fmt!= m_pCodecs->pix_fmt ) m_pScales = new CSwsScale(m_pCodecs->width, m_pCodecs->height, fmt, m_pCodecs->width, m_pCodecs->height, m_pCodecs->pix_fmt);
}

CVideoEncoder::~CVideoEncoder()
{
	if( m_pScales != 0 ) delete m_pScales;
	av_frame_free(&m_frame);
	THIS_LOGT_print("is deleted.");
}

CBuffer *CVideoEncoder::Encode(CBuffer *pRawdata/*YUV[420p]*/)
{//yuv[420p]->xxx
	COST_STAT_DECLARE(cost);

	do{
	CScopeBuffer pSrcdata(pRawdata->AddRef());
	THIS_LOGT_print("do encode[%p]: pts=%lld, data=%p, size=%d", pRawdata, pSrcdata->GetPacket()->pts, pSrcdata->GetPacket()->data, pSrcdata->GetPacket()->size);
	if( m_pScales != 0 ) pSrcdata.Attach(m_pScales->Scale(pSrcdata->GetPacket()->pts, pSrcdata->GetPacket()->data));
	if( pSrcdata.p== 0 ) return NULL;

	if( m_pBufferManager == 0 ) m_pBufferManager = new CBufferManager(0);
	CScopeBuffer pDstdata(m_pBufferManager->Pop());
	AVPacket  *pDstPacket = pDstdata->GetPacket();
	AVPacket  *pSrcPacket = pSrcdata->GetPacket();
	avpicture_fill((AVPicture*)m_frame, pSrcPacket->data, m_pCodecs->pix_fmt, m_pCodecs->width, m_pCodecs->height);
	m_frame->pts = pSrcPacket->pts; //fix pts

	int got = 0;
	int ret = avcodec_encode_video2(m_pCodecs, pDstPacket, m_frame, &got);
	if( ret < 0 )
	{
		THIS_LOGE_print("call avcodec_encode_video2 return %d", ret);
		break;
	}
	if( got > 0 )
	{
		if( m_pCodecs->coded_frame->key_frame )	pDstPacket->flags |= AV_PKT_FLAG_KEY;
		pDstPacket->pos 	 =-1;
		pDstPacket->duration = 1;
		#ifdef _DEBUG
		if( m_pCodecs->codec_id == AV_CODEC_ID_H264 )
		THIS_LOGD_print("got frame[%p]: pts=%lld, data=%p, size=%d, nalu.type=%d, cost=%lld", pDstdata.p, pDstPacket->pts, pDstPacket->data, pDstPacket->size, (int)pDstPacket->data[4] & 0x1f, cost.Get());
		else
		THIS_LOGD_print("got frame[%p]: pts=%lld, data=%p, size=%d, cost=%lld", pDstdata.p, pDstPacket->pts, pDstPacket->data, pDstPacket->size, cost.Get());
		#endif
		return pDstdata.Detach();
	}
	}while(0);
	return NULL;
}

CBuffer *CVideoEncoder::GetDelayedFrame()
{
	if( m_pBufferManager == 0 ) return NULL;

	COST_STAT_DECLARE(cost);
	CScopeBuffer pDstdata(m_pBufferManager->Pop());
	AVPacket  *pDstPacket = pDstdata->GetPacket( );

	do{
	int got = 0;
	int ret = avcodec_encode_video2(m_pCodecs, pDstPacket, NULL, &got);
	if( ret < 0 )
	{
		THIS_LOGE_print("call avcodec_encode_video2 return %d", ret);
		break;
	}
	if( got > 0 )
	{
		if( m_pCodecs->coded_frame->key_frame ) pDstPacket->flags |= AV_PKT_FLAG_KEY;
		pDstPacket->pos      =-1;
		pDstPacket->duration = 1;
		#ifdef _DEBUG
		if( m_pCodecs->codec_id == AV_CODEC_ID_H264 )
		THIS_LOGD_print("got frame[%p]: pts=%lld, data=%p, size=%d, nalu.type=%d, cost=%lld", pDstdata.p, pDstPacket->pts, pDstPacket->data, pDstPacket->size, (int)pDstPacket->data[4] & 0x1f, cost.Get());
		else
		THIS_LOGD_print("got frame[%p]: pts=%lld, data=%p, size=%d, cost=%lld", pDstdata.p, pDstPacket->pts, pDstPacket->data, pDstPacket->size, cost.Get());
		#endif
		return pDstdata.Detach();
	}
	} while(0);

	return NULL;
}
