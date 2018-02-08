#include "player/Player.h"
#include "RtmpRecordThread.h"
#include "common.h"
#include <amf.h>

CLASS_LOG_IMPLEMENT(CRtmpRecordThread, "CRecordThread[rtmp]");
///////////////////////////////////////////////////////////////////
int CRtmpRecordThread::PrepareWrite()
{
	AVCodecContext *pVideoctx = m_pCodecs[2];
	if( pVideoctx ) {
		#ifdef _FFMPEG_THREAD
		pVideoctx->thread_count = 4;
		pVideoctx->thread_type = FF_THREAD_FRAME;		
		#endif
		int iret = avcodec_open2(pVideoctx, pVideoctx->codec, NULL);
		if( iret < 0 ) {
		#ifdef _DEBUG
		char buf[128];
	    av_strerror(iret, buf, 128);
		THIS_LOGE_print("call avcodec_open2 return %d, error: %s", iret, buf);
		#endif
		}
	}
	AVCodecContext *pAudioctx = m_pCodecs[3];
	if( pAudioctx ) {
		int iret = avcodec_open2(pAudioctx, pAudioctx->codec, NULL);
		if( iret < 0 ) {
		#ifdef _DEBUG
		char buf[128];
	    av_strerror(iret, buf, 128);
		THIS_LOGE_print("call avcodec_open2 return %d, error: %s", iret, buf);
		#endif
		}
	}

	return CMediaRecorder::PrepareWrite();
}

int CRtmpRecordThread::Write(int index, AVPacket *pPacket)
{
	if( m_ref < 0 )
	{
		RTMP_SetupURL(&m_rtmpctx, (char *)m_uri.c_str());
		RTMP_EnableWrite(&m_rtmpctx);	
		if( RTMP_Connect(&m_rtmpctx, 0) == 0)
		{
			THIS_LOGE_print("call RTMP_Connect return 0");
			return 1;
		}

		if( RTMP_ConnectStream(&m_rtmpctx, 0) == 0)
		{
			THIS_LOGE_print("call RTMP_ConnectStream return 0");		
			return 2;
		}
		else
		{
			THIS_LOGT_print("call RTMP_ConnectStream[%d] succ", m_rtmpctx.m_stream_id);		
		}

		SendMetadataPacket();
		m_ref = pPacket->pts;		
	}
	if( RTMP_IsConnected(&m_rtmpctx) == 0 || RTMP_IsTimedout(&m_rtmpctx) != 0 )
	{
		THIS_LOGE_print("rtmp[%d] is broke", m_rtmpctx.m_stream_id);	
		return 3;
	}

	if( m_ref < 0 ) m_ref = pPacket->pts;
	if( pPacket->pts > m_now ) m_now = pPacket->pts;

	if( index == 0 )
	{
		Send264Packet(pPacket);
	}
	else
	{
		SendAacPacket(pPacket);
	}

	m_nframes[index] ++;
	return 0;
}

void CRtmpRecordThread::SendMetadataPacket()
{
	static const AVal av_setDataFrame = AVC("@setDataFrame");
	static const AVal av_onMetaData = AVC("onMetaData");
	static const AVal av_videocodecid = AVC("videocodecid");
	static const AVal av_width = AVC("width");
	static const AVal av_height = AVC("height");
	static const AVal av_audiocodecid = AVC("audiocodecid");
	static const AVal av_copyright = AVC("copyright");
	static const AVal av_company = AVC("www.protruly.com.cn");

    RTMPPacket packet;
	RTMPPacket_Alloc(&packet, 512);
	char *pos = packet.m_body;
	char *end = packet.m_body + 512;

	pos = AMF_EncodeString(pos, end, &av_setDataFrame);
	pos = AMF_EncodeString(pos, end, &av_onMetaData);

	*pos ++ = AMF_OBJECT;

	{//copyright
		pos = AMF_EncodeNamedString(pos, end, &av_copyright, &av_company);
	}
	if( m_pCodecs[2] )
	{//video
		pos = AMF_EncodeNamedNumber(pos, end, &av_videocodecid, 0x07);
		pos = AMF_EncodeNamedNumber(pos, end, &av_width , m_pCodecs[2]->width);
		pos = AMF_EncodeNamedNumber(pos, end, &av_height, m_pCodecs[2]->height);
	}
	if( m_pCodecs[3] )
	{//audio
		pos = AMF_EncodeNamedNumber(pos, end, &av_audiocodecid, 0x0a);
	}

	*pos ++ = 0;
	*pos ++ = 0;
	*pos ++ = AMF_OBJECT_END;

    packet.m_packetType 	= RTMP_PACKET_TYPE_INFO;
    packet.m_nBodySize  	= pos - packet.m_body;
    packet.m_nChannel   	= 0x04;
    packet.m_nTimeStamp 	= 0;
    packet.m_hasAbsTimestamp= 0;
    packet.m_headerType 	= RTMP_PACKET_SIZE_LARGE;
    packet.m_nInfoField2 	= m_rtmpctx.m_stream_id;

	THIS_LOGI_print("SendMetadatas: len=%d", packet.m_nBodySize);
    int iret = RTMP_SendPacket(&m_rtmpctx, &packet, 0);
	RTMPPacket_Free(&packet);	
}

