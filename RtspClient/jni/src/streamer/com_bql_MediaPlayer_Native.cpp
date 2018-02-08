#include "com_bql_MediaPlayer_Native.h"
#include "Streamer.h"
#include "JNI_load.h"
#include "codec/HwCodec.h"
#include <unistd.h>
//由于使用stl，必须在jni/Application.mk里定义: APP_STL := stlport_static

#ifdef _DEBUG //定义日志文件句柄/打印锁
#ifdef _LOGFILE
CMutex __logmutex;
FILE  *__logfp = NULL;
#endif
#endif

extern JNIBuffer   g_jBQLBuffer;

LOCAL_LOG_IMPLEMENT(s_logger, "com_bql_streamuser");
///////////////////////////////////////////////////////////////////
JNIEXPORT jlong    JNICALL Java_com_bql_Malloc(JNIEnv *env, jobject thiz, jstring jcfg, jstring jpkg)
{
	char *pkg = NULL;
	JNI_GET_UTF_CHAR(pkg, jpkg);

	LOGI_print(s_logger, "call Malloc: pkg=%s", pkg);
	const std::string &path = std::string("/data/data/") + pkg;

	JNI_RELEASE_STR_STR(pkg, jpkg);

	#ifdef _DEBUG //初始化日志文件句柄 /data/data/<packageName>/streamuser.txt
	#ifdef _LOGFILE	
	if( __logfp == 0 ) {
	#define LOGNAME "/mnt/sdcard/streamuser.txt"
	__logmutex.Enter();
	#ifdef _RELEASE
	if( __logfp == 0 ) { __logfp = fopen(LOGNAME, "at+");
	if( __logfp != 0 ) {
		struct timeval val;
		gettimeofday(&val, 0);
		struct tm     *now = localtime(&val.tv_sec ); 
		char buf[64], *log = fgets(buf, 63, __logfp);
		if( log &&
			now->tm_mday != atol(log + 8) ) //YYYY/MM/DD hh:mm:ss,ms, 保留当天日志
		{
			fclose(__logfp);
			unlink(LOGNAME);
			__logfp = fopen(LOGNAME, "at+");
		}
	}
	}
	#else
	if( __logfp == 0 ) { __logfp = fopen(LOGNAME, "at+");}
	#endif
	__logmutex.Leave();
	}
	#endif
	#endif

	if( g_pHwCodec == NULL ) g_pHwCodec = new CHwCodec((path + "/lib/libhw-codec-jni.so").c_str()); // /data/data/<packageName>/lib/libhw-codec-jni.so
	g_pBufferManager->AddRef();

	LOGI_print(s_logger, "result: %p", g_pBufferManager);
	return (jlong)g_pBufferManager;
}

JNIEXPORT void     JNICALL Java_com_bql_Free(JNIEnv *env, jobject thiz, jlong ctx)
{
	LOGI_print(s_logger, "call Free: ctx=%p", (void*)ctx);
	g_pBufferManager->Release();
}

////////////////////////////////////////////////////////////////////////
JNIEXPORT jlong    JNICALL Java_com_bql_CreatePlayer(JNIEnv *env, jobject thiz, jlong ctx, jobject obj, jstring juri )
{
	char *uri = NULL;
	JNI_GET_UTF_CHAR(uri, juri);

	LOGI_print(s_logger, "call CreatePlayer: ctx=%p, obj=%p, url=%s", (void*)ctx, obj, uri);
	CPlayer *pPlayer = CStreamer::CreateMyPlayer(env, obj, uri);

	JNI_RELEASE_STR_STR(uri, juri);
	LOGI_print(s_logger, "result: %p", pPlayer);
	return (jlong)pPlayer;
}

