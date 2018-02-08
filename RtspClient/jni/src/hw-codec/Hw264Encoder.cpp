#include "Hw264Encoder.h"
#include "media/NdkMediaFormat.h"

CLASS_LOG_IMPLEMENT(CHw264Encoder, "video[264.encoder]");
extern bool g_IsMTK;
CHw264Encoder::CHw264Encoder(AMediaCodec *pCodec, StreamEncodedataParameter *Parameter)
  : m_pCodec(pCodec), m_ref(-1), m_adj(-1)
{
	THIS_LOGT_print("is created: %p of %s, IsMTK: %d", m_pCodec, Parameter->minetype, g_IsMTK? 1:0);
	AMediaFormat *format = AMediaFormat_new();
	AMediaFormat_setString(format, AMEDIAFORMAT_KEY_MIME, Parameter->minetype);
	AMediaFormat_setInt32 (format, AMEDIAFORMAT_KEY_WIDTH, Parameter->width);
	AMediaFormat_setInt32 (format, AMEDIAFORMAT_KEY_HEIGHT, Parameter->height);
	AMediaFormat_setInt32 (format, AMEDIAFORMAT_KEY_BIT_RATE, Parameter->bitrate);
	AMediaFormat_setInt32 (format, AMEDIAFORMAT_KEY_I_FRAME_INTERVAL, Parameter->gop);
	AMediaFormat_setInt32 (format, AMEDIAFORMAT_KEY_FRAME_RATE, Parameter->fps);
	AMediaFormat_setInt32 (format, AMEDIAFORMAT_KEY_COLOR_FORMAT, g_IsMTK? 19:21);
	THIS_LOGI_print("configure %s", AMediaFormat_toString(format));

	media_status_t ret1 = AMediaCodec_configure(m_pCodec, format, NULL, NULL, AMEDIACODEC_CONFIGURE_FLAG_ENCODE);
	THIS_LOGD_print("call AMediaCodec_configure return %d", ret1);

	media_status_t ret2 = AMediaCodec_start(m_pCodec);
	THIS_LOGD_print("call AMediaCodec_start return %d", ret2);
	AMediaFormat_delete(format);

	m_codecid = g_IsMTK? 0/*AV_PIX_FMT_YUV420P*/:25/*AV_PIX_FMT_NV12*/;
	m_bHasCSD = g_IsMTK == false;
}

CHw264Encoder::~CHw264Encoder()
{
	AMediaCodec_stop(m_pCodec);
	AMediaCodec_delete(m_pCodec);
	THIS_LOGT_print("is deleted");
}

int CHw264Encoder::Encode(unsigned char *pktBuf, int pktLen, int64_t &presentationTimeUs, unsigned char *pout)
{
	COST_STAT_DECLARE(cost);
	ssize_t nIndex = AMediaCodec_dequeueInputBuffer(m_pCodec, 100000);
	if( nIndex < 0 )
	{
		THIS_LOGE_print("call AMediaCodec_dequeueInputBuffer return %d", nIndex);	
		return nIndex;
	}

	size_t nlen;
	uint8_t *ibuf = AMediaCodec_getInputBuffer(m_pCodec, nIndex, &nlen);
	if( pktBuf != 0)
	{
		assert(ibuf && pktLen <= nlen);	
		memcpy(ibuf, pktBuf, pktLen);
	}
	ssize_t nStatus = AMediaCodec_queueInputBuffer(m_pCodec, nIndex, 0, pktLen, presentationTimeUs, 0);
	if( nStatus < 0)
	{
		THIS_LOGE_print("call AMediaCodec_queueInputBuffer return %d", nStatus);
		return nStatus;
	}
	else
	{
		AMediaCodecBufferInfo info;
		ssize_t nIndex = AMediaCodec_dequeueOutputBuffer(m_pCodec, &info, 10000);
		if( nIndex < 0)
		{
			if( m_bHasCSD != false )
			{
				THIS_LOGE_print("call AMediaCodec_dequeueOutputBuffer return %d", nIndex);			
				return nIndex;			
			}
			else
			{
				unsigned char *sps = NULL, *pps = NULL;
				unsigned char *pos = pout;
				size_t sps_len = 0, pps_len = 0;
				AMediaFormat *format = AMediaCodec_getOutputFormat(m_pCodec);
				AMediaFormat_getBuffer(format, "csd-0", (void**)&sps, &sps_len); 
				memcpy(pos, sps, sps_len); pos += sps_len;
				AMediaFormat_getBuffer(format, "csd-1", (void**)&pps, &pps_len);
				memcpy(pos, pps, pps_len); pos += pps_len;
				AMediaFormat_delete(format);
				THIS_LOGD_print("got frame[0x0]: pts=%lld, data=%p, size=%d, nalu.type=%d, cost=%lld", presentationTimeUs, pout, sps_len + pps_len, (int)pout[4]&0x1f, GetTickCount() - cost);				
				m_ref = presentationTimeUs;
				m_bHasCSD = true;
				return sps_len + pps_len;
			}
		}

		if( info.size )
		{
			uint8_t *obuf = AMediaCodec_getOutputBuffer(m_pCodec, nIndex, &nlen);
			assert(obuf && info.size <= nlen);
			memcpy(pout, obuf, info.size);
			if( m_ref < 0 )
			{
				m_ref = presentationTimeUs;				
			}
			else
			{
				if( m_adj < 0 )
				{
					m_adj = m_ref - info.presentationTimeUs;
					presentationTimeUs = m_ref;
				}
				else
				{
					presentationTimeUs = info.presentationTimeUs + m_adj;				
				}
			}
			THIS_LOGD_print("got frame[%p]: pts=%lld, data=%p, size=%d, nalu.type=%d, cost=%lld", obuf, presentationTimeUs, pout, info.size, (int)pout[4] & 0x1f, GetTickCount() - cost);
		}

		AMediaCodec_releaseOutputBuffer(m_pCodec, nIndex, info.size != 0);
		return info.size;
	}
}
