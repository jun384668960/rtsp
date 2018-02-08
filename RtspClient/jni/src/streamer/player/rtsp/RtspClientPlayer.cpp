#include "RtspClientPlayer.h"
#include "RtspMediaSink.h"
#include "JNI_load.h"
#include <pthread.h>

// Even though we're not going to be doing anything with the incoming data, we still need to receive it.
// Define the size of the buffer that we'll use:
class userRTSPClient : public RTSPClient 
{
public:
	userRTSPClient(CRtspClientPlayer *pPlayer, UsageEnvironment& env, char const* rtspURL, int verbosityLevel = 0, char const* applicationName = NULL, portNumBits tunnelOverHTTPPortNum = 0, int socketNumToServer = -1)
  	  : RTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum, socketNumToServer), m_pPlayer(pPlayer)
	{
	}

public:
	CRtspClientPlayer *m_pPlayer;
};

CLASS_LOG_IMPLEMENT(CRtspClientPlayer, "CPlayer[rtsp]");
////////////////////////////////////////////////////////////////////////////////
CRtspClientPlayer::CRtspClientPlayer(const char *url)
  : m_pThread(0), m_pJNIEnv(0), m_nCycles(0), m_pRTSPClient(0), m_bEventLoopWatchVariable(0), m_url(url), m_pTimeDealTask(0), m_pMediaSubsessionIterator(0), m_pMediaSession(0), m_pMediaSubsession(0)
{
	THIS_LOGT_print("is created of %s", url);
	m_MEDIASINK_RECV_BUFFER_SIZE = 512 * 1024; //512K, 过小导致接收丢到了I帧，从而引发马赛克
	m_STREAMING_METHOD  = 0; //auto
	m_CHKSTREAM_PERIOD  = 5; //5sec
	m_VIDEOMFPS = 30;
	m_VIDEOFFPS = 0;
	m_DISABLERTCP       = 0; //rtcp is enabled
	m_pTaskScheduler    = BasicTaskScheduler::createNew();
	m_pUsageEnvironment = BasicUsageEnvironment::createNew(*m_pTaskScheduler );
}

CRtspClientPlayer::~CRtspClientPlayer()
{
	if( m_pMediaSubsessionIterator ) delete m_pMediaSubsessionIterator;
	if( m_pMediaSession )
	{
		m_pTaskScheduler->unscheduleDelayedTask(m_pTimeDealTask);   
		Medium::close(m_pMediaSession);
	}
	m_pUsageEnvironment->reclaim();
	delete m_pTaskScheduler;

	if( m_videoctx != 0 ) avcodec_free_context(&m_videoctx); //will call avcodec_close
	if( m_audioctx != 0 ) avcodec_free_context(&m_audioctx); //will call avcodec_close
	THIS_LOGT_print("is deleted");	
}

////////////////////////////////////////////////////////////////////////////////
int CRtspClientPlayer::Prepare()
{
	if( GetStatus() != PS_Stopped ) 
	{
		THIS_LOGW_print("Prepare error: invalid status[%d]", m_nStatus);
		return 1; //invalid status
	}

	THIS_LOGD_print("do Prepare...");
	assert(m_pThread == 0);
	SetStatus( PS_Prepare);
	int iret = ::pthread_create(&m_pThread, NULL, CRtspClientPlayer::ThreadProc, this );
	return 0;
}

int CRtspClientPlayer::Play()
{
	if( GetStatus() != PS_Paused &&
		GetStatus() != PS_Pause2 /*not recved PAUSE ack packet*/)
	{
		THIS_LOGW_print("Play error: invalid status[%d]", m_nStatus);
		return 1; //invalid status
	}

	THIS_LOGD_print("do Play...");
	SetStatus(m_pRTSPClient==0? PS_Restart:PS_Buffering1);
	m_bEventLoopWatchVariable = 1; //break doEventLoop	
	return 0;
}

int CRtspClientPlayer::Pause()
{
	if( GetStatus() != PS_Playing &&
		GetStatus() != PS_Buffering2)
	{
		THIS_LOGW_print("Pause error: invalid status[%d]", m_nStatus);
		return 1; //invalid status
	}

	THIS_LOGD_print("do Pause...");
	SetStatus(PS_Pause1);
	m_bEventLoopWatchVariable = 1; //break doEventLoop	
	return 0;
}

