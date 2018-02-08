#include "VideoDecoder.h"

CLASS_LOG_IMPLEMENT(CVideoDecoder, "CDecoder[video]");
////////////////////////////////////////////////////////////////////////////////
CVideoDecoder::CVideoDecoder(AVCodecContext *pCodecs)
  : CMediaDecoder(pCodecs)
{
	THIS_LOGT_print("is created: codecid=%d, fmt=%d", m_pCodecs->codec_id, m_pCodecs->pix_fmt);
	m_frame = av_frame_alloc();
}

CVideoDecoder::~CVideoDecoder()
{
	av_frame_free(&m_frame);
	THIS_LOGT_print("is deleted.");
}

CBuffer *CVideoDecoder::Decode(CBuffer *pSrcdata)
{
	COST_STAT_DECLARE(cost);
	AVPicture avpic;

	do{
	AVPacket *pSrcPacket = pSrcdata->GetPacket();
	#ifdef _DEBUG
	if( m_pCodecs->codec_id == AV_CODEC_ID_H264)
	THIS_LOGT_print("do decode[%p]: pts=%lld, data=%p, size=%d, nalu.type=%d", pSrcdata, pSrcPacket->pts, pSrcPacket->data, pSrcPacket->size, (int)pSrcPacket->data[4] & 0x1f);	
	else
	THIS_LOGT_print("do decode[%p]: pts=%lld, data=%p, size=%d", pSrcdata, pSrcPacket->pts, pSrcPacket->data, pSrcPacket->size);	
	#endif
	pSrcPacket->flags = AV_PKT_FLAG_KEY;

	int got = 0;
	int len = avcodec_decode_video2(m_pCodecs, m_frame, &got, pSrcPacket);
	if( len < 0 )
	{
		THIS_LOGE_print("call avcodec_decode_video2 return %d", len);
		break;
	}
	if( got > 0 )
	{
		if( m_pBufferManager == NULL) m_pBufferManager = new CBufferManager(avpicture_get_size(m_pCodecs->pix_fmt, m_pCodecs->width, m_pCodecs->height));
		CScopeBuffer pYuvdata(m_pBufferManager->Pop(pSrcPacket->pts));
		memcpy(avpic.data    , m_frame->data    , sizeof(avpic.data));
		memcpy(avpic.linesize, m_frame->linesize, sizeof(avpic.linesize));
		int ret = avpicture_layout(&avpic, m_pCodecs->pix_fmt, m_pCodecs->width, m_pCodecs->height, pYuvdata->GetPacket()->data, pYuvdata->GetPacket()->size);
#ifdef _CAST_SHOW
		static int delayCount = 0;
		static long long delayTime = 0;

		delayCount++;
		delayTime += cost.Get();
		if(delayCount >= 100)
		{
			THIS_LOGE_print("delayTime=%lld",delayTime);
			delayCount = 0;
			delayTime = 0;
		}
#endif
		THIS_LOGD_print("got frame[%p]: pts=%lld, data=%p, size=%d, iret=%d, cost=%lld", pYuvdata.p, pYuvdata->GetPacket()->pts, pYuvdata->GetPacket()->data, pYuvdata->GetPacket()->size, ret, cost.Get());	
		return pYuvdata.Detach();
	}
	}while(0);

	return NULL;
}
