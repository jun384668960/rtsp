#include "MediaDecoder.h"
#include "VideoDecoder.h"
#include "AudioDecoder.h"
#include "Hw264Decoder.h"
#include "HwAacDecoder.h"
#include "HwCodec.h"

CMediaDecoder *CMediaDecoder::Create(AVCodecContext *pCodecs, bool hw, void *param2)
{
	LOGT_print("CMediaDecoder", "Create Codecid: %d", pCodecs->codec_id);		
	switch(pCodecs->codec_id)
	{
		case AV_CODEC_ID_H264: {
			StreamDecodedataParameter Parameter;
			Parameter.minetype = "video/avc";
			Parameter.width    = pCodecs->width;
			Parameter.height   = pCodecs->height;
			Parameter.window   = hw==false? 0:(ANativeWindow*)param2;
			CHwMediaDecoder *pHwDecoder = hw==false? 0:g_pHwCodec->CreateH264Decoder(&Parameter);
			if( pHwDecoder )
			{
				return new CHw264Decoder(pCodecs, pHwDecoder);
			}
			else
			{
				return new CVideoDecoder(pCodecs);
			}
		}

		case AV_CODEC_ID_MJPEG: {
			return new CVideoDecoder(pCodecs);
		}

		case AV_CODEC_ID_AAC: {
			StreamDecodedataParameter Parameter;
			Parameter.minetype 	 = "audio/mp4a-latm";			
			Parameter.channels   = pCodecs->channels;
			Parameter.samplerate = pCodecs->sample_rate;
			CHwMediaDecoder *pHwDecoder = 0;//hw==false? 0:g_pHwCodec->CreateAacDecoder(&Parameter); //È¡ÏûaacÓ²½âÂë
			if( pHwDecoder )
			{
				return new CHwAacDecoder(pCodecs, pHwDecoder);
			}
			else
			{
				return new CAudioDecoder(pCodecs);
			}
		}

		default: {
			LOGW_print("CMediaDecoder", "not support decode codecid: %d", pCodecs->codec_id);			
			return NULL;
		}
	}
}