int CRtspClientPlayer::Seek(int seek/*时间偏移，毫秒*/)
{
	if( GetStatus() != PS_Playing &&
		GetStatus() != PS_Stopped &&
		GetStatus() != PS_Buffering2 &&
		GetStatus() != PS_Paused )
	{
		THIS_LOGW_print("Seek error: invalid status[%d]", m_nStatus);
		return 1; //invalid status
	}

	if( m_duration == 0 ) 
	{
		THIS_LOGW_print("Seek error: can't seek while live");
		return 2;
	}

	THIS_LOGD_print("do Seek: %d", seek);
	assert(seek >= 0);
	m_nSeekpos = seek;
	if( GetStatus() == PS_Paused ) return 0;

	SetStatus(PS_Buffering1);
	m_bEventLoopWatchVariable = 1; //break doEventLoop
	return 0;
}

int CRtspClientPlayer::Stop()
{
	if( GetStatus() == PS_Stopped ) return 0;

	THIS_LOGD_print("do Stop...");
	SetStatus( PS_Stopped);
	assert(m_pThread != 0);
	m_bEventLoopWatchVariable = 1; //break doEventLoop
	::pthread_join(m_pThread, 0); 
	m_pThread = 0;
	m_pVideoDecodeThread->Stop();
	m_pAudioDecodeThread->Stop();
	return 0;
}

int CRtspClientPlayer::GetVideoConfig(int *fps, int *videow, int *videoh)
{
	if( m_videoctx == 0 ) return 1;
	if( fps    ) *fps    = m_pMediaSubsession->videoFPS();
	if( videow ) *videow = m_videoctx->width;
	if( videoh ) *videoh = m_videoctx->height;
	return 0;
}

int CRtspClientPlayer::GetAudioConfig(int *channels, int *depth, int *samplerate)
{
	if( m_audioctx == 0 ) return 1;
	if( channels   ) *channels   = m_audioctx->channels;
	if( depth      ) *depth      = m_audioctx->sample_fmt==AV_SAMPLE_FMT_U8? 8:16;
	if( samplerate ) *samplerate = m_audioctx->sample_rate;
	return 0;
}

int CRtspClientPlayer::SetParamConfig(const char *key, const char *val)
{
	if( strcmp(key, "buffersize") == 0 )
	{
		m_MEDIASINK_RECV_BUFFER_SIZE = atoi(val);
		return 0;
	}
	else
	if( strcmp(key, "chkstream") == 0 )
	{
		m_CHKSTREAM_PERIOD = atoi(val); //sec
		return 0;
	}
	else
	if( strcmp(key, "viamethod") == 0 )
	{
		m_STREAMING_METHOD = atoi(val)? 2/*tcp*/:1/*udp*/;
		return 0;
	}
	else
	if( strcmp(key, "videomfps") == 0 )
	{
		m_VIDEOMFPS = atoi(val);
		return 0;
	}
	else
	if( strcmp(key, "videoffps") == 0 )
	{
		m_VIDEOFFPS = atoi(val);
		return 0;
	}
	else
	if( strcmp(key, "disablertcp") == 0 )
	{
		m_DISABLERTCP= atoi(val);
		return 0;
	}

	return 1;
}


////////////////////////////////////////////////////////////////////////////////
void *CRtspClientPlayer::ThreadProc(void *pParam)
{
	CRtspClientPlayer *pThread = (CRtspClientPlayer *)pParam;
	JNI_ATTACH_JVM(pThread->m_pJNIEnv);

	pThread->Loop();

	JNI_DETACH_JVM(pThread->m_pJNIEnv);		
	return 0;
}