JNIEXPORT jboolean JNICALL Java_com_bql_SetParamConfig(JNIEnv *env, jobject thiz, jlong ctx, jstring jval)
{
	char  key[64];
	char  val[64];
	char *str = NULL;
	JNI_GET_UTF_CHAR(str, jval);

	LOGI_print(s_logger, "call SetParamConfig: ctx=%p, val=%s", (void*)ctx, str);
	CPlayer *pPlayer = (CPlayer *)ctx;
	int ret = 2!=sscanf(str, "%[^=]=%s", key, val)? 1:pPlayer->SetParamConfig(key, val);

	JNI_RELEASE_STR_STR(str, jval);
	LOGI_print(s_logger, "result: %d", ret);
	return ret == 0;
}

JNIEXPORT void     JNICALL Java_com_bql_SetHardwareDecode(JNIEnv *env, jobject thiz, jlong ctx, jint mode)
{
	LOGI_print(s_logger, "call SetHardwareDecode: ctx=%p, mode=%d", (void*)ctx, mode);
	CPlayer *pPlayer = (CPlayer *)ctx;	
	pPlayer->m_hwmode= mode + 1; //0 1 2
}

JNIEXPORT jboolean JNICALL Java_com_bql_SetSurfaceWindows(JNIEnv *env, jobject thiz, jlong ctx, jobject objSurface)
{
	LOGI_print(s_logger, "call SetSurfaceWindows: ctx=%p, objSurface=%p", (void*)ctx, objSurface);
	CPlayer *pPlayer = (CPlayer *)ctx;	
	int ret = pPlayer->SetSurfaceWindows(env, ANativeWindow_fromSurface(env, objSurface));

	LOGI_print(s_logger, "result: %d", ret);
	return ret == 0;
}

JNIEXPORT jboolean JNICALL Java_com_bql_SetVolume(JNIEnv *env, jobject thiz, jlong ctx, jfloat a, jfloat b)
{
	LOGI_print(s_logger, "call SetVolume: ctx=%p, a=%.2f, b=%.2f", (void*)ctx, a, b);
	CPlayer *pPlayer = (CPlayer *)ctx;	
	int ret = pPlayer->SetVolume(env, a, b);

	LOGI_print(s_logger, "result: %d", ret);
	return ret == 0;
}

JNIEXPORT jboolean JNICALL Java_com_bql_SetMute(JNIEnv *env, jobject thiz, jlong ctx, jboolean mute)
{
	LOGI_print(s_logger, "call SetMute: ctx=%p, mute=%d", (void*)ctx, mute? 1:0);
	CPlayer *pPlayer = (CPlayer *)ctx;	
	int ret = pPlayer->SetMute(env, mute);

	LOGI_print(s_logger, "result: %d", ret);
	return ret == 0;
}

JNIEXPORT void     JNICALL Java_com_bql_PrepareAsync(JNIEnv *env, jobject thiz, jlong ctx)
{
	LOGI_print(s_logger, "call Prepare: ctx=%p", (void*)ctx);
	CPlayer *pPlayer = (CPlayer *)ctx;	
	int ret = pPlayer->Prepare();

	LOGI_print(s_logger, "result: %d", ret);
}

JNIEXPORT jint     JNICALL Java_com_bql_GetDuration(JNIEnv *env, jobject thiz, jlong ctx)
{
	LOGI_print(s_logger, "call GetDuration: ctx=%p", (void*)ctx);
	CPlayer *pPlayer = (CPlayer *)ctx;	
	int ret = pPlayer->GetDuration();

	LOGI_print(s_logger, "result: %d", ret);
	return ret;
}

JNIEXPORT jint     JNICALL Java_com_bql_GetPosition(JNIEnv *env, jobject thiz, jlong ctx)
{
//	LOGI_print(s_logger, "call GetPosition: ctx=%p", (void*)ctx);
	CPlayer *pPlayer = (CPlayer *)ctx;	
	return pPlayer->GetPosition();
}

