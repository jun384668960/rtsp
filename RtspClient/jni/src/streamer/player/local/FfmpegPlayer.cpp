#include "FfmpegPlayer.h"
#include "JNI_load.h"

CLASS_LOG_IMPLEMENT(CFfmpegPlayer, "CPlayer[local]");
////////////////////////////////////////////////////////////////////////////////
CFfmpegPlayer::CFfmpegPlayer(const char *file)
  : m_feof(0), m_astream(-1), m_vstream(-1), m_avfmtctx(0), m_pThread(0), m_pJNIEnv(0)
{
	THIS_LOGT_print("is created of %s", file);
	int ret = avformat_open_input(&m_avfmtctx, file, NULL, NULL);
	if( ret < 0 )
	{
		#ifdef _DEBUG
		char buf[128];
	    av_strerror(ret, buf, 128);
		THIS_LOGE_print("call avformat_open_input return %d, error: %s", ret, buf);
		#endif
		return;
	}

	if((ret = avformat_find_stream_info(m_avfmtctx, 0)) < 0 ) 
	{
		THIS_LOGE_print("call avformat_find_stream_info return %d", ret);
		return;
	}

	for(int i = 0; i < m_avfmtctx->nb_streams; i ++) 
	{
		if( m_avfmtctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO ) m_astream = i;
		else if( m_avfmtctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO ) m_vstream = i;
	}

	if( m_vstream != -1 )
	{
		m_videoctx = m_avfmtctx->streams[m_vstream]->codec; 
		const AVCodec *vcodec = avcodec_find_decoder(m_videoctx->codec_id);
		#ifdef _FFMPEG_THREAD
		m_videoctx->thread_count = 4;
		m_videoctx->thread_type = FF_THREAD_SLICE;	
		#endif
		if((ret = avcodec_open2(m_videoctx, vcodec, NULL)) < 0) 
		{// Could not open video codec
			THIS_LOGW_print("call avcodec_open2 return %d", ret);
			m_vstream = -1;
		}
		else
		{
			THIS_LOGI_print("get videoctx=%p, codecid=%d from stream[%d]", m_videoctx, m_videoctx->codec_id, m_vstream);
		}
	}
	if( m_astream != -1 )
	{
		m_audioctx = m_avfmtctx->streams[m_astream]->codec; 	
		const AVCodec *acodec = avcodec_find_decoder(m_audioctx->codec_id); 
		if((ret = avcodec_open2(m_audioctx, acodec, NULL)) < 0) 
		{// Could not open audio codec
			THIS_LOGW_print("call avcodec_open2 return %d", ret);
			m_astream = -1;
		}
		else
		{
			THIS_LOGI_print("get audioctx=%p, codecid=%d from stream[%d]", m_audioctx, m_audioctx->codec_id, m_astream);
		}
	}
}

CFfmpegPlayer::~CFfmpegPlayer()
{
	if( m_avfmtctx ) {
		if( m_vstream != -1 ) avcodec_close( m_videoctx );
		if( m_astream != -1 ) avcodec_close( m_audioctx );
		avformat_close_input(&m_avfmtctx );
	}
	THIS_LOGT_print("is deleted");	
}

////////////////////////////////////////////////////////////////////////////////
int CFfmpegPlayer::Prepare()
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
		int iret = ::pthread_create(&m_pThread, NULL, CFfmpegPlayer::ThreadProc, this );
		return 0;
	}
}

int CFfmpegPlayer::Play() 
{
	if( GetStatus() != PS_Paused ) 
	{
		THIS_LOGW_print("Play error: invalid status[%d]", m_nStatus);
		return 1; //invalid status
	}
	if( m_vstream == -1 &&
		m_astream == -1 )
	{
		return 2; //invalid file
	}

	if( m_feof &&		
		GetAudioFrameSize() + GetVideoFrameSize() == 0 )
	{ //do replay
		THIS_LOGI_print("do rewind");	
		avformat_seek_file(m_avfmtctx, -1, 0, 0, INT64_MAX, AVSEEK_FLAG_FRAME);
		m_position = 0;
		m_feof     = 0;
	}
	THIS_LOGD_print("do Play...feof: %d", m_feof);
	if( m_feof )
		SetStatus(PS_Playing);
	else
		SetStatus(m_vstream==-1? PS_Bufferinga:PS_Bufferingv);
	return 0;
}