void CRtspClientPlayer::Loop()
{
/*	struct sched_param param;
	int policy;

	pthread_getschedparam(pthread_self(), &policy, &param);
	param.sched_priority = sched_get_priority_max(policy); policy = SCHED_RR;
	pthread_setschedparam(pthread_self(),  policy, &param); */
	if( m_pBufferManager == 0 ) m_pBufferManager = new CBufferManager( m_MEDIASINK_RECV_BUFFER_SIZE, 36 );

	do{
	int status = GetStatus( );
	if( status == PS_Stopped)
	{
		Shutdown(0);	
		THIS_LOGI_print("thread is exit");
		break;
	}

	if( m_pRTSPClient==NULL &&
		status != PS_Paused )
	{// send a RTSP "DESCRIBE" command, to get a SDP description for the stream.
//		m_pRTSPClient = new userRTSPClient(this, *m_pUsageEnvironment, m_url.c_str(), 0); 
		m_pRTSPClient = new userRTSPClient(this, *m_pUsageEnvironment, m_url.c_str(), 0, "stream Play", 0); //for f06
		THIS_LOGI_print("Send DESCRIBE req packet");
		m_pRTSPClient->sendDescribeCommand(CRtspClientPlayer::continueAfterDESCRIBE);
	}

	if( m_pRTSPClient==NULL )
	{
		usleep(5000/* 5ms */);
	}
	else
	{
		if( status == PS_Pause1     )
		{
			THIS_LOGI_print("Send PAUSE req packet");					
			status = SetStatus(PS_Paused    );
			m_nSeekpos =-2; //mark do paused			
			m_pRTSPClient->sendPauseCommand(*m_pMediaSession, CRtspClientPlayer::continueAfterPAUSE);			
		}

		if( status == PS_Buffering1 &&
			m_pMediaSubsession->sink == NULL )
		{
			// Having successfully setup the subsession, create a data sink for it, and call "startPlaying()" on it.
			// (This will prepare the data sink to receive data; the actual flow of data from the client won't start happening until later,
			// after we've sent a RTSP "PLAY" command.)
			m_pMediaSubsession->sink = new CRtspMediaSink(this, m_pRTSPClient->url());
			m_pMediaSubsession->miscPtr = m_pRTSPClient; // a hack to let subsession handle functions get the "RTSPClient" from the subsession 
			m_pMediaSubsession->sink->startPlaying(*(m_pMediaSubsession->readSource()), CRtspClientPlayer::subsessionAfterPlaying, m_pMediaSubsession);

			// Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
			if( m_pMediaSubsession->rtcpInstance() ) {
				m_pMediaSubsession->rtcpInstance()->setByeHandler(subsessionByeHandler, m_pMediaSubsession);
			}
		}

		if( status == PS_Buffering1 )
		{
			// We've finished setting up all of the subsessions.  Now, send a RTSP "PLAY" command to start the streaming:
			status = SetStatus(PS_Buffering2);
			if( m_nSeekpos < 0 )
			{
				THIS_LOGI_print("Send PLAY req packet");
				m_pRTSPClient->sendPlayCommand(*m_pMediaSession, CRtspClientPlayer::continueAfterPLAY);
			}
			else
			{// seek
				THIS_LOGI_print("Send PLAY req packet, range: %f", m_nSeekpos/1000.0);
				m_pRTSPClient->sendPlayCommand(*m_pMediaSession, CRtspClientPlayer::continueAfterPLAY, m_nSeekpos/1000.0);
				if( m_STREAMING_METHOD == 2 ) {
				THIS_LOGI_print("pts=%lld->%lld, now=%lld", m_clock.pts, m_clock.pts + m_nSeekpos, m_clock.Now());				
				if( m_pAudioDecodeThread!=0 ) m_pAudioDecodeThread->DropFrames();
				if( m_pVideoDecodeThread!=0 ) m_pVideoDecodeThread->DropFrames();
				m_position = m_nSeekpos;
				m_idr	   = true; //stream over tcp				
				}
			}
		}

		m_bEventLoopWatchVariable = 0; //must be reset
		m_pUsageEnvironment->taskScheduler().doEventLoop(&m_bEventLoopWatchVariable );
	}

	}while(1);
}