JNIEXPORT jint     JNICALL Java_com_bql_GetVideoWidth(JNIEnv *env, jobject thiz, jlong ctx)
{
	LOGI_print(s_logger, "call GetVideoW: ctx=%p", (void*)ctx);
	CPlayer *pPlayer = (CPlayer *)ctx;	
	int val = 0, ret = pPlayer->GetVideoConfig(0, &val, 0);

	LOGI_print(s_logger, "result: %d", val);
	return val;
}

JNIEXPORT jint     JNICALL Java_com_bql_GetVideoHeight(JNIEnv *env, jobject thiz, jlong ctx)
{
	LOGI_print(s_logger, "call GetVideoH: ctx=%p", (void*)ctx);
	CPlayer *pPlayer = (CPlayer *)ctx;
	int val = 0, ret = pPlayer->GetVideoConfig(0, 0, &val);

	LOGI_print(s_logger, "result: %d", val);	
	return val;
}

JNIEXPORT jint     JNICALL Java_com_bql_GetAudioChannels(JNIEnv *env, jobject thiz, jlong ctx)
{
	LOGI_print(s_logger, "call GetAudioChannels: ctx=%p", (void*)ctx);
	CPlayer *pPlayer = (CPlayer *)ctx;	
	int val = 0, ret = pPlayer->GetAudioConfig(&val, 0, 0);

	LOGI_print(s_logger, "result: %d", val);		
	return val;
}

JNIEXPORT jint     JNICALL Java_com_bql_GetAudioDepth(JNIEnv *env, jobject thiz, jlong ctx)
{
	LOGI_print(s_logger, "call GetAudioDepth: ctx=%p", (void*)ctx);
	CPlayer *pPlayer = (CPlayer *)ctx;	
	int val = 0, ret = pPlayer->GetAudioConfig(0, &val, 0);

	LOGI_print(s_logger, "result: %d", val);		
	return val;
}

JNIEXPORT jint     JNICALL Java_com_bql_GetAudioSampleRate(JNIEnv *env, jobject thiz, jlong ctx)
{
	LOGI_print(s_logger, "call GetAudioSampleRate: ctx=%p", (void*)ctx);
	CPlayer *pPlayer = (CPlayer *)ctx;	
	int val = 0, ret = pPlayer->GetAudioConfig(0, 0, &val);

	LOGI_print(s_logger, "result: %d", val);		
	return val;
}

JNIEXPORT jint     JNICALL Java_com_bql_GetPlayBufferTime(JNIEnv *env, jobject thiz, jlong ctx)
{
	LOGI_print(s_logger, "call GetPlayBufferTime: ctx=%p", (void*)ctx);
	CPlayer *pPlayer = (CPlayer *)ctx;	
	int ret = pPlayer->GetPlayBufferTime();
	LOGI_print(s_logger, "result: %d",ret);		
	return ret;
}

JNIEXPORT jboolean JNICALL Java_com_bql_SetPlayBufferTime(JNIEnv *env, jobject thiz, jlong ctx, jint time)
{
	LOGI_print(s_logger, "call SetPlayBufferTime: ctx=%p, time=%d", (void*)ctx, time);
	CPlayer *pPlayer = (CPlayer *)ctx;	
	int ret = pPlayer->SetPlayBufferTime(time);
	LOGI_print(s_logger, "result: %d",ret);		
	return ret == 0;
}

JNIEXPORT void     JNICALL Java_com_bql_Subscribe(JNIEnv * env, jobject thiz, jlong ctx, jint type, jboolean isON)
{
	LOGI_print(s_logger, "call Subscribe: ctx=%p, type=%d, onoff=%d", (void*)ctx, type, isON? 1:0);
	CPlayer *pPlayer = (CPlayer *)ctx;	
	int ret = pPlayer->Subscribe(type, isON != false);

	LOGI_print(s_logger, "result: %d", ret);
}

