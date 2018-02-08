#ifndef __RTMPRECORDTHREAD_H__
#define __RTMPRECORDTHREAD_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <rtmp.h>
#ifdef __cplusplus
}
#endif
#include <string>
#include "MediaRecorder.h"

//远程录制线程: rtmp，264/aac编码格式
///////////////////////////////////////////////////////////////////
class CRtmpRecordThread : public CMediaRecorder
{
public:
	CRtmpRecordThread(CPlayer *pPlayer, const char *uri, int vcodecid, int acodecid)
	  : CMediaRecorder(pPlayer, vcodecid, acodecid), m_uri(uri), m_ref(-1)
	{
		THIS_LOGT_print("is created of %s", uri);
		assert(vcodecid == 0 ||
			   vcodecid == AV_CODEC_ID_H264);
		assert(acodecid == 0 ||
			   acodecid == AV_CODEC_ID_AAC );
		if( vcodecid != 0 ) m_pCodecs[2] = avcodec_alloc_context3(avcodec_find_encoder(vcodecid));
		if( acodecid != 0 ) m_pCodecs[3] = avcodec_alloc_context3(avcodec_find_encoder(acodecid));
		RTMP_Init(&m_rtmpctx);
	}
	virtual ~CRtmpRecordThread()
	{
		RTMP_Close(&m_rtmpctx);	
		if( m_pCodecs[2] != 0 ) avcodec_free_context(&m_pCodecs[2]); //will call avcodec_close
		if( m_pCodecs[3] != 0 ) avcodec_free_context(&m_pCodecs[3]); //will call avcodec_close	
		THIS_LOGT_print("is deleted");
	}

public: //interface of CWriter
	virtual int Write(JNIEnv *env, void *buffer = 0)
	{
		int ret = CMediaRecorder::Write(env, buffer);
		if( ret == 0 &&
			m_rtmpctx.m_bPlaying == 0 )	m_rtmpctx.m_sb.sb_ctrl = 1;
		return ret;
	}

protected: //interface of CMediaRecorder
	virtual int PrepareWrite();
	virtual int Write(int index, AVPacket *pPacket);

protected:
	void SendMetadataPacket();	
	void SendAacConfig(int64_t pts, AVCodecContext *pCodecs);
	void Send264Config(AVPacket *pRawdata);
	void SendAacPacket(AVPacket *pRawdata);
	void Send264Packet(AVPacket *pRawdata);

protected:
	RTMP    m_rtmpctx;	
	int64_t     m_ref;
	std::string m_uri;
	CLASS_LOG_DECLARE(CRtmpRecordThread);	
};

#endif
