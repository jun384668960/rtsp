#include "com_bql_MSG_Native.h"
#include "buffer/Buffer.h"

JNIMediaPlayer   g_jBQLMediaPlayer;
JNIBuffer        g_jBQLBuffer;
JNIAudioTrack    g_jAudioTrack;
JNIMediaRecorder g_jBQLMediaRecorder;
const int rgb_pixel = 4;
////////////////////////////////////////////////////////////////////////////////
bool Java_drawNativeWindow(ANativeWindow *pSurfaceWindows, unsigned char* pBuf, int width, int height)
{
	assert(pSurfaceWindows != NULL);

	ANativeWindow_Buffer buffer;
	if( ANativeWindow_lock(pSurfaceWindows, &buffer, 0) != 0 )
	{
		LOGE_print("com_bql_streamuser", "call ANativeWindow_lock return false");			
		return false;
	}
	else
	{
		if( buffer.width != width || buffer.height != height || buffer.format != WINDOW_FORMAT_RGBX_8888 )
		{
			LOGW_print("com_bql_streamuser", "call ANativeWindow_setBuffersGeometry(%dx%d, fmt=%d/%d)", width, height, WINDOW_FORMAT_RGBX_8888, buffer.format);
			ANativeWindow_unlockAndPost(pSurfaceWindows);
			ANativeWindow_setBuffersGeometry(pSurfaceWindows, width, height, WINDOW_FORMAT_RGBX_8888);

			if( ANativeWindow_lock(pSurfaceWindows, &buffer, 0) != 0 )
			{
				LOGE_print("com_bql_streamuser", "call ANativeWindow_lock return false");
				return false;
			}

			LOGI_print("com_bql_streamuser", "ANativeWindow: %dx%d, fmt=%d", buffer.width, buffer.height, buffer.format);
			if( buffer.width != width || buffer.height != height || buffer.format != WINDOW_FORMAT_RGBX_8888 )
			{
				LOGE_print("com_bql_streamuser", "fail to try draw Rgbx(%dx%d) on ANativeWindow(%dx%d, fmt=%d)", width, height, buffer.width, buffer.height, buffer.format);
				ANativeWindow_unlockAndPost(pSurfaceWindows);
				return false;
			}
		}

		if( buffer.stride <= buffer.width )
		{
			memcpy(buffer.bits, pBuf, width * rgb_pixel * height);
		}
		else
		{
			for(int i = 0; i < height; ++i)
			{
				memcpy(buffer.bits + buffer.stride * i * rgb_pixel, pBuf + width * i * rgb_pixel, width * rgb_pixel);
			}
		}

		ANativeWindow_unlockAndPost(pSurfaceWindows);		
		return true;
	}
}

////////////////////////////////////////////////////////////////////////////////
void Java_fireNotify(JNIEnv *env, JNIStreamerContext *pParams, int type, int arg1, int arg2, void* obj)
{
	COST_STAT_DECLARE(cost);
	env->CallStaticVoidMethod(g_jBQLMediaPlayer.thizClass, g_jBQLMediaPlayer.post, pParams->m_owner, type, arg1, arg2, obj);
	LOGD_print("com_bql_streamuser", "Notify MediaPlayer[%p]: %d, %d, %d, %p, cost=%lld", pParams->m_owner, type, arg1, arg2, obj, cost.Get());	
}

void Java_fireBuffer(JNIEnv *env, JNIStreamerContext *pParams, int cid, void *obj, int val)
{
	CScopeBuffer pBuffer((CBuffer *)obj);
	pBuffer->m_codecid = cid;

	for(int i = 0; i < 2; i ++) {//hook
	if( pParams->m_pHook[i] &&
		pParams->HasCodecid(1 + i, cid) != 0 &&
		pParams->m_pHook[i]->Write(env, pBuffer->AddRef()) == 0 )
	{
		pParams->m_pHook[i] = 0;
	}
	}

	if( pParams->HasCodecid(0, cid) == 0 )
	{
		return;
	}

	jobject jbuf = 0;
	jobject jobj = 0;

	do{
	jbuf = env->NewDirectByteBuffer(pBuffer->GetPacket()->data, pBuffer->GetPacket()->size);
	if( env->ExceptionCheck() ) {
		env->ExceptionClear();
		LOGE_print("com_bql_streamuser", "call NewDirectByteBuffer error");
		break;
	}

	/* 请保持参数的强制类型转换 */
	jobj = env->NewObject( g_jBQLBuffer.thizClass, g_jBQLBuffer.constructor,
					(uint64_t)obj, 										/* long nativeThiz 	*/
					(uint32_t)cid, 										/* int data_type 	*/
					jbuf,												/* ByteBuffer data 	*/
					(int64_t)pBuffer->GetPacket()->pts,					/* long arg1 		*/
					(int64_t)val,										/* long arg2 		*/
					(int64_t)0,											/* long arg3 		*/
					(int64_t)0  										/* long arg4 		*/
					);
	if( env->ExceptionCheck() ) {
		env->ExceptionClear();
		LOGE_print("com_bql_streamuser", "call NewObject error");
		break;
	}

	Java_fireNotify(env, pParams, MEDIA_DATA, 0, 0, jobj);
	pBuffer.Detach();//转交给jBuffer释放
	}while(0);

	if( jbuf ) env->DeleteLocalRef(jbuf);
	if( jobj ) env->DeleteLocalRef(jobj);
}

int GetCodecID(int id)
{
	switch(id) {
		case MEDIA_DATA_TYPE_264: return AV_CODEC_ID_H264 ;
		case MEDIA_DATA_TYPE_JPG: return AV_CODEC_ID_MJPEG;
		case MEDIA_DATA_TYPE_AAC: return AV_CODEC_ID_AAC  ;
		default: return AV_CODEC_ID_NONE;
	}
}
