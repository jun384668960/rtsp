#include "JNI_load.h"
#include "com_bql_MediaPlayer_Native.h"
#include "com_bql_MSG_Native.h"
#include "codec/HwCodec.h"
#include "Streamer.h"
#ifdef __cplusplus
extern "C" {
#endif
#include <libavformat/avformat.h>
#include <log.h> //rtmplog
#ifdef __cplusplus
}
#endif

#ifdef _DEBUG //ʵ��д��־�ļ��ӿ�
#ifdef _LOGFILE
extern FILE  *__logfp; //��com_bql_MediaPlayer_Native.cpp�ﶨ��/��ʼ��, ��JNI_load.cpp���ͷ�
#endif
#endif

JavaVM *g_JVM = NULL; //global vm
CHwCodec *g_pHwCodec = NULL; //global hw
///////////////////////////////////////////////////////////////////
extern JNIMediaPlayer   g_jBQLMediaPlayer; //com_bql_MSG_Native.cpp
extern JNIBuffer        g_jBQLBuffer;
extern JNIMediaRecorder g_jBQLMediaRecorder;
extern JNIAudioTrack    g_jAudioTrack;
///////////////////////////////////////////////////////////////////
#define MEDIAPLAYER_JAVA_CLASS_PATH "com/core/BQLMediaPlayer"
static JNINativeMethod s_method_table1[] = { //for mediaplayer
	{ "native_malloc", 				"(Ljava/lang/String;Ljava/lang/String;)J", 	(void*)Java_com_bql_Malloc },
	{ "native_setup", 				"(JLjava/lang/Object;Ljava/lang/String;)J", (void*)Java_com_bql_CreatePlayer },
	{ "native_setParams",			"(JLjava/lang/String;)Z",		(void*)Java_com_bql_SetParamConfig },	
	{ "native_prepareAsync",		"(J)V", 						(void*)Java_com_bql_PrepareAsync },
	{ "native_setDisplay",			"(JLandroid/view/Surface;)Z",	(void*)Java_com_bql_SetSurfaceWindows },
	{ "native_setVolume", 			"(JFF)Z", 						(void*)Java_com_bql_SetVolume },
	{ "native_setMute", 			"(JZ)Z", 						(void*)Java_com_bql_SetMute },
	{ "native_getDuration", 		"(J)I", 						(void*)Java_com_bql_GetDuration },
	{ "native_getCurrentPosition",	"(J)I", 						(void*)Java_com_bql_GetPosition },
	{ "native_getVideoWidth",		"(J)I", 						(void*)Java_com_bql_GetVideoWidth },
	{ "native_getVideoHeight",		"(J)I", 						(void*)Java_com_bql_GetVideoHeight },
	{ "native_getAudioChannel",		"(J)I", 						(void*)Java_com_bql_GetAudioChannels },
	{ "native_getAudioDepth",		"(J)I", 						(void*)Java_com_bql_GetAudioDepth },
	{ "native_getAudioSampleRate",	"(J)I", 						(void*)Java_com_bql_GetAudioSampleRate },
	{ "native_getPlayBufferTime",	"(J)I", 						(void*)Java_com_bql_GetPlayBufferTime },
	{ "native_setPlayBufferTime",	"(JI)Z", 						(void*)Java_com_bql_SetPlayBufferTime },
	{ "native_setHardwareDecode",	"(JI)V",						(void*)Java_com_bql_SetHardwareDecode },	
	{ "native_snapshot", 			"(JIILjava/lang/String;)Z",		(void*)Java_com_bql_Snapshot },
	{ "native_setData", 			"(JIZ)V",						(void*)Java_com_bql_Subscribe },
	{ "native_play",				"(J)Z", 						(void*)Java_com_bql_Play },	
	{ "native_pause",				"(J)Z", 						(void*)Java_com_bql_Pause },
	{ "native_seekTo",				"(JI)Z", 						(void*)Java_com_bql_Seek },
	{ "native_stop", 				"(J)V", 						(void*)Java_com_bql_DeletePlayer },
	{ "native_free",				"(J)V", 						(void*)Java_com_bql_Free },
};

static int register_com_bql_MediaPlayer(JNIEnv *env) 
{
	jclass clazz = env->FindClass( MEDIAPLAYER_JAVA_CLASS_PATH );
	if( clazz == NULL ) {
		g_jBQLMediaPlayer.thizClass = 0;
		return -1;
	}
	if( env->RegisterNatives(clazz, s_method_table1, sizeof(s_method_table1) / sizeof(JNINativeMethod)) < 0) {
		g_jBQLMediaPlayer.thizClass = 0;
	    return -2;
	}

	g_jBQLMediaPlayer.thizClass = (jclass)env->NewGlobalRef(clazz);
	g_jBQLMediaPlayer.post = env->GetStaticMethodID( clazz, "postEventFromNative", "(Ljava/lang/Object;IIILjava/lang/Object;)V");
	return 0;
}

