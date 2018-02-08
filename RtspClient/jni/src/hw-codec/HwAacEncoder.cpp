#include "HwAacEncoder.h"
#include "media/NdkMediaFormat.h"

CLASS_LOG_IMPLEMENT(CHwAacEncoder, "audio[aac.encoder]");

CHwAacEncoder::CHwAacEncoder(AMediaCodec *pCodec, StreamEncodedataParameter *Parameter)
  : m_pCodec(pCodec)
{
	THIS_LOGT_print("is created: %p of %s", m_pCodec, Parameter->minetype);
	AMediaFormat *format = AMediaFormat_new();

	AMediaFormat_setString(format, AMEDIAFORMAT_KEY_MIME, Parameter->minetype);
	AMediaFormat_setInt32 (format, AMEDIAFORMAT_KEY_CHANNEL_COUNT, Parameter->channels);
	AMediaFormat_setInt32 (format, AMEDIAFORMAT_KEY_SAMPLE_RATE, Parameter->samplerate);
	AMediaFormat_setInt32 (format, AMEDIAFORMAT_KEY_BIT_RATE, Parameter->bitrate);
	AMediaFormat_setInt32 (format, AMEDIAFORMAT_KEY_AAC_PROFILE, Parameter->profile); 
	THIS_LOGI_print("configure %s", AMediaFormat_toString(format));

	media_status_t ret1 = AMediaCodec_configure(m_pCodec, format, NULL, NULL, AMEDIACODEC_CONFIGURE_FLAG_ENCODE);
	THIS_LOGD_print("call AMediaCodec_configure return %d", ret1);

	media_status_t ret2 = AMediaCodec_start(m_pCodec);
	THIS_LOGD_print("call AMediaCodec_start return %d", ret2);

	AMediaFormat_delete(format);	
}

CHwAacEncoder::~CHwAacEncoder()
{
	AMediaCodec_stop(m_pCodec);
	AMediaCodec_delete(m_pCodec);
	THIS_LOGT_print("is deleted");
}

int CHwAacEncoder::Encode(unsigned char *pktBuf, int pktLen, int64_t &presentationTimeUs, unsigned char *pout)
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
			THIS_LOGE_print("call AMediaCodec_dequeueOutputBuffer return %d", nIndex);
			return nIndex;
		}

		if( info.size )
		{
			uint8_t *obuf = AMediaCodec_getOutputBuffer(m_pCodec, nIndex, &nlen);
			assert(obuf && info.size <= nlen);
			memcpy(pout, obuf, info.size);
			presentationTimeUs = info.presentationTimeUs;
			THIS_LOGD_print("got frame[%p]: pts=%lld, data=%p, size=%d, cost=%lld", obuf, presentationTimeUs, pout, info.size, GetTickCount() - cost);
		}
	
		AMediaCodec_releaseOutputBuffer(m_pCodec, nIndex, info.size != 0);
		return info.size;
	}
}