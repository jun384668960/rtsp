#include "RtmpClientPlayer.h"
#include "JNI_load.h"
#include <pthread.h>

CLASS_LOG_IMPLEMENT(CRtmpClientPlayer, "CPlayer[rtmp]");
////////////////////////////////////////////////////////////////////////////////
CRtmpClientPlayer::CRtmpClientPlayer(const char *uri)
  : m_pThread(0), m_pJNIEnv(0), m_uri(uri), m_feof(false)
{
	THIS_LOGT_print("is created of %s", uri);
	m_MEDIASINK_RECV_BUFFER_SIZE = 512 * 1024; //512K, 过小导致接收丢到了I帧，从而引发马赛克	
	m_VIDEOMFPS = 30;
	RTMP_Init(&m_rtmpctx);
}

CRtmpClientPlayer::~CRtmpClientPlayer()
{
	if( m_videoctx != 0 ) avcodec_free_context(&m_videoctx); //will call avcodec_close
	if( m_audioctx != 0 ) avcodec_free_context(&m_audioctx); //will call avcodec_close
	THIS_LOGT_print("is deleted");	
}

////////////////////////////////////////////////////////////////////////////////
int CRtmpClientPlayer::Prepare()
{
	if( GetStatus() != PS_Stopped ) 
	{
		THIS_LOGW_print("Prepare error: invalid status[%d]", m_nStatus);
		return 1; //invalid status
	}
	else
	{
		THIS_LOGD_print("do Prepare...");		
		assert(m_pThread == 0);
		SetStatus( PS_Prepare);
		int iret = ::pthread_create(&m_pThread, NULL, CRtmpClientPlayer::ThreadProc, this );
		return 0;
	}
}

int CRtmpClientPlayer::Play() 
{
	if( GetStatus() != PS_Paused ) 
	{
		THIS_LOGW_print("Play error: invalid status[%d]", m_nStatus);
		return 1; //invalid status
	}

	THIS_LOGD_print("do Play...");
	SetStatus(m_rtmpctx.m_pauseStamp!=0? PS_Buffering1:PS_Bufferingv);
//	m_rtmpctx.m_sb.sb_ctrl = 1;
	return 0;
}

int CRtmpClientPlayer::Pause()
{
	if( GetStatus() != PS_Playing ) 
	{
		THIS_LOGW_print("Pause error: invalid status[%d]", m_nStatus);
		return 1; //invalid status
	}
	else
	{
		THIS_LOGD_print("do Pause...");
		SetStatus(PS_Pause1);
//		m_rtmpctx.m_sb.sb_ctrl = 1;
		return 0;
	}
}

int CRtmpClientPlayer::Seek(int seek/*时间偏移，毫秒*/)
{
	if( m_duration == 0 ) 
	{
		THIS_LOGW_print("Seek error: can't seek while live");
		return 2;
	}

	THIS_LOGD_print("do Seek: %d", seek);
	assert(seek >= 0 &&
		   seek < m_duration);
	m_nSeekpos = seek;
	return 0;
}

int CRtmpClientPlayer::Stop()
{
	if( GetStatus() == PS_Stopped ) return 0;

	THIS_LOGD_print("do Stop...");
	SetStatus( PS_Stopped);
	assert(m_pThread != 0);
	m_rtmpctx.m_sb.sb_ctrl = 1;
	::pthread_join(m_pThread, 0); 
	m_pThread = 0;
	m_pVideoDecodeThread->Stop();
	m_pAudioDecodeThread->Stop();
	return 0;
}

int CRtmpClientPlayer::GetVideoConfig(int *fps, int *videow, int *videoh)
{
	if( m_videoctx == 0 ) return 1;

	if( fps    ) *fps    = m_rtmpctx.m_fps;
	if( videow ) *videow = m_videoctx->width;
	if( videoh ) *videoh = m_videoctx->height;
	return 0;
}

int CRtmpClientPlayer::GetAudioConfig(int *channels, int *depth, int *samplerate)
{
	if( m_audioctx == 0 ) return 1;

	if( channels   ) *channels   = m_audioctx->channels;
	if( depth      ) *depth      = m_audioctx->sample_fmt==AV_SAMPLE_FMT_U8? 8:16;
	if( samplerate ) *samplerate = m_audioctx->sample_rate;
	return 0;
}

int CRtmpClientPlayer::SetParamConfig(const char *key, const char *val)
{
	if( strcmp(key, "buffersize") == 0 )
	{
		m_MEDIASINK_RECV_BUFFER_SIZE = atoi(val);
		return 0;
	}
	if( strcmp(key, "videomfps") == 0 )
	{
		m_VIDEOMFPS = atoi(val);
		return 0;
	}

	return 1;
}


