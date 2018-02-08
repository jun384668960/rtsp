#ifndef HARD_WARED_INIT_H
#define HARD_WARED_INIT_H

#include "logimp.h"
#include "HwMediaEncoder.h"
#include "HwMediaDecoder.h"

#ifdef __cplusplus
extern "C" {
#endif

CHwMediaDecoder *GetVideoDecoderClass(StreamDecodedataParameter *parameter);
CHwMediaDecoder *GetAudioDecoderClass(StreamDecodedataParameter *parameter);
CHwMediaEncoder *GetVideoEncoderClass(StreamEncodedataParameter *Parameter);
CHwMediaEncoder *GetAudioEncoderClass(StreamEncodedataParameter *Parameter);

#ifdef _LOGFILE //±ÿ–Î∂®“Â_DEBUG
int Init(char *model, char *board, char *szmtk, FILE *logfp, void *logfpmutex);
#else
int Init(char *model, char *board, char *szmtk);
#endif

#ifdef __cplusplus
}
#endif

#endif
