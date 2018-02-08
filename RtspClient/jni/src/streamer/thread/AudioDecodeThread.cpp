#include "player/Player.h"
#include "AudioDecodeThread.h"
#include "JNI_load.h"

CLASS_LOG_IMPLEMENT(CAudioDecodeThread, "CAudioDecodeThread");
///////////////////////////////////////////////////////////////////
void CAudioDecodeThread::Start(AVCodecContext *pCodecs, CBuffer *pNilframe)
{
	assert(m_pThread == 0);
	assert(m_pPlayer != 0);
	m_pCodecs = pCodecs;
	THIS_LOGI_print("do start to decode audio thread");		
	int isucc = ::pthread_create(&m_pThread, NULL, CAudioDecodeThread::ThreadProc, this );
	pNilframe->GetPacket()->size = 0; //init
	Push(pNilframe);
}

void CAudioDecodeThread::Stop()
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
bool CAudioDecodeThread::Push(CBuffer *pFrame)
{
//	THIS_LOGT_print("add frame[%p]: pts=%lld, data=%p, size=%d", pFrame, pFrame->GetPacket()->pts, pFrame->GetPacket()->data, pFrame->GetPacket()->size);
	m_mtxWaitDecodeFrames.Enter();
	m_lstWaitDecodeFrames.push_back(pFrame);
	m_nFrames++;
	m_mtxWaitDecodeFrames.Leave();
	Post();
	return true;
}

void CAudioDecodeThread::DropFrames()
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

CBuffer *CAudioDecodeThread::Take(int64_t now, int a, int &val)
{
	CScopeMutex mutex(m_mtxWaitDecodeFrames);
	if( m_lstWaitDecodeFrames.empty() != 0 )
	{
		val = 0;
	}
	else
	{
		CBuffer *framef = m_lstWaitDecodeFrames.front();
//		CBuffer *frameb = m_lstWaitDecodeFrames.back();		
		int64_t pts = framef->GetPacket()->pts;
		val = pts - now - a;

		if( framef->GetPacket()->size != 0 ) {
		if( val > m_pPlayer->m_nBufferInterval || -8 > val )
		{
//			THIS_LOGW_print("adjust clock while got audio frame[%d], pts=%lld, list=%d", val, pts, m_nFrames);
			val = m_pPlayer->m_clock.Adjust(pts, m_nFrames>9? 0:m_pPlayer->m_nBufferInterval);
		}
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
void *CAudioDecodeThread::ThreadProc(void *pParam)
{
	CAudioDecodeThread *pThread = (CAudioDecodeThread *)pParam;
	JNI_ATTACH_JVM(pThread->m_pJNIEnv);

	pThread->Loop();

	JNI_DETACH_JVM(pThread->m_pJNIEnv);
	return 0;
}

void CAudioDecodeThread::Loop()
{
	int ret = m_pPlayer->m_pAudioTrack->Create(m_pJNIEnv, m_pCodecs->channels, m_pCodecs->sample_rate);
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
			if( m_pDecode == 0 ) m_pDecode = CMediaDecoder::Create(m_pCodecs);
			if( pSrcframe->GetPacket()->size &&
				m_pDecode != 0 )
			{
				CCostHelper cost; //cost of handle
				THIS_LOGD_print("play frame[%p]: pts=%lld, now=%lld, list=%d", pSrcframe.p, pSrcframe->GetPacket()->pts, now, m_nFrames);				
				m_pPlayer->OnUpdatePosition(m_pPlayer->m_clock.Get(pSrcframe->GetPacket()->pts));
				Play(m_pJNIEnv, pSrcframe.Detach());
				adj = cost.Get() / 1000 + 8;
			}
		}
	}
	}while(1);

	m_pPlayer->m_pAudioTrack->Delete(m_pJNIEnv);
}

void CAudioDecodeThread::Play(JNIEnv *env, CBuffer *pRawframe)
{
	CScopeBuffer pSrcdata(pRawframe);
	CScopeBuffer pPcmdata(m_pDecode->Decode(pRawframe));

	if( pPcmdata.p )
	{
		if( m_pPlayer->m_bIsMute==0 )
		{
			m_pPlayer->m_pAudioTrack->Write(env, pPcmdata->AddRef());
		}

		{//check
			Java_fireBuffer(env, m_pPlayer, MEDIA_DATA_TYPE_PCM, pPcmdata.Detach());
		}
	}

	if( pSrcdata.p &&
		m_pCodecs->codec_id == AV_CODEC_ID_AAC )
	{
		if( pSrcdata->GetPacket()->size )
		{
			Java_fireBuffer(env, m_pPlayer, MEDIA_DATA_TYPE_AAC, pSrcdata.Detach());
		}
	}
}