void CRtmpRecordThread::Send264Config(AVPacket *pRawdata)
{//264: index[2] + cts[3] + nalu.len[4] + nalu.index[1] + nalu.data[n]	
	AVal sps, pps, dat;
	int len1 = nalu_find_first_of(pRawdata->data, pRawdata->size, sps.av_val);
	int len2 = nalu_find_first_of(sps.av_val, pRawdata->size- (sps.av_val- (char*)pRawdata->data), pps.av_val);
	int len3 = nalu_find_first_of(pps.av_val, pRawdata->size- (pps.av_val- (char*)pRawdata->data), dat.av_val);		
	sps.av_len = pps.av_val - len2 - sps.av_val;
	if( len3 ) {
	pps.av_len = dat.av_val - len3 - pps.av_val;
	pRawdata->data = dat.av_val - 4;
	pRawdata->size-=(dat.av_val - 4 - (char*)pRawdata->data);
	} else {
	pps.av_len = pRawdata->size- (pps.av_val- (char*)pRawdata->data);
	pRawdata->data = 0;
	pRawdata->size = 0;
	}

    RTMPPacket packet;
	RTMPPacket_Alloc(&packet, 2 + 3 + 5 + 1 + 2 + sps.av_len + 1 + 2 + pps.av_len);

    packet.m_packetType 	= RTMP_PACKET_TYPE_VIDEO;
    packet.m_nBodySize  	= 2 + 3 + 5 + 1 + 2 + sps.av_len + 1 + 2 + pps.av_len;
    packet.m_nChannel   	= 0x04;
    packet.m_nTimeStamp 	= pRawdata->pts - m_ref;
    packet.m_hasAbsTimestamp= 0;
    packet.m_headerType 	= RTMP_PACKET_SIZE_LARGE;
    packet.m_nInfoField2 	= m_rtmpctx.m_stream_id;

	uint8_t *body = (uint8_t*)packet.m_body;
    int i = 0;
    body[i ++] = 0x17;
    body[i ++] = 0x00;
    body[i ++] = 0x00;
    body[i ++] = 0x00;
    body[i ++] = 0x00;

    //AVCDecoderConfigurationRecord
    body[i ++] = 0x01;
    body[i ++] = sps.av_val[1];
    body[i ++] = sps.av_val[2];
    body[i ++] = sps.av_val[3];
    body[i ++] = 0xff;

    //sps
    body[i ++] = 0xe1;
    body[i ++] = (sps.av_len >> 8) & 0xff;
    body[i ++] = (sps.av_len) & 0xff;
    memcpy(body + i, sps.av_val, sps.av_len); i += sps.av_len;
    
    //pps
    body[i ++] = 0x01;
    body[i ++] = (pps.av_len >> 8) & 0xff;
    body[i ++] = (pps.av_len) & 0xff;
    memcpy(body + i, pps.av_val, pps.av_len); i += pps.av_len;

	THIS_LOGI_print("Send264Config: sps=%d, pps=%d", sps.av_len, pps.av_len);
    int iret = RTMP_SendPacket(&m_rtmpctx, &packet, 0);
	RTMPPacket_Free(&packet);
}

