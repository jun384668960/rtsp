#include "RtspMediaSink.h"

#define _LIVE_RTSP_AAC 1

CLASS_LOG_IMPLEMENT(CRtspMediaSink, "CRtspMediaSink");
///////////////////////////////////////////////////////////////////
CRtspMediaSink::CRtspMediaSink(CRtspClientPlayer *pRtspClientPlayer, char const *streamId)
  : MediaSink(*pRtspClientPlayer->m_pUsageEnvironment ), m_pPlayer(pRtspClientPlayer), m_gop(0), m_pts(-1)
{
	THIS_LOGT_print("is created: %s, bitrate: %dbps", m_pPlayer->m_pMediaSubsession->codecName(), m_pPlayer->m_pMediaSubsession->bandwidth());
	m_pPlayer->m_idr = true;
}

///////////////////////////////////////////////////////////////////
Boolean CRtspMediaSink::continuePlaying()
{
	if( fSource == NULL ) return False; // sanity check (should not happen)

	// Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
	if( m_pBuffer.p == 0) 
	{
		m_pBuffer.Attach(m_pPlayer->m_pBufferManager->Pop());
		int s = m_pBuffer->Skip(4); /*留出4个字节填充avc.h264帧前缀*/
	}

//	LOGT_print(s_logger, "%p set Buffer: %p, data=%p, size=%d", this, m_pBuffer.p, m_pBuffer->GetPacket()->data, m_pBuffer->GetPacket()->size);
	fSource->getNextFrame(m_pBuffer->GetPacket()->data, m_pBuffer->GetPacket()->size, CRtspMediaSink::GotFrameProc, this, onSourceClosure, this);
	return True;
}

///////////////////////////////////////////////////////////////////
void CRtspMediaSink::GotFrameProc(void* clientData, unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds)
{
	CRtspMediaSink *pMediaSink = (CRtspMediaSink  *)clientData;
	if( pMediaSink->m_pPlayer->GetStatus() == PS_Buffering2 )
	{
		if( pMediaSink->m_pPlayer->m_nSeekpos ==-2 )
		{//pause->play
			pMediaSink->m_pPlayer->SetStatus(PS_Playing   );
		}
		if( pMediaSink->m_pPlayer->m_nSeekpos ==-1 )
		{
			LOGW_print(s_logger, "%p Received media while wait first PLAY ack packet: lost PLAY ack packet", pMediaSink);
			pMediaSink->m_pPlayer->SetStatus(PS_Bufferingv);
		}
	}

	if( pMediaSink->OnFrame(frameSize, presentationTime, durationInMicroseconds) == false )
	{
//		LOGT_print(s_logger, "%p drop frame: %p", pMediaSink, pMediaSink->m_pBuffer.p);	
		pMediaSink->m_pBuffer.Release(); //reset size because size may be modified
	}

	// Then continue, to request the next frame of data:  
	pMediaSink->continuePlaying();
}