////////////////////////////////////////////////////////////////////////////////
void *CRtmpClientPlayer::ThreadProc(void *pParam)
{
	CRtmpClientPlayer *pThread = (CRtmpClientPlayer *)pParam;
	JNI_ATTACH_JVM(pThread->m_pJNIEnv);

	pThread->Loop();

	JNI_DETACH_JVM(pThread->m_pJNIEnv);	
	return 0;
}

void CRtmpClientPlayer::Loop()
{
/*	struct sched_param param;
	int policy;

	pthread_getschedparam(pthread_self(), &policy, &param);
	param.sched_priority = sched_get_priority_max(policy); policy = SCHED_RR;
	pthread_setschedparam(pthread_self(),  policy, &param); */
	if( m_pBufferManager == 0 ) m_pBufferManager = new CBufferManager( m_MEDIASINK_RECV_BUFFER_SIZE, 36 );					

	RTMPPacket cPacket = {0};
	int bidr = 0;

	do{
	int status = GetStatus( );
	if( status == PS_Stopped)
	{
		RTMP_Close(&m_rtmpctx);
		THIS_LOGI_print("thread is exit");
		break;
	}

	if( status == PS_Prepare &&
		m_rtmpctx.m_bPlaying == 0 )
	{
		int isucc = 0;
		do{
		if( RTMP_SetupURL(&m_rtmpctx, (char*)m_uri.c_str()) == 0)
		{
			THIS_LOGE_print("call RTMP_SetupURL return 0");
			break;
		}
		if( std::string::npos == m_uri.rfind('.')) m_rtmpctx.Link.lFlags |= RTMP_LF_LIVE;
		if( RTMP_Connect(&m_rtmpctx, 0) == 0)
		{
			THIS_LOGE_print("call RTMP_Connect return 0");
			break;
		}
	  	if( RTMP_ConnectStream(&m_rtmpctx, m_nSeekpos<0? 0:m_nSeekpos) == 0)
  		{
			THIS_LOGE_print("call RTMP_ConnectStream return 0");  		
  			break;
  		}
		else
		{
			THIS_LOGT_print("call RTMP_ConnectStream succ");
			m_nSeekpos = -1;
			isucc = 1;
		}
		}while(0);
		if( isucc == 0 )
		{
			status = SetStatus(PS_Stopped); 	
			Java_fireNotify(m_pJNIEnv, this, MEDIA_ERR_PREPARE	 , 0, 0, 0);
			break;
		}
	}

	if( status == PS_Playing &&
		GetAudioFrameSize() + GetVideoFrameSize() == 0 )
	{
		if( m_feof != 0 )
		{
			m_isFirst = 1; //reset
			status = SetStatus(PS_Paused);
			if( m_duration != 0 ) {
			THIS_LOGW_print("play file complete, duration: %d", m_duration);
			Java_fireNotify(m_pJNIEnv, this, MEDIA_PLAY_COMPLETED, 0, 0, 0);
			}
		}
	}

	if( status == PS_Paused ||
		m_feof != 0 )
	{
		usleep(5000/* 5ms */);
		continue;
	}

	if( RTMP_IsConnected(&m_rtmpctx) == 0 || RTMP_IsTimedout(&m_rtmpctx) != 0 )
	{
		THIS_LOGE_print("rtmp[%d] is broke", m_rtmpctx.m_stream_id);	
		if( status == PS_Prepare)
		{
			status = SetStatus(PS_Stopped);
			Java_fireNotify(m_pJNIEnv, this, MEDIA_ERR_PREPARE	 , 0, 0, 0);
			break;
		}
		else
		{
			status = SetStatus(PS_Paused );
			Java_fireNotify(m_pJNIEnv, this, MEDIA_ERR_PLAY   	 , 0, 0, 0);
			continue;
		}
	}

	if( m_rtmpctx.m_bPlaying == 3 )
	{
		if( status == PS_Pause1    )
		{
			RTMP_Pause(&m_rtmpctx, 1 ); //do pause
			status = SetStatus(PS_Paused);
			continue;
		}

		if( status == PS_Buffering1)
		{
			RTMP_Pause(&m_rtmpctx, 0 ); //do resume
			status = SetStatus(PS_Bufferingv);
			continue;
		}

		if( m_nSeekpos >= 0 )
		{
			int iret = RTMP_SendSeek(&m_rtmpctx, m_nSeekpos );
			m_pAudioDecodeThread->DropFrames();
			m_pVideoDecodeThread->DropFrames();
			m_position = m_nSeekpos; //refix position
			m_nSeekpos = -1;
			m_feof = 0;
			if( status == PS_Playing ) status = SetStatus(PS_Bufferingv);		
			Java_fireNotify(m_pJNIEnv, this, iret<0? MEDIA_ERR_SEEK:MEDIA_SEEK_COMPLETED, 0, 0, 0);			
		}
	}

	if( m_rtmpctx.m_read.status < 0 )
	{
		if( m_rtmpctx.m_read.status == RTMP_READ_EOF ||
			m_rtmpctx.m_read.status == RTMP_READ_COMPLETE)
		{
			THIS_LOGW_print("mark feof: 1");			
			m_feof = 1; 
		}
		else
		{
			THIS_LOGE_print("rtmp occur error");
			status = SetStatus(PS_Paused);				
			Java_fireNotify(m_pJNIEnv, this, MEDIA_ERR_PLAY 	 , 0, 0, 0);			
		}
	}
	else
	{
		m_rtmpctx.m_sb.sb_ctrl = 0; //reset
        int iret = RTMP_ReadPacket(&m_rtmpctx, &cPacket);
		if( iret &&
			RTMPPacket_IsReady(&cPacket ) )
		{
			if( RTMP_ClientPacket(&m_rtmpctx, &cPacket ) != 0 )
			{
				if( cPacket.m_nBodySize == 0 )
				{					
					if( m_rtmpctx.m_videoCodecs != 0 &&
						m_videoctx == 0 &&
						cPacket.m_packetType == RTMP_PACKET_TYPE_VIDEO &&
						m_rtmpctx.m_sps.av_val > 0 && m_rtmpctx.m_sps.av_len < 512 && m_rtmpctx.m_pps.av_val > 0 && m_rtmpctx.m_pps.av_len < 256 )						
					{
						AVCodec *vcodec = avcodec_find_decoder(AV_CODEC_ID_H264);
						m_videoctx = avcodec_alloc_context3(vcodec);
						THIS_LOGI_print("get videoctx=%p, codecid=%d", m_videoctx, m_videoctx->codec_id);			
						m_videoctx->width	= m_rtmpctx.m_videoW;
						m_videoctx->height  = m_rtmpctx.m_videoH;
						m_videoctx->pix_fmt = AV_PIX_FMT_YUV420P;
						#ifdef _FFMPEG_THREAD
						m_videoctx->thread_count = 4;
						m_videoctx->thread_type = FF_THREAD_SLICE;
						#endif
						THIS_LOGT_print("video(%dx%d), %dfps", m_videoctx->width, m_videoctx->height, m_rtmpctx.m_fps);

						int iret = avcodec_open2(m_videoctx, vcodec, NULL);
						THIS_LOGT_print("call avcodec_open2 return %d", iret);
						m_pVideoDecodeThread->Start(m_videoctx, m_pBufferManager->Pop());	

						SetVideoSpspps(0, 4, m_rtmpctx.m_sps.av_val, m_rtmpctx.m_sps.av_len);
						SetVideoSpspps(1, 4, m_rtmpctx.m_pps.av_val, m_rtmpctx.m_pps.av_len);						

						m_pVideoDecodeThread->Push(GetVideoSpspps(0, m_pBufferManager->Pop()));
						m_pVideoDecodeThread->Push(GetVideoSpspps(1, m_pBufferManager->Pop()));
					}
					if( m_rtmpctx.m_audioCodecs != 0 &&
						m_audioctx == 0 &&
						cPacket.m_packetType == RTMP_PACKET_TYPE_AUDIO )
					{
						AVCodec *acodec = avcodec_find_decoder(AV_CODEC_ID_AAC);
						m_audioctx = avcodec_alloc_context3(acodec);
						THIS_LOGI_print("get audioctx=%p, codecid=%d", m_audioctx, m_audioctx->codec_id);			
						m_audioctx->bit_rate		= 128000; //128kbps
						m_audioctx->sample_rate		= m_rtmpctx.m_samplerate;  //44.1khz
						m_audioctx->frame_size 		= 1024;
						m_audioctx->sample_fmt 		= AV_SAMPLE_FMT_S16;	
						m_audioctx->channels		= m_rtmpctx.m_channel;
						m_audioctx->channel_layout 	= av_get_default_channel_layout(m_audioctx->channels);

						int iret = avcodec_open2(m_audioctx, acodec, NULL);
						THIS_LOGT_print("call avcodec_open2 return %d", iret);
						m_pAudioDecodeThread->Start(m_audioctx, m_pBufferManager->Pop());						
					}

					if( status == PS_Prepare )
					{
						m_duration = RTMP_GetDuration(&m_rtmpctx);
						if( m_duration > 0 && m_nBufferInterval < 300 ) m_nBufferInterval = 300;
						THIS_LOGI_print("get media duration=%d, acodecid=%d, vcodecid=%d", m_duration, m_rtmpctx.m_audioCodecs, m_rtmpctx.m_videoCodecs);
						status = SetStatus(PS_Paused);
						Java_fireNotify(m_pJNIEnv, this, MEDIA_PREPARED 	 , m_rtmpctx.m_bPlaying==3? (0):(cPacket.m_packetType==RTMP_PACKET_TYPE_AUDIO? 1:2), 0, 0);
					}
				}
				else
				{
					if((m_rtmpctx.m_bPlaying != 0) && 
					   (status != PS_Prepare) &&
					   (m_rtmpctx.m_read.flags & RTMP_READ_SEEKING) == 0 )
					{
						CScopeBuffer pBuffer(m_pBufferManager->Pop(cPacket.m_nTimeStamp));	
						AVPacket	*pPacket = pBuffer->GetPacket();

						if( status == PS_Bufferinga || status == PS_Bufferingv )
						{
							if( m_duration != 0 )
							SetStatus(PS_Playing, 0, m_nBufferInterval - pPacket->pts);
							else
							SetStatus(PS_Playing, pPacket->pts, m_nBufferInterval);
						}

						if( m_duration == 0 &&
							GetVideoFrameSize() == 0 &&
							GetAudioFrameSize() == 0 &&
							m_clock.Now() > pPacket->pts ) 
						{
							m_clock.Adjust( pPacket->pts, m_nBufferInterval );
//							THIS_LOGW_print("adjust clock, pts=%lld, ref=%lld", pPacket->pts, m_clock.ref);				
						}

						if( cPacket.m_packetType == RTMP_PACKET_TYPE_AUDIO &&
							cPacket.m_nBodySize > 2 &&
							m_rtmpctx.m_audioCodecs != 0 )
						{//aac: type[2] + data[n]
							int   size = cPacket.m_nBodySize - 2;
							char *data = cPacket.m_body + 2;							
							if( memcmp(cPacket.m_body, "\xAF\x01", 2) == 0 )
							{
								pBuffer->SetSize(size);								
								memcpy( pPacket->data, data, size ); 
								THIS_LOGT_print("recv audio[%p]: pts=%lld, data=%p, size=%d, list=%d", pBuffer.p, pPacket->pts, pPacket->data, pPacket->size, GetAudioFrameSize());
								pPacket->duration = m_audioctx->frame_size;
								m_pAudioDecodeThread->Push(pBuffer.Detach());
							}
						}

						if( cPacket.m_packetType == RTMP_PACKET_TYPE_VIDEO &&
							cPacket.m_nBodySize > 9 &&
							m_rtmpctx.m_videoCodecs != 0 )
						{//264: type[2] + cts[3] + nalu.len[4] + nalu.type[1] + nalu.data[n]	
							int   ncts = AMF_DecodeInt24(cPacket.m_body + 2); if( ncts > 0 ) pPacket->pts += ncts;
							int   size = AMF_DecodeInt32(cPacket.m_body + 5);
							char *nalu = cPacket.m_body + 9;
							int   type = nalu[0] & 0x1f;

							do{
							if( type== 6 ) 
							{//SEI
								nalu+= size; cPacket.m_nBodySize -= size;
								if( cPacket.m_nBodySize <= 9 + 5 ) break;
								size = AMF_DecodeInt32(nalu);
								nalu+= 4; //skip len
								type = nalu[0] & 0x1f;
							}

							if( size < 1 || size > cPacket.m_nBodySize- 9 ) break;
/*							if( m_duration == 0 &&
							    status == PS_Playing &&
								GetVideoFrameSize() > m_VIDEOMFPS )
							{
								bidr = 0;
								break;
							} *///È¡Ïû¼ì²âÖ±²¥ÊÓÆµÖ¡Êý
							if(!bidr ) 
							{
								if( type != 5 ) {
									THIS_LOGW_print("drop nalu.type=%d size=%d", (int)type, pPacket->size);								
									break;
								}
								bidr = 1;
							}

							pBuffer->SetSize(size + 4);
							memcpy( pPacket->data	, "\x00\x00\x00\x01", 4 );
							memcpy( pPacket->data + 4, nalu, size );
							THIS_LOGT_print("recv video[%p]: pts=%lld, cts=%u, data=%p, size=%d, nalu.type=%d, list=%d", pBuffer.p, pPacket->pts, ncts, pPacket->data, pPacket->size, type, GetVideoFrameSize());
							pPacket->duration = 1;
							m_pVideoDecodeThread->Push(pBuffer.Detach());
							}while(0);
						}
					}
				}
			}		

			RTMPPacket_Free(&cPacket);
		}		
	}
	}while(1);
}