JNIEXPORT jboolean JNICALL Java_com_bql_Snapshot(JNIEnv *env, jobject thiz, jlong ctx, jint width, jint height, jstring jpath)
{
	char *path = NULL;
	JNI_GET_UTF_CHAR(path, jpath);
	LOGI_print(s_logger, "call Snapshot: ctx=%p, videow=%d, videoh=%d, path=%s", (void*)ctx, width, height, path);	
	CPlayer *pPlayer = (CPlayer*)ctx;	
	int ret = pPlayer->Snapshot(env, width, height, path);
	JNI_RELEASE_STR_STR(path, jpath);

	LOGI_print(s_logger, "result: %d", ret);
	return ret == 0;
}

JNIEXPORT jboolean JNICALL Java_com_bql_Play(JNIEnv *env, jobject thiz, jlong ctx )
{
	LOGI_print(s_logger, "call Play: ctx=%p", (void*)ctx);
	CPlayer *pPlayer = (CPlayer *)ctx;	
	int ret = pPlayer->Play();

	LOGI_print(s_logger, "result: %d", ret);
	return ret == 0;
}

JNIEXPORT jboolean JNICALL Java_com_bql_Pause(JNIEnv *env, jobject thiz, jlong ctx)
{
	LOGI_print(s_logger, "call Pause: ctx=%p", (void*)ctx);
	CPlayer *pPlayer = (CPlayer *)ctx;	
	int ret = pPlayer->Pause();

	LOGI_print(s_logger, "result: %d", ret);	
	return ret == 0;
}

JNIEXPORT jboolean JNICALL Java_com_bql_Seek(JNIEnv *env, jobject thiz, jlong ctx, jint seek )
{
	LOGI_print(s_logger, "call Seek: ctx=%p, seek=%d", (void*)ctx, seek);
	CPlayer *pPlayer = (CPlayer *)ctx;	
	int ret = pPlayer->Seek(seek);

	LOGI_print(s_logger, "result: %d", ret);
	return ret == 0;
}

JNIEXPORT void     JNICALL Java_com_bql_DeletePlayer(JNIEnv *env, jobject thiz, jlong ctx)
{
	LOGI_print(s_logger, "call DeletePlayer: ctx=%p", (void*)ctx);
	CPlayer *pPlayer = (CPlayer *)ctx;	
	assert(pPlayer != 0);
	CStreamer::DeleteMyPlayer(env, pPlayer);
}

////////////////////////////////////////////////////////////////////////
JNIEXPORT jlong    JNICALL Java_com_bql_CreateRecorder(JNIEnv *env, jobject thiz, jobject weak, jlong ctx, jstring juri, jint vcodecid, jint acodecid)
{
	char *uri = NULL;
	JNI_GET_UTF_CHAR(uri, juri);

	LOGI_print(s_logger, "call CreateRecorder: obj=%p, ctx=%p, uri=%s, vcodecid=%d, acodecid=%d", weak, (void*)ctx, uri, vcodecid, acodecid);
	CMediaRecorder *pRecorder = CStreamer::CreateRecorder(env, weak, (CPlayer*)ctx, uri, GetCodecID(vcodecid), GetCodecID(acodecid));

	JNI_RELEASE_STR_STR(uri, juri);	
	LOGI_print(s_logger, "result: %p", pRecorder);
	return (jlong)pRecorder;	
}

JNIEXPORT jboolean JNICALL Java_com_bql_SetVideoStream(JNIEnv *env, jobject thiz, jlong ctx, jint fps, jint width, jint height, jint qval, jint bitrate)
{
	LOGI_print(s_logger, "call SetVideoStream: ctx=%p, fps=%d, videow=%d, videoh=%d, qval=%d, bitrate=%d", (void*)ctx, fps, width, height, qval, bitrate);
	CMediaRecorder *pRecorder = (CMediaRecorder *)ctx;	
	int ret = pRecorder->SetVideoStream(fps, width, height, qval, bitrate);

	LOGI_print(s_logger, "result: %d", ret);
	return ret == 0;
}

