#ifndef __HWCODEC_H__
#define __HWCODEC_H__

#include <string>
#include "common.h"
#include "HwMediaDecoder.h" //from libhwcodec
#include "HwMediaEncoder.h"

typedef CHwMediaDecoder *(*CreateDecoderfunc)(StreamDecodedataParameter *Parameter);
typedef CHwMediaEncoder *(*CreateEncoderfunc)(StreamEncodedataParameter *Parameter);

//封装硬件编解码库[bql]
////////////////////////////////////////////////////////////////////////////////
class CHwCodec
{
public:
	CHwCodec(const char* path);
	~CHwCodec();

public:
	CHwMediaDecoder* CreateH264Decoder(StreamDecodedataParameter *Parameter);
	CHwMediaDecoder* CreateAacDecoder(StreamDecodedataParameter *Parameter);
	CHwMediaEncoder* CreateH264Encoder(StreamEncodedataParameter *Parameter);
	CHwMediaEncoder* CreateAacEncoder(StreamEncodedataParameter *Parameter);

private:
	void *m_dll;
	CreateDecoderfunc m_videoDecoderfunc;
	CreateDecoderfunc m_audioDecoderfunc;
	CreateEncoderfunc m_videoEncoderfunc;
	CreateEncoderfunc m_audioEncoderfunc;
	CLASS_LOG_DECLARE(CHwCodec);
};

extern CHwCodec *g_pHwCodec; //在JNI_load.cpp定义/释放 ��Cstreamer.cpp��ʼ��

#endif
