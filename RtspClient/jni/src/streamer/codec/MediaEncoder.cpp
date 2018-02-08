#include "MediaEncoder.h"
#include "Hw264Encoder.h"
#include "HwAacEncoder.h"
#include "VideoEncoder.h"
#include "AudioEncoder.h"
#include "HwCodec.h"

CMediaEncoder *CMediaEncoder::Create(AVCodecContext *pCodecs, int fmt)
{
	LOGT_print("CMediaEncoder", "Create Codecid: %d", pCodecs->codec_id);
	switch(pCodecs->codec_id)
	{
		case AV_CODEC_ID_H264: {
			StreamEncodedataParameter Parameter;
			Parameter.minetype = "video/avc";
			Parameter.bitrate  = pCodecs->bit_rate;
			Parameter.gop      = pCodecs->gop_size;
			Parameter.fps      = pCodecs->time_base.den<30? (pCodecs->time_base.den):(pCodecs->bit_rate<=800*1000? 30:25);
			Parameter.height   = pCodecs->height;
			Parameter.width    = pCodecs->width;
			CHwMediaEncoder *pHwEncoder = g_pHwCodec->CreateH264Encoder(&Parameter);
			if( pHwEncoder )
			{
				return new CHw264Encoder(pCodecs, fmt, pHwEncoder);
			}
			else
			{
				return new CVideoEncoder(pCodecs, fmt);
			}
		}

		case AV_CODEC_ID_MJPEG: {
			return new CVideoEncoder(pCodecs, fmt);
		}

		case AV_CODEC_ID_AAC: {
			StreamEncodedataParameter Parameter;
			Parameter.minetype 	  = "audio/mp4a-latm";
			Parameter.bitrate     = pCodecs->bit_rate;
			Parameter.channels    = pCodecs->channels;
			Parameter.samplerate  = pCodecs->sample_rate;
			Parameter.profile     = 2; //LC
			CHwMediaEncoder *pHwEncoder = g_pHwCodec->CreateAacEncoder(&Parameter);
			if( pHwEncoder )
			{
				return new CHwAacEncoder(pCodecs, fmt, pHwEncoder);
			}
			else
			{
				return new CAudioEncoder(pCodecs, fmt);
			}
		}

		default: {
			LOGW_print("CMediaEncoder", "not support encode codecid: %d", pCodecs->codec_id);			
			return NULL;
		}
	}
}