JNIEXPORT jboolean JNICALL Java_com_bql_SetAudioStream(JNIEnv *env, jobject thiz, jlong ctx, jint channels, jint depth, jint samplerate, jint bitrate)
{
	LOGI_print(s_logger, "call SetAudioStream: ctx=%p, channels=%d, depth=%d, samplerate=%d, bitrate=%d", (void*)ctx, channels, depth, samplerate, bitrate);
	CMediaRecorder *pRecorder = (CMediaRecorder *)ctx;	
	int ret = pRecorder->SetAudioStream(channels, depth, samplerate, bitrate);

	LOGI_print(s_logger, "result: %d", ret);
	return ret == 0;
}

JNIEXPORT jboolean JNICALL Java_com_bql_PrepareWrite(JNIEnv *env, jobject thiz, jlong ctx)
{
	LOGI_print(s_logger, "call PrepareWrite: ctx=%p", (void*)ctx);
	CMediaRecorder *pRecorder = (CMediaRecorder *)ctx;	
	int ret = pRecorder->PrepareWrite();

	LOGI_print(s_logger, "result: %d", ret);
	return ret == 0;
}

JNIEXPORT jint     JNICALL Java_com_bql_GetRecordTime(JNIEnv *env, jobject thiz, jlong ctx)
{
	CMediaRecorder *pRecorder = (CMediaRecorder *)ctx;	
	return pRecorder->GetRecordTime();
}

JNIEXPORT jboolean JNICALL Java_com_bql_Write(JNIEnv *env, jobject thiz, jlong ctx, jint codecid, jlong pts, jlong buf)
{
	if( pts == AV_NOPTS_VALUE ) pts = GetTickCount() / 1000;
	CBuffer   *pBuffer = (CBuffer*)buf;
	AVPacket  *pPacket = pBuffer->GetPacket();	
	pBuffer->m_codecid = codecid; //设置	
	pPacket->pts = pts;
	pPacket->dts = pts;	
	LOGT_print(s_logger, "call Write[%d]: ctx=%p, buf=%p, pts=%lld, data=%p, size=%d", codecid, (void*)ctx, (void*)buf, pPacket->pts, pPacket->data, pPacket->size);
	CMediaRecorder *pRecorder = (CMediaRecorder*)ctx;
	return pRecorder->Write(env, pBuffer) != 0;
}

JNIEXPORT void     JNICALL Java_com_bql_DeleteRecorder(JNIEnv *env, jobject thiz, jlong ctx)
{
	LOGI_print(s_logger, "call DeleteRecorder: ctx=%p", (void*)ctx);
	CMediaRecorder *pRecorder = (CMediaRecorder *)ctx;
	assert(pRecorder != 0);	
	CStreamer::DeleteRecorder(env, pRecorder);		
}

////////////////////////////////////////////////////////////////////////
JNIEXPORT jobject  JNICALL Java_com_bql_AcquireBuffer(JNIEnv *env, jobject thiz, jlong ctx, jint len)
{
	LOGT_print(s_logger, "call AcquireBuffer: ctx=%p, len=%d", (void*)ctx, len);
	assert(len > 0);
	CScopeBuffer pBuffer(g_pBufferManager->Pop());
	pBuffer->SetSize(len);
	jobject jbuf = 0;
	jobject jobj = 0;

	do{
	jbuf = env->NewDirectByteBuffer(pBuffer->GetPacket()->data, pBuffer->GetPacket()->size);
	if( env->ExceptionCheck() ) {
		env->ExceptionClear();
		LOGT_print(s_logger, "call NewDirectByteBuffer error");
		break;
	}

	/* 请保持参数的强制类型转换 */
	jobj = env->NewObject( g_jBQLBuffer.thizClass, g_jBQLBuffer.constructor,
					(uint64_t)pBuffer.p, 									/* long nativeThiz 	*/
					(uint32_t)0, 										/* int data_type 	*/
					jbuf,												/* ByteBuffer data 	*/
					(int64_t)0,											/* long arg1 		*/
					(int64_t)0,											/* long arg2 		*/
					(int64_t)0,											/* long arg3 		*/
					(int64_t)0  										/* long arg4 		*/
					);
	if( env->ExceptionCheck() ) {
		env->ExceptionClear();
		LOGT_print(s_logger, "call NewObject error");
		break;
	}

	LOGT_print(s_logger, "result: %p, data=%p", jobj, pBuffer->GetPacket()->data);
	pBuffer.Detach();
	return jobj;
	}while(0);

	LOGE_print(s_logger, "result: null");
	if( jbuf ) env->DeleteLocalRef(jbuf);
	if( jobj ) env->DeleteLocalRef(jobj);
	return NULL;
}

