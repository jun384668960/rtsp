#include "player/Player.h"
#include "MediaRecorder.h"
#include "com_bql_MSG_Native.h"
#include "JNI_load.h"

extern JNIMediaRecorder g_jBQLMediaRecorder;
CLASS_LOG_IMPLEMENT(CMediaRecorder, "CMediaRecorder");
////////////////////////////////////////////////////////////////////////////////
int CMediaRecorder::SetVideoStream(int fps, int videow, int videoh, int qval, int bitrate)
{
	AVCodecContext *pCodecs = m_pCodecs[2];
	if( pCodecs == 0 ) return 1;

	if( pCodecs->codec_id == AV_CODEC_ID_H264 ) 
	{//for264
		av_opt_set(pCodecs->priv_data, "preset" 	, "ultrafast"  , 0);
		av_opt_set(pCodecs->priv_data, "tune"		, "zerolatency", 0);
//		av_opt_set(pCodecs->priv_data, "x265-params", "qp=20"	   , 0);
		av_opt_set(pCodecs->priv_data, "crf"		, "18"		   , 0);
		pCodecs->gop_size		= 20;
		pCodecs->max_b_frames	= 0; // no use b frame.
		pCodecs->qmin = 1;
		pCodecs->qmax = qval; //25
	}

	if( m_pPlayer &&
	    m_pPlayer->m_videoctx )
	{
		if( bitrate== 0 || bitrate > m_pPlayer->m_videoctx->bit_rate ) bitrate = m_pPlayer->m_videoctx->bit_rate/1000*1000;
		if( videow == 0 ) videow = m_pPlayer->m_videoctx->width;
		if( videoh == 0 ) videoh = m_pPlayer->m_videoctx->height;
	}

	{//common
		THIS_LOGI_print("video(%dX%d): fps=%d, qval=%d, bitrate=%d", videow, videoh, fps, qval, bitrate);		
		pCodecs->time_base  	= (AVRational){1, fps};
		pCodecs->bit_rate		= bitrate;
		pCodecs->height 		= videoh;
		pCodecs->width  		= videow;
		pCodecs->pix_fmt		= pCodecs->codec_id==AV_CODEC_ID_MJPEG? AV_PIX_FMT_YUVJ420P:AV_PIX_FMT_YUV420P;
		pCodecs->ticks_per_frame= 2;
	}
	return 0;
}

int CMediaRecorder::SetAudioStream(int channel, int depth, int samplerate, int bitrate)
{
	AVCodecContext *pCodecs = m_pCodecs[3];
	if( pCodecs == 0 ) return 1;

	if( m_pPlayer &&
	    m_pPlayer->m_audioctx )
	{
		if( bitrate == 0 || bitrate > m_pPlayer->m_audioctx->bit_rate ) bitrate = m_pPlayer->m_audioctx->bit_rate/1000*1000;
		if( channel == 0 ) channel = m_pPlayer->m_audioctx->channels;
		if( samplerate == 0 ) samplerate = m_pPlayer->m_audioctx->sample_rate;		
	}

	{//common
		THIS_LOGI_print("audio: channels=%d, depth=%d, samplerate=%d, bitrate=%d", channel, depth, samplerate, bitrate);	
		pCodecs->time_base		= (AVRational){1, samplerate};
		pCodecs->bit_rate		= bitrate;
		pCodecs->sample_rate	= samplerate;
		pCodecs->sample_fmt 	= depth==8? AV_SAMPLE_FMT_U8:AV_SAMPLE_FMT_S16;
		pCodecs->channels		= channel; 			
		pCodecs->channel_layout = av_get_default_channel_layout(pCodecs->channels);
	}
	return 0;
}