////////////////////////////////////////////////////////////////////////////////
void CRtspClientPlayer::continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString)
{
	CRtspClientPlayer *pPlayer = ((userRTSPClient*)rtspClient)->m_pPlayer;
	UsageEnvironment  &env = rtspClient->envir(); // alias

	do{
	LOGI_print(s_logger, "%p Received DESCRIBE ack packet: %d\n%s", pPlayer, resultCode, resultString);		

	if( resultCode != 0 ) {
		break;
	}

	// Create a media session object from this SDP description:
	pPlayer->m_pMediaSession = MediaSession::createNew(env, resultString);
	if( pPlayer->m_pMediaSession == NULL ) {
		LOGE_print(s_logger, "%p Failed to create Session from SDP: %s", pPlayer, env.getResultMsg());
		resultCode = 400;
		break;
	}
	if(!pPlayer->m_pMediaSession->hasSubsessions()) {
		LOGE_print(s_logger, "%p has no subsessions (i.e., no \"m=\" lines)", pPlayer);		
		resultCode = 400;
		break;
	}

	delete[] resultString; // because we don't need it anymore	

	if( pPlayer->m_pMediaSession->playEndTime() )
		pPlayer->m_duration = (pPlayer->m_pMediaSession->playEndTime() - pPlayer->m_pMediaSession->playStartTime()) * 1000; //ms	
	else
		pPlayer->m_duration = 0;

	if( pPlayer->m_duration > 0 && pPlayer->m_nBufferInterval < 300 ) pPlayer->m_nBufferInterval = 300;

	// Then, create and set up our data source objects for the session.  We do this by iterating over the session's 'subsessions',
	// calling "MediaSubsession::initiate()", and then sending a RTSP "SETUP" command, on each one.
	// (Each 'subsession' will have its own data source.)
	pPlayer->m_pMediaSubsessionIterator = new MediaSubsessionIterator(*pPlayer->m_pMediaSession );
	CRtspClientPlayer::setupNextSubsession(rtspClient);
	return;
	} while(0);

	delete[] resultString; // because we don't need it anymore	

	pPlayer->Shutdown(resultCode);
}

void CRtspClientPlayer::continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString)
{
	CRtspClientPlayer *pPlayer = ((userRTSPClient*)rtspClient)->m_pPlayer;
	UsageEnvironment  &env = rtspClient->envir();

	LOGI_print(s_logger, "%p Received SETUP ack packet: %d", pPlayer, resultCode);
	delete[] resultString;
		
	if( resultCode != 0 ) 
	{
		pPlayer->Shutdown(resultCode);
	}
	else
	{
		LOGI_print(s_logger, "%p get media[%s] duration=%d", pPlayer, pPlayer->m_pMediaSubsession->codecName(), pPlayer->m_duration);
		if( pPlayer->m_audioctx== 0 ) {
			AVCodec *acodec = avcodec_find_decoder(AV_CODEC_ID_AAC);
			pPlayer->m_audioctx = avcodec_alloc_context3(acodec);
			LOGI_print(s_logger, "%p get audioctx=%p, codecid=%d", pPlayer, pPlayer->m_audioctx, pPlayer->m_audioctx->codec_id); 			
//			AVCodecParameters params;
//			avcodec_parameters_to_context(m_pPlayer->m_videoctx, &params);		
			pPlayer->m_audioctx->bit_rate 	    = 128000; //128kbps
			pPlayer->m_audioctx->sample_rate    = pPlayer->m_pMediaSubsession->attrVal_unsigned("audio-sample-rate");  //44.1khz
			if( pPlayer->m_audioctx->sample_rate== 0 ) pPlayer->m_audioctx->sample_rate = 44100;
			pPlayer->m_audioctx->frame_size	    = 1024;
			pPlayer->m_audioctx->sample_fmt	    = AV_SAMPLE_FMT_S16;	
			pPlayer->m_audioctx->channels 	    = pPlayer->m_pMediaSubsession->attrVal_unsigned("audio-channel");
			if( pPlayer->m_audioctx->channels   == 0 ) pPlayer->m_audioctx->channels    = 2;
			pPlayer->m_audioctx->channel_layout = av_get_default_channel_layout(pPlayer->m_audioctx->channels);

			int iret = avcodec_open2(pPlayer->m_audioctx, acodec, NULL);
			LOGT_print(s_logger, "%p call avcodec_open2 return %d", pPlayer, iret);
			pPlayer->m_pAudioDecodeThread->Start(pPlayer->m_audioctx, pPlayer->m_pBufferManager->Pop());			
		}
		if( pPlayer->m_videoctx== 0 ) {
			AVCodec *vcodec = avcodec_find_decoder(0==strcmp(pPlayer->m_pMediaSubsession->codecName(), "H264")? AV_CODEC_ID_H264:AV_CODEC_ID_MJPEG);
			pPlayer->m_videoctx = avcodec_alloc_context3(vcodec);
			LOGI_print(s_logger, "%p get videoctx=%p, codecid=%d", pPlayer, pPlayer->m_videoctx, pPlayer->m_videoctx->codec_id); 			
//			AVCodecParameters params;
//			avcodec_parameters_to_context(m_pPlayer->m_videoctx, &params);
			#ifdef _FFMPEG_THREAD
			pPlayer->m_videoctx->thread_count = 4;
			pPlayer->m_videoctx->thread_type =FF_THREAD_SLICE;
			#endif
			pPlayer->m_videoctx->width   = pPlayer->m_pMediaSubsession->videoWidth()==0? 1280:pPlayer->m_pMediaSubsession->videoWidth();
			pPlayer->m_videoctx->height  = pPlayer->m_pMediaSubsession->videoHeight()==0? 960:pPlayer->m_pMediaSubsession->videoHeight();
			pPlayer->m_videoctx->pix_fmt = pPlayer->m_videoctx->codec_id==AV_CODEC_ID_MJPEG? AV_PIX_FMT_YUVJ420P:AV_PIX_FMT_YUV420P;
			LOGI_print(s_logger, "%p video(%dx%d), %dfps", pPlayer, pPlayer->m_videoctx->width, pPlayer->m_videoctx->height, pPlayer->m_pMediaSubsession->videoFPS());

			int iret = avcodec_open2(pPlayer->m_videoctx, vcodec, NULL);
			LOGT_print(s_logger, "%p call avcodec_open2 return %d", pPlayer, iret);
			pPlayer->m_pVideoDecodeThread->Start(pPlayer->m_videoctx, pPlayer->m_pBufferManager->Pop());				
		}

		// Set a timer to be handled at the end of the stream's expected duration (if the stream does not already signal its end
		// using a RTCP "BYE").  This is optional.  If, instead, you want to keep the stream active - e.g., so you can later
		// 'seek' back within it and do another RTSP "PLAY" - then you can omit this code.
		// (Alternatively, if you don't want to receive the entire stream, you could set this timer for some shorter value.)
		if( pPlayer->m_pTimeDealTask == 0 )
		{//fix server can't stop while playing completed
			pPlayer->m_pTimeDealTask = env.taskScheduler().scheduleDelayedTask(1 * 1000 * 1000/*1sec*/, (TaskFunc*)CRtspClientPlayer::streamTimeoutHandler, rtspClient);
		}

		pPlayer->m_bEventLoopWatchVariable = 1;
		int status = pPlayer->GetStatus();
		if( status == PS_Prepare )
		{
			pPlayer->SetStatus(PS_Paused);
			Java_fireNotify(pPlayer->m_pJNIEnv, pPlayer, MEDIA_PREPARED, 0, 0, 0);
		}
		if( status == PS_Restart )
		{
			pPlayer->SetStatus(PS_Buffering1);
		}
	}
}