JNIEXPORT jobject  JNICALL Java_com_bql_GetThumbnail(JNIEnv *env, jobject thiz, jstring juri, jint ipos, jint ifmt, jint w, jint h)
{
	char *uri = NULL;
	JNI_GET_UTF_CHAR(uri, juri);

	LOGI_print(s_logger, "call GetThumbnail[%dx%d]: pos=%d, fmt=%d, url=%s", w, h, ipos, ifmt, uri);
	int t = 0;
	CScopeBuffer pBuffer(CStreamer::GetThumbnail(uri, ipos, ifmt, t, w, h));

	JNI_RELEASE_STR_STR(uri, juri);
	if( pBuffer.p != 0 ) {
	jobject jbuf = 0;
	jobject jobj = 0;

	do{
	jbuf = env->NewDirectByteBuffer(pBuffer->GetPacket()->data, pBuffer->GetPacket()->size);
	if( env->ExceptionCheck() ) {
		env->ExceptionClear();
		LOGT_print(s_logger, "call NewDirectByteBuffer error");
		break;
	}

	/* 请保持参数的强制类型转换 */
	jobj = env->NewObject( g_jBQLBuffer.thizClass, g_jBQLBuffer.constructor,
					(uint64_t)pBuffer.p, 									/* long nativeThiz 	*/
					(uint32_t)ifmt, 										/* int data_type 	*/
					jbuf,												/* ByteBuffer data 	*/
					(int64_t)pBuffer->GetPacket()->pts,					/* long arg1 		*/
					(int64_t)t,											/* long arg2 		*/
					(int64_t)w,											/* long arg3 		*/
					(int64_t)h  										/* long arg4 		*/
					);
	if( env->ExceptionCheck() ) {
		env->ExceptionClear();
		LOGT_print(s_logger, "call NewObject error");
		break;
	}

	LOGT_print(s_logger, "result: %p(%dX%d), pts=%lld, data=%p, size=%d", jobj, w, h, pBuffer->GetPacket()->pts, pBuffer->GetPacket()->data, pBuffer->GetPacket()->size);
	pBuffer.Detach();
	return jobj;
	}while(0);
	if( jbuf ) env->DeleteLocalRef(jbuf);
	if( jobj ) env->DeleteLocalRef(jobj);
	}
	LOGE_print(s_logger, "result: null");
	return NULL;
}

JNIEXPORT jlong    JNICALL Java_com_bql_SetBufferSize(JNIEnv *env, jobject thiz, jlong ctx, jint len)
{
	LOGT_print(s_logger, "call SetBufferSize: ctx=%p, len=%d", (void*)ctx, len);
	AVPacket *pPacket = ((CBuffer *)ctx)->GetPacket();
	assert( len <= pPacket->size );
	if( len != 0 ) pPacket->size = len;
	return (jlong) pPacket->data;
}

JNIEXPORT void     JNICALL Java_com_bql_ReleaseBuffer(JNIEnv *env, jobject thiz, jlong ctx)
{
	LOGT_print(s_logger, "call ReleaseBuffer: ctx=%p", (void*)ctx);
	CBuffer *pBuffer = (CBuffer *)ctx;
	assert(pBuffer != 0);
	pBuffer->Release();
}