bool CRtspMediaSink::OnFrame(unsigned frameSize, struct timeval &presentationTime, unsigned durationInMicroseconds)
{//不能丢弃任何数据帧: 只需区分音视频帧，然后投递到对应解码线程
	m_pPlayer->m_nCycles = 0; //detect no stream

	if( m_pPlayer->m_STREAMING_METHOD == 2 &&
		m_pPlayer->m_nSeekpos >= 0 )
	{
		return false;
	}

	AVPacket *pPacket = m_pBuffer->GetPacket();
	pPacket->size= frameSize;
	pPacket->dts = pPacket->pts = (int64_t)presentationTime.tv_sec * 1000 + presentationTime.tv_usec / 1000; //ms

//	THIS_LOGT_print("got frame: %p, pts=%lld, data=%p, size=%d", m_pBuffer.p, pPacket->pts, pPacket->data, pPacket->size);
	int status = m_pPlayer->GetStatus();
	uint8_t *frame = pPacket->data;

	#if _LIVE_RTSP_AAC
	if( frame[0] == 0x00 && 
		frame[1] == 0x10 )
	{//audio aac
		if( m_pPlayer->m_audioctx == 0 )			
		{
			return false;
		}		
		else
		{// m_pPlayer->m_audioctx->codec_id == AV_CODEC_ID_AAC
			if( status == PS_Bufferinga) m_pPlayer->SetStatus(PS_Playing, pPacket->pts, m_pPlayer->m_nBufferInterval);
			if( status == PS_Bufferingv)
			{
				return false;
			}

			m_pBuffer->Skip( 4 ); //跳过4个字节的au头长度	
			THIS_LOGT_print("find aac: pts=%lld, data=%p, size=%d", pPacket->pts, pPacket->data, pPacket->size);

			pPacket->duration = m_pPlayer->m_audioctx->frame_size;
			if( m_pPlayer->m_duration == 0 &&
				m_pPlayer->GetVideoFrameSize() == 0 &&
				m_pPlayer->GetAudioFrameSize() == 0 &&
				m_pPlayer->m_clock.Now() > pPacket->pts ) 
			{
				m_pPlayer->m_clock.Adjust( pPacket->pts, m_pPlayer->m_nBufferInterval );
//				THIS_LOGW_print("adjust clock, pts=%lld, ref=%lld", pPacket->pts, m_pPlayer->m_clock.ref);
			}

			return m_pPlayer->m_pAudioDecodeThread->Push(m_pBuffer.Detach());
		}
	}
	else
	#endif
	{
		if( m_pPlayer->m_videoctx == 0 )
		{
			return false;
		}
		else
		{
			if( m_pPlayer->m_videoctx->codec_id == AV_CODEC_ID_MJPEG)
			{//video JPEG		
				if( m_pPlayer->m_duration == 0 &&
					status == PS_Playing   )
				{
					if( m_pPlayer->GetVideoFrameSize() > m_pPlayer->m_VIDEOMFPS )
					{
						return false;
					}
				}
				if( status == PS_Bufferingv) m_pPlayer->SetStatus(PS_Playing, pPacket->pts, m_pPlayer->m_nBufferInterval);			
			}
			else
			if( m_pPlayer->m_videoctx->codec_id != AV_CODEC_ID_H264 )
			{
				return false;
			}
			else
			{//video H264
				int H264_TYPE_POS = memcmp(pPacket->data, "\x00\x00\x00\x01", 4)==0? 4:0;
				uint8_t ctype = pPacket->data[H264_TYPE_POS] & 0x1f;
				THIS_LOGD_print("find 264: pts=%lld, nalu.type=%d, ipos=%d, size=%d", pPacket->pts, (int)ctype, (int)H264_TYPE_POS, pPacket->size);
				if( ctype == 7 )
				{
					const char *sps = (char*)pPacket->data + H264_TYPE_POS;
					int len = nalu_find_prefix(sps, pPacket->size - H264_TYPE_POS);
					if( m_pPlayer->m_videoSps.empty() ||
						memcmp(m_pPlayer->m_videoSps.c_str() + 4, sps, len) != 0 )
					{
						THIS_LOGW_print("set video sps=%d/%d", len, m_pPlayer->m_videoSps.size());
						int hasPps = m_pPlayer->m_videoSps.empty()!=false&&m_pPlayer->m_videoPps.empty()==false? true:false;
						m_pPlayer->SetVideoSpspps(0, 4, sps, len);
						m_pPlayer->m_pVideoDecodeThread->Push(m_pPlayer->GetVideoSpspps(0, m_pPlayer->m_pBufferManager->Pop(pPacket->pts)));
						if( hasPps ) m_pPlayer->m_pVideoDecodeThread->Push(m_pPlayer->GetVideoSpspps(1, m_pPlayer->m_pBufferManager->Pop(pPacket->pts)));
					}

					if( m_pBuffer->Skip(H264_TYPE_POS + len) < 4) return false;

					H264_TYPE_POS = memcmp(pPacket->data, "\x00\x00\x00\x01", 4)==0? 4:0;
					ctype = pPacket->data[H264_TYPE_POS] & 0x1f;
					THIS_LOGT_print("find nalu.type=%d size=%d after nalu.type=7", (int)ctype, pPacket->size);
				}
				if( ctype == 8 )
				{
					const char *pps = (char*)pPacket->data + H264_TYPE_POS;
					int len = nalu_find_prefix(pps, pPacket->size - H264_TYPE_POS);
					if( m_pPlayer->m_videoPps.empty() ||
						memcmp(m_pPlayer->m_videoPps.c_str() + 4, pps, len) != 0 )
					{
						THIS_LOGW_print("set video pps=%d/%d", len, m_pPlayer->m_videoPps.size());
						m_pPlayer->SetVideoSpspps(1, 4, pps, len);
						int hasSps = m_pPlayer->m_videoSps.empty()==false? true:false;
						if( hasSps ) m_pPlayer->m_pVideoDecodeThread->Push(m_pPlayer->GetVideoSpspps(1, m_pPlayer->m_pBufferManager->Pop(pPacket->pts)));
					}

					if( m_pBuffer->Skip(H264_TYPE_POS + len) < 4) return false;

					H264_TYPE_POS = memcmp(pPacket->data, "\x00\x00\x00\x01", 4)==0? 4:0;				
					ctype = pPacket->data[H264_TYPE_POS] & 0x1f;
					THIS_LOGT_print("find nalu.type=%d size=%d after nalu.type=8", (int)ctype, pPacket->size);		
				}
				if( ctype == 6 )
				{
					const char *sei = (char*)pPacket->data + H264_TYPE_POS;
					int len = nalu_find_prefix(sei, pPacket->size - H264_TYPE_POS);

					if( m_pBuffer->Skip(H264_TYPE_POS + len) < 4) return false;

					H264_TYPE_POS = memcmp(pPacket->data, "\x00\x00\x00\x01", 4)==0? 4:0;				
					ctype = pPacket->data[H264_TYPE_POS] & 0x1f;
					THIS_LOGT_print("find nalu.type=%d size=%d after nalu.type=6", (int)ctype, pPacket->size);						
				}

				if( status == PS_Bufferingv)
				{
					if( m_pPlayer->m_videoSps.empty() ||
						m_pPlayer->m_videoPps.empty() ||
						ctype != 5 )
					{
						return false; //skip frame as first IDR not arrived
					}
					else
					{
						m_pPlayer->SetStatus(PS_Playing, pPacket->pts, m_pPlayer->m_nBufferInterval);
						m_pPlayer->m_idr = false;
						m_gop = 0;
					}
				}
				else
				{
					{// calc gop size: need calc drop frame[P/B]
						m_gop ++;
					}

					if( m_pPlayer->m_duration == 0 &&
						status==PS_Playing )
					{
						if( m_pPlayer->GetVideoFrameSize() > m_pPlayer->m_VIDEOMFPS )
						{
							if( ctype != 5 )
							{
								m_pPlayer->m_idr = true;
							}
						}
						if( m_pPlayer->m_idr != false )
						{
							if( ctype != 5 )
							{
								THIS_LOGW_print("drop nalu.type=%d size=%d list=%d", (int)ctype, pPacket->size, m_pPlayer->GetVideoFrameSize());								
								return false;
							}
							else
							{
								m_pPlayer->m_idr = false;
							}
						}
					}

					if( ctype == 5 )
					{
						THIS_LOGI_print("calc gops.size=%d", m_gop);
						m_gop = 0;
					}
				}

				if( H264_TYPE_POS == 0 ) 
				{
					m_pBuffer->Skip(-4 ); //补上H264前导码
					memcpy(pPacket->data, "\x00\x00\x00\x01", 4);
				}
			}

			if( m_pPlayer->m_duration == 0 &&
				m_pPlayer->m_VIDEOFFPS ) 
			{// 平滑处理，只有直播/视频的情况
				if( m_pts < 0 )
					m_pts = pPacket->pts;
				else
					m_pts+= 1000/m_pPlayer->m_VIDEOFFPS;
				pPacket->pts = m_pts;
				pPacket->dts = m_pts;
			}

			pPacket->duration = 1;
			if( m_pPlayer->m_duration == 0 &&
				m_pPlayer->GetVideoFrameSize() == 0 &&
				m_pPlayer->GetAudioFrameSize() == 0 &&
				m_pPlayer->m_clock.Now() > pPacket->pts ) 
			{
				m_pPlayer->m_clock.Adjust( pPacket->pts, m_pPlayer->m_nBufferInterval );
//				THIS_LOGW_print("adjust clock, pts=%lld, ref=%lld", pPacket->pts, m_pPlayer->m_clock.ref);				
			}

			return m_pPlayer->m_pVideoDecodeThread->Push(m_pBuffer.Detach());
		}
	}
}
