#include "HwAacDecoder.h"
#include "media/NdkMediaFormat.h"

// refer ffmpeg mpeg4audio.h
enum AudioObjectType {
    AOT_NULL,					// Support?                Name
    AOT_AAC_MAIN,              ///< Y                       Main
    AOT_AAC_LC,                ///< Y                       Low Complexity
    AOT_AAC_SSR,               ///< N (code in SoC repo)    Scalable Sample Rate
    AOT_AAC_LTP,               ///< Y                       Long Term Prediction
    AOT_SBR,                   ///< Y                       Spectral Band Replication
    AOT_AAC_SCALABLE,          ///< N                       Scalable
};

// refer ffmpeg mpeg4audio.c
const int avpriv_mpeg4audio_sample_rates[16] = {
    96000, 88200, 64000, 48000, 44100, 32000,
    24000, 22050, 16000, 12000, 11025, 8000, 7350
};

CLASS_LOG_IMPLEMENT(CHwAacDecoder, "audio[aac.decoder]");

CHwAacDecoder::CHwAacDecoder(AMediaCodec *pCodec, StreamDecodedataParameter *Parameter)
  : m_pCodec(pCodec)
{
	THIS_LOGT_print("is created: %p of %s", m_pCodec, Parameter->minetype);
	AMediaFormat *format = AMediaFormat_new();

	AMediaFormat_setString(format, AMEDIAFORMAT_KEY_MIME, Parameter->minetype);
	AMediaFormat_setInt32 (format, AMEDIAFORMAT_KEY_BIT_RATE, 128000);
	AMediaFormat_setInt32 (format, AMEDIAFORMAT_KEY_SAMPLE_RATE, Parameter->samplerate);
	AMediaFormat_setInt32 (format, AMEDIAFORMAT_KEY_CHANNEL_COUNT, Parameter->channels);

	int sampling_index = 0;
	for(; sampling_index < sizeof(avpriv_mpeg4audio_sample_rates); sampling_index++ ) {
		if( avpriv_mpeg4audio_sample_rates[sampling_index] == Parameter->samplerate ) {
			break;
		}
	}
	if( sampling_index == sizeof(avpriv_mpeg4audio_sample_rates) ) {
		sampling_index = 	4; //default
	}

	uint16_t val =  (0xFFFF & (AOT_AAC_LC << 11)) |
					(0xFFFF & (sampling_index << 7)) |
					(0xFFFF & (Parameter->channels << 3) );

	unsigned char esds[2];
	esds[0] = (val >> 8);
	esds[1] = (val);
	AMediaFormat_setBuffer(format, "csd-0", esds, sizeof(esds));	// sps
	THIS_LOGD_print("samplerate=%d, channels=%d, csd-0=%02X%02X", Parameter->samplerate, Parameter->channels, esds[0], esds[1]);
	THIS_LOGI_print("configure %s", AMediaFormat_toString(format));

	media_status_t ret1 = AMediaCodec_configure(m_pCodec, format, NULL, NULL, 0);
	THIS_LOGD_print("call AMediaCodec_configure return %d", ret1);

	media_status_t ret2 = AMediaCodec_start(m_pCodec);
	THIS_LOGD_print("call AMediaCodec_start return %d", ret2);

	AMediaFormat_delete(format);
}

CHwAacDecoder::~CHwAacDecoder()
{
    AMediaCodec_stop(m_pCodec);
    AMediaCodec_delete(m_pCodec);
	THIS_LOGT_print("is deleted");
}

int CHwAacDecoder::Decode(unsigned char *data, int size, int64_t presentationTimeUs, unsigned char *pout)
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
	assert(ibuf && size <= nlen);
	memcpy(ibuf, data, size);
	ssize_t nStatus = AMediaCodec_queueInputBuffer(m_pCodec, nIndex, 0, size, presentationTimeUs, 0);
	if( nStatus < 0)
	{
		THIS_LOGE_print("call AMediaCodec_queueInputBuffer return %d", nStatus);
		return -2;
	}
	else
	{
		AMediaCodecBufferInfo info;
		ssize_t nIndex = AMediaCodec_dequeueOutputBuffer(m_pCodec, &info, 10000);
		if( nIndex < 0)
		{
			THIS_LOGE_print("call AMediaCodec_dequeueOutputBuffer return %d", nIndex);
			if( nIndex != AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED ) return -3;
			AMediaFormat *format = AMediaCodec_getOutputFormat(m_pCodec);
			THIS_LOGW_print("configure: %s", AMediaFormat_toString(format));
			AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_CHANNEL_COUNT, &m_param1);			
			AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_SAMPLE_RATE  , &m_param2);
			AMediaFormat_delete(format);
			return -4;
		}

		if( info.size )
		{
			uint8_t *obuf = AMediaCodec_getOutputBuffer(m_pCodec, nIndex, &nlen);
			assert(obuf && info.size <= nlen);
			memcpy(pout, obuf, info.size);
			THIS_LOGD_print("got frame[%p]: pts=%lld, data=%p, size=%d, cost=%lld", obuf, info.presentationTimeUs, pout, info.size, GetTickCount() - cost);
		}

		AMediaCodec_releaseOutputBuffer(m_pCodec, nIndex, info.size != 0);
		return info.size;
	}
}
