#include "AudioDecoder.h"

CLASS_LOG_IMPLEMENT(CAudioDecoder, "CDecoder[audio]");
////////////////////////////////////////////////////////////////////////////////
CAudioDecoder::CAudioDecoder(AVCodecContext *pCodecs)
  : CMediaDecoder(pCodecs)
{
	THIS_LOGT_print("is created: codecid=%d", m_pCodecs->codec_id);
	m_frame = av_frame_alloc();
}

CAudioDecoder::~CAudioDecoder()
{
	av_frame_free(&m_frame );
	THIS_LOGT_print("is deleted");
}

////////////////////////////////////////////////////////////////////////////////
CBuffer *CAudioDecoder::Decode(CBuffer *pSrcdata)
{//xxx->pcm
	COST_STAT_DECLARE(cost);

	do{
	AVPacket *pSrcPacket = pSrcdata->GetPacket();
	if( m_pBufferManager==NULL) m_pBufferManager = new CBufferManager(m_pCodecs->frame_size * m_pCodecs->channels * sizeof(short));
	THIS_LOGT_print("do decode[%p]: pts=%lld, data=%p, size=%d", pSrcdata, pSrcPacket->pts, pSrcPacket->data, pSrcPacket->size);

	int got = 0;
	int len = avcodec_decode_audio4(m_pCodecs, m_frame, &got, pSrcPacket);
	if( len < 0 )
	{
		THIS_LOGE_print("call avcodec_decode_audio4 return %d", len);
		break;
	}
	if( got > 0 )
	{
//		THIS_LOGT_print("call avcodec_decode_audio4 return %d", len);
		CScopeBuffer pPcmframe(m_pBufferManager->Pop(pSrcPacket->pts));
		short *pPcmbuf = (short*)pPcmframe->GetPacket()->data; //has been set size
		for(int i = 0; i < m_pCodecs->frame_size; i ++)
		{
			for(int c = 0; c < m_pCodecs->channels; c ++)
			{
				float *extend_data = (float*)m_frame->extended_data[c];
				float  sample      = extend_data[i];
				if( sample <-1.0f )
				{
					sample =-1.0f;
				}
				else if( sample > 1.0f )
				{
					sample = 1.0f;
				}
				pPcmbuf[i * m_pCodecs->channels + c] = (short)round(sample * 32767.0f);
			}
		}

		THIS_LOGD_print("got frame[%p]: pts=%lld, data=%p, size=%d, cost=%lld", pPcmframe.p, pSrcPacket->pts, pPcmframe->GetPacket()->data, pPcmframe->GetPacket()->size, cost.Get());
		return pPcmframe.Detach();
	}
	}while(0);

	return NULL;
 }