static void unregister_com_bql_MediaPlayer(JNIEnv *env) 
{
	if( g_jBQLMediaPlayer.thizClass ) {
		env->DeleteGlobalRef(g_jBQLMediaPlayer.thizClass);
	}
}

#define BQLBUFFER_JAVA_CLASS_PATH "com/core/BQLBuffer"
static JNINativeMethod s_method_table2[] = { //for Buffer
	{ "native_getThumbnail", 	"(Ljava/lang/String;IIII)Ljava/lang/Object;",  (void*)Java_com_bql_GetThumbnail },
	{ "native_releaseBuffer", 	"(J)V",  (void*)Java_com_bql_ReleaseBuffer },
};

static int register_com_bql_Buffer(JNIEnv *env)
{
	jclass clazz = env->FindClass( BQLBUFFER_JAVA_CLASS_PATH );
	if( clazz == NULL ) {
		g_jBQLBuffer.thizClass = 0;
		return -1;
	}
	if( env->RegisterNatives(clazz, s_method_table2, sizeof(s_method_table2) / sizeof(JNINativeMethod)) < 0) {
		g_jBQLBuffer.thizClass = 0;
	    return -2;
	}

	g_jBQLBuffer.thizClass = (jclass)env->NewGlobalRef(clazz);
	g_jBQLBuffer.constructor = env->GetMethodID(clazz, "<init>", "(JILjava/nio/ByteBuffer;JJJJ)V");
	return 0;
}

static void unregister_com_bql_Buffer(JNIEnv *env)
{
	if( g_jBQLBuffer.thizClass ) {
		env->DeleteGlobalRef(g_jBQLBuffer.thizClass);
	}
}

#define BQLMEDIARECORDER_JAVA_CLASS_PATH "com/core/BQLMediaRecorder"
static JNINativeMethod s_method_table3[] = { //for MediaRecorder
	{ "native_malloc", 		 	"(Ljava/lang/String;Ljava/lang/String;)J", 		(void*)Java_com_bql_Malloc },
	{ "native_free",			"(J)V", 										(void*)Java_com_bql_Free },
	{ "native_createRecorder", 	"(Ljava/lang/Object;JLjava/lang/String;II)J", 	(void*)Java_com_bql_CreateRecorder },
	{ "native_setVideoStream", 	"(JIIIII)Z", 									(void*)Java_com_bql_SetVideoStream },
	{ "native_setAudioStream", 	"(JIIII)Z", 									(void*)Java_com_bql_SetAudioStream },
	{ "native_prepareWrite"  , 	"(J)Z",     									(void*)Java_com_bql_PrepareWrite },
	{ "native_write"         , 	"(JIJJ)Z",  									(void*)Java_com_bql_Write },
	{ "native_getRecordTime" ,	"(J)I",											(void*)Java_com_bql_GetRecordTime },
	{ "native_deleteRecorder", 	"(J)V",     									(void*)Java_com_bql_DeleteRecorder },
	{ "native_acquireBuffer", 	"(JI)Ljava/lang/Object;", 						(void*)Java_com_bql_AcquireBuffer },
	{ "native_setBufferSize", 	"(JI)J", 										(void*)Java_com_bql_SetBufferSize },
};

static int register_com_bql_MediaRecorder(JNIEnv *env)
{
	jclass clazz = env->FindClass( BQLMEDIARECORDER_JAVA_CLASS_PATH );
	if( clazz == NULL ) {
		g_jBQLMediaRecorder.thizClass = 0;
		return -1;
	}
	if( env->RegisterNatives(clazz, s_method_table3, sizeof(s_method_table3) / sizeof(JNINativeMethod)) < 0) {
		g_jBQLMediaRecorder.thizClass = 0;
	    return -2;
	}

	g_jBQLMediaRecorder.thizClass = (jclass)env->NewGlobalRef(clazz);
	g_jBQLMediaRecorder.post = env->GetStaticMethodID( clazz, "postEventFromNative", "(Ljava/lang/Object;II)V");	
	return 0;
}

static void unregister_com_bql_MediaRecorder(JNIEnv *env)
{
	if( g_jBQLMediaRecorder.thizClass ) {
		env->DeleteGlobalRef(g_jBQLMediaRecorder.thizClass);
	}
}

