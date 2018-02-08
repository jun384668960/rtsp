#include "AudioTrack.h"
#include "JNI_load.h"

#define AudioManager_STREAM_MUSIC 3

#define AudioFormat_CHANNEL_OUT_MONO 0x4
#define AudioFormat_CHANNEL_OUT_STEREO 0x4|0x8

#define AudioFormat_ENCODING_PCM_16BIT 2
#define AudioFormat_ENCODING_PCM_8BIT  3

#define AudioTrack_MODE_STATIC 0
#define AudioTrack_MODE_STREAM 1

extern JNIAudioTrack  g_jAudioTrack;  //定义在 com_bql_MSG_Native.cpp 初始化在jni_onload.cpp

CLASS_LOG_IMPLEMENT(CAudioTrack, "CAudioTrack");

int CAudioTrack::Create(JNIEnv *env, uint32_t channel, uint32_t sample_rate, uint32_t bit_depth)
{
	THIS_LOGT_print("create channels=%d, samplerate=%d, depth=%d", channel, sample_rate, bit_depth );

	assert(bit_depth == 8 || bit_depth == 16);
	assert(channel == 1 || channel == 2);

	m_audio_format_bit_depth = bit_depth==8? AudioFormat_ENCODING_PCM_8BIT:AudioFormat_ENCODING_PCM_16BIT;
	m_audio_format_channel = channel==1? AudioFormat_CHANNEL_OUT_MONO:AudioFormat_CHANNEL_OUT_STEREO;
	m_sample_rate = sample_rate ;

	mEventLoopExit = false ;
	pthread_mutex_init(&mAudioEventMutex, NULL );
	pthread_cond_init(&mAudioEventCond , NULL);
	int ret = ::pthread_create(&mEventLoopTh, NULL , CAudioTrack::ThreadProc, (void*)this );

	return 0;
}

int CAudioTrack::SetVolume(JNIEnv *env, jfloat r, jfloat l)
{
	m_avr = r;
	m_avl = l;
	if( m_audioTrack == 0 ) 
		return 0;
	else
		return env->CallIntMethod(m_audioTrack, g_jAudioTrack.setVolume, m_avr, m_avl);
}

int CAudioTrack::Write(JNIEnv *env, CBuffer* pPcmdata )
{
	//COST_STAT_DECLARE(cost);

	//THIS_LOGT_print("CAudioTrack::Write pPcmdata %p" , pPcmdata);
	jboolean result = sendEvent(AUD_RENDER_PCM , pPcmdata);
	if( result == JNI_FALSE ){
		THIS_LOGE_print("CAudioTrack::Thread is going to exit but Write");
		pPcmdata->Release() ;
	}

	//THIS_LOGT_print("write audio, cost: %lld", cost.Get());
	return 0;
}

int CAudioTrack::Delete(JNIEnv *env)
{
	if(  mEventLoopTh != -1 ){
		THIS_LOGT_print("send msg to exit loop");
		sendEvent( AUD_RENDER_EXIT , NULL);
		::pthread_join(mEventLoopTh , NULL);
		mEventLoopTh = -1;
		pthread_mutex_destroy(&mAudioEventMutex);
		pthread_cond_destroy(&mAudioEventCond);
		THIS_LOGT_print("loop exit done");
	}else{
		THIS_LOGE_print("loop not created before ");
	}
	return 0;
}

