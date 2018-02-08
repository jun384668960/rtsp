#ifndef __AUDIOTRACK_H__
#define __AUDIOTRACK_H__

#include <list>
#include <jni.h>
#include "com_bql_MSG_Native.h"

#include "buffer/BufferManager.h"

typedef struct{
#define AUD_RENDER_PCM  1
#define AUD_RENDER_EXIT 2
	int msg_type ;
	void* ptr ;
} JNIAUDMsg ;


class CAudioTrack 
{
public:
	CAudioTrack()
	  : m_audioTrack(0), m_jByteArray(0), m_avr(1.0), m_avl(1.0) , m_bufferSize(0) ,
	    m_audio_format_bit_depth(0) , m_audio_format_channel(0) , m_sample_rate(0) , mEventLoopTh(-1)
	{
	}

public:
	int SetVolume(JNIEnv *env, jfloat a, jfloat b);
	int Write(JNIEnv *env, CBuffer* pPcmdata );

	int Create(JNIEnv *env, uint32_t channel, uint32_t sample_rate, uint32_t bit_depth = 16);
	int Delete(JNIEnv *env);
	jboolean sendEvent(int msg_type , void* ptr ) ;

public:
	JNIEnv         *mJNIPlayEnv;
	pthread_t		mEventLoopTh ;
	pthread_mutex_t mAudioEventMutex ;
	pthread_cond_t 	mAudioEventCond ;
	jboolean 		mEventLoopExit;
	std::list<JNIAUDMsg*> mEventList ;


private:

	jobject    m_audioTrack;
	int32_t    m_bufferSize;
	jbyteArray m_jByteArray;
	jfloat     m_avr, m_avl; //volume:0.0~1.0
	CLASS_LOG_DECLARE(CAudioTrack);

public:
	int m_audio_format_bit_depth ;
	int m_audio_format_channel ;
	int m_sample_rate ;
	void Loop();
	static void *ThreadProc(void *pParam);

};

#endif