int CFfmpegPlayer::Pause()
{
	if( GetStatus() != PS_Playing ) 
	{
		THIS_LOGW_print("Pause error: invalid status[%d]", m_nStatus);
		return 1; //invalid status
	}
	else
	{
		THIS_LOGD_print("do Pause...");
		SetStatus(PS_Paused);
		return 0;
	}
}

int CFfmpegPlayer::Seek(int seek/*时间偏移，毫秒*/)
{
	if( m_astream == -1 &&
		m_vstream == -1 ) return 2;

	THIS_LOGD_print("do Seek: %d", seek);
	assert(seek >= 0);
	m_nSeekpos = seek;
	return 0;
}

int CFfmpegPlayer::Stop()
{
	if( GetStatus() == PS_Stopped ) return 0;

	THIS_LOGD_print("do Stop...");
	SetStatus( PS_Stopped);
	assert(m_pThread != 0);
	::pthread_join(m_pThread, 0); 
	m_pThread = 0;
	m_pVideoDecodeThread->Stop();
	m_pAudioDecodeThread->Stop();
	return 0;
}

int CFfmpegPlayer::GetVideoConfig(int *fps, int *videow, int *videoh)
{
	if( m_vstream == -1 ) return 1;

	if( fps    ) *fps    = av_q2d(av_guess_frame_rate(m_avfmtctx, m_avfmtctx->streams[m_vstream], NULL));
	if( videow ) *videow = m_videoctx->width;
	if( videoh ) *videoh = m_videoctx->height;
	return 0;
}