void CAudioTrack::Loop()
{
	int miniSize = mJNIPlayEnv->CallStaticIntMethod(g_jAudioTrack.thizClass, g_jAudioTrack.getMinBufferSize,
																m_sample_rate,
																m_audio_format_channel,
																m_audio_format_bit_depth);

	jobject audioTrack = mJNIPlayEnv->NewObject(g_jAudioTrack.thizClass, g_jAudioTrack.constructor,
																AudioManager_STREAM_MUSIC,
																m_sample_rate,
																m_audio_format_channel,
																m_audio_format_bit_depth,
																2*miniSize ,
																AudioTrack_MODE_STREAM);
	m_audioTrack = mJNIPlayEnv->NewGlobalRef(audioTrack);
	mJNIPlayEnv->DeleteLocalRef(audioTrack);

	mJNIPlayEnv->CallVoidMethod(m_audioTrack, g_jAudioTrack.play);

//	THIS_LOGT_print("call AudioTrack sample_rate = %d  format_channel = %d  bit_depth = %d  miniSize = %d "  ,
//								m_sample_rate , m_audio_format_channel , m_audio_format_bit_depth ,  miniSize );

	jboolean isloop = JNI_TRUE ;
	while( isloop ){

		JNIAUDMsg* msg = NULL ;
		pthread_mutex_lock(&mAudioEventMutex);
		if( mEventList.empty() == false )
		{
			msg = mEventList.front();
			mEventList.pop_front();
		}else{
			pthread_cond_wait(&mAudioEventCond , &mAudioEventMutex );
		}
		pthread_mutex_unlock(&mAudioEventMutex);

		if( msg != NULL ){
			switch( msg->msg_type ){
				case AUD_RENDER_PCM  :
				{

					CScopeBuffer pPcmdata((CBuffer*)msg->ptr );
					uint8_t * pData = pPcmdata->GetPacket()->data;
					int data_len = pPcmdata->GetPacket()->size;

					if( m_bufferSize == 0 ){ // 第一次写数据 1.创建ByteArray 2.调用play准备播放
						m_jByteArray = mJNIPlayEnv->NewByteArray(data_len);
						m_bufferSize = data_len ;
						THIS_LOGT_print("call first NewByteArray %d " , m_bufferSize);
					}else if( data_len > m_bufferSize ){
						mJNIPlayEnv->DeleteLocalRef(m_jByteArray);
						m_bufferSize = data_len ;
						m_jByteArray = mJNIPlayEnv->NewByteArray( m_bufferSize );
						THIS_LOGT_print("recreate NewByteArray %d " , m_bufferSize);
					}

					mJNIPlayEnv->SetByteArrayRegion(m_jByteArray, 0, data_len, (jbyte *)pData);
					if( mJNIPlayEnv->ExceptionCheck()) {
						mJNIPlayEnv->ExceptionClear();
						THIS_LOGT_print("call SetByteArrayRegion error! ");
						break;
					}

					mJNIPlayEnv->CallIntMethod(m_audioTrack, g_jAudioTrack.write, m_jByteArray, 0, data_len);
					if( mJNIPlayEnv->ExceptionCheck()) {
						mJNIPlayEnv->ExceptionClear();
						THIS_LOGT_print("call AudioTrack.write error! ");
						break;
					}
				}
				break;

				case AUD_RENDER_EXIT :
				{
					isloop = JNI_FALSE ;
				}
				break;
				default:
					THIS_LOGD_print("unknown msg %d " , msg->msg_type );
				break;
			}
			delete msg ;
		}
	}


	THIS_LOGD_print("remaining list size = %d " , mEventList.size()  );
	while( mEventList.empty() == false  )
	{
		JNIAUDMsg* msg = mEventList.front();
		mEventList.pop_front();
		CBuffer* data = (CBuffer*)msg->ptr ;
		data->Release();
		delete msg ;
		THIS_LOGT_print("remaining release CBuffer %p", data);
	}


	if( m_audioTrack ){
		mJNIPlayEnv->CallVoidMethod(m_audioTrack, g_jAudioTrack.stop);
		mJNIPlayEnv->CallVoidMethod(m_audioTrack, g_jAudioTrack.release);
		mJNIPlayEnv->DeleteGlobalRef(m_audioTrack);

		THIS_LOGT_print("delete AudioTrack: %p", m_audioTrack);
		m_audioTrack = 0;
	}

	if( m_jByteArray ){
		mJNIPlayEnv->DeleteLocalRef(m_jByteArray);
		m_jByteArray = 0;
	}

}

void* CAudioTrack::ThreadProc(void *pParam)
{
	CAudioTrack *pThis = (CAudioTrack *)pParam;
	JNI_ATTACH_JVM_WITH_NAME( pThis->mJNIPlayEnv,"CAudioTrack");
	pThis->Loop();
	JNI_DETACH_JVM(pThis->mJNIPlayEnv);
}

jboolean CAudioTrack::sendEvent(int msg_type , void* ptr )
{
	pthread_mutex_lock(&mAudioEventMutex);
	if( mEventLoopExit ) {
		THIS_LOGE_print("Event Loop Already Exit ! drop msg !");
		pthread_mutex_unlock(&mAudioEventMutex);
		return JNI_FALSE ;
	}else if(msg_type == AUD_RENDER_EXIT ){
		mEventLoopExit = true ;
	}

	JNIAUDMsg* pmsg = new JNIAUDMsg();
	pmsg->msg_type = msg_type;
	pmsg->ptr = ptr;

	bool old_empty = mEventList.empty() ;
	//THIS_LOGD_print("list size = %d " , mEventList.size()  );
	if( msg_type == AUD_RENDER_EXIT){
		/*
		 *  Note:
		 *  	if exit, stop track writing right away
		 * 		push the exit message at the front
		 * */
		mEventList.push_front(pmsg);
	}else{
		mEventList.push_back(pmsg);
	}
	if( old_empty ){
		pthread_cond_signal(&mAudioEventCond);
	}
	pthread_mutex_unlock(&mAudioEventMutex);
	return JNI_TRUE ;
}

