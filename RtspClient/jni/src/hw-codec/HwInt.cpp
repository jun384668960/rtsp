#include "HwInt.h"
#include "HwAacDecoder.h"
#include "Hw264Decoder.h"
#include "HWAacEncoder.h"
#include "Hw264Encoder.h"
#include "media/NdkMediaExtractor.h"
#include <fcntl.h>

#ifdef _LOGFILE //必须定义_DEBUG
FILE *__logfp    = NULL;
void *__logmutex = NULL;
#endif
bool g_IsMTK = false; //标识是否是高通芯片
///////////////////////////////////////////////////////////////////////////////////////
CHwMediaDecoder *GetVideoDecoderClass(StreamDecodedataParameter *parameter)
{
	AMediaCodec *pCodec = AMediaCodec_createDecoderByType(parameter->minetype);
	if( pCodec )
		return new CHw264Decoder(pCodec, parameter);
	else
		return NULL;
}

CHwMediaDecoder *GetAudioDecoderClass(StreamDecodedataParameter *parameter)
{
	AMediaCodec *pCodec = AMediaCodec_createDecoderByType(parameter->minetype);
	if( pCodec )
		return new CHwAacDecoder(pCodec, parameter);
	else
		return NULL;
}

CHwMediaEncoder *GetVideoEncoderClass(StreamEncodedataParameter *parameter)
{
  	AMediaCodec *pCodec = AMediaCodec_createEncoderByType(parameter->minetype);
	if( pCodec )
		return new CHw264Encoder(pCodec, parameter);
	else
		return NULL;
}

CHwMediaEncoder *GetAudioEncoderClass(StreamEncodedataParameter *parameter)
{
  	AMediaCodec *pCodec = AMediaCodec_createEncoderByType(parameter->minetype);
	if( pCodec )
		return new CHwAacEncoder(pCodec, parameter);
	else
		return NULL;
}

#ifdef _LOGFILE //必须定义_DEBUG
int Init(char *model, char *board, char*szmtk, FILE *logfp, void *logfpmutex)
{
	g_IsMTK    = strncmp(szmtk, "MT", 2) == 0;
	__logfp    = logfp;
	__logmutex = logfpmutex;
	return 0;
}
#else
int Init(char *model, char *board, char*szmtk)
{
	g_IsMTK    = strncmp(szmtk, "MT", 2) == 0;
	return 0;
}
#endif