void CRtspClientPlayer::continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString)
{
	CRtspClientPlayer *pPlayer = ((userRTSPClient*)rtspClient)->m_pPlayer;
	UsageEnvironment  &env = rtspClient->envir();

	LOGI_print(s_logger, "%p Received PLAY ack packet: %d", pPlayer, resultCode);
	delete[] resultString;

	if( resultCode != 0 )
	{
		pPlayer->Shutdown(resultCode);
	}
	else
	{
		LOGW_print(s_logger, "%p Prepare to recv media from timestamp=%d, A/V=%d/%d", pPlayer, pPlayer->m_pMediaSubsession->rtpInfo.timestamp, pPlayer->GetAudioFrameSize(), pPlayer->GetVideoFrameSize());
		pPlayer->SetStatus( pPlayer->m_nSeekpos!=-1? PS_Playing:PS_Bufferingv );
		if( pPlayer->m_STREAMING_METHOD == 2 &&
			pPlayer->m_nSeekpos >= 0 ) 
		{
			LOGI_print(s_logger, "pts=%lld->%lld, now=%lld", pPlayer->m_clock.pts, pPlayer->m_clock.pts + pPlayer->m_nSeekpos, pPlayer->m_clock.Now());
			pPlayer->m_clock.Adjust(pPlayer->m_clock.pts + pPlayer->m_nSeekpos, pPlayer->m_nBufferInterval);
			Java_fireNotify(pPlayer->m_pJNIEnv, pPlayer, MEDIA_SEEK_COMPLETED, 0, 0, 0);					
		}
		if( pPlayer->m_nSeekpos ==-2 ) 
		{
			pPlayer->SetStatus(PS_Playing);
		}
		pPlayer->m_nSeekpos =-1; //reset
	}
}