void CRtmpRecordThread::SendAacConfig(int64_t pts, AVCodecContext *pCodecs)
{
	uint8_t data[2];

	int idx = GetIndexBySamplerate(pCodecs->sample_rate);
	int csd = (2/*AOT_AAC_LC*/ << 11) | (idx << 7) | (pCodecs->channels << 3);
	data[0] = (csd >> 8);
	data[1] = (csd);

    RTMPPacket packet;
	RTMPPacket_Alloc(&packet, 2 + 2);

    packet.m_packetType 	= RTMP_PACKET_TYPE_AUDIO;
    packet.m_nBodySize  	= 2 + 2;
    packet.m_nChannel   	= 0x05;
    packet.m_nTimeStamp 	= pts - m_ref;
    packet.m_hasAbsTimestamp= 0;
    packet.m_headerType 	= RTMP_PACKET_SIZE_LARGE;
    packet.m_nInfoField2 	= m_rtmpctx.m_stream_id;

	memcpy(packet.m_body    , "\xAF\x00", 2);
    memcpy(packet.m_body + 2, data, 2);

	THIS_LOGI_print("SendAacConfig: csd=%02x%02x", data[0], data[1]);
    int iret = RTMP_SendPacket(&m_rtmpctx, &packet, 0);
	RTMPPacket_Free(&packet);
}

void CRtmpRecordThread::Send264Packet(AVPacket *pRawdata)
{//264: index[2] + cts[3] + nalu.len[4] + nalu.type[1] + nalu.data[n]
	uint8_t  type = pRawdata->data[4] & 0x1f;
	if( type == 7 ) Send264Config(pRawdata);
	if( pRawdata->size < 4 ) return;
	uint8_t *data = pRawdata->data + 4/*skip 0001*/; 
	int      size = pRawdata->size - 4;

    RTMPPacket packet;
	RTMPPacket_Alloc(&packet, 9 + size);

    packet.m_packetType 	= RTMP_PACKET_TYPE_VIDEO;
    packet.m_nBodySize  	= 9 + size;
    packet.m_nChannel   	= 0x04;
    packet.m_nTimeStamp 	= pRawdata->pts - m_ref;
    packet.m_hasAbsTimestamp= 0;
    packet.m_headerType 	= RTMP_PACKET_SIZE_MEDIUM;
    packet.m_nInfoField2 	= m_rtmpctx.m_stream_id;

	memcpy(packet.m_body    , (data[0]&0x1f)!=1? "\x17\x01":"\x27\x01", 2);
    memcpy(packet.m_body + 2, "\x00\x00\x00", 3);
	AMF_EncodeInt32(packet.m_body + 5, packet.m_body + 9, size);
    memcpy(packet.m_body + 9, data, size);

	THIS_LOGD_print("Send264Packet: pts=%u, len=%d, nalu.type=%d", packet.m_nTimeStamp, size, (int)data[0]&0x1f);
    int iret = RTMP_SendPacket(&m_rtmpctx, &packet, 1);
	RTMPPacket_Free(&packet);
}

void CRtmpRecordThread::SendAacPacket(AVPacket *pRawdata)
{//aac: index[2] + data[n]
	if( m_nframes[1] == 0 ) SendAacConfig(pRawdata->pts, m_pCodecs[3]);

	uint8_t *data = pRawdata->data;
	int      size = pRawdata->size;

    RTMPPacket packet;
	RTMPPacket_Alloc(&packet, 2 + size);

    packet.m_packetType 	= RTMP_PACKET_TYPE_AUDIO;
    packet.m_nBodySize  	= 2 + size;
    packet.m_nChannel   	= 0x05;
    packet.m_nTimeStamp 	= pRawdata->pts - m_ref;
    packet.m_hasAbsTimestamp= 0;
    packet.m_headerType 	= RTMP_PACKET_SIZE_MEDIUM;
    packet.m_nInfoField2 	= m_rtmpctx.m_stream_id;

	memcpy(packet.m_body    , "\xAF\x01", 2);
    memcpy(packet.m_body + 2, data, size);

	THIS_LOGD_print("SendAacPacket: pts=%u, len=%d", packet.m_nTimeStamp, size);
    int iret = RTMP_SendPacket(&m_rtmpctx, &packet, 1);
	RTMPPacket_Free(&packet);
}
