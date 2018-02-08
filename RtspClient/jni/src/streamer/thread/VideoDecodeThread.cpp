#include "player/Player.h"
#include "VideoDecodeThread.h"
#include "JNI_load.h"

CLASS_LOG_IMPLEMENT(CVideoDecodeThread, "CVideoDecodeThread");
///////////////////////////////////////////////////////////////////
void CVideoDecodeThread::Start(AVCodecContext *pCodecs, CBuffer *pNilframe)
{
	assert(m_pThread == 0);
	assert(m_pPlayer != 0);
	m_pCodecs = pCodecs;
	THIS_LOGI_print("do start to decode video thread");	
	int isucc = ::pthread_create(&m_pThread, NULL, CVideoDecodeThread::ThreadProc, this );
	pNilframe->GetPacket()->size = 0; //init
	Push(pNilframe);
}

void CVideoDecodeThread::Stop()
{
	if( m_pThread == 0 ) return;

	assert(m_pPlayer->GetStatus() == PS_Stopped);
	THIS_LOGI_print("do stop");
	Post();
	::pthread_join(m_pThread, 0);
	if( m_pDecode != 0 ) delete m_pDecode;
	m_pThread = 0; //必须复位
	m_pCodecs = 0;
	m_pDecode = 0; //必须复位
	DropFrames( );
}

///////////////////////////////////////////////////////////////////
bool CVideoDecodeThread::Push(CBuffer *pFrame)
{
//	THIS_LOGT_print("add frame[%p]: pts=%lld, data=%p, size=%d", pFrame, pFrame->GetPacket()->pts, pFrame->GetPacket()->data, pFrame->GetPacket()->size);
	m_mtxWaitDecodeFrames.Enter();
	m_lstWaitDecodeFrames.push_back(pFrame);
	m_nFrames++;
	m_mtxWaitDecodeFrames.Leave();
	Post();
	return true;
}

void CVideoDecodeThread::DropFrames()
{
	m_mtxWaitDecodeFrames.Enter();
	std::list<CBuffer*>::iterator it = m_lstWaitDecodeFrames.begin();
	while(it != m_lstWaitDecodeFrames.end())
	{
		CScopeBuffer pBuffer(*it);
		m_lstWaitDecodeFrames.erase(it ++);
		m_nFrames--;
	}
	m_mtxWaitDecodeFrames.Leave();
}

CBuffer *CVideoDecodeThread::Take(int64_t now, int a, int &val)
{
	CScopeMutex mutex(m_mtxWaitDecodeFrames);
	if( m_lstWaitDecodeFrames.empty() != 0 )
	{
		val = 0;
	}
	else
	{
		CBuffer *framef = m_lstWaitDecodeFrames.front();
		CBuffer *frameb = m_lstWaitDecodeFrames.back();
		int64_t pts = framef->GetPacket()->pts;
		val = pts - now - a;

		if( val > 0 &&
			frameb->GetPacket()->pts >= pts + m_pPlayer->m_nBufferInterval &&				
			m_pPlayer->m_duration == 0 && m_pPlayer->GetAudioFrameSize() == 0 )
		{
//			THIS_LOGW_print("adjust clock while got video frame[%d], pts=%lld, list=%d", val, pts, m_nFrames);		
			val = m_pPlayer->m_clock.Adjust(pts, 0);
		}

		if( val < 1 )
		{
			m_lstWaitDecodeFrames.pop_front();
			m_nFrames--;
			return framef;
		}
	}
	return NULL;
}


///////////////////////////////////////////////////////////////////
void *CVideoDecodeThread::ThreadProc(void *pParam)
{
	CVideoDecodeThread *pThread = (CVideoDecodeThread *)pParam;
	JNI_ATTACH_JVM(pThread->m_pJNIEnv);

	pThread->Loop();

	JNI_DETACH_JVM(pThread->m_pJNIEnv);
	return 0;
}

void CVideoDecodeThread::Loop()
{
	int adj = 0, val = 0;

	do{
	int status =  m_pPlayer->GetStatus( );
	if( status == PS_Stopped )
	{//播放结束
		THIS_LOGI_print("thread is exit");	
		break;
	}

	if( m_lstWaitDecodeFrames.empty() != false ||
		status == PS_Pause2 ||
		status == PS_Paused )
	{
		this->Wait(5/* 0.005 sec */ );
	}
	else
	{
		int64_t now = m_pPlayer->m_clock.Now();		
		CScopeBuffer pSrcframe(Take(now, adj, val));
		if( pSrcframe.p == NULL)
		{
			this->Wait(val>5? 5:val);
		}
		else
		{
			if( m_pDecode == 0 ) m_pDecode = CMediaDecoder::Create(m_pCodecs, m_pPlayer->m_hwmode!=1, m_pPlayer->m_hwmode!=3? 0:m_pPlayer->m_pSurfaceWindow);
			if( pSrcframe->GetPacket()->size &&
				m_pDecode != 0 )
			{
				CCostHelper cost; //cost of handle
				THIS_LOGD_print("show frame[%p]: pts=%lld, now=%lld, list=%d", pSrcframe.p, pSrcframe->GetPacket()->pts, now, m_nFrames);
				m_pPlayer->OnUpdatePosition(m_pPlayer->m_clock.Get(pSrcframe->GetPacket()->pts));
				Draw(m_pJNIEnv, pSrcframe.Detach());
				adj = cost.Get() / 1000;
			}
		}
	}
	}while(1);
}