#define AUDIOTRACK_JAVA_CLASS_PATH "android/media/AudioTrack"
static int register_android_media_AudioTrack(JNIEnv *env)
{
	jclass clazz = env->FindClass( AUDIOTRACK_JAVA_CLASS_PATH );
	if( clazz == NULL ) {
		g_jAudioTrack.thizClass = 0;
		return -1;
	}

	g_jAudioTrack.thizClass = (jclass)env->NewGlobalRef(clazz);
	g_jAudioTrack.constructor = env->GetMethodID(clazz, "<init>","(IIIIII)V");
	g_jAudioTrack.getMinBufferSize = env->GetStaticMethodID(clazz, "getMinBufferSize", "(III)I");
	g_jAudioTrack.setVolume = env->GetMethodID(clazz, "setStereoVolume", "(FF)I");
	g_jAudioTrack.play = env->GetMethodID(clazz, "play", "()V");
	g_jAudioTrack.write = env->GetMethodID(clazz, "write", "([BII)I");
	g_jAudioTrack.stop = env->GetMethodID(clazz, "stop", "()V");
	g_jAudioTrack.release = env->GetMethodID(clazz, "release", "()V");
	return 0;
}

static void unregister_android_media_AudioTrack(JNIEnv *env)
{
	if( g_jAudioTrack.thizClass ) {
		env->DeleteGlobalRef(g_jAudioTrack.thizClass);
	}
}

#ifdef _DEBUG //关闭rtmp日志文件句柄
static void rtmp_log(int level, const char *format, va_list vl)
{	
	char txt[2048]; strcpy(txt, "%s"), vsnprintf(txt + 2, 2048 - 1, format, vl);
	if( level < 0 ) level = 0;
	__log_write(level + 2, "rtmp", txt, "");
}
#endif

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
    JNIEnv *env;

	if( vm->GetEnv((void**)&env, JNI_VERSION_1_4) != JNI_OK ) {
		return -1;
	}

	//打印版本信息以及编译时间
	__android_log_print(ANDROID_LOG_INFO, "com_bql_streamuser", "1.3.1-20170323 builded on %s %s"        , __DATE__, __TIME__);

	if( register_android_media_AudioTrack(env) < 0) {
		__android_log_print(ANDROID_LOG_ERROR, "com_bql_streamuser", "register AudioTrack");
		return -1;
	}

	if( register_com_bql_MediaPlayer(env) < 0 ) {
		__android_log_print(ANDROID_LOG_ERROR, "com_bql_streamuser", "register MediaPlayer");
		return -1;
	}

	if( register_com_bql_Buffer(env) < 0 ) {
		__android_log_print(ANDROID_LOG_ERROR, "com_bql_streamuser", "register Buffer");
		return -1;
	}

	if( register_com_bql_MediaRecorder(env) < 0 ) {
		__android_log_print(ANDROID_LOG_ERROR, "com_bql_streamuser", "register MediaRecorder");
		return -1;
	}

	g_JVM = vm; //init g_JVM
	av_register_all(); //init ffmpeg

	#ifdef _DEBUG //关闭rtmp日志文件句柄
	#ifdef _RELEASE
//	RTMP_LogSetCallback(2/* INFO*/, rtmp_log);
	#else
	RTMP_LogSetCallback(0/*TRACE*/, rtmp_log);
	#endif
	#endif

	g_pBufferManager = new CBufferManager(0, 8);
	return JNI_VERSION_1_4;
}

JNIEXPORT void JNI_OnUnload(JavaVM *vm, void *reserved)
{
    JNIEnv *env;

	if( vm->GetEnv((void**)&env, JNI_VERSION_1_4) != JNI_OK ) {
		return;
	}

	unregister_com_bql_Buffer(env);
	unregister_com_bql_MediaRecorder(env);
	unregister_com_bql_MediaPlayer(env);
	unregister_android_media_AudioTrack(env);

	if( g_pHwCodec ) {
		delete g_pHwCodec;
		g_pHwCodec = NULL; //必须复位
	}

	g_pBufferManager->Release();
	g_pBufferManager = NULL;

	#ifdef _DEBUG //关闭日志文件句柄
	#ifdef _LOGFILE
	if( __logfp!=0 ) {
		fclose(__logfp);
		__logfp = NULL; //必须复位
	}
	#endif
	#endif

	__android_log_print(ANDROID_LOG_INFO, "com_bql_streamuser", "OnUnload");	
}