void CRtspClientPlayer::continueAfterPAUSE(RTSPClient* rtspClient, int resultCode, char* resultString)
{
	CRtspClientPlayer *pPlayer = ((userRTSPClient*)rtspClient)->m_pPlayer;
	UsageEnvironment  &env = rtspClient->envir();

	LOGI_print(s_logger, "%p Received PAUSE ack packet: %d", pPlayer, resultCode);
	delete[] resultString;
}

void CRtspClientPlayer::streamTimeoutHandler(void* rtspClient)
{//检测是否完成播放
	CRtspClientPlayer *pPlayer = ((userRTSPClient*)rtspClient)->m_pPlayer;

	if( pPlayer->GetStatus() == PS_Playing )
	{		
		if( pPlayer->m_duration != 0 )
		{
			int interval = pPlayer->m_duration - 60/*60ms*/ - pPlayer->m_position;
			int AVframes = pPlayer->GetAudioFrameSize() + pPlayer->GetVideoFrameSize();
			if( interval<= 0 &&
				AVframes== 0 )
			{
				LOGW_print(s_logger, "%p shutdown, duration: %d", pPlayer, pPlayer->m_duration);	
				pPlayer->m_pTimeDealTask = 0;
				pPlayer->Shutdown(0);
				return;
			}
		}

		if((++ pPlayer->m_nCycles) >= pPlayer->m_CHKSTREAM_PERIOD )
		{
			Java_fireNotify(pPlayer->m_pJNIEnv, pPlayer, MEDIA_ERR_NO_STREAM, 0, 0, 0);		
			pPlayer->m_nCycles = 0;
		}
	}

	LOGT_print(s_logger, "%p checkpos: %d/%d, status: %d", pPlayer, pPlayer->m_position, pPlayer->m_duration, pPlayer->GetStatus());
	pPlayer->m_pTimeDealTask = pPlayer->m_pTaskScheduler->scheduleDelayedTask(1 * 1000 * 1000/*1sec*/, (TaskFunc*)CRtspClientPlayer::streamTimeoutHandler, rtspClient);
}

void CRtspClientPlayer::subsessionByeHandler(void* clientData)
{
	MediaSubsession *subsession= (MediaSubsession*)clientData;
	CRtspClientPlayer *pPlayer = ((userRTSPClient*)subsession->miscPtr)->m_pPlayer;

	LOGW_print(s_logger, "%p Received RTCP \"BYE\"", pPlayer);	
	// Now act as if the subsession had closed:
	CRtspClientPlayer::subsessionAfterPlaying(clientData);
}

void CRtspClientPlayer::subsessionAfterPlaying(void* clientData)
{
	MediaSubsession *subsession = (MediaSubsession*)clientData;  
	RTSPClient *rtspClient = (RTSPClient*)(subsession->miscPtr);
	CRtspClientPlayer *pPlayer = ((userRTSPClient*)rtspClient)->m_pPlayer;

	// Begin by closing this subsession's stream:
	Medium::close(subsession->sink);
	subsession->sink = NULL;

	// Next, check whether *all* subsessions' streams have now been closed:
	MediaSession &session = subsession->parentSession();
	MediaSubsessionIterator iter(session);
	while((subsession = iter.next()) != NULL) 
	{
		if(subsession->sink != NULL) 
			return; // this subsession is still active
	}

	// All subsessions' streams have now been closed, so shutdown the client:
	LOGI_print(s_logger, "%p shutdown", pPlayer);		
	pPlayer->Shutdown(0);
}