int CMediaRecorder::PrepareWrite()
{
	if( m_pPlayer != 0 ) {
		memset(m_pPlayer->m_medias + 1 * MAX_MEDIA_FMT, 0, MAX_MEDIA_FMT * sizeof(m_pPlayer->m_medias[0])); //zero rec subcribe
		const AVCodecContext *videoctx = m_pCodecs[2];
		if( videoctx != NULL ) {
		if( videoctx->width != m_pPlayer->m_videoctx->width || videoctx->height != m_pPlayer->m_videoctx->height ||
			videoctx->bit_rate/1000 < m_pPlayer->m_videoctx->bit_rate/1000 )
		{
			if( videoctx->width != m_pPlayer->m_videoctx->width || videoctx->height != m_pPlayer->m_videoctx->height ) m_pScales = new CSwsScale(m_pPlayer->m_videoctx->width, m_pPlayer->m_videoctx->height, m_pPlayer->m_videoctx->pix_fmt, videoctx->width, videoctx->height, m_pPlayer->m_videoctx->pix_fmt);
			m_pPlayer->SetCodecidEnabled(1, MEDIA_DATA_TYPE_YUV, true);
			THIS_LOGI_print("subscribe video: YUV");
		}
		else
		{
			m_pPlayer->SetCodecidEnabled(1, videoctx->codec_id==AV_CODEC_ID_MJPEG? MEDIA_DATA_TYPE_JPG:MEDIA_DATA_TYPE_264, true);
			THIS_LOGI_print("subscribe video: %s", videoctx->codec_id==AV_CODEC_ID_MJPEG? "JPG":"264");				
		}
		}
//		if( videoctx != NULL ) m_pPlayer->SetCodecidEnabled(1, MEDIA_DATA_TYPE_YUV, true);
		const AVCodecContext *audioctx = m_pCodecs[3];
		if( audioctx != NULL ) {
			m_pPlayer->SetCodecidEnabled(1, audioctx->codec_id!=m_pPlayer->m_audioctx->codec_id? MEDIA_DATA_TYPE_PCM:MEDIA_DATA_TYPE_AAC, true);
			THIS_LOGI_print("subscribe audio: %s", audioctx->codec_id!=m_pPlayer->m_audioctx->codec_id? "PCM":"AAC"); 
		}
//		if( audioctx != NULL ) m_pPlayer->SetCodecidEnabled(1, MEDIA_DATA_TYPE_PCM, true);
		m_pPlayer->m_pHook[0] = this;
	}

	assert(m_pThread == 0);
	THIS_LOGI_print("do start to record thread");	
	int succ = ::pthread_create(&m_pThread, NULL, CMediaRecorder::ThreadProc, this );
	return 0;
}

int CMediaRecorder::Write(JNIEnv *env, void *buffer)
{
	std::list<CBuffer*> lstWaitEncodeFrames;
	CScopeBuffer pBuffer((CBuffer *)buffer);
	int ret = 0;

	m_mtxWaitEncodeFrames.Enter();
	if( buffer == 0 ) lstWaitEncodeFrames.swap(m_lstWaitEncodeFrames);
	if( m_lstWaitEncodeFrames.empty() != 0 ||
		m_lstWaitEncodeFrames.front() != 0) //check it is mark exit
	{
		if( pBuffer.p != 0 ) ret = 1;
		m_lstWaitEncodeFrames.push_back(pBuffer.Detach());
	}
	m_mtxWaitEncodeFrames.Leave();
	ReleaseBuffers(lstWaitEncodeFrames);

	Post();
	return ret;
}

int CMediaRecorder::Stop()
{
	if( m_pThread == 0 ) return 0;

	THIS_LOGI_print("do stop");
	if( m_pPlayer != 0 ) m_pPlayer->m_pHook[0] = NULL;
	Write(0);
	::pthread_join(m_pThread, 0);
	m_pThread = 0; //必须复位	
	return 0;
}

void CMediaRecorder::Doinit(JNIEnv *env, jobject weak)
{
	m_owner = env->NewGlobalRef(weak);
}

void CMediaRecorder::Uninit(JNIEnv *env)
{
	env->DeleteGlobalRef(m_owner);
}

void *CMediaRecorder::ThreadProc(void *pParam)
{
	CMediaRecorder *pThread = (CMediaRecorder *)pParam;
	pThread->Loop();
	return 0;
}