int CFfmpegPlayer::GetAudioConfig(int *channels, int *depth, int *samplerate)
{
	if( m_astream == -1 ) return 1;

	if( channels   ) *channels   = m_audioctx->channels;
	if( depth      ) *depth      = m_audioctx->sample_fmt==AV_SAMPLE_FMT_U8? 8:16;
	if( samplerate ) *samplerate = m_audioctx->sample_rate;
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
void *CFfmpegPlayer::ThreadProc(void *pParam)
{
	CFfmpegPlayer *pThread = (CFfmpegPlayer *)pParam;
	JNI_ATTACH_JVM(pThread->m_pJNIEnv);

	pThread->Loop();

	JNI_DETACH_JVM(pThread->m_pJNIEnv);	
	return 0;
}

void CFfmpegPlayer::Loop()
{
	int aps = 0, fps = 0;

	do{
	int status = GetStatus( );
	if( status == PS_Stopped)
	{
		THIS_LOGI_print("thread is exit");
		break;
	}

	if( m_nSeekpos >= 0 )
	{
		int idx = m_vstream!=-1? m_vstream:m_astream;
		const AVStream *pStream = m_avfmtctx->streams[idx];
		const int64_t skips = av_rescale(m_nSeekpos, pStream->time_base.den, pStream->time_base.num);
		int ret = avformat_seek_file(m_avfmtctx, idx, 0, skips/1000, INT64_MAX, AVSEEK_FLAG_FRAME);
		THIS_LOGT_print("call avformat_seek_file(%d) return %d", m_nSeekpos, ret);
		m_pAudioDecodeThread->DropFrames();
		m_pVideoDecodeThread->DropFrames();
		m_position = m_nSeekpos; //refix position
		m_nSeekpos = -1;
		m_feof = 0;
		if( status == PS_Playing ) status = SetStatus(m_vstream==-1? PS_Bufferinga:PS_Bufferingv);		
		Java_fireNotify(m_pJNIEnv, this, ret<0? MEDIA_ERR_SEEK:MEDIA_SEEK_COMPLETED, 0, 0, 0);
	}

	if( status == PS_Prepare)
	{
		if( m_astream == -1 &&
			m_vstream == -1 )
		{// prepare fail
			status = SetStatus(PS_Stopped);		
			Java_fireNotify(m_pJNIEnv, this, MEDIA_ERR_PREPARE   , 0, 0, 0);
			break;
		}
		else
		{// prepare succ
			if( m_vstream != -1 ) fps = 100 * av_q2d(av_guess_frame_rate(m_avfmtctx, m_avfmtctx->streams[m_vstream], NULL));
			if( m_astream != -1 ) aps = 100 * m_audioctx->sample_rate / m_audioctx->frame_size;
			m_duration = m_avfmtctx->duration / 1000; //ms
			#ifdef _DEBUG
			THIS_LOGI_print("get media duration=%lld, bitrate=%lld", m_avfmtctx->duration, m_avfmtctx->bit_rate);
			if( m_vstream != -1 ) {
			const AVStream *pStream = m_avfmtctx->streams[m_vstream];
			THIS_LOGI_print("get video(%dX%d), bitrate=%lld, base=%d/%d, fps=%.2f, val=%d, nb_frames=%lld, duration=%lld", m_videoctx->width, m_videoctx->height, m_videoctx->bit_rate, pStream->time_base.num, pStream->time_base.den, fps/100.0, m_videoctx->qmax, pStream->nb_frames, pStream->duration);
			}
			if( m_astream != -1 ) {
			const AVStream *pStream = m_avfmtctx->streams[m_astream];
			THIS_LOGI_print("get audio, bitrate=%lld, base=%d/%d, channels=%d, sample_rate=%d, frame_size=%d, nb_frames=%lld, duration=%lld", m_audioctx->bit_rate, pStream->time_base.num, pStream->time_base.den, m_audioctx->channels, m_audioctx->sample_rate, m_audioctx->frame_size, pStream->nb_frames, pStream->duration);
			}
			#endif
			status = SetStatus(PS_Paused);
			m_pBufferManager = new CBufferManager( 0/*don't malloc*/, fps/100 + aps/100 );	
			if( m_astream != -1 ) m_pAudioDecodeThread->Start(m_audioctx, m_pBufferManager->Pop());				
			if( m_vstream != -1 ) m_pVideoDecodeThread->Start(m_videoctx, m_pBufferManager->Pop());
			Java_fireNotify(m_pJNIEnv, this, MEDIA_PREPARED      , 0, 0, 0);

			if( m_vstream != -1 &&
				m_videoctx->codec_id == AV_CODEC_ID_H264 && m_videoctx->extradata != 0 ) {
				uint8_t *src = m_videoctx->extradata + 5;
				{//sps
				src += 1;
				int len = ((int)src[0] << 8) + src[1]; 
				src += 2;
				SetVideoSpspps(0, 4, src, len); src += len;
				}
				{//pps
				src += 1;
				int len = ((int)src[0] << 8) + src[1]; 
				src += 2;
				SetVideoSpspps(1, 4, src, len);
				}

				m_pVideoDecodeThread->Push(GetVideoSpspps(0, m_pBufferManager->Pop()));
				m_pVideoDecodeThread->Push(GetVideoSpspps(1, m_pBufferManager->Pop()));
			}
		}
	}
	if( status == PS_Playing &&
		m_feof != 0 &&
		GetAudioFrameSize() + GetVideoFrameSize() == 0 )		
	{
		m_isFirst = 1; //reset
		status = SetStatus(PS_Paused);
		THIS_LOGW_print("play file complete, duration: %d", m_duration);
		Java_fireNotify(m_pJNIEnv, this, MEDIA_PLAY_COMPLETED, 0, 0, 0);			
	}
	if( status == PS_Paused ||
		m_feof != 0 ||
		GetAudioFrameSize() + GetVideoFrameSize() > fps/100 + aps/100 )
	{
		usleep(5000/* 5ms */);
	}
	else
	{//PS_Buffering/PS_Playing
		CScopeBuffer pBuffer(m_pBufferManager->Pop());	
		AVPacket    *pPacket = pBuffer->GetPacket();
		int iret = av_read_frame(m_avfmtctx, pPacket);
		if( iret < 0 )
		{
			if((iret == AVERROR_EOF) || avio_feof(m_avfmtctx->pb))
			{
				THIS_LOGW_print("mark feof: 1");
				m_feof = 1;
			}
			else
			{
				THIS_LOGE_print("call av_read_frame return %d", iret);
				status = SetStatus(PS_Paused);				
				Java_fireNotify(m_pJNIEnv, this, MEDIA_ERR_PLAY      , 0, 0, 0);
			}
		}
		else
		{
			pPacket->pts = pPacket->pts * 1000 * av_q2d(m_avfmtctx->streams[pPacket->stream_index]->time_base);
			pPacket->dts = pPacket->dts * 1000 * av_q2d(m_avfmtctx->streams[pPacket->stream_index]->time_base);

			if( status== PS_Bufferinga ||
				status== PS_Bufferingv ) SetStatus(PS_Playing, 0, m_nBufferInterval - pPacket->pts);

			if( pPacket->stream_index == m_vstream )
			{
				THIS_LOGT_print("recv video[%p]: pts=%lld, dts=%lld, duration=%lld, data=%p, size=%d, list=%d", pBuffer.p, pPacket->pts, pPacket->dts, pPacket->duration, pPacket->data, pPacket->size, GetVideoFrameSize());
				if( m_videoctx->codec_id == AV_CODEC_ID_H264 )
				{
					uint8_t ctype = pPacket->data[4]&0x1f;
					if( ctype == 7/* SPS */)
					{
						int len = ((int)pPacket->data[0] << 24) | ((int)pPacket->data[1] << 16) | ((int)pPacket->data[2] << 8) | (int)pPacket->data[3];
						if( len > pPacket->size - 4 ) continue;
						SetVideoSpspps(0, 4, pPacket->data + 4, len);
						m_pVideoDecodeThread->Push(GetVideoSpspps(0, m_pBufferManager->Pop(pPacket->pts)));
						if( pBuffer->Skip(4 + len) < 4 ) continue;
						ctype = pPacket->data[4]&0x1f;
					}
					if( ctype == 8/* PPS */)
					{
						int len = ((int)pPacket->data[0] << 24) | ((int)pPacket->data[1] << 16) | ((int)pPacket->data[2] << 8) | (int)pPacket->data[3];
						if( len > pPacket->size - 4 ) continue;						
						SetVideoSpspps(1, 4, pPacket->data + 4, len);
						m_pVideoDecodeThread->Push(GetVideoSpspps(1, m_pBufferManager->Pop(pPacket->pts)));
						if( pBuffer->Skip(4 + len) < 4 ) continue;
						ctype = pPacket->data[4]&0x1f;
					}
					if( ctype == 6/* SEI */)
					{
						int len = ((int)pPacket->data[0] << 24) | ((int)pPacket->data[1] << 16) | ((int)pPacket->data[2] << 8) | (int)pPacket->data[3];
						if( len > pPacket->size - 4 ) continue;						
						if( pBuffer->Skip(4 + len) < 4 ) continue;
						ctype = pPacket->data[4]&0x1f;
					}

					unsigned int pos = 0;
					while(pos + 4 < pPacket->size) {
						int len = ((int)pPacket->data[pos] << 24) + ((int)pPacket->data[pos + 1] << 16) + ((int)pPacket->data[pos + 2] << 8) + pPacket->data[pos + 3];
						memcpy(pPacket->data + pos, "\x00\x00\x00\x01", 4); 
						pos += 4 + len;
					}
				}
				pPacket->duration = 1;
				m_pVideoDecodeThread->Push(pBuffer.Detach());
			}
			if( pPacket->stream_index == m_astream )
			{
				THIS_LOGT_print("recv audio[%p]: pts=%lld, dts=%lld, duration=%lld, data=%p, size=%d, list=%d", pBuffer.p, pPacket->pts, pPacket->dts, pPacket->duration, pPacket->data, pPacket->size, GetAudioFrameSize());
				m_pAudioDecodeThread->Push(pBuffer.Detach());
			}
		}
	}
	}while(1);
}