void CRtspClientPlayer::setupNextSubsession(RTSPClient* rtspClient) 
{
	CRtspClientPlayer *pPlayer = ((userRTSPClient*)rtspClient)->m_pPlayer;
 
	pPlayer->m_pMediaSubsession = pPlayer->m_pMediaSubsessionIterator->next();
	if( pPlayer->m_pMediaSubsession != NULL ) 
	{
		//setRtcpDisable
		LOGW_print(s_logger, "rtcp is %s", pPlayer->m_DISABLERTCP? "disabled":"enabled");
		pPlayer->m_pMediaSubsession->DisRTCPInstance(pPlayer->m_DISABLERTCP);
		if(!pPlayer->m_pMediaSubsession->initiate()) 
		{
			LOGE_print(s_logger, "%p Failed to initiate subsession: %s", pPlayer, pPlayer->m_pUsageEnvironment->getResultMsg());			
			CRtspClientPlayer::setupNextSubsession(rtspClient); // give up on this subsession; go to the next one
		} 
		else
		{
			#ifdef _DEBUG
			if( pPlayer->m_pMediaSubsession->rtcpIsMuxed()) 
			LOGD_print(s_logger, "%p Initiated subsession client port: %d"   , pPlayer, pPlayer->m_pMediaSubsession->clientPortNum());	
			else
			LOGD_print(s_logger, "%p Initiated subsession client port: %d-%d", pPlayer, pPlayer->m_pMediaSubsession->clientPortNum(), pPlayer->m_pMediaSubsession->clientPortNum() + 1);
			#endif

			// By default, we request that the server stream its data using RTP/UDP.
			// If, instead, you want to request that the server stream via RTP-over-TCP, change the following to True:
			if( pPlayer->m_STREAMING_METHOD == 0 ) pPlayer->m_STREAMING_METHOD = pPlayer->m_duration!=0? 2:1;

			// Continue setting up this subsession, by sending a RTSP "SETUP" command:
			LOGI_print(s_logger, "%p stream via RTP-over-%s", pPlayer, pPlayer->m_STREAMING_METHOD==2? "TCP":"UDP");			
			rtspClient->sendSetupCommand(*pPlayer->m_pMediaSubsession, CRtspClientPlayer::continueAfterSETUP, False, pPlayer->m_STREAMING_METHOD==2);
		}
	}
}

void CRtspClientPlayer::Shutdown(int exitCode)
{
	if( m_pRTSPClient == NULL ) return;

	int status = GetStatus();
	if( m_pTimeDealTask ) 
	{
		m_pUsageEnvironment->taskScheduler().unscheduleDelayedTask(m_pTimeDealTask);
		m_pTimeDealTask = NULL;
	}
	// First, check whether any subsessions have still to be closed:
	if( m_pMediaSession )
	{
		Boolean someSubsessionsWereActive = False;
		MediaSubsessionIterator iter(*m_pMediaSession );
		MediaSubsession *subsession;
		while((subsession = iter.next()) != NULL) 
		{
			if( subsession->sink != NULL) 
			{
				Medium::close(subsession->sink);
				subsession->sink = NULL;

				if( subsession->rtcpInstance() != NULL ) subsession->rtcpInstance()->setByeHandler(NULL, NULL); // in case the server sends a RTCP "BYE" while handling "TEARDOWN"

				someSubsessionsWereActive = True;
			}
		}

		if( someSubsessionsWereActive ) 
		{
			// Send a RTSP "TEARDOWN" command, to tell the server to shutdown the stream.
			// Don't bother handling the response to the "TEARDOWN".
			THIS_LOGI_print("Send TEARDOWN req packet while status=%d", status);			
			m_pRTSPClient->sendTeardownCommand(*m_pMediaSession, NULL);
		}

		m_pMediaSession = NULL;
	}

	#ifdef _DEBUG
	if( exitCode == 0 )
		THIS_LOGI_print("Close stream: %p", m_pRTSPClient);
	else
		THIS_LOGE_print("Close stream: %p, errorcode: %d", m_pRTSPClient, exitCode);
	#endif
	Medium::close(m_pRTSPClient); m_pRTSPClient = NULL;
	m_nSeekpos =-1; //must reset
	m_position = 0;

	if( status!= PS_Stopped)
	{
		SetStatus(PS_Paused);
		if( exitCode == 0 )
		{
			m_isFirst = 1; //reset
			if( status == PS_Playing) Java_fireNotify(m_pJNIEnv, this, m_duration!=0? MEDIA_PLAY_COMPLETED:MEDIA_ERR_PLAY, 0, 0, 0);
		}
		else
		{
			if( status == PS_Prepare) Java_fireNotify(m_pJNIEnv, this, MEDIA_ERR_PREPARE   , 0, 0, 0);
//			if( status == PS_Seeking) Java_fireNotify(m_pJNIEnv, this, MEDIA_ERR_SEEK      , 0, 0, 0);
			if( status == PS_Pause1 ||
				status == PS_Pause2 ) Java_fireNotify(m_pJNIEnv, this, MEDIA_ERR_PAUSE     , 0, 0, 0);
			if( status == PS_Playing) Java_fireNotify(m_pJNIEnv, this, MEDIA_ERR_PLAY      , 0, 0, 0);		
		}
	}
 }