void CMediaRecorder::Loop()
{
	JNIEnv *env = NULL;
	int idr = 1; //h264

	do{
	if( m_lstWaitEncodeFrames.empty() != false )
	{
		this->Wait( 1000/* 1.0sec */);
	}

	if( m_lstWaitEncodeFrames.empty() == false )
	{
		m_mtxWaitEncodeFrames.Enter();
		CScopeBuffer pSrcdata(m_lstWaitEncodeFrames.front());
		m_lstWaitEncodeFrames.pop_front();
		m_mtxWaitEncodeFrames.Leave();
		CScopeBuffer pDstdata;
		int idx = -1;
		int fmt = -1;

		if( pSrcdata.p == 0 )
		{
			if( m_nframes[0] + m_nframes[1] != 0 ) {//check delay frames
			THIS_LOGI_print("start to check delay frames...");
			for(int i = 0; i < 2; i ++)
			{
				pDstdata.Attach( m_pEncode[i]==0? 0:m_pEncode[i]->GetDelayedFrame());
				if( pDstdata.p ) Write(i, pDstdata->GetPacket());
			}
			}
			THIS_LOGI_print("thread is exit, frames: %lld/%lld", m_nframes[0], m_nframes[1]);
			break;
		}

		switch(pSrcdata->m_codecid&0xffff)
		{
			case MEDIA_DATA_TYPE_JPG:
			{
				 AVCodecContext *pCodecs = m_pCodecs[2];
				 if( pCodecs->codec_id == AV_CODEC_ID_MJPEG )
			 	 {//jpg
			 	 	 pDstdata.Attach(pSrcdata.Detach());
			 	 }
				 else
			 	 {//jpg->yuv, outside
			 	 	 if( m_pDecode[0] == 0 ) {//decode jpg to yuv
						 const AVCodec *vcodec = avcodec_find_decoder(AV_CODEC_ID_MJPEG);
						 m_pCodecs[0] = avcodec_alloc_context3(vcodec);
						 m_pCodecs[0]->width   = pCodecs->width;
						 m_pCodecs[0]->height  = pCodecs->height;
						 m_pCodecs[0]->pix_fmt = AV_PIX_FMT_YUVJ420P;
						 #ifdef _FFMPEG_THREAD
						 m_pCodecs[0]->thread_count = 4;
						 m_pCodecs[0]->thread_type =FF_THREAD_SLICE;
						 #endif
						 int iret = avcodec_open2(m_pCodecs[0], vcodec, NULL);						 
						 m_pDecode[0] = CMediaDecoder::Create(m_pCodecs[0]);
			 	 	 }

					 pSrcdata.Attach(m_pDecode[0]->Decode(pSrcdata.p));
					 fmt = m_pCodecs[0]->pix_fmt;
			 	 }

				 assert(m_pScales == 0);
				 idx = 0;
				 break;
			}

			case MEDIA_DATA_TYPE_264:
			{
				 uint8_t ctype = pSrcdata->GetPacket()->data[4] & 0x1f;
				 if( idr ) {//wait for idr/sps+pps frame
			 	 	 if( ctype != 5 && 
					 	 ctype != 7 )
					 {
						 THIS_LOGT_print("drop video[%p]: pts=%lld, data=%p, size=%d, nalu.type=%d", pSrcdata.p, pSrcdata->GetPacket()->pts, pSrcdata->GetPacket()->data, pSrcdata->GetPacket()->size, ctype);
					 	 break;
			 	 	 }
					 idr = 0;
			 	 }

				 AVCodecContext *pCodecs = m_pCodecs[2];
				 if( pCodecs->codec_id == AV_CODEC_ID_H264 )
				 {//264
				 	 if( m_nframes[0] + m_nframes[1] == 0 && 
					 	 ctype != 7 )
				 	 {
				 	 	 assert(m_pPlayer != 0);
						 CScopeBuffer pBuffer(m_pPlayer->m_pBufferManager->Pop(pSrcdata->GetPacket()->pts));
						 m_pPlayer->GetVideoSpspps(2, pBuffer.p);
						 AVPacket    *pPacket = pBuffer->GetPacket();	
						 pPacket->pos =-1;
						 int iret = Write(0, pPacket);
						 if( iret )
					 	 {
							 Write(0); //设置失败							 
							 JNI_ATTACH_JVM(env);
							 env->CallStaticVoidMethod(g_jBQLMediaRecorder.thizClass, g_jBQLMediaRecorder.post, m_owner, 0, iret);
							 JNI_DETACH_JVM(env);
							 return;		 	 
					 	 }
				 	 }
					 pDstdata.Attach(pSrcdata.Detach());
				 }
				 else
				 {//264->yuv, outside
			 	 	 if( m_pDecode[0] == 0 ) {
						 const AVCodec *vcodec = avcodec_find_decoder(AV_CODEC_ID_H264);
						 m_pCodecs[0] = avcodec_alloc_context3(vcodec);
						 m_pCodecs[0]->width   = pCodecs->width;
						 m_pCodecs[0]->height  = pCodecs->height;
						 m_pCodecs[0]->pix_fmt = AV_PIX_FMT_YUV420P;
						 #ifdef _FFMPEG_THREAD
						 m_pCodecs[0]->thread_count = 4;
						 m_pCodecs[0]->thread_type =FF_THREAD_SLICE;
						 #endif
						 int iret = avcodec_open2(m_pCodecs[0], vcodec, NULL);
						 m_pDecode[0] = CMediaDecoder::Create(m_pCodecs[0]);
			 	 	 }

					 pSrcdata.Attach(m_pDecode[0]->Decode(pSrcdata.p));
					 fmt = m_pCodecs[0]->pix_fmt;
				 }

				 assert(m_pScales == 0);				 
				 idx = 0;
				 break;
			}

			case MEDIA_DATA_TYPE_YUV:
			{
				 AVCodecContext *pCodecs = m_pCodecs[2]; 		   
				 if( m_pScales != 0 ) pSrcdata.Attach(m_pScales->Scale(pSrcdata->GetPacket()->pts, pSrcdata->GetPacket()->data));
				 fmt = m_pScales? (m_pScales->m_dstFormat):(pSrcdata->m_codecid >> 16);
				 idx = 0;
				 break;
			}

			case MEDIA_DATA_TYPE_RGB:
			{//outside
				 AVCodecContext *pCodecs = m_pCodecs[2];			
				 if( m_pScales == 0 ) m_pScales = new CSwsScale(pCodecs->width, pCodecs->height, AV_PIX_FMT_RGBA, pCodecs->width, pCodecs->height, pCodecs->pix_fmt);				
				 if( m_pScales != 0 ) pSrcdata.Attach(m_pScales->Scale(pSrcdata->GetPacket()->pts, pSrcdata->GetPacket()->data));
				 idx = 0;
				 break;
			}

			case MEDIA_DATA_TYPE_AAC:
			{
				 if( m_pCodecs[2] &&
				 	 m_nframes[0] == 0 )
			 	 {//skip audio until get video
			 	 	 THIS_LOGT_print("drop audio[%p]: pts=%lld, data=%p, size=%d", pSrcdata.p, pSrcdata->GetPacket()->pts, pSrcdata->GetPacket()->data, pSrcdata->GetPacket()->size);
			 	 	 break;
			 	 }

				 AVCodecContext *pCodecs = m_pCodecs[3];
				 pDstdata.Attach(pSrcdata.Detach());
				 idx = 1;
				 break;
			}

			case MEDIA_DATA_TYPE_PCM:
			{
				 if( m_pCodecs[2] &&
				 	 m_nframes[0] == 0 )
			 	 {//skip audio until get video
			 	 	 THIS_LOGT_print("drop audio[%p]: pts=%lld, data=%p, size=%d", pSrcdata.p, pSrcdata->GetPacket()->pts, pSrcdata->GetPacket()->data, pSrcdata->GetPacket()->size);
			 	 	 break;
			 	 }

				 idx = 1;
				 break;
			}

			default:
			{
				 assert(false);
				 break;
			}
		}

		if( idx !=-1 &&
			pDstdata.p == 0 &&
			pSrcdata.p != 0 )
		{
			if( m_pEncode[idx] == 0 ) m_pEncode[idx] = CMediaEncoder::Create(m_pCodecs[2 + idx], fmt);
			pDstdata.Attach(m_pEncode[idx]->Encode(pSrcdata.p));
		}

		if( pDstdata.p != 0 )
		{
			AVPacket *pPacket = pDstdata->GetPacket();
			THIS_LOGT_print("send %s[%p]: pts=%lld, data=%p, size=%d", idx==0? "video":"audio", pDstdata.p, pPacket->pts, pPacket->data, pPacket->size);						
			int iret = Write(idx, pPacket);
			if( iret )
			{
				Write(0); //设置失败
				JNI_ATTACH_JVM(env);
				env->CallStaticVoidMethod(g_jBQLMediaRecorder.thizClass, g_jBQLMediaRecorder.post, m_owner, 0, iret);
				JNI_DETACH_JVM(env);
				return;
			}
		}
	}
	}while(1);
}

