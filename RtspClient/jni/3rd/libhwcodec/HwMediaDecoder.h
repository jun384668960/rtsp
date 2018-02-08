#ifndef HWMEDIADECODER_H
#define HWMEDIADECODER_H

#include <android/native_window_jni.h>

struct StreamDecodedataParameter
{
	char *minetype;
	int width; 
	int height; 
	ANativeWindow *window;
	int channels;
	int samplerate;
};

class CHwMediaDecoder
{
public:
	CHwMediaDecoder() { m_codecid = -1;}
	virtual ~CHwMediaDecoder() {}

public: //音频输出: aac, 视频输出: yuv
	virtual int Decode(unsigned char *pktBuf, int pktLen, int64_t presentationTimeUs, unsigned char *outBuf) = 0;	

public:
	int m_codecid; //输出编码格式
	int m_param1;
	int m_param2;
	int m_videow;
	int m_videoh;
};

#endif

