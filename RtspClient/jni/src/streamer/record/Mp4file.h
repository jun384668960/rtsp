#ifndef __MP4FILE_H__
#define __MP4FILE_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#ifdef __cplusplus
}
#endif
#include "common.h"

////////////////////////////////////////////////////////////////////////////////
class CMp4file
{
public:
	CMp4file(const char *path, int vcodecid, int acodecid);
	virtual ~CMp4file();

public:
	int Start();
	int Write(AVMediaType type, AVPacket *pPacket);
	AVCodecContext *GetCodecContext(unsigned int index) const { return m_stream[index]==0? 0:m_stream[index]->codec;}
	int64_t         Get(int64_t pts) { if( m_ref ==-1 ) m_ref = pts; return pts - m_ref;}

private:
    AVFormatContext *m_fmtctx;
	AVStream *m_stream[2]; //0-video 1-audio 
	int64_t   m_ref;
	CLASS_LOG_DECLARE(CMp4file);
};

#endif
