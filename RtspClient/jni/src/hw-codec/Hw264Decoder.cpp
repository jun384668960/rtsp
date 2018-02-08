#include "Hw264Decoder.h"
#include "media/NdkMediaFormat.h"

CLASS_LOG_IMPLEMENT(CHw264Decoder, "video[264.decoder]");

CHw264Decoder::CHw264Decoder(AMediaCodec *pCodec, StreamDecodedataParameter *Parameter)
  : m_pCodec(pCodec), m_pWindow(Parameter->window)
{
	THIS_LOGT_print("is created: %p of %s", m_pCodec, Parameter->minetype);
	AMediaFormat *format = AMediaFormat_new();
	AMediaFormat_setString(format, AMEDIAFORMAT_KEY_MIME  , Parameter->minetype);	
	AMediaFormat_setInt32 (format, AMEDIAFORMAT_KEY_WIDTH , Parameter->width);
	AMediaFormat_setInt32 (format, AMEDIAFORMAT_KEY_HEIGHT, Parameter->height);
	THIS_LOGI_print("configure %s", AMediaFormat_toString(format));

	media_status_t ret1 = AMediaCodec_configure(m_pCodec, format, m_pWindow, NULL, 0);
	THIS_LOGD_print("call AMediaCodec_configure return %d", ret1);

	media_status_t ret2 = AMediaCodec_start(m_pCodec);
	THIS_LOGD_print("call AMediaCodec_start return %d", ret2);

	AMediaFormat_delete(format);
	
	m_param1 = 0;
	m_param2 = 0;
}

CHw264Decoder::~CHw264Decoder()
{
    AMediaCodec_stop(m_pCodec);
    AMediaCodec_delete(m_pCodec);
	THIS_LOGT_print("is deleted");
}

int CHw264Decoder::Decode(unsigned char *data, int size, int64_t presentationTimeUs, unsigned char *pout)
{
	COST_STAT_DECLARE(cost);
	ssize_t nIndex = AMediaCodec_dequeueInputBuffer(m_pCodec, 100000);
	if( nIndex < 0)
	{
		THIS_LOGE_print("call AMediaCodec_dequeueInputBuffer return %d", nIndex);
		return -1;
	}

	size_t nlen;
	uint8_t *ibuf = AMediaCodec_getInputBuffer(m_pCodec, nIndex, &nlen);
    if( ibuf == NULL || size > nlen )
    {
        THIS_LOGE_print("call AMediaCodec_getInputBuffer nIndex=%d ibuf=%p size=%d nlen=%d", nIndex, ibuf, size, nlen);
        return -1;
    }
	memcpy(ibuf    , "\x00\x00\x00\x01", 4);
	memcpy(ibuf + 4, data + 4, size - 4);
	ssize_t nStatus = AMediaCodec_queueInputBuffer(m_pCodec, nIndex, 0, size, presentationTimeUs, 0);
	if( nStatus < 0)
	{
		THIS_LOGE_print("call AMediaCodec_queueInputBuffer return %d", nStatus);
		return -2;
	}
	else
	{
		uint8_t cframe = ibuf[4] & 0x1f;
		if( cframe== 7 || cframe== 8 ) return 0; //sps/pps

		AMediaCodecBufferInfo info;
		ssize_t nIndex = AMediaCodec_dequeueOutputBuffer(m_pCodec, &info, 10000);
		if( nIndex < 0)
		{
			THIS_LOGE_print("call AMediaCodec_dequeueOutputBuffer return %d", nIndex);
			if( nIndex != AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED ) return -3;
			int codecid;			
			AMediaFormat *format = AMediaCodec_getOutputFormat(m_pCodec);
			THIS_LOGW_print("configure: %s", AMediaFormat_toString(format));
			AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_COLOR_FORMAT, &codecid);
			AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_WIDTH , &m_videow);
			AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_HEIGHT, &m_videoh);
			AMediaFormat_getInt32(format, "stride", &m_param1);	if( m_param1 == 0 ) m_param1 = m_videow;		
			AMediaFormat_getInt32(format, "slice-height", &m_param2); if( m_param2 == 0 ) m_param2 = m_videoh;	
			AMediaFormat_delete(format);
			m_codecid = codecid==19? 0/*AV_PIX_FMT_YUV420P*/:25/*AV_PIX_FMT_NV12*/;			
			return -4;
		}

		if( m_codecid <  0 )
		{
			int codecid;
			AMediaFormat *format = AMediaCodec_getOutputFormat(m_pCodec);
			AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_COLOR_FORMAT, &codecid);
			AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_WIDTH , &m_videow);
			AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_HEIGHT, &m_videoh);
			AMediaFormat_getInt32(format, "stride", &m_param1);	if( m_param1 == 0 ) m_param1 = m_videow;		
			AMediaFormat_getInt32(format, "slice-height", &m_param2); if( m_param2 == 0 ) m_param2 = m_videoh;	
			AMediaFormat_delete(format);
			m_codecid = codecid==19? 0/*AV_PIX_FMT_YUV420P*/:25/*AV_PIX_FMT_NV12*/;			
		}

		if( info.size &&
			m_pWindow == 0 )
		{
			uint8_t *obuf = AMediaCodec_getOutputBuffer(m_pCodec, nIndex, &nlen);
			assert(obuf && info.size <= nlen);
			memcpy(pout, obuf, info.size);
			#ifdef _CAST_SHOW
			static int delayCount = 0;
			static long long delayTime = 0;

			delayCount++;
			delayTime += GetTickCount() - cost;
			if(delayCount >= 100)
			{
				THIS_LOGE_print("delayTime=%lld",delayTime);
				delayCount = 0;
				delayTime = 0;
			}
			#endif
			THIS_LOGD_print("got frame[%p]: pts=%lld, data=%p, size=%d, codecid=%d, cost=%lld", obuf, info.presentationTimeUs, pout, info.size, m_codecid, GetTickCount() - cost);
		}

		AMediaCodec_releaseOutputBuffer(m_pCodec, nIndex, info.size!= 0);
		return m_pWindow? 0:info.size;
	}
}
