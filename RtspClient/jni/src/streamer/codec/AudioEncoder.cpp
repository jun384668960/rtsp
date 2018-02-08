#include "AudioEncoder.h"

CLASS_LOG_IMPLEMENT(CAudioEncoder, "CEncoder[audio]");
////////////////////////////////////////////////////////////////////////////////
CAudioEncoder::CAudioEncoder(AVCodecContext *pCodecs, int fmt)
  : CMediaEncoder(pCodecs), m_ref(-1)
{
	THIS_LOGT_print("is created. audio: channels=%d, sample_rate=%d, bitrate=%lld, fmt=%d", m_pCodecs->channels, m_pCodecs->sample_rate, m_pCodecs->bit_rate, m_pCodecs->sample_fmt);
	m_frame = av_frame_alloc();
}

CAudioEncoder::~CAudioEncoder()
{
	av_frame_free(&m_frame);
	THIS_LOGT_print("is deleted.");
}

CBuffer *CAudioEncoder::Encode(CBuffer *pSrcdata/*PCM*/)
{//pcm->xxx
	COST_STAT_DECLARE(cost);

	if( m_pBufferManager == 0 ) m_pBufferManager = new CBufferManager(0);
	CScopeBuffer pDstdata(m_pBufferManager->Pop());
	AVPacket  *pDstPacket = pDstdata->GetPacket();
	AVPacket  *pSrcPacket = pSrcdata->GetPacket();

	do{
	THIS_LOGT_print("do encode[%p]: pts=%lld, data=%p, size=%d", pSrcdata, pSrcPacket->pts, pSrcPacket->data, pSrcPacket->size);
	m_frame->nb_samples = m_pCodecs->frame_size;
	m_frame->format     = m_pCodecs->sample_fmt;
	avcodec_fill_audio_frame(m_frame, m_pCodecs->channels, m_pCodecs->sample_fmt, pSrcPacket->data, pSrcPacket->size, 0); 
	if( m_ref == -1 ) m_ref = pSrcPacket->pts;
	m_frame->pts = pSrcPacket->pts;

	int got = 0;
	int ret = avcodec_encode_audio2(m_pCodecs, pDstPacket, m_frame, &got);
	if( ret < 0 )
	{
		THIS_LOGE_print("call avcodec_encode_audio2 return %d", ret);
		break;
	}
	if( got > 0 &&
		pDstPacket->pts >= m_ref )
	{
		pDstPacket->pos 	 =-1;
		pDstPacket->duration = m_pCodecs->frame_size;
		THIS_LOGD_print("got frame[%p]: pts=%lld, data=%p, size=%d, cost=%lld", pDstdata.p, pDstPacket->pts, pDstPacket->data, pDstPacket->size, cost.Get());			
		return pDstdata.Detach();
	}
	}while(0);

	return NULL;
}

CBuffer *CAudioEncoder::GetDelayedFrame()
{
	if( m_pBufferManager == 0 ||
		m_ref < 0 ) return NULL;

	COST_STAT_DECLARE(cost);
	CScopeBuffer pDstdata(m_pBufferManager->Pop());
	AVPacket  *pDstPacket = pDstdata->GetPacket( );

	do{
	int got = 0;
	int ret = avcodec_encode_audio2(m_pCodecs, pDstPacket, NULL, &got);
	if( ret < 0 )
	{
		THIS_LOGE_print("call avcodec_encode_audio2 return %d", ret);
		break;
	}
	if( got > 0 &&
		pDstPacket->pts >= m_ref )
	{
		pDstPacket->pos      = -1;
		pDstPacket->duration = m_pCodecs->frame_size;
		THIS_LOGD_print("got frame[%p]: pts=%lld, data=%p, size=%d, cost=%lld", pDstdata.p, pDstPacket->pts, pDstPacket->data, pDstPacket->size, cost.Get());
		return pDstdata.Detach();
	}
	}while(0);

	return NULL;
}