void CVideoDecodeThread::Draw(JNIEnv *env, CBuffer *pRawframe)
{
#ifdef _CAST_SHOW
	COST_STAT_DECLARE(cost);
#endif
	
	CScopeBuffer pSrcdata(pRawframe);
	CScopeBuffer pYuvdata(m_pDecode->Decode(pRawframe));
	CScopeBuffer pRgbdata;
	CScopeBuffer p264data;

	if( m_pCodecs->codec_id == AV_CODEC_ID_H264 )
	{
		p264data.Attach(pSrcdata.Detach());
	}

	if( pYuvdata.p &&
		m_pPlayer->HasCodecid(0, MEDIA_DATA_TYPE_264) != 0 )
	{
		if( m_pEncode== 0 )
		{
			AVCodecContext *p264Codecs  = avcodec_alloc_context3(avcodec_find_encoder(AV_CODEC_ID_H264)); //AV_CODEC_ID_MJPEG/AV_CODEC_ID_H264
			p264Codecs->time_base		= m_pCodecs->time_base;
			p264Codecs->gop_size 		= 20;
			p264Codecs->max_b_frames	= 0;// no use b frame.
			av_opt_set(p264Codecs->priv_data, "preset"	    , "ultrafast"  , 0);
			av_opt_set(p264Codecs->priv_data, "tune" 	    , "zerolatency", 0);
//			av_opt_set(p264Codecs->priv_data, "x265-params" , "qp=20" 	   , 0);
			av_opt_set(p264Codecs->priv_data, "crf"			, "18"		   , 0);
			p264Codecs->qmin 			=  1;
			p264Codecs->qmax 			= 25;
			p264Codecs->bit_rate 		= 800000;
			p264Codecs->height 			= m_pCodecs->height;
			p264Codecs->width  			= m_pCodecs->width;
			p264Codecs->pix_fmt			= AV_PIX_FMT_YUV420P; //h264: AV_PIX_FMT_YUV420P/mjpg: AV_PIX_FMT_YUVJ420P
			p264Codecs->ticks_per_frame = 2;
			#ifdef _FFMPEG_THREAD
			p264Codecs->thread_count = 4;
			p264Codecs->thread_type =FF_THREAD_FRAME;
			#endif
			do{
			int ret = avcodec_open2(p264Codecs, p264Codecs->codec, NULL);
			if( ret < 0 )
			{
				THIS_LOGE_print("call avcodec_open2 return %d", ret);
				avcodec_free_context(&p264Codecs);
				break;
			}
   			CMediaEncoder *p264Encode = CMediaEncoder::Create(p264Codecs, m_pCodecs->pix_fmt);
			if( p264Encode == 0 )
			{
				avcodec_free_context(&p264Codecs);
				break;
			}
			else
			{
				m_p264Codecs = p264Codecs;
				m_pEncode    = p264Encode;					
			}
			}while(0);			
		}

		if( m_pEncode!= 0 )
		{
			p264data.Attach(m_pEncode->Encode(pYuvdata.p));
		}
	}

	if( pYuvdata.p )
	{
		if( m_pPlayer->m_isFirst )
		{
			Java_fireNotify(env, m_pPlayer, MEDIA_SHOWFIRSTFRAME, 0, 0, 0);
			m_pPlayer->m_isFirst = 0;
		}

		if( m_pPlayer->HasCodecid(0, MEDIA_DATA_TYPE_RGB) != 0 ||
			m_pPlayer->HasCodecid(1, MEDIA_DATA_TYPE_RGB) != 0 ||
			m_pPlayer->HasCodecid(2, MEDIA_DATA_TYPE_RGB) != 0 ||
			m_pPlayer->m_pSurfaceWindow )
		{//build rgb
			if( m_pScales == NULL) m_pScales = new CSwsScale(m_pCodecs->width, m_pCodecs->height, m_pCodecs->pix_fmt, m_pCodecs->width, m_pCodecs->height, AV_PIX_FMT_RGBA);
			pRgbdata.Attach(m_pScales->Scale(pYuvdata->GetPacket()->pts, pYuvdata->GetPacket()->data));
		}

		{//check
			Java_fireBuffer(env, m_pPlayer, MEDIA_DATA_TYPE_YUV, pYuvdata.Detach(), m_pCodecs->pix_fmt);
		}
	}

	if( pRgbdata.p )
	{
		if( m_pPlayer->m_pSurfaceWindow )
		{
			Java_drawNativeWindow(m_pPlayer->m_pSurfaceWindow, pRgbdata->GetPacket()->data, m_pCodecs->width, m_pCodecs->height);
		}

		{//check
			Java_fireBuffer(env, m_pPlayer, MEDIA_DATA_TYPE_RGB, pRgbdata.Detach());
		}
	}

	if( p264data.p )
	{
		uint8_t ctype = p264data->GetPacket()->data[4] & 0x1f; 
		if( ctype != 7 && ctype != 8 )
		{//skip sps/pps
			Java_fireBuffer(env, m_pPlayer, MEDIA_DATA_TYPE_264, p264data.Detach(), ctype);	
		}
	}

	if( pSrcdata.p &&
		m_pCodecs->codec_id == AV_CODEC_ID_MJPEG)
	{
		if( pSrcdata->GetPacket()->size )
		{
			Java_fireBuffer(env, m_pPlayer, MEDIA_DATA_TYPE_JPG, pSrcdata.Detach());	
		}
	}

#ifdef _CAST_SHOW
	static int delayCount = 0;
	static long long delayTime = 0;

	delayCount++;
	delayTime += cost.Get();
	if(delayCount >= 100)
	{
		THIS_LOGE_print("delayTime=%lld",delayTime);
		delayCount = 0;
		delayTime = 0;
	}
#endif
}
